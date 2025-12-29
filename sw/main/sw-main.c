#include <stdint.h>
#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"

// LCD
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_sh8601.h"

// WIFI
#include "esp_wifi.h"
#include "esp_tls_crypto.h"
#include "esp_http_server.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "sdkconfig.h"

#include "pages.h"
#include "iosevka.h"

// pins  QFN40

// #define BAT_ADC       0   // pin6
// #define GPIO1         1   // pin7
// #define GPIO2         2   // pin8
#define LCD_RST       3   // pin9
#define LCD_DATA0     4   // pin10
#define LCD_DATA1     5   // pin11
#define LCD_DATA2     6   // pin12
#define LCD_DATA3     7   // pin13
#define SCL           8   // pin14
// #define BOOT0         9   // pin15
#define LCD_CS        10  // pin16
#define LCD_PCLK      11  // pin17
// #define USB_N         12  // pin18
// #define USB_P         13  // pin19

// #define GPIO14        15  // pin27
// #define TXD           16  // pin28
// #define RXD           17  // pin29
#define SDA           18  // pin31
// #define GPIO19        19  // pin32
// #define GPIO20        20  // pin33
// #define GPIO21        21  // pin34
// #define GPIO22        22  // pin35
// #define GPIO23        23  // pin36

// LCD parameters
constexpr int lcd_h_res = 466;
constexpr int lcd_v_res = 466;

// MT6701 Register addresses
constexpr uint8_t ANGLE13_6_REG = 0x03;
// constexpr uint8_t ANGLE15_0_nn_REG = 0x04;

static const sh8601_lcd_init_cmd_t lcd_init_cmds[] = {
  {0x11, (uint8_t []){0x00}, 0, 80},
  {0xC4, (uint8_t []){0x80}, 1, 0},
  {0x53, (uint8_t []){0x20}, 1, 1},
  {0x63, (uint8_t []){0xFF}, 1, 1},
  {0x51, (uint8_t []){0x00}, 1, 1},
  {0x29, (uint8_t []){0x00}, 0, 10},
  {0x51, (uint8_t []){0xFF}, 1, 0}
};

typedef struct {
  uint16_t angle;
  uint16_t angle_correction;
  uint16_t prev_angle;
  uint32_t freq;
  // uint32_t freq_offset;
  const void *config_ptr;
  uint8_t angle_buff[2];
} state_t;

static state_t state;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";

static int s_retry_num = 0;

// I2C

i2c_master_bus_config_t i2c_mst_config = {
  .clk_source = I2C_CLK_SRC_DEFAULT, // LP_I2C_SCLK_DEFAULT
  .i2c_port   = -1, // auto
  .scl_io_num = SCL,
  .sda_io_num = SDA,
  .glitch_ignore_cnt = 7,
  .flags.enable_internal_pullup = true
};

i2c_master_bus_handle_t bus_handle;

// MT6701 encoder I2C device
i2c_device_config_t dev_cfg_enc = {
  .dev_addr_length = I2C_ADDR_BIT_LEN_7,
  .device_address = 0x6, // p.22
  .scl_speed_hz = 100000, // 100000
};

i2c_master_dev_handle_t dev_handle_enc;

// LCD

esp_lcd_panel_io_handle_t BrigPanelHandle = NULL;

static void event_handler(
  void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data
) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
     esp_wifi_connect();
  } else
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (s_retry_num < 100 /* EXAMPLE_ESP_MAXIMUM_RETRY */) {
      esp_wifi_connect();
      s_retry_num++;
      ESP_LOGI(TAG, "retry to connect to the AP");
    } else {
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
    ESP_LOGI(TAG,"connect to the AP fail");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

void wifi_init_sta(state_t *cntxt) {
  s_wifi_event_group = xEventGroupCreate();
  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
    WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id
  ));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
    IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip
  ));

  wifi_config_t wifi_config = {0};

  memcpy(
    &wifi_config.sta.ssid,
    (char *)cntxt->config_ptr,
    sizeof(wifi_config.sta.ssid)
  );
  memcpy(
    &wifi_config.sta.password,
    ((char *)cntxt->config_ptr) + 32,
    sizeof(wifi_config.sta.password)
  );


  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "wifi_init_sta finished.");

  /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
   * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
  EventBits_t bits = xEventGroupWaitBits(
    s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY
  );

  /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually happened. */

  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
  } else if (bits & WIFI_FAIL_BIT) {
    ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
  } else {
    ESP_LOGE(TAG, "UNEXPECTED EVENT");
  }
}

