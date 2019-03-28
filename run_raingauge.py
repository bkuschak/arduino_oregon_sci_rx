#!/bin/bash
cd /root/git/arduino_oregon_sci_rx && config-pin overlay cape-universaln && ./config_port.sh && python rain_gauge.py > /dev/null 2>&1  &

