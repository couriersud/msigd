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
