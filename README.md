# arduino_oregon_sci_rx
Oregon Scientific 433MHz OOK receiver and demodulator for use with the Oregon Scientific PCR800 rain guage.

## Arduino
Arduino code originally came from: https://jeelabs.net/projects/cafe/wiki/Decoding_the_Oregon_Scientific_V2_protocol   
Modified to use [ATMEGA328 Arduino Pro Mini](https://www.digikey.com/product-detail/en/sparkfun-electronics/DEV-11114/1568-1054-ND) and the RF Solutions [QAM-RX10-433](https://www.digikey.com/product-detail/en/rf-solutions/QAM-RX10-433/QAM-RX10-433-ND/) demodulator.

Arduino Pin 2 connects to the 433 MHz RX digital output.

## Beaglebone
The Arduino and the demodulator are mounted on a Beaglebone proto cape.  This is connected to a BBB that is already being used for other purposes.  RX/TX connects to BBB UART4.  Reset connects to BBB header pin P9_15 (GPIO_48).

The Python script runs on the BBB and logs the rain guage measurements to a file.

![Board photo](photo.jpg?raw=true "Board photo")

## Output
Console outputs some data on every packet:
```
OSV3 2A190453000090572980  
SUCCESS: PCR800: TS: 1551051846.209447, CODE: 53, RATE: 0.000, TOTAL: 29.579, CSUM: 0x8, COMPUTED: 0x8  
RAIN 29.58 0.00  
CRES 5618EA010060F1F700E501  
HEZ 04FE3C0040C203  
```

Logfile is concise:
```
Timestamp, rolling code, rain total (in), rain rate (in/hr):
1551050812.230 53 29.579 0.000
1551050859.229 53 29.579 0.000
```

My PCR800 generates a packet every 47 seconds.  When things aren't changing the code will skip logging most of these packets.  This is controlled by the variables SKIP_TRIGGER and SKIP_COUNT
```
SKIP_TRIGGER = 3                # Skip logging after this many identical measurements
SKIP_COUNT = 919                # While skipping, only log one out of every SKIP_COUNT messages
```
