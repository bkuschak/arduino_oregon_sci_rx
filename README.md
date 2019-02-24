# arduino_oregon_sci_rx
Oregon Scientific 433MHz OOK receiver and demodulator

Originally from: https://jeelabs.net/projects/cafe/wiki/Decoding_the_Oregon_Scientific_V2_protocol  
Modified to use [ATMEGA328 Arduino Pro Mini](https://www.digikey.com/product-detail/en/sparkfun-electronics/DEV-11114/1568-1054-ND) and RF Solutions [QAM-RX10-433](https://www.digikey.com/product-detail/en/rf-solutions/QAM-RX10-433/QAM-RX10-433-ND/)

Arduino Pin 2 connects to the 433 MHz RX digital output.

Mounted on a BBB proto cape.  RX/TX connects to BBB UART4.  Reset connects to BBB header pin P9_15 (GPIO_48).
