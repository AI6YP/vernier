# vernier

ESP32-C6 + AMOLED + Hall Sensor = Vernier

A digital tuning knob (VFO) for a homebrew radio transceiver. A magnetic
rotary encoder senses knob rotation, and a round AMOLED touch display shows
the frequency and front-panel controls.

Interactive notebook: https://observablehq.com/@drom/vernier

![](assets/device-photo.jpg)

## Hardware

Built around the **Waveshare ESP32-C6-Touch-AMOLED-1.43** module:

- **MCU** — ESP32-C6 (Wi-Fi, RISC-V)
- **Display** — 1.43" round AMOLED, 466×466, SH8601 / CO5300 driver (QSPI)
- **Angle sensor** — MT6701 magnetic rotary encoder (I2C) on the tuning shaft
- **IMU** — QMI8658C accelerometer/gyro
- **RTC** — PCF85063
- **Audio** — ES7210 ADC

Component datasheets live in [ref/](ref/).

## Repository layout

| Path | Contents |
|------|----------|
| [sw/](sw/) | ESP-IDF firmware (v6.1) — display, encoder, Wi-Fi, UI |
| [3dmodel/](3dmodel/) | Enclosure parts as [Replicad](https://replicad.xyz) `.js` scripts + generated `.stl` |
| [panel/](panel/) | Front-panel layout studies (SVG) |
| [assets/](assets/) | Photos and diagrams |
| [ref/](ref/) | Vendor datasheets and demo code |

### 3D model versions

Two mechanical designs coexist:

- `vernier-*` — original layout (basic / handle / shaft-half)
- `vernier-center-*` — variant with a centered display ([center.md](center.md))

Each `.js` is a parametric Replicad script; the matching `.stl` is its export.

### Front panel

[panel/](panel/) explores a modular transceiver front-panel layout — screen
only, screen + VFO, screen + VFO + RIT.

## Firmware

See [sw/README.md](sw/README.md) for the full build/flash flow. Quick version:

```bash
cd sw
. ~/esp-idf/export.sh
idf.py set-target esp32c6
idf.py -p /dev/ttyACM0 flash monitor
```

Set flash size to **16MB** in `menuconfig` (Serial flasher config → Flash size).

## License

See [LICENSE](LICENSE).
