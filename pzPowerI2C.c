#include "pzPowerI2C.h"


typedef struct {
	int8 serial_prefix;
	int16 serial_number;

	int16 adc_sample_ticks;

	int16 startup_power_on_delay;

	/* command_off in current */
	int16 command_off_hold_time;

	int16 read_watchdog_off_threshold;
	int16 read_watchdog_off_hold_time;

	int16 write_watchdog_off_threshold;
	int16 write_watchdog_off_hold_time;
} struct_config;



typedef struct {
	/* circular buffer for ADC readings */
	int16 adc_buffer[2][16];
	int8  adc_buffer_index;

	int16 sequence_number;
	int16 uptime_minutes;
	int16 interval_milliseconds;

	int8 factory_unlocked;

	int16 read_watchdog_seconds;
	int16 write_watchdog_seconds;

	int8 compile_year;
	int8 compile_month;
	int8 compile_day;

	/* bit position
		7
		6 htd
		5 ltd
		4 hvd
		3 lvd
		2 write watchdog
		1 read watchdog
		0 command
	*/

	int8 power_on_flags;
	int8 power_off_flags; 	

	/* magnet sensor on board */
	int8 latch_sw_magnet;

	int8 default_params_written;

	int16 command_off;
} struct_current;

typedef struct {
	int8 led_on_green;
	int16 load_off_seconds;

	int1 now_adc_sample;
	int1 now_adc_reset_count;

	int1 now_millisecond;

	int1 now_write_config;
	int1 now_reset_config;
} struct_time_keep;

/* global structures */
struct_config config={0};
struct_current current={0};
struct_time_keep timers={0};

#include "adc_pzPowerI2C.c"
#include "param_pzPowerI2C.c"
#include "i2c_handler_pzPowerI2C.c"
#include "interrupt_pzPowerI2C.c"

void init(void) {
	int8 buff[32];
	setup_oscillator(OSC_16MHZ);

	setup_adc(ADC_CLOCK_DIV_16);
	/* NTC thermistor on sAN2 and input voltage divider on sAN4, voltage spans between 0 and Vdd */
	setup_adc_ports(sAN2 | sAN4,VSS_VDD);


	set_tris_a(0b00101111);
	set_tris_b(0b01110000);
	set_tris_c(0b00000001);
//               76543210

	port_a_pullups(0b00101011);
	port_b_pullups(0b00000000);
//                   76543210

	/* data structure initialization */
	timers.led_on_green=0;
	timers.load_off_seconds=2;
	timers.now_adc_sample=0;
	timers.now_adc_reset_count=0;
	timers.now_millisecond=0;
	timers.now_write_config=0;
	timers.now_reset_config=0;


	current.sequence_number=0;
	current.uptime_minutes=0;
	current.interval_milliseconds=0;
	current.adc_buffer_index=0;
	current.factory_unlocked=0;
	current.read_watchdog_seconds=0;
	current.write_watchdog_seconds=0;
	current.latch_sw_magnet=0;
	current.default_params_written=0;

	/* get our compiled date from constant */
	strcpy(buff,__DATE__);
	current.compile_day =(buff[0]-'0')*10;
	current.compile_day+=(buff[1]-'0');
	/* determine month ... how annoying */
	if ( 'J'==buff[3] ) {
		if ( 'A'==buff[4] )
			current.compile_month=1;
		else if ( 'N'==buff[5] )
			current.compile_month=6;
		else
			current.compile_month=7;
	} else if ( 'A'==buff[3] ) {
		if ( 'P'==buff[4] )
			current.compile_month=4;
		else
			current.compile_month=8;
	} else if ( 'M'==buff[3] ) {
		if ( 'R'==buff[5] )
			current.compile_month=3;
		else
			current.compile_month=5;
	} else if ( 'F'==buff[3] ) {
		current.compile_month=2;
	} else if ( 'S'==buff[3] ) {
		current.compile_month=9;
	} else if ( 'O'==buff[3] ) {
		current.compile_month=10;
	} else if ( 'N'==buff[3] ) {
		current.compile_month=11;
	} else if ( 'D'==buff[3] ) {
		current.compile_month=12;
	} else {
		/* error parsing, shouldn't happen */
		current.compile_month=255;
	}
	current.compile_year =(buff[7]-'0')*10;
	current.compile_year+=(buff[8]-'0');


	/* one periodic interrupt @ 1mS. Generated from system 16 MHz clock */
	/* prescale=16, match=249, postscale=1. Match is 249 because when match occurs, one cycle is lost */
	setup_timer_2(T2_DIV_BY_16,249,1);

	enable_interrupts(INT_TIMER2);
}


