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
