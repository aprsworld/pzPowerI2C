#include "pzPowerI2C_registers.h"

void write_i2c(int8 address, int16 value) {
	switch ( address ) {
		case  PZP_I2C_REG_SWITCH_MAGNET_LATCH: 
			current.latch_sw_magnet=0;
			break;
		case PZP_I2C_REG_TIME_WATCHDOG_WRITE_SECONDS:
			current.write_watchdog_seconds=0;
			break;
		case PZP_I2C_REG_CONFIG_SERIAL_PREFIX: 
			if ( current.factory_unlocked && value >= 'A' && value <='Z' ) 
				config.serial_prefix=value;
			break;
		case PZP_I2C_REG_CONFIG_SERIAL_NUMBER:
			if (  current.factory_unlocked  ) {
				config.serial_number=value;
			}
			break;
		case PZP_I2C_REG_CONFIG_PARAM_WRITE:
			if ( 1 == value ) {
				timers.now_write_config=1;
			} else if ( 2 == value ) {
				timers.now_reset_config=1;
			} else if ( 1802 == value ) {
				current.factory_unlocked =1;
			} else if ( 65535 == value ) {
				reset_cpu();
			}
			break;
		case PZP_I2C_REG_CONFIG_TICKS_ADC:
			config.adc_sample_ticks=value;
			break;
		case PZP_I2C_REG_CONFIG_STARTUP_POWER_ON_DELAY:
			config.startup_power_on_delay=value;
			break;
		default:
			/* do nothing */
	}

}


int16 map_i2c(int8 addr) {

	timers.led_on_green=100;



	switch ( addr ) {
		/* analog channels */
		/* input voltage */
		case PZP_I2C_REG_VOLTAGE_INPUT_NOW: 
			return (int16) current.adc_buffer[0][current.adc_buffer_index];
		case PZP_I2C_REG_VOLTAGE_INPUT_AVG: 
			return (int16) adc_get(0);

		/* temperature sensor */
		case PZP_I2C_REG_TEMPERATURE_BOARD_NOW: 
			return (int16) current.adc_buffer[1][current.adc_buffer_index];
		case PZP_I2C_REG_TEMPERATURE_BOARD_AVG: 
			return (int16) adc_get(1);

		/* switch channels */
		case PZP_I2C_REG_SWITCH_MAGNET_NOW: 
			return (int16) ! input(SW_MAGNET);
		case PZP_I2C_REG_SWITCH_MAGNET_LATCH: 
			return (int16) current.latch_sw_magnet;
		
		/* status */
		case PZP_I2C_REG_SEQUENCE_NUMBER: 
			return (int16) current.sequence_number++;
		case PZP_I2C_REG_TIME_INTERVAL_MILLISECONDS: 
			return (int16) current.interval_milliseconds; /* milliseconds since last query */
		case PZP_I2C_REG_TIME_UPTIME_MINUTES: 
			return (int16) current.uptime_minutes; 
		case PZP_I2C_REG_TIME_WATCHDOG_READ_SECONDS: 
			return (int16) current.read_watchdog_seconds; 
		case PZP_I2C_REG_TIME_WATCHDOG_WRITE_SECONDS: 
			return (int16) current.write_watchdog_seconds;


		/* configuration */
		case PZP_I2C_REG_CONFIG_SERIAL_PREFIX: 
			return (int16) config.serial_prefix;
		case PZP_I2C_REG_CONFIG_SERIAL_NUMBER: 
			return (int16) config.serial_number;
		case PZP_I2C_REG_CONFIG_HARDWARE_MODEL: 
			return (int16) 'P';
		case PZP_I2C_REG_CONFIG_HARDWARE_VERSION: 
			return (int16) 'Z';
		case PZP_I2C_REG_CONFIG_SOFTWARE_MODEL: 
			return (int16) 'P';
		case PZP_I2C_REG_CONFIG_SOFTWARE_VERSION: 
			return (int16) 3;
		case PZP_I2C_REG_CONFIG_PARAM_WRITE:
			/* 1 if factory unlocked */ 
			return (int16) current.factory_unlocked; 	
		case PZP_I2C_REG_CONFIG_TICKS_ADC: 
			return (int16) config.adc_sample_ticks;
		case PZP_I2C_REG_CONFIG_STARTUP_POWER_ON_DELAY: 
			return (int16) config.startup_power_on_delay;
		

		/* we should have range checked, and never gotten here ... or read unimplemented (future) register */
		default: return (int16) addr;
	}

}


#if 0
/* 
try to write the specified register
if successful, return 0, otherwise return a modbus exception
*/
int i2c_write_register(int16 address, int16 value) {
//	int8 n,o;

	if ( address >= MIN_EE_REGISTER && address < MAX_EE_REGISTER ) {
		if ( value > 256 ) return ILLEGAL_DATA_VALUE;
		write_eeprom(address - MIN_EE_REGISTER + EE_FOR_HOST_ADDRESS,(int8) value);
		return 0;
	}



	/* if we have been unlocked, then we can modify serial number */
	if ( current.factory_unlocked ) {
		if ( 1000 == address ) {
			config.serial_prefix=value;
			return 0;
		} else if ( 1001 == address ) {
			config.serial_number=value;
			return 0;
		}
	}

	/* publicly writeable addresses */
	switch ( address ) {
		case 7:
			if ( 0 != value ) return ILLEGAL_DATA_VALUE;
			current.latch_sw_magnet=0;
			break;

		case 1006:
			/* Modbus address {0 to 127 or 128 for respond to any} */
			if ( value > 128 ) return ILLEGAL_DATA_VALUE;
			config.modbus_address=value;
			break;

		case 1007:
			/* ADC sample interval */
			timers.now_adc_reset_count=1;
			config.adc_sample_ticks=value;
			break;

		case 1008:
			/* allow this processor to follow requests of the PIC BOOTLOAD REQUEST line to reset ourselves */
			if ( value > 1 ) return ILLEGAL_DATA_VALUE;
			config.allow_bootload_request=value;
			break;

		case 1009:
			config.watchdog_seconds_max=value;
			break;

		case 1010:
			if ( value < 1 ) return ILLEGAL_DATA_VALUE;
			config.pi_offtime_seconds=value;
			break;
		
		case 1011:
			if ( value > 1 ) return ILLEGAL_DATA_VALUE;
			config.power_startup=value;
			break;

		case 1012:
			if ( value > 1 ) return ILLEGAL_DATA_VALUE;
			config.pic_to_pi_latch_mask=value;
			break;
		

		case 1997:
			/* reset CPU */
			if ( 1 != value ) return ILLEGAL_DATA_VALUE;
			reset_cpu();
		case 1998:
			/* write default config to EEPROM */
			if ( 1 != value ) return ILLEGAL_DATA_VALUE;
			write_default_param_file();
			break;
		case 1999:
			/* write config to EEPROM */
			if ( 1 != value ) return ILLEGAL_DATA_VALUE;
			write_param_file();
			break;
		case 19999:
			/* unlock factory programming registers when we get 1802 in passcode register */
			if ( 1802 != value ) {
				current.factory_unlocked=0;
				return ILLEGAL_DATA_VALUE;
			}
			current.factory_unlocked=1;
			/* green LED for 2 seconds */
			timers.led_on_green=200;
			break;
		default:
			return ILLEGAL_DATA_ADDRESS;

	}

	/* must not have triggered an exception */
	return 0;
}


#endif