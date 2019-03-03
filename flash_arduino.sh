#!/bin/bash
#
# In the Arduino GUI run Sketch -> Export Compiled Binary
# Copy the .hex file to this directory
# Run this script to relash the Arduino
#

FILENAME=arduino_oregon_sci_rx.ino.eightanaloginputs.hex

ARDUINO_RESET_PIN=P9_15
PORT=/dev/ttyO4
#BAUD=115200
BAUD=57600
#BAUD=38400

# Reset
config-pin $ARDUINO_RESET_PIN low
sleep 0.1
config-pin $ARDUINO_RESET_PIN high
sleep 0.5

# Flash it
avrdude -v -p atmega328p -c arduino -P $PORT -b $BAUD -D -U flash:w:$FILENAME:i

#avrdude -C/Applications/Arduino.app/Contents/Java/hardware/tools/avr/etc/avrdude.conf -v -patmega328p -carduino -P/dev/cu.usbserial-FT0J5BG5 -b57600 -D -Uflash:w:/var/folders/bm/dxwbvqr97ql579pw84_3zc800000gn/T/arduino_build_512124/arduino_oregon_sci_rx.ino.hex:i
