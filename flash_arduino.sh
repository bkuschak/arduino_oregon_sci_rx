#!/bin/bash
#
# In the Arduino GUI run Sketch -> Export Compiled Binary.
# Copy the .hex file to this directory.
# Be sure to kill any running python rain_gauge.py process first.
# Run this script to relash the Arduino.
#

FILENAME=arduino_oregon_sci_rx.ino.eightanaloginputs.hex

ARDUINO_RESET_PIN=P9_15
PORT=/dev/ttyO4
BAUD=57600

# Reset
config-pin $ARDUINO_RESET_PIN low
sleep 0.1
config-pin $ARDUINO_RESET_PIN high
#sleep 0.5

# Flash it
avrdude -v -p atmega328p -c arduino -P $PORT -b $BAUD -D -U flash:w:$FILENAME:i

