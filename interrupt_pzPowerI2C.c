#int_timer2
void isr_timer2() {
	timers.now_millisecond=1;
}

unsigned int8 address, buffer[16];

#INT_SSP
void ssp_interrupt () {
	unsigned int8 incoming, state;

	state = i2c_isr_state();

	if(state <= 0x80) {                      //Master is sending data
		if(state == 0x80)
			incoming = i2c_read(2);          //Passing 2 as parameter, causes the function to read the SSPBUF without releasing the clock
		else
			incoming = i2c_read();

		if(state == 1)                      //First received byte is address
			address = incoming;
		else if(state >= 2 && state != 0x80)   //Received byte is data
			buffer[address++] = incoming;
	}

	if(state >= 0x80) {                     //Master is requesting data
		i2c_write(buffer[address++]);
	}

}