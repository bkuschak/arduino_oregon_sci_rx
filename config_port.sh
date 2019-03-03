#!/bin/bash
# Enable UART4 if necessary

/usr/local/bin/config-pin P9.11 uart
/usr/local/bin/config-pin -q P9.11
/usr/local/bin/config-pin P9.13 uart
/usr/local/bin/config-pin -q P9.13
exit 0
