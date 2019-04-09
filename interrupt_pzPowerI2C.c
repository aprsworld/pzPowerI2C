#int_timer2
void isr_timer2() {
	timers.now_millisecond=1;
}

unsigned int8 address;

#INT_SSP
void ssp_interrupt () {
	unsigned int8 incoming, state;

	static int16 lastValue;


	state = i2c_isr_state();

	if(state <= 0x80) {                      //Master is sending data
		if(state == 0x80)
			incoming = i2c_read(2);          //Passing 2 as parameter, causes the function to read the SSPBUF without releasing the clock
		else
			incoming = i2c_read();

		if(state == 1)                      //First received byte is address
			address = incoming;
		else if(state >= 2 && state != 0x80)   //Received byte is data
			// stub to do nothing for now
			incoming = incoming;
			// buffer[address++] = incoming;
	}


	if(state >= 0x80) {                     //Master is requesting data
		if ( ! bit_test(address,0) ) {
			/* read 16 bit register on even address */
			lastValue=map_i2c(address>>1);
			i2c_write(make8(lastValue,1));
		} else {
			/* send other byte of 16 bit register on odd address */
			i2c_write(make8(lastValue,0));
		}
//		i2c_write(buffer[address++]);

		address++;
	}

	/* reset watchdog timer */
	current.watchdog_seconds=0;
}