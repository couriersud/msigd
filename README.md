# 1. msigd

The `msigd` command line tools allows you to change all settings for MSI monitors which can be set in the monitor's OSD menu. 

Update 01.05.2021: In October 2020 my partner, wife and mother of my daughter passed away. This is the reason why there has not been any activity on the msgid project. The project is not abandoned and it will slowly get more attention again.

<!-- TOC depthFrom:2 depthTo:3 orderedList:false -->

- [1. Build status:](#1-build-status)
- [2. Supported monitors](#2-supported-monitors)
    - [2.1. MSI Monitors](#21-msi-monitors)
    - [2.2. Service menu](#22-service-menu)
    - [2.3. USB manufacturer and product id](#23-usb-manufacturer-and-product-id)
- [3. Compile](#3-compile)
    - [3.1. Linux](#31-linux)
    - [3.2. Windows](#32-windows)
    - [3.3. OSX](#33-osx)
- [4. Security](#4-security)
    - [4.1. Linux](#41-linux)
    - [4.2. Windows](#42-windows)
    - [4.3. OSX](#43-osx)
    - [4.4. Building with libusb](#44-building-with-libusb)
- [5. Usage](#5-usage)
- [6. Settings not supported](#6-settings-not-supported)
    - [6.1. MAG241 series](#61-mag241-series)
    - [6.2. PS341 series](#62-ps341-series)
- [7. Examples](#7-examples)
    - [7.1. Automatically switch input source](#71-automatically-switch-input-source)
    - [7.2. Change settings depending on foreground window](#72-change-settings-depending-on-foreground-window)
    - [7.3. Using msigd in a libvirt-qemu hook](#73-using-msigd-in-a-libvirt-qemu-hook)
- [8. TODO](#8-todo)
- [9. Credits](#9-credits)

<!-- /TOC -->

## 1. Build status:


| OS      | Compiler   | Status        | 
|:------- |:---------- |:------------- | 
| Linux   | g++        | ![](https://github.com/couriersud/msigd/workflows/Ubuntu%20latest/badge.svg?branch=master)  |
| Windows | MinGW++    | ![](https://github.com/couriersud/msigd/workflows/Windows%20latest/badge.svg?branch=master) |
| OSX     | clang++    | ![](https://github.com/couriersud/msigd/workflows/OSX%20latest/badge.svg?branch=master)     |

## 2. Supported monitors

All monitors for which the OSD Gaming Device App is available most likely
should be supported. There are differences between those monitors which `msigd` 
currently does not know about.
 
If you own an MSI monitor supported by the MSI App, please provide the following
information by opening an issue.

- The id of your monitor, e.g. MAG321CURV
- The output of `msigd --debug --info --query`
- For linux the output of `lsusb`
- Your operating system and version
- any other information which might be helpful, e.g. OSD setting xyz is not
  supported.


### 2.1. MSI Monitors

| ID            | Firmware | Supported     | Version | Special | Panel |
|:------------- |:-------- |:-------------:|:----- |:---- |:-------------- |
| MPG27CQ       | ?        | Yes           | "V18" | "001"| ?              |
| MPG341CQR     | ?        | ?             | ?     |   ?  | ?              |
| MPG341CQRV    | ?        | ?             | ?     |   ?  | ?              |
| MAG241C       | ?        | Yes           | "V18" | "002"| ?              |
| MAG241CP      | ?        | Yes           | "V18" | "002"| ?              |
| MAG241CR      | ?        | Yes           | "V18" | "004"| ?              |
| MAG241CV      | ?        | Yes           | "V18" | "002"| ?              |
| MAG251RX      | ?        | ?             | ?     |   ?  | ?              |
| MAG270CR      | ?        | ?             | ?     |   ?  | ?              |
| MAG271C       | ?        | ?             | "V18" | "002"| ?              |
| MAG271CR      | ?        | ?             | "V18" | "004"| ?              |
| MAG271CP      | ?        | ?             | "V18" | "002"| ?              |
| MAG271CQP     | ?        | ?             | "V19" | "006"| ?              | 
| MAG271CQR     | ?        | Yes           | "V19" | "006"| TPM270WQ1_DP01 |
| MAG271CV      | ?        | ?             | "V18" | "002"| ?              |
| MAG271QR      | ?        | ?             | ?     |   ?  | ?              |
| MAG271R       | ?        | ?             | ?     |   ?  | ?              |
| MAG271V       | ?        | ?             | ?     |   ?  | ?              |
| MAG272        | ?        | Yes           | "V18" | "00L"| ?              |
| MAG272C       | ?        | ?             | "V18" | "00O"| ?              |
| MAG272CQR     | ?        | Yes           | "V18" | "00E"| ?              |
| MAG272CR      | ?        | ?             | "V18" | "00O"| ?              |
| MAG272CRX     | ?        | ?             | "V18" | "00O"| ?              |
| MAG272QP      | ?        | Yes           | "V18" | "00O"| ?              |
| MAG272QR      | ?        | Partial 1)    | "V18" | "00G"| ?              |
| MAG272R       | ?        | ?             | "V18" | "00O"| ?              |
| MAG321CQR     | ?        | Yes           | "V18" | "00:"| ?              |
| MAG321CURV    | FW.009   | Yes           | "V18" | "00;"| SAM_LSM315FP01 |
| MAG322CQR     | ?        | ?             | ?     |   ?  | ?              |
| MAG322CQRV    | ?        | ?             | ?     |   ?  | ?              |
| MAG322CR      | ?        | ?             | ?     |   ?  | ?              |
| PS341WU       | FW.024   | Yes           | "V06" | "00?"| ?              |

1) Mystic support is not working. Has 12 lights - Steel series interface? 

### 2.2. Service menu

The panel information and more is displayed by the service menu. 

-	Hold the joystick button down
-	Unplug your monitor power supply - keep holding down
-	Plug in your monitor supply - keep holding down
-   When msi logo appears - stop holding down
-	When monitor displays screen - push joystick button down to show OSD
-   Push joystick button down again
-	Service menu opens

The service menu also has more information about the preset color
temperature modes. 

### 2.3. USB manufacturer and product id

```
ID 1462:3fa4 Micro Star International
```

## 3. Compile

### 3.1. Linux

Make sure you have libusb installed. On debian based systems

```sh
sudo apt install libusb-dev libhidapi-dev
```

On Fedora use

```sh
dnf install libusb-devel hidapi-devel
```

Compile with

```sh
make
```

For Arch-Linux the name of the usb library differs (usb-1.0). Please use

```sh
sudo pacman -S libusb hidapi
make TARGETOS=arch
```

to install dependencies and compile.

### 3.2. Windows

To compile on windows you need a working mingw environment.

Make sure you have libusb installed:

```sh
pacman -S mingw-w64-x86_64-libusb-compat-git
pacman -S mingw-w64-x86_64-hidapi
```

Compile 

```sh
make  TARGETOS=windows
```

### 3.3. OSX

Make sure you have [homebrew](https://brew.sh/) installed. 

```
brew install libusb-compat
brew install hidapi
```

Compile 

```sh
make TARGETOS=osx
```

## 4. Security

### 4.1. Linux
This program needs root privilidges. Use with care.

Alternatively you may use udev to grant user access rights. More information is
available on here: [Documentation on askubuntu](https://askubuntu.com/questions/978552/how-do-i-make-libusb-work-as-non-root)

In a nutshell: 

* Ensure that your system has a group `plugdev` and that the current user is a member of the `plugdev` group.

* Create `/etc/udev/rules.d/51-msi-gaming-device.rules`:

```
# Allow access to members of plugdev - both for usb and hidraw access
SUBSYSTEM=="usb", ATTR{idVendor}=="1462", ATTR{idProduct}=="3fa4", GROUP="plugdev", TAG+="uaccess"
KERNEL=="hidraw*", ATTRS{idVendor}=="1462", ATTRS{idProduct}=="3fa4", GROUP="plugdev", TAG+="uaccess"
```
* Execute

```sh
sudo udevadm control --reload-rules
```
* Turn your monitor off and on.

### 4.2. Windows

On Windows 7 `msigd` does need no additional user rights. It however conflicts
with OSD Gaming device software. 

You have two alternatives:

1.	Completely remove the MSI Gaming Device software to run the application.

2.	Use Task Manager and stop `MonitorMicroKeyDetector.exe`

### 4.3. OSX

On OSX no elevated user rights are needed.

### 4.4. Building with libusb

You can also use `make USE_HIDAPI=0` to build with `libusb` instead of the default `libhidapi`. 
This introduces issues on Windows and OSX builds and therefore is not recommended.

- For Windows administrator rights are needed.

- On OSX the usb interface will be claimed by the OSX HID driver. Basically this prevents
`msigd` to claim the usb interface. There is no easy way around this. More information
can be found here: [libusb FAQ](https://github.com/libusb/libusb/wiki/FAQ). 
A solution is described on [stackoverflow](https://stackoverflow.com/a/29610161).
This includes turning of security settings and thus I am not going to
pursue this further here. 

## 5. Usage

[`msgid --help`](https://htmlpreview.github.io/?https://raw.githubusercontent.com/couriersud/msigd/master/html/msigd.html)

```sh
Usage: msigd [OPTION]... 
Query or set monitor settings by usb directly on the device.
For supported devices please refer to the documentation.

Options are processed in the order they are given. You may
specify an option more than once with identical or different
values. After processing, mystic led settings are sent first
to the device. Afterwards all setting changes are sent to
the device. Once completed settings specified are queried.
The --wait option which will be executed last

In addition to preset modes the --mystic option also accepts numeric
values. 0xff0000 will set all leds to red. '0,255,0' will set all leds
to green.

All device settings provide which operations are possible:
R: Read, W: Write, RW: Read/Write


Options:

  -q, --query                display all monitor settings. This will also
                               list readonly settings and settings whose
                               function is currently unknown.
  -s, --serial               serial number of the monitor to control.
                               Use the serial number to identify the target
                               monitor in a multi-monitor environment
                               If omitted, the first monitor found is used
      --info                 display device information. This can be used
                               with --query
  -f, --filter               limits query result to comma separated list
                               of settings, e.g. -f contrast,gamma
  -w, --wait                 SETTING=VALUE. Wait for SETTING to become
                               VALUE, e.g. macro_key=pressed
  -n, --numeric              monitor settings are displayed as numeric
                                settings
       --mystic              off, static, breathing, blinking, flashing,
                               blinds, meteor, rainbow, random,
                               0xRRGGBB, RRR,GGG,BBB
                               Only on MAG series monitors.

Multi monitor support:
    On libHid systems use 'lsusb -v' to get the serial number
    of attached monitors.
    On libUSB systems use 'msgid --debug -s unknown` to get a list
    of attached monitors.


All monitors:
    These options apply to all monitors:

      --power                 W values: off 
      --macro_key             R values: off pressed 
      --serial                R values: 0 to 100
      --frequency             R values: 0 to 100
      --response_time        RW values: normal fast fastest 
      --alarm_clock          RW values: off 1 2 3 4 
      --alarm4x               W a1,a2,a3,a4,n where a<5999 and n<=4
      --eye_saver            RW values: off on 
      --image_enhancement    RW values: off weak medium strong strongest 
      --brightness           RW values: 0 to 100
      --contrast             RW values: 0 to 100
      --sharpness            RW values: 0 to 5
      --color_rgb            RW tripple: v1,v2,v3 where v<=100
      --unknown440            W values: off on 
      --osd_transparency     RW values: 0 to 5
      --osd_timeout          RW values: 0 to 30
      --reset                 W values: on 

MAG series monitors:
    These options apply to MAG272 and MAG32 monitors:

      --game_mode            RW values: user fps racing rts rpg 
      --enable_dynamic       RW values: on off 
      --hdcr                 RW values: off on 
      --refresh_display      RW values: off on 
      --refresh_position     RW values: left_top right_top left_bottom right_bottom 
      --alarm_clock_index    RW values: 1 to 4
      --alarm_clock_time     RW values: 0 to 5999
      --alarm_position       RW values: left_top right_top left_bottom right_bottom 
      --screen_assistance    RW values: off red1 red2 red3 red4 red5 red6 white1 white2 white3 white4 white5 white6 
      --color_preset         RW values: cool normal warm custom 
      --color_red            RW values: 0 to 100
      --color_green          RW values: 0 to 100
      --color_blue           RW values: 0 to 100
      --unknown435            R values: 0 to 100
      --osd_language         RW values: 0 to 19
      --sound_enable         RW values: off on 
      --rgb_led              RW values: off on 

MAG321CQR:
    These options apply to the MAG321CQR:

      --mode                 RW values: user fps racing rts rpg mode5 mode6 mode7 mode8 mode9 user reader cinema designer 
      --free_sync            RW values: off on 
      --zero_latency         RW values: off on 
      --screen_size          RW values: 19 24 4:3 16:9 
      --pro_mode             RW values: user reader cinema designer 
      --input                RW values: hdmi1 hdmi2 dp 
      --pip                  RW values: off pip pbp 
      --pip_input            RW values: hdmi1 hdmi2 dp 
      --pbp_input            RW values: hdmi1 hdmi2 dp 
      --pip_size             RW values: small medium large 
      --pip_position         RW values: left_top right_top left_bottom right_bottom 
      --toggle_display        W values: on 
      --toggle_sound          W values: on 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 

MAG32 Series:
    These options apply to the MAG32 Series:

      --mode                 RW values: user fps racing rts rpg mode5 mode6 mode7 mode8 mode9 user reader cinema designer 
      --unknown210            W values: 0 to 10
      --unknown210            R values: 0 to 10
      --unknown280            R values: 0 to 100
      --zero_latency         RW values: off on 
      --screen_size          RW values: 19 24 4:3 16:9 
      --night_vision         RW values: off normal strong strongest ai 
      --pro_mode             RW values: user reader cinema designer 
      --input                RW values: hdmi1 hdmi2 dp usbc 
      --pip                  RW values: off pip pbp 
      --pip_input            RW values: hdmi1 hdmi2 dp usbc 
      --pbp_input            RW values: hdmi1 hdmi2 dp usbc 
      --pip_size             RW values: small medium large 
      --pip_position         RW values: left_top right_top left_bottom right_bottom 
      --toggle_display        W values: on 
      --toggle_sound          W values: on 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 

MAG241 Series:
    These options apply to the MAG241 Series:

      --black_tuner          RW values: 0 to 20
      --free_sync            RW values: off on 
      --pro_mode             RW values: user reader cinema designer 
      --input                RW values: hdmi1 hdmi2 dp 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate 

MAG241 Series:
    These options apply to the MAG241 Series:

      --black_tuner          RW values: 0 to 20
      --free_sync            RW values: off on 
      --pro_mode             RW values: user reader cinema designer 
      --input                RW values: hdmi1 hdmi2 dp 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate 

MAG271CQ Series:
    These options apply to the MAG271CQ Series:

      --black_tuner          RW values: 0 to 20
      --free_sync            RW values: off on 
      --zero_latency         RW values: off on 
      --screen_size          RW values: 19 24 4:3 16:9 
      --pro_mode             RW values: user reader cinema designer 
      --input                RW values: hdmi1 hdmi2 dp 
      --pip                  RW values: off pip pbp 
      --pip_input            RW values: hdmi1 hdmi2 dp 
      --pbp_input            RW values: hdmi1 hdmi2 dp 
      --pip_size             RW values: small medium large 
      --pip_position         RW values: left_top right_top left_bottom right_bottom 
      --toggle_display        W values: on 
      --toggle_sound          W values: on 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 

MAG272 Series:
    These options apply to the MAG272 Series:

      --mode                 RW values: user fps racing rts rpg mode5 mode6 mode7 mode8 mode9 user reader cinema designer HDR 
      --unknown210            W values: 0 to 10
      --unknown210            R values: 0 to 10
      --free_sync            RW values: off on 
      --zero_latency         RW values: off on 
      --screen_size          RW values: auto 4:3 16:9 
      --night_vision         RW values: off normal strong strongest ai 
      --pro_mode             RW values: user reader cinema designer HDR 
      --input                RW values: hdmi1 hdmi2 dp usbc 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 

MAG272 Series:
    These options apply to the MAG272 Series:

      --mode                 RW values: user fps racing rts rpg mode5 mode6 mode7 mode8 mode9 user reader cinema designer HDR 
      --unknown210            W values: 0 to 10
      --unknown210            R values: 0 to 10
      --free_sync            RW values: off on 
      --zero_latency         RW values: off on 
      --screen_size          RW values: auto 4:3 16:9 
      --night_vision         RW values: off normal strong strongest ai 
      --pro_mode             RW values: user reader cinema designer HDR 
      --input                RW values: hdmi1 hdmi2 dp usbc 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 

MAG272 Series:
    These options apply to the MAG272 Series:

      --mode                 RW values: user fps racing rts rpg mode5 mode6 mode7 mode8 mode9 user reader cinema designer HDR 
      --unknown210            W values: 0 to 10
      --unknown210            R values: 0 to 10
      --free_sync            RW values: off on 
      --zero_latency         RW values: off on 
      --screen_size          RW values: auto 4:3 16:9 
      --night_vision         RW values: off normal strong strongest ai 
      --pro_mode             RW values: user reader cinema designer HDR 
      --input                RW values: hdmi1 hdmi2 dp usbc 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 

MPG27 Series:
    These options apply to the MPG27 Series:

      --black_tuner          RW values: 0 to 20
      --free_sync            RW values: off on 
      --zero_latency         RW values: off on 
      --screen_size          RW values: 19 24 4:3 16:9 
      --pro_mode             RW values: user reader cinema designer 
      --input                RW values: hdmi1 hdmi2 dp 
      --pip                  RW values: off pip pbp 
      --pip_input            RW values: hdmi1 hdmi2 dp 
      --pbp_input            RW values: hdmi1 hdmi2 dp 
      --pip_size             RW values: small medium large 
      --pip_position         RW values: left_top right_top left_bottom right_bottom 
      --toggle_display        W values: on 
      --toggle_sound          W values: on 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 

PS Series:
    These options apply to the PS Series:

      --mode                 RW values: user adobe_rgb dci_p3 srgb hdr cinema reader bw dicom eyecare cal1 cal2 cal3 
      --quick_charge          R values: off on 
      --unknown190            R values: 0 to 100
      --alarm_position       RW values: left_top right_top left_bottom right_bottom custom 
      --screen_assistance    RW values: off center edge scale_v scale_h line_v line_h grid thirds 3D_assistance 
      --screen_size          RW values: auto 4:3 16:9 21:9 1:1 
      --pro_mode             RW values: user adobe_rgb dci_p3 srgb hdr cinema reader bw dicom eyecare cal1 cal2 cal3 
      --color_preset         RW values: 5000K 5500K 6500K 7500K 9300K 10000K custom 
      --gray_level           RW values: 0 to 20
      --low_blue_light       RW values: off on 
      --local_dimming        RW values: off on 
      --hue_rgb              RW tripple: v1,v2,v3 where v<=100
      --hue_cmy              RW tripple: v1,v2,v3 where v<=100
      --zoom                 RW values: off on 
      --zoom_location        RW values: center left_top right_top left_bottom right_bottom 
      --saturation_rgb       RW tripple: v1,v2,v3 where v<=100
      --saturation_cmy       RW tripple: v1,v2,v3 where v<=100
      --gamma                RW values: 1.8 2 2.2 2.4 2.6 
      --input                RW values: hdmi1 hdmi2 dp usbc 
      --pip                  RW values: off pip pbp_x2 pbp_x3 pbp_x4 
      --pip_input            RW values: hdmi1 hdmi2 dp usbc 
      --pip_size             RW values: small medium large 
      --pip_position         RW values: left_top right_top left_bottom right_bottom 
      --toggle_display        W values: on 
      --pip_sound_source     RW values: hdmi1 hdmi2 dp usbc 
      --pbp_input1           RW values: hdmi1 hdmi2 dp usbc 
      --pbp_input2           RW values: hdmi1 hdmi2 dp usbc 
      --pbp_input3           RW values: hdmi1 hdmi2 dp usbc 
      --pbp_input4           RW values: hdmi1 hdmi2 dp usbc 
      --pbp_sound_source     RW values: hdmi1 hdmi2 dp usbc 
      --osd_language         RW values: 0 to 28
      --screen_info          RW values: off on 
      --audio_source         RW values: analog digital 
      --navi_up              RW values: off brightness pro_mode screen_assistance alarm_clock input pip zoom_in info 
      --navi_down            RW values: off brightness pro_mode screen_assistance alarm_clock input pip zoom_in info 
      --navi_left            RW values: off brightness pro_mode screen_assistance alarm_clock input pip zoom_in info 
      --navi_right           RW values: off brightness pro_mode screen_assistance alarm_clock input pip zoom_in info 

General options:
    These options always apply:

  -d, --debug                enable debug output
                               Enables raw output for query command
  -h, --help                 display this help and exit
      --version              output version information and exit

Exit status:
 0  if OK,
 1  if error during option parsing,
 2  if error during device identification,
 3  if error during setting parameters on device,
 4  if error during reading parameters from device,

Report bugs on <https://github.com/couriersud/msigd/issues>
msigd home page: <https://github.com/couriersud/msigd>
```

You may also use the following to display man style help

```sh
help2man --include=msigd.help2man --no-info ./msigd | nroff -man |less
```
## 6. Settings not supported

### 6.1. MAG241 series

#### 6.1.1. Screen size

Although documented in the manual setting the screen size (4:3, 16:9) is not 
supported. The setting will kill the usb interface and the monitor needs a 
plug/unplug power cable cycle.

### 6.2. PS341 series

#### 6.2.1. Quick charge

Changing the quick charge setting is not supported. We were not able to identify
how to enable or disable quick charge programmatically. The status however 
can be queried.

#### 6.2.2. Eye saver

`--eye_saver` works with the modes `user`, `hdr`, `cinema`, `reader` and `bw`.
In mode `eyecare` it is always reported as `on` while in modes `adobe_rgb`, 
`dci_p3`, `srgb` and `dicom` it is always `off`.

## 7. Examples

### 7.1. Automatically switch input source

I have a usb swiched port. Upon pressing a button on the switch, keyboard and 
mouse (or up to four devices) are switched between two computers: my desktop and
my laptop. There is a USB-C docking station connected to the switch. The laptop
is connected via USB-C. The docking station is connected to the USB-C hub and via
HDMI to input ```hdmi1``` to the monitor.

The following script runs on the desktop. If the keyboard gets disconnect - switched
to laptop, it will switch monitor input to ```hdmi1```. Once the keyboard 
reconnects, it will switch back monitor input to ```dp```.

The `inotifywait` tool needed by this script is available on debian systems. 
Use `sudo apt install inotify-tools` to install it.

```sh
#!/bin/sh

WATCH_DIR=/dev/input/by-id
WATCH_INPUT=usb-046a_010d-event-kbd
DISP_INPUT=dp
DISP_ALTERNATIVE=hdmi1

inotifywait -q -m ${WATCH_DIR} | while read event
	do
		f=`echo $event | cut -f 3 "-d "`
		if [ _$f = _${WATCH_INPUT} ]; then
			ev=`echo $event | cut -f 2 "-d "`
			test _$ev = _DELETE && ./msigd --input $DISP_ALTERNATIVE
			test _$ev = _CREATE && ./msigd --input $DISP_INPUT
		fi
	done
```

### 7.2. Change settings depending on foreground window

The script below is an example on how to change display settings automatically
depending on the active application. When watching movies, you may want to use
image enhancement while when using the terminal you may prefer a reduced 
brightness.

This script is an example what you can do using `msigd`:

```sh
#!/bin/sh

DISP_DEFAULT='--brightness 80 --eye_saver off --image_enhancement off'
MSIGD=./msigd

if [ ! -x "$(command -v xdotool)" ]; then
	echo Error: xdotool required >&2
	exit 1
fi

setting=""
while true; do
	# Get foreground window commandline
	vwin=`xdotool getwindowfocus`
	vpid=`xdotool getwindowpid $vwin 2>/dev/null`
	if [ "$vpid" != "" ]; then
		cl=`cat /proc/${vpid}/cmdline | sed -e "s/\\x0/ /g"`
	else
		cl=`xdotool getwindowname $vwin`
	fi
	case "$cl" in
		*gnome-terminal-server*)
			nsetting='--brightness 30'
			;;
		*eclipse*)
			nsetting='--eye_saver on'
			;;
		*xine*)
			nsetting='--image_enhancement strong'
			;;
		*)
			nsetting=''
			;;
	esac 
	if [ "$nsetting" != "$setting" ]; then
		$MSIGD $DISP_DEFAULT $nsetting
		setting="$nsetting"
	fi
	sleep 1
done
```

### 7.3. Using msigd in a libvirt-qemu hook

[mike-vivas](https://github.com/mike-vivas) contributed an example of using
msigd within a [libvirt-qemu](contrib/qemu) hook to automatically switch the monitor to
virtual guests booting up.


## 8. TODO

- Code cleanup
- Support more monitors - depends on user contributions

## 9. Credits

[John Wehin](https://github.com/Wehin) - PS341WU support and bug fixing

[Daniel Connolly](https://github.com/cosmicdan) - MAG271CQR support

[Maxime](https://github.com/intello21) - MAG241C support

[mike-vivas](https://github.com/mike-vivas) - Arch linux support, `contrib`

[gbirchley](https://github.com/gbirchley) - MAG272 support

[Térence Clastres](https://github.com/terencode) - MPG27CQ support and steel series LED support

[elric1789](https://github.com/elric1789) - MAG272QP support

[andaag](https://github.com/andaag) - MAG272CQR support

[usrErr0r](https://github.com/usrErr0r) - MAG272QR support
