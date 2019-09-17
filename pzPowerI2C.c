#include "pzPowerI2C.h"


typedef struct {
	int8 serial_prefix;
	int16 serial_number;

	int16 adc_sample_ticks;


	/* power control switch settings */
	int8 power_startup; /* 0==start with PI off, 1==start with PI on */


//	int8 pic_to_pi_latch_mask;
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

	/* power control switch */
	int8 p_on;
	int16 power_on_delay;
	int16 power_off_delay;
	int16 power_override_timeout;

	/* magnet sensor on board */
	int8 latch_sw_magnet;
} struct_current;

typedef struct {
	int8 led_on_green;
	int16 load_off_seconds;

	int1 now_adc_sample;
	int1 now_adc_reset_count;

	int1 now_millisecond;

	int1 now_write_config;
	int1 now_reset_config;
	int1 now_factory_unlock;
} struct_time_keep;

/* global structures */
struct_config config;
struct_current current;
struct_time_keep timers;

#include "adc_pzPowerI2C.c"
#include "param_pzPowerI2C.c"
#include "i2c_handler_pzPowerI2C.c"
#include "interrupt_pzPowerI2C.c"

void init(void) {
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
	timers.now_factory_unlock=0;


	current.sequence_number=0;
	current.uptime_minutes=0;
	current.interval_milliseconds=0;
	current.adc_buffer_index=0;
	current.factory_unlocked=0;
	current.read_watchdog_seconds=0;
	current.write_watchdog_seconds=0;
	current.latch_sw_magnet=0;


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

	/* flash on startup */
	for ( i=0 ; i<5 ; i++ ) {
		restart_wdt();
		output_high(PIC_LED_GREEN);
		delay_ms(200);
		output_low(PIC_LED_GREEN);
		delay_ms(200);
	}


	read_param_file();



//	if ( config.modbus_address > 128 ) {
		write_default_param_file();
//	}

	timers.led_on_green=500;

	enable_interrupts(GLOBAL);

	/* Prime ADC filter */
	for ( i=0 ; i<30 ; i++ ) {
		adc_update();
	}

	/* set power switch to initial state */
	current.p_on=config.power_startup;


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
			timers.now_write_config=0;
			write_default_param_file();
		}


	}


}