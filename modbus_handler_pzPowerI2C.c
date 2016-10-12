#define MAX_STATUS_REGISTER          19

#define MIN_CONFIG_REGISTER          1000
#define MAX_CONFIG_REGISTER          1012

#define MIN_EE_REGISTER              2000
#define MAX_EE_REGISTER              MIN_EE_REGISTER + 512


/* This function may come in handy for you since MODBUS uses MSB first. */
int8 swap_bits(int8 c) {
	return ((c&1)?128:0)|((c&2)?64:0)|((c&4)?32:0)|((c&8)?16:0)|((c&16)?8:0)|((c&32)?4:0)|((c&64)?2:0)|((c&128)?1:0);
}

void reset_modbus_stats(void) {
	current.modbus_our_packets=0;
	current.modbus_other_packets=0;
	current.modbus_last_error=0;
}

int16 map_modbus(int16 addr) {
//	static u_lblock ps;
//	int8 n,o;
//	int8 *p;

	if ( addr >= MIN_EE_REGISTER && addr < MAX_EE_REGISTER ) {
		return (int16) read_eeprom(addr - MIN_EE_REGISTER + EE_FOR_HOST_ADDRESS);
	}


	switch ( addr ) {
		/* analog channels */
		/* input voltage */
		case  0: return (int16) current.adc_buffer[0][current.adc_buffer_index];
		case  1: return (int16) adc_get(0);
		/* fixed 1.024 volt reference */
		case  2: return (int16) current.adc_buffer[1][current.adc_buffer_index];
		case  3: return (int16) adc_get(1);


		/* switch channels */
		case  6: return (int16) ! input(SW_MAGNET);
		case  7: return (int16) current.latch_sw_magnet;
		
		/* status */
		case 10: return (int16) current.sequence_number++;
		case 11: return (int16) current.interval_milliseconds; /* milliseconds since last query */
		case 12: return (int16) current.uptime_minutes; 
		case 13: return (int16) current.watchdog_seconds; 

		/* modbus statistics */
		case 16: return (int16) current.modbus_our_packets;
		case 17: return (int16) current.modbus_other_packets;
		case 18: return (int16) current.modbus_last_error;
		/* triggers a modbus statistics reset */
		case 19: reset_modbus_stats(); return (int16) 0;

		/* configuration */
		case 1000: return (int16) config.serial_prefix;
		case 1001: return (int16) config.serial_number;
		case 1002: return (int16) 'P';
		case 1003: return (int16) 'C';
		case 1004: return (int16) 'P';
		case 1005: return (int16) 1;
		case 1006: return (int16) config.modbus_address;
		case 1007: return (int16) config.adc_sample_ticks;
		case 1008: return (int16) config.allow_bootload_request;
		case 1009: return (int16) config.watchdog_seconds_max;
		case 1010: return (int16) config.pi_offtime_seconds;
		case 1011: return (int16) config.power_startup;
		case 1012: return (int16) config.pic_to_pi_latch_mask;

		/* we should have range checked, and never gotten here ... or read unimplemented (future) register */
		default: return (int16) 65535;
	}

}


int8 modbus_valid_read_registers(int16 start, int16 end) {
	if ( 19999==start && 20000==end)
		return 1;

	if ( start >= MIN_CONFIG_REGISTER && end <= MAX_CONFIG_REGISTER+1 )
		return 1;

	if ( start >= MIN_EE_REGISTER && end <= MAX_EE_REGISTER+1 )
		return 1;
	

	/* end is always start + at least one ... so no need to test for range starting at 0 */
	if ( end <= MAX_STATUS_REGISTER+1)
		return 1;

	return 0;
}

int8 modbus_valid_write_registers(int16 start, int16 end) {
	if ( 19999==start && 20000==end)
		return 1;

	if ( 7==start && 8==end)
		return 1;

	if ( start >= MIN_EE_REGISTER && end <= MAX_EE_REGISTER+1 )
		return 1;

	if ( start >= MIN_CONFIG_REGISTER && end <= MAX_CONFIG_REGISTER+1 )
		return 1;
	
	/* end is always start + at least one ... so no need to test for range starting at 0 */
	if ( end <= MAX_STATUS_REGISTER+1)
		return 1;

	return 0;
}