esp_err_t index_get_handler(httpd_req_t *req) {
  /* Send a simple response */
  ESP_LOGI(TAG, "INDEX GET HANDLER");
  httpd_resp_send(req, PAGE_index, PAGE_index_length);
  return ESP_OK;
}

esp_err_t get_status_handler(httpd_req_t *req) {
  /* Send a simple response */
  // ESP_LOGI(TAG, "INDEX GET HANDLER");
  httpd_resp_send(req, (const char *)&state, sizeof(state));
  return ESP_OK;
}

esp_err_t msg_handler(httpd_req_t *req) {
  if (req->method == HTTP_GET) {
    ESP_LOGI(TAG, "Handshake done, the new connection was opened");
    return ESP_OK;
  }
  httpd_ws_frame_t ws_pkt;
  uint8_t *buf = NULL;
  // uint8_t *tx_buf = NULL;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
  ws_pkt.type = HTTPD_WS_TYPE_BINARY;
  /* Set max_len = 0 to get the frame len */
  esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
    return ret;
  }
  // ESP_LOGI(TAG, "frame len is %d", ws_pkt.len);
  if (ws_pkt.len) {
    /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
    buf = calloc(ws_pkt.len, 1);
    // tx_buf = calloc(8, 1);
    if (buf == NULL) {
      // ESP_LOGE(TAG, "Failed to calloc memory for buf");
      ESP_LOGE(TAG, "Failed to malloc memory for buf");
      return ESP_ERR_NO_MEM;
    }
    ws_pkt.payload = buf;
    /* Set max_len = ws_pkt.len to get the frame payload */
    ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
      free(buf);
      return ret;
    }
    // rdwr_i2c(i2c_master, buf, tx_buf, ws_pkt.len);

    ws_pkt.payload = (uint8_t *)&state;
    ws_pkt.len = sizeof(state);
  }

  ret = httpd_ws_send_frame(req, &ws_pkt);

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "httpd_ws_send_frame failed with %d", ret);
  }

  free(buf);
  return ret;
}

/* Our URI handler function to be called during POST /uri request */

httpd_uri_t uri_get = {
  .uri      = "/",
  .method   = HTTP_GET,
  .handler  = index_get_handler,
  .user_ctx = NULL
};

httpd_uri_t uri_get_dev = {
  .uri      = "/dev",
  .method   = HTTP_GET,
  .handler  = get_status_handler,
  .user_ctx = NULL
};


httpd_uri_t msg_get = {
  .uri      = "/dev1",
  .method   = HTTP_GET,
  .handler  = msg_handler,
  .user_ctx = NULL,
  .is_websocket = true
};

httpd_uri_t msg_post = {
  .uri      = "/dev1",
  .method   = HTTP_POST,
  .handler  = msg_handler,
  .user_ctx = NULL,
  .is_websocket = true
};


httpd_handle_t start_webserver() {
  /* Generate default configuration */
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.stack_size = 16384;

  /* Empty handle to esp_http_server */
  httpd_handle_t server = NULL;

  /* Start the httpd server */
  if (httpd_start(&server, &config) == ESP_OK) {
      /* Register URI handlers */
      httpd_register_uri_handler(server, &uri_get);
      httpd_register_uri_handler(server, &msg_get);
      httpd_register_uri_handler(server, &msg_post);
      httpd_register_uri_handler(server, &uri_get_dev);
  }
  /* If server failed to start, handle will be NULL */
  return server;
}

void config_map_init(state_t *cntxt) {
  const esp_partition_t *config_partition = esp_partition_find_first(
    ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "config"
  );

  assert(config_partition != NULL);

  esp_partition_mmap_handle_t config_map_handle;
  ESP_ERROR_CHECK(esp_partition_mmap(
    config_partition, // partition -- Pointer to partition structure obtained using esp_partition_find_first or esp_partition_get. Must be non-NULL
    0, // offset -- Offset from the beginning of partition where mapping should start.
    config_partition->size, // size -- Size of the area to be mapped.
    ESP_PARTITION_MMAP_DATA, // memory -- Memory space where the region should be mapped
    &cntxt->config_ptr, // const void **out_ptr -- Output, pointer to the mapped memory region
    &config_map_handle // out_handle -- Output, handle which should be used for esp_partition_munmap call
  ));
}

