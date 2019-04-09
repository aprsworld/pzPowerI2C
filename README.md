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

We have added support to mbusd for direction control using the linux sysfs interface.

Example command:

`mbusd -d -y /sys/class/gpio/gpio4/value -p /dev/ttyAMA0 -s 9600`

The `-y filename` option is used to specify the name of the file that should have a `1` written to it when the RS-485 should transmit and a `0` written to it when it should not transmit. Although it not used with this board, we have added a `-Y` flag that is the inverse of this.

Before the GPIO can be used for direction control, it must be exported and set to an output using the sysfs interface.

```
echo 4 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio4/direction
```

Here is a script that does all that
```
#!/bin/bash
GPIO_N="4"

echo "# using GPIO$GPIO_N for RTS485 transmit enable"

if [ ! -e /sys/class/gpio/gpio$GPIO_N/direction ]; then
	echo "# exporting GPIO$GPIO_N for sysfs control"
	echo $GPIO_N > /sys/class/gpio/export
fi

# give system a chance to export the gpio and make direction available
sleep 0.1

echo "# setting GPIO$GPIO_N to be output"
echo out > /sys/class/gpio/gpio4/direction

echo "# starting mbusd in foreground"
mbusd -d -y /sys/class/gpio/gpio4/value -p /dev/ttyAMA0 -s 9600 -v 9 -W100 -T0

if [ -e /sys/class/gpio/gpio$GPIO_N/direction ]; then
	echo "# un-exporting GPIO$GPIO_N from sysfs control"
	echo $GPIO_N > /sys/class/gpio/unexport
fi
```