void periodic_millisecond(void) {
	static int8 uptimeticks=0;
	static int16 adcTicks=0;
	static int16 ticks=0;


	timers.now_millisecond=0;

	/* set magnet latch. Reset by writing 0 to magnet latch register */
	if ( ! input(SW_MAGNET) ) {
		current.latch_sw_magnet=1;
	}

	/* green LED control */
	if ( 0==timers.led_on_green ) {
		output_low(PIC_LED_GREEN);
	} else {
		output_high(PIC_LED_GREEN);
		timers.led_on_green--;
	}

	/* some other random stuff that we don't need to do every cycle in main */
	if ( current.interval_milliseconds < 65535 ) {
		current.interval_milliseconds++;
	}


	/* seconds */
	ticks++;
	if ( 1000 == ticks ) {
		ticks=0;

		/* watchdog power control of pi */
		if ( current.read_watchdog_seconds != 65535 ) {
			current.read_watchdog_seconds++;
		}
		if ( current.write_watchdog_seconds != 65535 ) {
			current.write_watchdog_seconds++;
		}

#if 0
		/* shut off when:
			a) watchdog_seconds_max != 0 AND watchdog_seconds is greater than watchdog_seconds_max AND it isn't already off 
		*/
		if ( 0 != config.watchdog_seconds_max && current.watchdog_seconds > config.watchdog_seconds_max && 0 == timers.load_off_seconds ) {
			timers.load_off_seconds=config.pi_offtime_seconds;
		}
#endif

		/* control power to the raspberrry pi load */
		if ( 0==timers.load_off_seconds ) {
			output_high(PI_POWER_EN);
			output_high(WIFI_POWER_EN);
		} else {
//			output_low(PI_POWER_EN);
//			output_low(WIFI_POWER_EN);
			timers.load_off_seconds--;

			if ( 0 == timers.load_off_seconds ) {
				/* reset watchdog seconds so we can turn back on */
				current.read_watchdog_seconds=0;
			}
		}

		
		/* uptime counter */
		uptimeTicks++;
		if ( 60 == uptimeTicks ) {
			uptimeTicks=0;
			if ( current.uptime_minutes < 65535 ) 
				current.uptime_minutes++;
		}
	}

	/* ADC sample counter */
	if ( timers.now_adc_reset_count ) {
		timers.now_adc_reset_count=0;
		adcTicks=0;
	}

	/* ADC sampling trigger */
	adcTicks++;
	if ( adcTicks == config.adc_sample_ticks ) {
		adcTicks=0;
		timers.now_adc_sample=1;
	}

}


void main(void) {
	int8 i;

	i=restart_cause();

	init();

	output_low(PI_POWER_EN);
	output_low(WIFI_POWER_EN);

	/* read parameters from EEPROM and write defaults if CRC doesn't match */
	read_param_file();

	if ( config.startup_power_on_delay > 100 )
		config.startup_power_on_delay=100;

	/* flash on startup */
	for ( i=0 ; i<config.startup_power_on_delay ; i++ ) {
		restart_wdt();
		output_high(PIC_LED_GREEN);
		delay_ms(200);
		output_low(PIC_LED_GREEN);
		delay_ms(200);
	}





//	if ( config.modbus_address > 128 ) {
//		write_default_param_file();
//	}

	timers.led_on_green=500;

	enable_interrupts(GLOBAL);

	/* Prime ADC filter */
	for ( i=0 ; i<30 ; i++ ) {
		adc_update();
	}

	/* set power switch to initial state */
//	current.p_on=config.power_startup;


	fprintf(STREAM_PI,"# pzPowerI2C %s\r\n",__DATE__);

	/* enable I2C slave interrupt */
	enable_interrupts(INT_SSP);

	for ( ; ; ) {
		restart_wdt();

		if ( timers.now_millisecond ) {
			periodic_millisecond();
		}


		if ( timers.now_adc_sample ) {
			timers.now_adc_sample=0;
			adc_update();
//			sprintf(buffer,">i=%lu,r=%lu<",adc_get(0),adc_get(1));
		}

		if ( timers.now_write_config ) {
			timers.now_write_config=0;
			write_param_file();
		}
		if ( timers.now_reset_config ) {
			timers.now_reset_config=0;
			write_default_param_file();
		}


	}


}