void setBrightnes(uint8_t brig) {
  uint32_t lcd_cmd = 0x51;
  lcd_cmd &= 0xff;
  lcd_cmd <<= 8;
  lcd_cmd |= 0x02 << 24;
  uint8_t param = brig;
  esp_lcd_panel_io_tx_param(BrigPanelHandle, lcd_cmd, &param,1);
}

static uint16_t *screen_buffer = NULL;
constexpr int screen_buffer_size = 65536;

void fillScreen(esp_lcd_panel_handle_t panel_handle, uint16_t color) {
  for (int i = 0; i < screen_buffer_size; i++) {
    screen_buffer[i] = color;
  }
  for (int y = 0; y < 4; y++) {
    int y1 = y * 128;
    int y2 = y1 + 128;
    y2 = (y2 > 466) ? 466 : y2;
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, y1, 480, y2, screen_buffer));
    // vTaskDelay(pdMS_TO_TICKS(1)); // Wait 10ms
  }
  vTaskDelay(pdMS_TO_TICKS(50)); // Wait 10ms
}

typedef struct {
  uint16_t x;
  uint8_t w;
  uint8_t h;
} freq_digit_t;

static const freq_digit_t freqDigitSpecs[] = {
  {440, 32, 48}, {408, 32, 48}, // HZ
  {336, 64, 96}, {272, 64, 96}, {208, 64, 96}, // kHz
  // dot x: 168
  {128, 64, 96}, {64,  64, 96}, {0,   64, 96} // MHz
};

void drawLetter(int charCode, int x, int y, int w, int h, uint16_t color, uint16_t *screen_buffer, int buffer_w) {
  for (int y1 = 0; y1 < h; y1++) {
    for (int x1 = 0; x1 < w; x1++) {
      int letterByteSize = w * h / 8;
      int idx = charCode * letterByteSize + (x1 / 8) + y1 * (w / 8);
      uint8_t byte = (h == 48) ? iosevka_32_48[idx] : iosevka_64_96[idx];
      int bit = byte & (1 << (7 - (x1 % 8)));
      if (bit) {
        screen_buffer[y1 * buffer_w + x + x1] = color;
      } else {
        screen_buffer[y1 * buffer_w + x + x1] = 0x0000;
      }
    }
  }
}

void printMainFreq(esp_lcd_panel_handle_t panel_handle, int val) {
  const uint16_t color = 0x7777; // grey
  const uint16_t offsetX = 0;
  const uint16_t offsetY = 208;
  const uint16_t label_w = 480;
  const uint16_t label_h = 96;
  // clean screen buffer
  for (int i = 0; i < label_w * label_h; i++) {
    screen_buffer[i] = 0x0000;
  }
  // draw MHz dot separator
  drawLetter(10, 184, 0, 32, 48, color, screen_buffer, label_w);
  val = val / 10;
  for (int i = 0; i < 8; i++) { // LSB first
    int digit = val % 10;
    val /= 10;
    auto digitSpec = freqDigitSpecs[i];
    drawLetter(digit, digitSpec.x, 0, digitSpec.w, digitSpec.h, color, screen_buffer, label_w);
  }
  // render full label
  ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(
    panel_handle, offsetX, offsetY, offsetX + label_w, offsetY + label_h, screen_buffer
  ));
}

void line(
  int x1, int y1, int x2, int y2,
  uint16_t color, uint16_t *screen_buffer, int buffer_w, int buffer_h
) {
  // Bresenham's line drawing algorithm
  int dx = abs(x2 - x1);
  int dy = abs(y2 - y1);
  int sx = (x1 < x2) ? 1 : -1;
  int sy = (y1 < y2) ? 1 : -1;
  int err = dx - dy;
  while (1) {
    if (x1 >= 0 && x1 < buffer_w && y1 >= 0 && y1 < buffer_h) {
      screen_buffer[y1 * buffer_w + x1] = color;
    }
    if (x1 == x2 && y1 == y2) break;
    int e2 = 2 * err;
    if (e2 > -dy) { err -= dy; x1 += sx; }
    if (e2 < dx) { err += dx; y1 += sy; }
  }
}

void printCenterMark(esp_lcd_panel_handle_t panel_handle) {
  const uint16_t color = 0x0fff; // yellow
  const uint16_t offsetX = 240 - 1;
  const uint16_t offsetY = 100;
  const uint16_t label_w = 2;
  const uint16_t label_h = 100;
  // fill with color
  for (int i = 0; i < label_w * label_h; i++) {
    screen_buffer[i] = color;
  }
  ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(
    panel_handle, offsetX, offsetY, offsetX + label_w, offsetY + label_h, screen_buffer
  ));
}

