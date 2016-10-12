#include "pcPower.h"


typedef struct {
	int8 modbus_address;
	int8 modbus_mode;

	int8 serial_prefix;
	int16 serial_number;

	int16 adc_sample_ticks;

	int8 allow_bootload_request;
	int16 watchdog_seconds_max;
	int16 pi_offtime_seconds;


	/* power control switch settings */
	int8 power_startup; /* 0==start with PI off, 1==start with PI on */
	int16 power_off_below_adc;
	int16 power_off_below_delay;
	int16 power_on_above_adc;
	int16 power_on_above_delay;
	int16 power_override_timeout;

	int8 pic_to_pi_latch_mask;
} struct_config;



typedef struct {
	/* circular buffer for ADC readings */
	int16 adc_buffer[2][16];
	int8  adc_buffer_index;

	int16 modbus_our_packets;
	int16 modbus_other_packets;
	int16 modbus_last_error;

	int16 sequence_number;
	int16 uptime_minutes;
	int16 interval_milliseconds;

	int8 factory_unlocked;

	int16 watchdog_seconds;

	/* power control switch */
	int8 p_on;
	int16 power_on_delay;
	int16 power_off_delay;
	int16 power_override_timeout;

	/* serial byte counters. Roll over */
	int16 rda_bytes_received;

	/* push button on board */
	int8 latch_sw_magnet;
} struct_current;

typedef struct {
	int8 led_on_green;
	int16 load_off_seconds;

	int1 now_adc_sample;
	int1 now_adc_reset_count;

	int1 now_millisecond;


	/* transmit buffer for PIC to PI */
	int8 rda_tx_buff[56];
	int8 rda_tx_length;
	int8 rda_tx_pos;
	int1 now_rda_tx_ready;
	int1 now_rda_tx_done;
} struct_time_keep;

/* global structures */
struct_config config;
struct_current current;
struct_time_keep timers;

#include "adc_pcPower.c"
#include "param_pcPower.c"

#include "modbus_slave_pcPower.c"
#include "modbus_handler_pcPower.c"
#include "interrupt_pcPower.c"

