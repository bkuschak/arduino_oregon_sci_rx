#!/bin/sh
ARDUINO_RESET_PIN=P9_15
PORT=/dev/ttyO4
BAUD=57600

# Reset
config-pin $ARDUINO_RESET_PIN low
sleep 0.1
config-pin $ARDUINO_RESET_PIN high