void radiant(int angle, int r1, int r2, int offsetX, int offsetY, uint16_t color, uint16_t *screen_buffer, int w, int h) {
  if (abs(angle) > 1600 && abs(angle) < (16384 - 1600)) { // skip marks that are too far away
    return;
  }
  float angle_rad = angle * 2 * 3.141592653589793 / 16384;
  float mark_sin = sinf(angle_rad);
  float mark_cos = cosf(angle_rad);
  int x1 = mark_sin * r1 + offsetX;
  int y1 = mark_cos * r1 + offsetY;
  int x2 = mark_sin * r2 + offsetX;
  int y2 = mark_cos * r2 + offsetY;
  line(x1, y1, x2, y2, color, screen_buffer, w, h);
}

void printVernierMarks(esp_lcd_panel_handle_t panel_handle, int freq) {
  const uint16_t color = 0x0fff; // yellow
  const uint16_t w = 384; // screen buffer width
  const uint16_t h = 152; // screen buffer height
  // w * h = 58368px < (65536px screen buffer)
  const uint16_t offsetX = 48; // offset of screen buffer from left edge of display
  const uint16_t offsetY = 16; // offset of screen buffer from bottom edge of display
  const int offsetCircle = 256; // offset circle center from center of screen buffer
  // clean screen buffer
  for (int i = 0; i < w * h; i++) {
    screen_buffer[i] = 0x0000;
  }
  // +-20MHz marks
  int range = 20; // range of marks
  int step = 100; // 100Hz
  // const uint32_t from = (freq - range) / step * step; // floor to nearest step
  // const uint32_t to = (freq + range);
  for (int i = -range; i < range; i++) {
    int delta = i * step; // expected marks -2MHz ... +2MHz
    int err = (freq - delta) % step; // error from expected mark
    int deltaf = delta + err; // makrs corrected to the step
    int angle = deltaf;
    int f = deltaf - freq;
    int r2 = offsetCircle + h;
    int r1;
    if ((f / 1000 * 1000) == f) {
      r1 = offsetCircle + (h * 1 / 4); // long marks
      radiant(angle + 10, r1, r2, w / 2, -offsetCircle, color, screen_buffer, w, h);
      radiant(angle - 10, r1, r2, w / 2, -offsetCircle, color, screen_buffer, w, h);
    } else
    if ((f / 500 * 500) == f) {
      r1 = offsetCircle + (h * 1 / 2); // medium marks
    } else {
      r1 = offsetCircle + (h * 3 / 4); // short marks
    }
    radiant(angle, r1, r2, w / 2, -offsetCircle, color, screen_buffer, w, h);
  }

  ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(
    panel_handle, offsetX, offsetY, offsetX + w, offsetY + h, screen_buffer
  ));
}

