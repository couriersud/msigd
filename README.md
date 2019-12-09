
![](https://github.com/couriersud/msigd/workflows/msigd_ubuntu_CI/badge.svg)

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
      --game_mode            user fps racing rts rpg 
      --response_time        normal fast fastest 
      --hdcr                 off on 
      --refresh_rate_display off on 
      --refresh_rate_position left_top right_top left_bottom right_bottom 
      --alarm_clock          off 1 2 3 4 
      --alarm_clock_index    1 to 4
      --alarm_clock_time     0 to 5999
      --alarm_clock_position left_top right_top left_bottom right_bottom 
      --screen_assistance    0 to 12
      --zero_latency         off on 
      --screen_size          19 24 4:3 16:9 
      --night_vision         off normal strong strongest ai 
      --pro_mode             user reader cinema designer 
      --eye_saver            off on 
      --image_enhancement    off weak medium strong strongest 
      --brightness           0 to 100
      --contrast             0 to 100
      --sharpness            0 to 5
      --color_preset         cool normal warm custom 
      --red                  0 to 100
      --green                0 to 100
      --blue                 0 to 100
      --input                hdmi1 hdmi2 dp usbc 
      --pip                  off pip pbp 
      --pip_input            hdmi1 hdmi2 dp usbc 
      --pbp_input            hdmi1 hdmi2 dp usbc 
      --pip_size             small medium large 
      --pip_position         left_top right_top left_bottom right_bottom 
      --osd_language         0 to 19
      --osd_transparency     0 to 5
      --osd_timeout          0 to 30
      --navi_up              off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_down            off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_left            off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_right           off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
  -d, --debug                enable debug output
  -h, --help                 display this help and exit
      --version              output version information and exit

Options are processed in the order they are given. You may specify an option
more than once with identical or different values.

Exit status:
 0  if OK,
 1  if error during option parsing,
 2  if error during device access,

```
You may also use the following to display man style help

```
help2man --include=msigd.help2man --no-info ./msigd | nroff -man |less
```
