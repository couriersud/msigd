
![](https://github.com/couriersud/msigd/workflows/Ubuntu%20latest/badge.svg?branch=master)

# msigd
MSI Gaming Device control application

Controls MSI Monitor OSD settings.

## Supported monitors

### MSI Optix MAG321CURV

```
ID 1462:3fa4 Micro Star International
```

## Compile

Make sure you have libusb installed. On debian based systems

```
sudo apt install libusb-dev
```

compile 

```
make
```

## Security

This program needs root privilidges. Use with care.

Alternatively you may use udev to grant user access rights. I have not tested 
this yet: [Documentation on askubuntu](https://askubuntu.com/questions/978552/how-do-i-make-libusb-work-as-non-root)
 

## Usage
```
./msigd --help
```
```
Usage: msigd [OPTION]... 
Query or set monitor settings by usb directly on the device.
For supported devices please refer to the documentation.

  -q, --query                display all monitor settings. This will also
                               list readonly settings and settings whose
                               function is currently unknown.
      --info                 display device information. This can be used
                               with --query
      --power                values: off on 
      --game_mode            values: user fps racing rts rpg 
      --unknown06            values: 0 to 100
      --response_time        values: normal fast fastest 
      --enable_dynamic       values: on off 
      --hdcr                 values: off on 
      --refresh_rate_display values: off on 
      --refresh_rate_position values: left_top right_top left_bottom right_bottom 
      --alarm_clock          values: off 1 2 3 4 
      --alarm_clock_index    values: 1 to 4
      --alarm_clock_time     values: 0 to 5999
      --alarm_clock_position values: left_top right_top left_bottom right_bottom 
      --screen_assistance    values: 0 to 12
      --zero_latency         values: off on 
      --screen_size          values: 19 24 4:3 16:9 
      --night_vision         values: off normal strong strongest ai 
      --pro_mode             values: user reader cinema designer 
      --eye_saver            values: off on 
      --image_enhancement    values: off weak medium strong strongest 
      --brightness           values: 0 to 100
      --contrast             values: 0 to 100
      --sharpness            values: 0 to 5
      --color_preset         values: cool normal warm custom 
      --red                  values: 0 to 100
      --green                values: 0 to 100
      --blue                 values: 0 to 100
      --rgb                  values: 0 to 1000000
      --unknown10            values: off on 
      --input                values: hdmi1 hdmi2 dp usbc 
      --pip                  values: off pip pbp 
      --pip_input            values: hdmi1 hdmi2 dp usbc 
      --pbp_input            values: hdmi1 hdmi2 dp usbc 
      --pip_size             values: small medium large 
      --pip_position         values: left_top right_top left_bottom right_bottom 
      --toggle_display       values: off on 
      --toggle_sound         values: off on 
      --osd_language         values: 0 to 19
      --osd_transparency     values: 0 to 5
      --osd_timeout          values: 0 to 30
      --reset                values: off on 
      --sound_enable         values: off on 
      --back_rgb             values: off on 
      --navi_up              values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_down            values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_left            values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_right           values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
  -d, --debug                enable debug output
  -h, --help                 display this help and exit
      --version              output version information and exit

Options are processed in the order they are given. You may specify an option
more than once with identical or different values.

Exit status:
 0  if OK,
 1  if error during option parsing,
 2  if error during device access,

Report bugs on <https://github.com/couriersud/msigd/issues>

msigd home page: <https://github.com/couriersud/msigd>
```
You may also use the following to display man style help

```
help2man --include=msigd.help2man --no-info ./msigd | nroff -man |less
```

## Examples

I have a usb swiched port. Upon pressing a button on the switch, keyboard and 
mouse (or up to four devices) are switched between two computers: my desktop and
my laptop. There is a USB-C docking station connected to the switch. The laptop
is connected via USB-C. The docking station is connected to the USB-C hub and via
HDMI to input ```hdmi1``` to the monitor.

The following script runs on the desktop. If the keyboard gets disconnect - switched
to laptop, it will switch monitor input to ```hdmi1```. Once the keyboard 
reconnects, it will switch back monitor input to ```dp```.

```bash
#!/bin/sh

WATCH_DIR=/dev/input/by-id
WATCH_INPUT=usb-046a_010d-event-kbd
DISP_INPUT=dp
DISP_ALTERNATIVE=hdmi1

while true; do
	while [ -e ${WATCH_DIR}/${WATCH_INPUT} ]; do sleep 1; done
	./msigd --input $DISP_ALTERNATIVE
	while [ ! -e ${WATCH_DIR}/$WATCH_INPUT ]; do sleep 1; done
	./msigd --input $DISP_INPUT
done
```

