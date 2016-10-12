# pzPowerI2C
PIC18F14K22 housekeeping micro controller for camera power supply. Communicates with Raspberry PI via I2C.

## I2C Interface

The pzPowerI2C micro controller is connected to the Raspberry Pi with a I2C interface. The pzPowerI2C acts as a I2C slave.

## Hardware Notes

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

### RS-485 port
The board has an RS-485 port that is connected to the UART on the Pi. Half duplex (2-wire) RS-485 requires a transmit enable signal. GPIO 4 is used for this. The software communicating on the RS-485 must have direction control support capable of toggling this GPIO.

We have added support to mbusd for direction control using the linux sysfs interface.

Example command:

`mbusd -d -y /sys/class/gpio/gpio4/value -p /dev/ttyAMA0 -s 9600`

The `-y filename` option is used to specify the name of the file that should have a `1` written to it when the RS-485 should transmit and a `0` written to it when it should not transmit. Although it not used with this board, we have added a `-Y` flag that is the inverse of this.

Before the GPIO can be used for direction control, it must be exported and set to an output using the sysfs interface.

```
echo 4 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio4/direction
```