# The Arduino is connected to UART4.  It receives and demodulates 433 MHz packets
# from the rain guage.  Decode them here and save results to log file.
#
# Prior to running this code must expose the UART4 port:
#   sudo config-pin P9.11 uart
#   sudo config-pin -q P9.11
#   sudo config-pin P9.13 uart
#   sudo config-pin -q P9.13

import time
import serial
import Adafruit_BBIO.GPIO as GPIO
import re
import sys
import binascii

FILENAME = "rain_gauge.log"
ARDUINO_RESET_PIN = "P9_15"


# Given an array of data, return the nibble at idx.  Our data is nibble-swapped, so unswap them.
def nib(data, idx):
	bi = idx / 2
	ni = (idx) % 2	
	if ni == 0:
		return int(data[bi] & 0xF);
	else:
		return int((data[bi]>>4) & 0xF);

# Decode the string of hex digits
# Return (0, rolling_code, rain_total, rain_rate) on success, (-1, ..., ...) on checksum failure
def decode_osv3(datastr, ts):

	# Convert to byte array
	data = bytearray(binascii.unhexlify(datastr))

	# print the nibbles
	#for i in range(len(data)*2):
		#print "%x" % (nib(data, i))

	# Parse fields for PCR800 packet
	sensor_id = nib(data, 1)<<12 | nib(data, 2)<<8 | nib(data, 3)<<4 | nib(data, 4)
	#print "Sensor ID: %x" % (sensor_id)
	if sensor_id == 0x2914:		# PCR800
		rolling_code = nib(data, 6)<<4 | nib(data, 7)
		rain_rate = 	nib(data, 12)*10000 + \
				nib(data, 11)*1000 + \
				nib(data, 10)*100 + \
				nib(data, 9)*10 + \
				nib(data, 8)
		rain_rate = float(rain_rate) / 1000
		rain_total  = 	nib(data, 18)*100000 + \
				nib(data, 17)*10000 + \
				nib(data, 16)*1000 + \
				nib(data, 15)*100 + \
				nib(data, 14)*10 + \
				nib(data, 13)
		rain_total = float(rain_total) / 1000
		csum = nib(data, 19)

		# Checksum is just a sum (not inverted) of the nibbles from 1 to n-1. 
		# Must match the value in nibble 19
		cs = 0
		for i in range(1, (len(data)*2)-1):
			cs = cs + nib(data, i)
		cs = cs & 0xF
		
		if cs == csum:
			print "SUCCESS: PCR800: TS: %f, CODE: %d, RATE: %.3f, TOTAL: %.3f, CSUM: 0x%x, COMPUTED: 0x%x" % (ts, rolling_code, rain_rate, rain_total, csum, cs)
			return (0, rolling_code, rain_total, rain_rate)
		else:
			print "CHECKSUM ERROR: TS: %f, RC: %d, RATE: %.3f, TOTAL: %.3f, CSUM: 0x%x, COMPUTED: 0x%x" % (ts, rolling_code, rain_rate, rain_total, csum, cs)
			return (-1, rolling_code, rain_total, rain_rate)
	else:
		return (-2, 0, 0)	# unrecognized sensor

		

# This doesn't work, so use config-pin as noted above
#import Adafruit_BBIO.UART as UART
#UART.setup("UART4")
 
# Serial comms
ser = serial.Serial(port = "/dev/ttyO4", baudrate=115200)
ser.close()
ser.open()
if ser.isOpen():
	print "Serial ported opened.  Resetting Arduino..."

	# Reset Arduino - active low
	GPIO.setup(ARDUINO_RESET_PIN, GPIO.OUT)
	GPIO.output(ARDUINO_RESET_PIN, GPIO.LOW)
	time.sleep(0.5)
	GPIO.output(ARDUINO_RESET_PIN, GPIO.HIGH)

	# log to file
	with open(FILENAME, "a+") as f:
		while True:
			line = ser.readline()
			t = time.time()
			#line = 'OSV3 2A190453313473282920\n'

			# discard blank lines
			if not re.match(r'^\s*$', line):
				sys.stdout.write(line)

				# Match the expected format
				if re.match(r'^OSV3 [0-9A-F]{20}\s*$', line):
					#print "Got properly formatted OSV3 msg", t
					data = re.sub(r'^OSV3 ', '', line)	
					data = re.sub(r'\s*$', '', data)
					(ret, code, total, rate) = decode_osv3(data, t)
					if ret == 0:
						f.write("%f %d %.3f %.3f\n" % (t, code, total, rate))
					
				
ser.close()


'''
Some examples
OSV3 2A1904530000001629A0
RAIN 29.16 0.00
OSV3 2A190453313473282920
RAIN 29.29 33.43
OSV3 2A190453001480322950
RAIN 29.33 1.40
OSV3 2A1904537009003729D0
RAIN 29.37 0.97
OSV3 2A1904532020204129E0
RAIN 29.41 2.02
OSV3 2A190453203430452980
RAIN 29.45 3.42
OSV3 2A190453309870532960
RAIN 29.54 9.83
OSV3 2A190453204790572950
RAIN 29.58 4.72
OSV3 2A190453204790572950
'''
