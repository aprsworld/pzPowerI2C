#include <18F14K22.h>
#device ADC=10
#device *=16
#use delay(clock=16MHz)
#use i2c(SLAVE, I2C1, address=0x34, FORCE_HW)
/* Linux / i2cdetect will use the CCS address >>1. So 0x34 becomes 0x1a */

#fuses INTRC_IO
#fuses NOPLLEN
#fuses NOFCMEN
#fuses NOIESO
#fuses PUT
#fuses BROWNOUT
#fuses WDT4096
#fuses NOHFOFST
#fuses NOMCLR
#fuses STVREN
#fuses NOLVP
#fuses NOXINST
#fuses NODEBUG
#fuses NOPROTECT
#fuses NOWRT
#fuses NOWRTC 
#fuses NOWRTB
#fuses NOWRTD
#fuses NOEBTR
#fuses NOEBTRB

#use standard_io(ALL)

#use rs232(UART1,stream=STREAM_PI,baud=9600,errors)	

/* program config CRC of 0 and a serial_prefix of 'A' ... that will trigger a write default on first boot */
#ROM 0xF00000 = { 0x00, 0x00, 0x40, 0x00 }

#define PI_POWER_EN          PIN_C4
#define WIFI_POWER_EN        PIN_C5
#define PIC_LED_GREEN        PIN_C6
#define SER_FROM_PI          PIN_B5
#define SW_MAGNET            PIN_A5
#define AN_IN_VOLTS          PIN_C0
#define AN_VTEMP             PIN_A2
#define I2C_SDA              PIN_B4
#define I2C_SCL              PIN_B6

/* 
Parameters are stored in EEPROM
*/
#define PARAM_CRC_ADDRESS  0x00
#define PARAM_ADDRESS      PARAM_CRC_ADDRESS+2


#define POWER_FLAG_POS_HTD            6
#define POWER_FLAG_POS_LTD            5
#define POWER_FLAG_POS_HVD            4
#define POWER_FLAG_POS_LVD            3
#define POWER_FLAG_POS_WRITE_WATCHDOG 2
#define POWER_FLAG_POS_READ_WATCHDOG  1
#define POWER_FLAG_POS_COMMAND_OFF    0


