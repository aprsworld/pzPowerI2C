# pzPowerI2C
PIC18F14K22 housekeeping micro controller for camera power supply. Communicates with Raspberry PI via I2C.

## I2C Interface

The pzPowerI2C micro controller is connected to the Raspberry Pi with a I2C interface. The pzPowerI2C acts as a I2C slave.

## Hardware Notes

### 5 position screw terminal (present on WiFi variant)

Pin | Function | Note
---|---|---
1|POWER|7 to 36 volts DC
2|COM|Common with power and RS-485
3|RS485 A|
4|RS485 B|
5|COM|Common with power and RS-485

### RJ-45 connector (present only on ethernet variant, I guess)

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

### RS-485 port
The board has an RS-485 port that is connected to the UART on the Pi. Half duplex (2-wire) RS-485 requires a transmit enable signal. GPIO 4 is used for this. The software communicating on the RS-485 must have direction control support capable of toggling this GPIO.

We have added support to mbusd for direction control using the linux sysfs interface. An example script for setting up direction control pin and starting
mbusd can be found in the [support](support/) directory.

Note that different Pi variants have different serial port names. `/dev/ttyAMA0` was traditionally the Pi's external serial port. But on Bluetooth enabled
Pi's, the external serial port usually becomes `/dev/ttyS0`