void modbus_read_register_response(function func, int8 address, int16 start_address, int16 register_count ) {
	int16 i;
	int16 l;

	modbus_serial_send_start(address, func); // FUNC_READ_HOLDING_REGISTERS);
	modbus_serial_putc(register_count*2);


	for( i=0 ; i<register_count ; i++ ) {
		l=map_modbus(start_address+i);
		modbus_serial_putc(make8(l,1));
  		modbus_serial_putc(make8(l,0));
	}

	modbus_serial_send_stop();
}

/* 
try to write the specified register
if successful, return 0, otherwise return a modbus exception
*/
exception modbus_write_register(int16 address, int16 value) {
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


void modbus_process(void) {
	int16 start_addr;
	int16 num_registers;
	exception result;
	int8 i;


	/* check for message */
	if ( modbus_kbhit() ) {

		if ( 128==config.modbus_address || modbus_rx.address==config.modbus_address ) {
			/* Modbus statistics */
			if ( current.modbus_our_packets < 65535 )
				current.modbus_our_packets++;
	
			/* green LED for 200 milliseconds */
			timers.led_on_green=20;

			switch(modbus_rx.func) {
				case FUNC_READ_HOLDING_REGISTERS: /* 3 */
				case FUNC_READ_INPUT_REGISTERS:   /* 4 */
					start_addr=make16(modbus_rx.data[0],modbus_rx.data[1]);
					num_registers=make16(modbus_rx.data[2],modbus_rx.data[3]);
	
					/* make sure our address is within range */
					if ( ! modbus_valid_read_registers(start_addr,start_addr+num_registers) ) {
					    modbus_exception_rsp(modbus_rx.address,modbus_rx.func,ILLEGAL_DATA_ADDRESS);
						current.modbus_last_error=ILLEGAL_DATA_ADDRESS;

						/* red LED for 1 second */
						timers.led_on_green=0;
					} else {
						modbus_read_register_response(modbus_rx.func,modbus_rx.address,start_addr,num_registers);
					}
					break;
				case FUNC_WRITE_SINGLE_REGISTER: /* 6 */
					start_addr=make16(modbus_rx.data[0],modbus_rx.data[1]);

					/* try the write */
					result=modbus_write_register(start_addr,make16(modbus_rx.data[2],modbus_rx.data[3]));

					if ( result ) {
						/* exception */
						modbus_exception_rsp(modbus_rx.address,modbus_rx.func,result);
						current.modbus_last_error=result;

						/* red LED for 1 second */
						timers.led_on_green=0;
					}  else {
						/* no exception, send ack */
						modbus_write_single_register_rsp(modbus_rx.address,
							start_addr,
							make16(modbus_rx.data[2],modbus_rx.data[3])
						);
					}
					break;
				case FUNC_WRITE_MULTIPLE_REGISTERS: /* 16 */
					start_addr=make16(modbus_rx.data[0],modbus_rx.data[1]);
					num_registers=make16(modbus_rx.data[2],modbus_rx.data[3]);

					/* attempt to write each register. Stop if exception */
					for ( i=0 ; i<num_registers ; i++ ) {
						result=modbus_write_register(start_addr+i,make16(modbus_rx.data[5+i*2],modbus_rx.data[6+i*2]));

						if ( result ) {
							/* exception */
							modbus_exception_rsp(modbus_rx.address,modbus_rx.func,result);
							current.modbus_last_error=result;
	
							/* red LED for 1 second */
							timers.led_on_green=0;
			
							break;
						}
					}
		
					/* we could have gotten here with an exception already send, so only send if no exception */
					if ( 0 == result ) {
						/* no exception, send ack */
						modbus_write_multiple_registers_rsp(modbus_rx.address,start_addr,num_registers);
					}

					break;  
				default:
					/* we don't support most operations, so return ILLEGAL_FUNCTION exception */
					modbus_exception_rsp(modbus_rx.address,modbus_rx.func,ILLEGAL_FUNCTION);
					current.modbus_last_error=ILLEGAL_FUNCTION;

					/* red led for 1 second */
					timers.led_on_green=0;
			}
			/* reset watchdog seconds now that we are done processing request */
			current.watchdog_seconds=0;

		} else {
			/* MODBUS packet for somebody else */
			if ( current.modbus_other_packets < 65535 )
				current.modbus_other_packets++;

			/* yellow LED 200 milliseconds */
			timers.led_on_green=10;
		}

	}
//	output_low(TP_RED);
}