# Hardware Variants


## pzPowerWifi rev3

### 5 position screw terminal 

Pin | Function | Note
---|---|---
1|POWER|7 to 36 volts DC
2|COM|Common with power and RS-485
3|RS485 A|
4|RS485 B|
5|COM|Common with power and RS-485

### JST1 expansion connector

Pin|Function|Harness Color|Note
---|---|---|---
1|VIN|Purple|Supply voltage to pzPowerI2C board
2|+5VDC|Orange|5 volt output from pzPowerI2C board
3|+3.3VDC|Red|3.3 volt output from pzPowerI2C or Pi Zero
4|I2C SDA|Blue|
5|I2C SCL|Yellow|
6|COM|BLACK|

### RS-485 port
The board has an RS-485 port that is connected to the UART on the Pi. Half duplex (2-wire) RS-485 requires a transmit enable signal. GPIO 4 is used for this. The software communicating on the RS-485 must have direction control support capable of toggling this GPIO.

We have added support to mbusd for direction control using the linux sysfs interface. An example script for setting up direction control pin and starting
mbusd can be found in the [support](support/) directory.

Note that different Pi variants have different serial port names. `/dev/ttyAMA0` was traditionally the Pi's external serial port. But on Bluetooth enabled
Pi's, the external serial port usually becomes `/dev/ttyS0`


## pzPower Ethernet(ENC) Passive POE

### IO

### Magnetic Switch
AH180N

### NTC thermistor
10k NTC thermistor as bottom half of 10k voltage divider.

### Pi power switch
TPS27082L power switch controls 5 volt power to Pi.

### Status LEDS


### Ethernet
A Microchip ENC28J60 on Pi SPI bus provides a 10 megabit ethernet interface. The ENC28J60 does not have a MAC address built-in, so a Microchip 24AA02E48T on the I2C bus provides the MAC address. The MAC address has to be set in userspace inLinux.

### P1 RJ-45 connector

Pin | Function | Note
---|---|---
1|Ethernet TD+|
2|Ethernet TD-|
3|Ethernet RD+|
4|VIN+|7 to 36VDC (bridged to P1.5)
5|VIN+|7 to 36VDC (bridged to P1.4)
6|Ethernet RD-|
7|COM|
8|COM|


### JST1 expansion connector

Pin|Function|Harness Color|Note
---|---|---|---
1|VIN|Purple|Supply voltage to pzPowerI2C board
2|+5VDC|Orange|5 volt output from pzPowerI2C board
3|+3.3VDC|Red|3.3 volt output from pzPowerI2C or Pi Zero
4|I2C SDA|Blue|
5|I2C SCL|Yellow|
6|COM|BLACK|

## TBD variant (prototype, not deployed)
### RJ-45 connector

Pin | Function | Note
---|---|---
1|Ethernet TD+ / Power+|
2|Ethernet TD- / Power+|
3|Ethernet RD+ / Power-| To MOXA pin 5 for RS-485 / To XRW2G P2.3
4|RS485 B|To MOXA pin 4 for RS-485 / To XRW2G P2.2
5|RS485 A|To Moxa pin 3 for RS-485 / To XRW2G P2.1
6|Ethernet RD- / Power-|
7|Unused|Brought to P6.1 near board edge
8|Unused|Brought to P6.2 near board edge

