# 1. msigd

The `msigd` command line tool allows you to change most settings for MSI monitors which can be set in the monitor's OSD menu.

**Warning: Use `msigd` only if you are sure what you are doing.** 
**The monitor firmware seems to have no protection against unsupported accesses.**

**Using `msigd` may make your monitor permantently unusable.**

**Please make sure you use at least msigd version 0.17**

With version 0.12 the syntax of the "--mystic" option has changed. For the 
MPG273CQR two led groups (0 and 1) are supported. For all other mystic light
monitors only group 1 is supported.

<!-- TOC depthFrom:2 depthTo:3 orderedList:false -->

- [1. Build status:](#1-build-status)
- [2. Supported monitors](#2-supported-monitors)
    - [2.1. MSI Monitors](#21-msi-monitors)
    - [2.2. Service menu](#22-service-menu)
    - [2.3. USB manufacturer and product id](#23-usb-manufacturer-and-product-id)
- [3. Compile](#3-compile)
    - [3.1. Linux](#31-linux)
    - [3.2. WSL](#32-wsl)
    - [3.3. Windows](#33-windows)
    - [3.4. OSX](#34-osx)
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

| ID            | Firmware | Supported     | Version | Special | Panel | Manual |
|:------------- |:-------- |:-------------:|:----- |:---- |:-------------- |-------|
| MPG27CQ       | ?        | Yes           | "V18" | "001"| ?              | |
| MAG241C       | ?        | Yes           | "V18" | "002"| ?              |[Manual](https://download.msi.com/archive/mnu_exe/monitor/MAG241C_CP_CR_CV_271C_CP_CR_CVv1.0_English.pdf)|
| MAG241C       | ?        | Yes           | "V49" | "002"| ?              | |
| MAG241CP      | ?        | Yes           | "V18" | "002"| ?              |[Manual](https://download.msi.com/archive/mnu_exe/monitor/MAG241C_CP_CR_CV_271C_CP_CR_CVv1.0_English.pdf)|
| MAG241CV      | ?        | Yes           | "V18" | "002"| ?              |[Manual](https://download.msi.com/archive/mnu_exe/monitor/MAG241C_CP_CR_CV_271C_CP_CR_CVv1.0_English.pdf)|
| MAG241CR      | ?        | Yes           | "V18" | "004"| ?              |[Manual](https://download.msi.com/archive/mnu_exe/monitor/MAG241C_CP_CR_CV_271C_CP_CR_CVv1.0_English.pdf)|
| MAG271CR      | ?        | Yes           | "V18" | "005"| ?              |[Manual](https://download.msi.com/archive/mnu_exe/monitor/MAG241C_CP_CR_CV_271C_CP_CR_CVv1.0_English.pdf)|
| MAG271CQR     | ?        | Yes           | "V19" | "006"| TPM270WQ1_DP01 | |
| MAG321CQR     | ?        | Yes           | "V18" | "00:"| ?              | |
| MAG321CURV    | FW.009   | Yes           | "V18" | "00;"| SAM_LSM315FP01 |[Manual](https://download.msi.com/archive/mnu_exe/monitor/MAG321CURV_322CQRVv1.0_English.pdf)|
| MAG321CURV    | FW.011   | Yes           | "V43" | "00;"| ?              |[Manual](https://download.msi.com/archive/mnu_exe/monitor/MAG321CURV_322CQRVv1.0_English.pdf)|
| MPG341CQR     | ?        | WIP           | "V09" | "00>"| ?              |[Manual](https://download.msi.com/archive/mnu_exe/monitor/MPG341CQRv1.0_English.pdf)|
| PS341WU       | FW.024   | Yes           | "V06" | "00?"| ?              | |
| MPG273CQR     | FW.022   | Yes           | "V51" | "00["| ?              |[Manual](https://download.msi.com/archive/mnu_exe/monitor/MPG_ARTYMIS_273CQR_MAG_ARTYMIS_274CPv1.0_English.pdf)|
| MAG251RX      | ?        | WIP           | "V18" | "00B"| ?              |[Manual](https://download.msi.com/archive/mnu_exe/monitor/MAG251RXv1.0_English.pdf)|
| MAG272CQR     | ?        | Yes           | "V18" | "00E"| ?              |[Manual](https://download.msi.com/archive/mnu_exe/monitor/MAG272C_CR_CQP_CQR_CRX_272_R_QP_QRv1.0_English.pdf)|
| MAG272QR      | ?        | Partial 1)    | "V18" | "00G"| ?              |[Manual](https://download.msi.com/archive/mnu_exe/monitor/MAG272C_CR_CQP_CQR_CRX_272_R_QP_QRv1.0_English.pdf)|
| MAG272        | ?        | Yes           | "V18" | "00L"| ?              |[Manual](https://download.msi.com/archive/mnu_exe/monitor/MAG272C_CR_CQP_CQR_CRX_272_R_QP_QRv1.0_English.pdf)|
| MAG272QP      | ?        | Yes           | "V18" | "00O"| ?              |[Manual](https://download.msi.com/archive/mnu_exe/monitor/MAG272C_CR_CQP_CQR_CRX_272_R_QP_QRv1.0_English.pdf)|
| MAG274R       | ?        | Yes           | "V41" | "00Z"| ?              |[Manual](https://download.msi.com/archive/mnu_exe/monitor/Optix_MAG274_274Rv1.0_English.pdf)|
| MAG274QRF-QD  | FW.011   | Yes           | "V43" | "00e"| AUO_M270DAN08_2| |
| MAG274QRF-QD  | FW.015   | Yes           | "V48" | "00e"| AUO_M270DAN08_2| |
| MAG274QRF-QD  | FW.020   | Yes           | "V56" | "00e"| AUO_M270DAN08_2|[Manual](https://download.msi.com/archive/mnu_exe/monitor/Optix_MAG274QRF_MAG274QRF-QDv1.0_English.pdf)|
| MAG274QRX     | ?        | ?             | "V53" | "00\|"| ?              |[Manual](https://download.msi.com/archive/mnu_exe/monitor/Optix_MAG274QRXv1.0_English.pdf)|
| MAG270CR      | ?        | ?             | ?     |   ?  | ?              | |
| MAG271C       | ?        | ?             | "V18" | "002"| ?              |[Manual](https://download.msi.com/archive/mnu_exe/monitor/MAG241C_CP_CR_CV_271C_CP_CR_CVv1.0_English.pdf)|
| MAG271CP      | ?        | ?             | "V18" | "002"| ?              |[Manual](https://download.msi.com/archive/mnu_exe/monitor/MAG241C_CP_CR_CV_271C_CP_CR_CVv1.0_English.pdf)|
| MAG271CQP     | ?        | ?             | "V19" | "006"| ?              | |
| MAG271CV      | ?        | ?             | "V18" | "002"| ?              |[Manual](https://download.msi.com/archive/mnu_exe/monitor/MAG241C_CP_CR_CV_271C_CP_CR_CVv1.0_English.pdf)|
| MAG271QR      | ?        | ?             | ?     |   ?  | ?              | |
| MAG271R       | ?        | ?             | ?     |   ?  | ?              | |
| MAG271V       | ?        | ?             | ?     |   ?  | ?              | |
| MAG272C       | ?        | ?             | "V18" | "00O"| ?              | |
| MAG272CR      | ?        | ?             | "V18" | "00O"| ?              | |
| MAG272CRX     | ?        | ?             | "V18" | "00O"| ?              | |
| MAG272R       | ?        | ?             | "V18" | "00O"| ?              | |
| MAG322CQR     | ?        | ?             | ?     |   ?  | ?              | |
| MAG322CQRV    | ?        | ?             | ?     |   ?  | ?              | |
| MAG322CR      | ?        | ?             | ?     |   ?  | ?              | |
| MPG341CQRV    | ?        | ?             | ?     |   ?  | ?              | |
| MD272QP       | ?        | WIP           | "V51" | "00\\x85" | ?              | |

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

### 3.2. WSL

Warning: you need Windows Subsystem for Linux 2 with a kernel version of at least 5.10.60.1

Make sure you have libusb installed. On debian based systems

```sh
sudo apt install libusb-dev libhidapi-dev
```

Compile with libusb

```sh
make USE_HIDAPI=0
```

On Windows, install usbipd-win as described [here](https://devblogs.microsoft.com/commandline/connecting-usb-devices-to-wsl/).

You need to attach two usb devices to WSL. You can either find them in Device Manager or by checking `usbpid wsl list` before and after pluggin in the monitor.

**Drawback:** Attached usb devices are not saved and need to be reattached every time the monitor is turned on.

Perform the steps of [4.1. Linux](#41-linux).

### 3.3. Windows

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

### 3.4. OSX

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

[`msigd --help`](md/msigd_help.md)

[`man msigd`](https://htmlpreview.github.io/?https://raw.githubusercontent.com/couriersud/msigd/master/html/msigd.html)

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
- Document `--mystic` option better
- Match options of `WIP` monitors against manuals.
- Provide link to monitor manual in table.

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

[glaon](https://github.com/glaon) - MPG341CQR support

[Marco Rodolfi](https://github.com/RodoMa92) - MAG274QRF-QD support

[Preston](https://github.com/PChild) - Multi-Monitor support

[Michael J Brancato](https://github.com/sgtcoder) - Multi-Monitor support

[Dominik Helfenstein](https://github.com/dippa-1) - WSL compile instructions

[Stephen Lee](https://github.com/sl33nyc) -- MAG321QR support

[Thorou](https://github.com/thorio) - MAG271CR support

[Skyyblaze](https://github.com/Skyyblaze) - MAG241CR support

[Deathof1](https://github.com/Deathof1) - MAG321CQR support

[Maxime](https://github.com/intello21) - MAG241C support

[Sahil Gupte](https://github.com/Ovenoboyo) - MAG241C V49 support

[Pontus Jensen Karlsson](https://github.com/wolfhechel) - MAG274R support

[kunver400](https://github.com/kunver400) - MAG274QRX support

