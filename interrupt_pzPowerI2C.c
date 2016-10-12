#int_timer2
void isr_timer2() {
	timers.now_millisecond=1;
}

/* transmit buffer empty for Modbus to raspberry pi buffer */
#int_tbe
void isr_uart1_tbe() {
	if ( timers.rda_tx_pos >= timers.rda_tx_length ) {
		/* done transmitting */
		timers.now_rda_tx_done=1;
		disable_interrupts(INT_TBE);
	} else {
		/* put another character into TX buffer */
		fputc(timers.rda_tx_buff[timers.rda_tx_pos], STREAM_PI);
		timers.rda_tx_pos++;
	}
}

/*  Raspberry PI connected serial port*/
#int_rda
void isr_uart1_rx() {
	int8 c;

	c=fgetc(STREAM_PI);

	current.rda_bytes_received++;

	/* Modbus */
	if (!modbus_serial_new) {
		if(modbus_serial_state == MODBUS_GETADDY) {
			modbus_serial_crc.d = 0xFFFF;
			modbus_rx.address = c;
			modbus_serial_state++;
			modbus_rx.len = 0;
			modbus_rx.error=0;
		} else if(modbus_serial_state == MODBUS_GETFUNC) {
			modbus_rx.func = c;
			modbus_serial_state++;
		} else if(modbus_serial_state == MODBUS_GETDATA) {
			if (modbus_rx.len>=MODBUS_SERIAL_RX_BUFFER_SIZE) {
				modbus_rx.len=MODBUS_SERIAL_RX_BUFFER_SIZE-1;
			}
			modbus_rx.data[modbus_rx.len]=c;
			modbus_rx.len++;
		}

		modbus_calc_crc(c);
		modbus_enable_timeout(TRUE);
	}
}