void app_main(void) {

  // configure I2C
  ESP_LOGI(TAG, "Initializing I2C");
  ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));
  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg_enc, &dev_handle_enc));

  // initialize MT6701 encoder
  ESP_LOGI(TAG, "Initializing MT6701");

  // initialize SPI bus
  ESP_LOGI(TAG, "Initializing SPI bus");
  spi_bus_config_t buscfg = {
    .sclk_io_num = LCD_PCLK,
    .data0_io_num = LCD_DATA0,
    .data1_io_num = LCD_DATA1,
    .data2_io_num = LCD_DATA2,
    .data3_io_num = LCD_DATA3,
    .max_transfer_sz = (lcd_h_res * lcd_v_res * 16 / 8),
    .isr_cpu_id = ESP_INTR_CPU_AFFINITY_0
  };
  ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

  // initialize LCD IO
  ESP_LOGI(TAG, "Initializing LCD IO");
  esp_lcd_panel_io_handle_t io_handle = NULL;
  const esp_lcd_panel_io_spi_config_t io_config = {
    .cs_gpio_num = LCD_CS,
    .dc_gpio_num = -1,
    .spi_mode = 0,
    .pclk_hz = 40 * 1000 * 1000,
    .trans_queue_depth = 10,
    .lcd_cmd_bits = 32,
    .lcd_param_bits = 8,
    .flags = {
      .quad_mode = true,
    },
  };

  sh8601_vendor_config_t vendor_config = {
    .init_cmds = lcd_init_cmds,
    .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(sh8601_lcd_init_cmd_t),
    .flags = {
      .use_qspi_interface = 1,
    },
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &io_handle));
  BrigPanelHandle = io_handle;
  ESP_LOGI(TAG, "Initializing LCD panel");
  esp_lcd_panel_handle_t panel_handle = NULL;
  const esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = LCD_RST,
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
    .bits_per_pixel = 16,
    .vendor_config = (void *) &vendor_config,
  };


  // install SH8601 panel driver
  ESP_LOGI(TAG, "Initializing LCD panel");
  ESP_ERROR_CHECK(esp_lcd_new_panel_sh8601(io_handle, &panel_config, &panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

  ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
  // ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
  // ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));

  // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

  vTaskDelay(pdMS_TO_TICKS(100)); // Wait 100ms

  setBrightnes(255);

  screen_buffer = (uint16_t *)heap_caps_malloc(screen_buffer_size * sizeof(uint16_t), MALLOC_CAP_DMA);
  if (screen_buffer == NULL) {
    ESP_LOGE(TAG, "Failed to malloc memory for screen buffer");
    return;
  }

  // fill with black
  fillScreen(panel_handle, 0x0000);
  printCenterMark(panel_handle);
  vTaskDelay(pdMS_TO_TICKS(10)); // Wait 100ms

  // color format: 565  bbbbbRRRRRRggggg
  // green: 0x001f
  // red: 0x07e0
  // blue: 0xf800



  {
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
  }

  ESP_LOGI(TAG, "HELLO VERNIER");

  config_map_init(&state);

  wifi_init_sta(&state);

  // httpd_handle_t server = NULL;
  // server = start_webserver();
  start_webserver();


  // initial state
  state.prev_angle = 0; // first time
  state.freq = 433000000; // [Hz]
  state.angle_correction = 16384 - 600; // first prototype
  while(1) {
    // read MT6701 encoder
    uint8_t reg = ANGLE13_6_REG;
    if (i2c_master_transmit_receive(dev_handle_enc, &reg, 1, state.angle_buff, 2, 100) != ESP_OK) { ESP_LOGI(TAG, "I2C Error"); };

    state.angle = (
      (state.angle_buff[0] << 6) | // Angle[13:6]
      (state.angle_buff[1] >> 2)   // Angle[5:0] + NA + NA
    );
    state.angle += state.angle_correction;
    state.angle &= 0x3fff;

    if (state.angle == state.prev_angle) { // no change
      continue;
    }
    // signed angle difference
    // full angle is 14bit 0...16383
    int angle_delta = state.prev_angle - state.angle;

    // small positive delta is 0...8192 + add to freq
    // small negative delta is -8192...0 - add to freq

    // rotate in direction of smaller angle
    if (angle_delta > 8192) { // large positive delta = small negative
      angle_delta -= 16384;
    } else if (angle_delta < -8192) { // large negative delta = small positive
      angle_delta += 16384;
    }

    state.prev_angle = state.angle;
    state.freq += angle_delta;

    printVernierMarks(panel_handle, state.freq);
    vTaskDelay(pdMS_TO_TICKS(20));
    printMainFreq(panel_handle, state.freq);
    vTaskDelay(pdMS_TO_TICKS(20));

    // ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL_0, &state.raw_azim));
    // ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL_2, &state.raw_elev));
    // state.val_azim = state.raw_azim * 125 -210000;
    // state.val_elev = state.raw_elev * 33   -10000;

    // int dif_azim = state.val_azim - state.tgt_azim;
    // if (abs(dif_azim) < 4000) {
    //     gpio_set_level(CW, 0); gpio_set_level(CCW, 0);
    // } else {
    //   if (dif_azim < 0) {
    //     gpio_set_level(CW, 1); gpio_set_level(CCW, 0);
    //   } else {
    //     gpio_set_level(CW, 0); gpio_set_level(CCW, 1);
    //   }
    // }
    // int dif_elev = state.val_elev - state.tgt_elev;
    // if (abs(dif_elev) < 4000) {
    //   gpio_set_level(UP, 0); gpio_set_level(DOWN, 0);
    // } else {
    //   if (dif_elev < 0) {
    //     gpio_set_level(UP, 1); gpio_set_level(DOWN, 0);
    //   } else {
    //     gpio_set_level(UP, 0); gpio_set_level(DOWN, 1);
    //   }
    // }
  }
}
