# msigd

The `msigd` command line tools allows you to change all settings for MSI monitors which can be set in the monitor's OSD menu. 

<!-- TOC depthFrom:2 orderedList:false -->

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
- [6. Examples](#6-examples)
    - [6.1. Automatically switch input source](#61-automatically-switch-input-source)
    - [6.2. Change settings depending on foreground window](#62-change-settings-depending-on-foreground-window)
- [7. TODO](#7-todo)

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

| ID            | Firmware | Supported     | Version output| Special | Panel |
|:------------- |:-------- |:-------------:|:----- |:---- |:-------------- |
| MAG272QP      | ?        | ?             | ?     |   ?  | ?              |
| MAG271CQP     | ?        | ?             | ?     |   ?  | ?              | 
| MPG27CQ       | ?        | ?             | ?     |   ?  | ?              |
| MAG271R       | ?        | ?             | ?     |   ?  | ?              |
| MAG272R       | ?        | ?             | ?     |   ?  | ?              |
| MAG270CR      | ?        | ?             | ?     |   ?  | ?              |
| MAG241CR      | ?        | ?             | ?     |   ?  | ?              |
| MAG271CR      | ?        | ?             | ?     |   ?  | ?              |
| MAG322CR      | ?        | ?             | ?     |   ?  | ?              |
| MAG272CR      | ?        | ?             | ?     |   ?  | ?              |
| MAG271QR      | ?        | ?             | ?     |   ?  | ?              |
| MAG272QR      | ?        | ?             | ?     |   ?  | ?              |
| MAG321CQR     | ?        | ?             | ?     |   ?  | ?              |
| MPG341CQR     | ?        | ?             | ?     |   ?  | ?              |
| MAG271CQR     | ?        | ?             | ?     |   ?  | ?              |
| MAG322CQR     | ?        | ?             | ?     |   ?  | ?              |
| MAG272CQR     | ?        | ?             | ?     |   ?  | ?              |
| MAG271V       | ?        | ?             | ?     |   ?  | ?              |
| MPG341CQRV    | ?        | ?             | ?     |   ?  | ?              |
| MAG322CQRV    | ?        | ?             | ?     |   ?  | ?              |
| MAG321CURV    | V009     | Y             | "V18" | "00;"| SAM_LSM315FP01 |
| MAG251RX      | ?        | ?             | ?     |   ?  | ?              |
| MAG272CRX     | ?        | ?             | ?     |   ?  | ?              |
| PS341WU       | ?        | ?             | "V06" | "00?"  | ?              |

### 2.2. Service menu

The panel information and more is displayed by the service menu. 

-	Hold the joystick button down
-	Unplug your monitor power supply - keep holding down
-	Plug in your monitor supply - keep holding down
-   When msi logo appears - stop holding down
-	When monitor displays screen - push joystick button down to show OSD
-   Push joystick button down again
-	Service menu opens

The service menu also has more information about the preset color temperature
modes. 

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
```sh
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
      --mystic               off, static, breathing, blinking, flashing, 
                               blinds, meteor, rainbow, random, 
                               0xRRGGBB, RRR,GGG,BBB

All monitors:

      --power                values: off on 
      --response_time        values: normal fast fastest 
      --alarm_clock          values: off 1 2 3 4 
      --alarm_clock_index    values: 1 to 4
      --alarm_clock_time     values: 0 to 5999
      --screen_assistance    values: 0 to 12
      --eye_saver            values: off on 
      --image_enhancement    values: off weak medium strong strongest 
      --brightness           values: 0 to 100
      --contrast             values: 0 to 100
      --sharpness            values: 0 to 5
      --rgb                  values: 0 to 100000000
      --unknown10            values: off on 
      --input                values: hdmi1 hdmi2 dp usbc 
      --pbp_input            values: hdmi1 hdmi2 dp usbc 
      --pip_size             values: small medium large 
      --pip_position         values: left_top right_top left_bottom right_bottom 
      --toggle_display       values: off on 
      --toggle_sound         values: off on 
      --osd_language         values: 0 to 19
      --osd_transparency     values: 0 to 5
      --osd_timeout          values: 0 to 30
      --reset                values: off on 

MAG Series:

      --mode                 values: user fps racing rts rpg mode5 mode6 mode7 mode8 mode9 user reader cinema designer 
      --game_mode            values: user fps racing rts rpg 
      --unknown06            values: 0 to 100
      --enable_dynamic       values: on off 
      --hdcr                 values: off on 
      --refresh_rate_display values: off on 
      --refresh_rate_position values: left_top right_top left_bottom right_bottom 
      --alarm_clock_position values: left_top right_top left_bottom right_bottom 
      --zero_latency         values: off on 
      --screen_size          values: 19 24 4:3 16:9 
      --night_vision         values: off normal strong strongest ai 
      --pro_mode             values: user reader cinema designer 
      --color_preset         values: cool normal warm custom 
      --red                  values: 0 to 100
      --green                values: 0 to 100
      --blue                 values: 0 to 100
      --pip                  values: off pip pbp 
      --pip_input            values: hdmi1 hdmi2 dp usbc 
      --sound_enable         values: off on 
      --back_rgb             values: off on 
      --navi_up              values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_down            values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_left            values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_right           values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 

PS Series:

      --alarm_clock_position values: left_top right_top left_bottom right_bottom custom 
      --screen_size          values: auto 4:3 16:9 21:9 1:1 
      --pro_mode             values: user adobe_rgb dcpi_p3 srgb hdr cinema reader bw dicom eyecare cal1 cal2 cal3 
      --color_preset         values: 5000K 5500K 6500K 7500K 9300K 10000K custom 
      --pip                  values: off pip pbp_x2 pbp_x3 pbp_x4 
      --quick_charge         values: off on 
      --audio_source         values: analog digital 
      --navi_up              values: off brightness pro_mode screen_assistance alarm_clock input pip zoom_in info 
      --navi_down            values: off brightness pro_mode screen_assistance alarm_clock input pip zoom_in info 
      --navi_left            values: off brightness pro_mode screen_assistance alarm_clock input pip zoom_in info 
      --navi_right           values: off brightness pro_mode screen_assistance alarm_clock input pip zoom_in info 

  -d, --debug                enable debug output
  -h, --help                 display this help and exit
      --version              output version information and exit

Options are processed in the order they are given. You may specify an option
more than once with identical or different values.

In addition to preset modes the --mystic option also accepts numeric
values. 0xff0000 will set all leds to red. '0,255,0' will set all leds
to green.

Exit status:
 0  if OK,
 1  if error during option parsing,
 2  if error during device access,

Report bugs on <https://github.com/couriersud/msigd/issues>
msigd home page: <https://github.com/couriersud/msigd>
```
You may also use the following to display man style help

```sh
help2man --include=msigd.help2man --no-info ./msigd | nroff -man |less
```

## 6. Examples

### 6.1. Automatically switch input source

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

### 6.2. Change settings depending on foreground window

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

## 7. TODO

- Code cleanup
- Support more monitors - depends on user contributions
