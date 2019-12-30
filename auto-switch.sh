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