void init(void) {
	setup_oscillator(OSC_16MHZ);

	setup_adc(ADC_CLOCK_DIV_16);
	setup_adc_ports(sAN4,VSS_VDD);
	setup_vref(VREF_1v024);


	port_a_pullups(0b00111111);
	port_b_pullups(0b01011111);

	/* data structure initialization */
	timers.led_on_green=0;
	timers.load_off_seconds=2;
	timers.now_adc_sample=0;
	timers.now_adc_reset_count=0;
	timers.now_millisecond=0;

	timers.rda_tx_length=0;
	timers.rda_tx_pos=0;
	timers.now_rda_tx_ready=0;
	timers.now_rda_tx_done=0;


	current.modbus_our_packets=0;
	current.modbus_other_packets=0;
	current.modbus_last_error=0;
	current.sequence_number=0;
	current.uptime_minutes=0;
	current.interval_milliseconds=0;
	current.adc_buffer_index=0;
	current.factory_unlocked=0;
	current.watchdog_seconds=0;
	current.latch_sw_magnet=0;

	/* power control switch */
	current.power_on_delay=config.power_on_above_delay;
	current.power_off_delay=config.power_off_below_delay;
	current.power_override_timeout=0;

	/* one periodic interrupt @ 1mS. Generated from system 16 MHz clock */
	/* prescale=16, match=249, postscale=1. Match is 249 because when match occurs, one cycle is lost */
	setup_timer_2(T2_DIV_BY_16,249,1);

	enable_interrupts(INT_TIMER2);

	/* RDA - PI is turned on in modbus_slave_pcwx's init */
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

	/* set PIC to PI line based on latch state(s) */
	if ( bit_test(config.pic_to_pi_latch_mask,0) && current.latch_sw_magnet ) {
		output_high(PIC_TO_PI);
	} else {
		output_low(PIC_TO_PI);
	}

	/* green LED control */
	if ( 0==timers.led_on_green ) {
		output_low(LED_GREEN);
	} else {
		output_high(LED_GREEN);
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
		if ( current.watchdog_seconds != 65535 ) {
			current.watchdog_seconds++;
		}

		/* shut off when:
			a) watchdog_seconds_max != 0 AND watchdog_seconds is greater than watchdog_seconds_max AND it isn't already off 
		*/
		if ( 0 != config.watchdog_seconds_max && current.watchdog_seconds > config.watchdog_seconds_max && 0 == timers.load_off_seconds ) {
			timers.load_off_seconds=config.pi_offtime_seconds;
		}

		/* control power to the raspberrry pi load */
		if ( 0==timers.load_off_seconds ) {
			output_high(PI_POWER_EN);
		} else {
			output_low(PI_POWER_EN);
			timers.load_off_seconds--;

			if ( 0 == timers.load_off_seconds ) {
				/* reset watchdog seconds so we can turn back on */
				current.watchdog_seconds=0;
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

//	output_high(PI_POWER_EN);

#if 0
	for ( j=0 ; j<100 ; j++ ) {
		output_high(PIC_LED_GREEN);
		delay_ms(50);
		output_low(PIC_LED_GREEN);
		delay_ms(50);
	}
#endif

#if 1
	fprintf(STREAM_PI,"# pcPower %s\r\n",__DATE__);
	fprintf(STREAM_PI,"# restart_cause()=%u ",i);

	switch ( i ) {
		case WDT_TIMEOUT: fprintf(STREAM_PI,"WDT_TIMEOUT"); break;
		case MCLR_FROM_SLEEP: fprintf(STREAM_PI,"MCLR_FROM_SLEEP"); break;
		case MCLR_FROM_RUN: fprintf(STREAM_PI,"MCLR_FROM_RUN"); break;
		case NORMAL_POWER_UP: fprintf(STREAM_PI,"NORMAL_POWER_UP"); break;
		case BROWNOUT_RESTART: fprintf(STREAM_PI,"BROWNOUT_RESTART"); break;
		case WDT_FROM_SLEEP: fprintf(STREAM_PI,"WDT_FROM_SLEEP"); break;
		case RESET_INSTRUCTION: fprintf(STREAM_PI,"RESET_INSTRUCTION"); break;
		default: fprintf(STREAM_PI,"unknown!");
	}
	fprintf(STREAM_PI,"\r\n");
#endif


	read_param_file();



	if ( config.modbus_address > 128 ) {
		write_default_param_file();
	}

	timers.led_on_green=500;

	/* start Modbus slave */
	setup_uart(TRUE);
	/* modbus_init turns on global interrupts */
	modbus_init();

	/* Prime ADC filter */
	for ( i=0 ; i<30 ; i++ ) {
		adc_update();
	}

	/* set power switch to initial state */
	current.p_on=config.power_startup;


	fprintf(STREAM_PI,"# pcPower %s config.modbus_address=%u\r\n",__DATE__,config.modbus_address);

	for ( ; ; ) {
		restart_wdt();

//		output_bit(PIC_LED_GREEN,input(SW_MAGNET));

		if ( timers.now_millisecond ) {
			periodic_millisecond();
		}


		if ( timers.now_adc_sample ) {
			timers.now_adc_sample=0;
			adc_update();
		}

		modbus_process();

		/* buffered modbus transmit */

		/* start transmitting */
		if ( timers.now_rda_tx_ready ) {
			timers.now_rda_tx_ready=0;

			RCV_OFF();

			/* 3.5 character delay (3500000/baud) */
			delay_us(61); /* 57600 */

			/* enable transmit buffer empty interrupt. It will feed itself */
			enable_interrupts(INT_TBE);
		}

		/* done transmitting */
		if ( timers.now_rda_tx_done ) {
			timers.now_rda_tx_done=0;

			/* 3.5 character delay (3500000/baud) */
			delay_us(61); /* 57600 */
   			RCV_ON();
		}
	}


}