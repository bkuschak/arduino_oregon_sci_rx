#!/bin/sh
# Enable UART4 if necessary

sudo config-pin P9.11 uart
sudo config-pin -q P9.11
sudo config-pin P9.13 uart
sudo config-pin -q P9.13
