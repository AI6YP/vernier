esp-idf v6.1


```bash
cd ~/work/github/espressif/esp-idf
git pull
git submodule update --init --recursive
./install.sh esp32c6

cd ~/work/github/AI6YP/vernier/sw
. ~/work/github/espressif/esp-idf/export.sh

idf.py menuconfig

# Set 16MB flash size.
#  Serial flasher config -> Flash size -> 16MB

idf.py set-target esp32c6

# just build
idf.py build

# build & flash
idf.py -p /dev/ttyACM0 flash

 # build & flash APP only & monitor -> exit Ctrl+]
idf.py -p /dev/ttyACM0 app-flash monitor
```
