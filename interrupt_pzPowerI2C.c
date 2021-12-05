#int_timer2
void isr_timer2() {
	timers.now_millisecond=1;
}






	/*
.................... 	state = i2c_isr_state();
*
025A:  BTFSC  FC7.5	// if data, go to 0264
025C:  BRA    0264

025E:  CLRF   x9D	// set i=0 

0260:  BTFSC  FC7.2	// if read, then set high bit of i 
0262:  BSF    x9D.7

0264:  MOVF   x9D,W // increment i and put result in state
0266:  INCF   x9D,F
0268:  MOVWF  xC7

	x9D is static variable for state
	xC7 is returned value of state
*/

/*
.................... 	state = i2c_isr_state();
025A:  BTFSC  FC7.5 // bit test SSPSTAT.5, skip instruction below if DATA/!ADDRESS is indicating address
025C:  BRA    0264  // goto 0264

025E:  CLRF   x9F   // set x9F to 0
0260:  BTFSC  FC7.2 // bit test SSPSTAT.2, skip instruction below if READ/!WRITE is indicating write
0262:  BSF    x9F.7 // set the high bit of x9F

0264:  MOVF   x9F,W	// move x9F to W register
0266:  INCF   x9F,F // increment x9F
0268:  MOVWF  x9A	// move W to x9A (ie move x9F to x9A)

that makes 9a the value of state that is used by the program and x9F as a static state variable
*/


#byte SSPSTAT=GETENV("SFR:SSPSTAT")
#INT_SSP
void ssp_interrupt () {
	static int8 sstate;
	int8 state;
	int8 incoming;
	static int16 lastValue;
	static int8 lastMSB;
	static int8 address;


//	state = i2c_isr_state();


	/* 
	our implementation of i2c_isr_state() that won't overflow and switch states

	but it will quit counting at 127 bytes. 

	If more bytes are needed, external counting variables can be used or the size of state can be made larger
	*/


	if ( ! bit_test(SSPSTAT,5) ) {
		/* address */
		sstate=0;

		if ( bit_test(SSPSTAT,2) ) {
			/* set high bit if read */
			bit_set(sstate,7);
		}
	} else {
		/* data */
	}

	/* state variable used below is not incremented */
	state=sstate;

	/* increment the state counter for next pass through unless it will overflow us into another state */
	if ( sstate != 0x7f && sstate != 0xff) {
		sstate++;
	}

	
	/* i2c_isr_state() return an 8 bit int
		0 - Address match received with R/W bit clear, perform i2c_read( ) to read the I2C address.

		1-0x7F - Master has written data; i2c_read() will immediately return the data

		0x80 - Address match received with R/W bit set; perform i2c_read( ) to read the I2C address,
		and use i2c_write( ) to pre-load the transmit buffer for the next transaction (next I2C read
		performed by master will read this byte).

		0x81-0xFF - Transmission completed and acknowledged; respond with i2c_write() to pre-load
		the transmit buffer for the next transition (the next I2C read performed by master will read this
		byte).

		Function:
		Returns the state of I2C communications in I2C slave mode after an SSP interrupt. The return
		value increments with each byte received or sent.
		If 0x00 or 0x80 is returned, an i2C_read( ) needs to be performed to read the I2C address that
		was sent (it will match the address configured by #USE I2C so this value can be ignored)
	*/

	if ( state <= 0x80 ) {                      
		/* I2C master is sending us data */
		if ( 0x80 == state ) {
			/* i2c_read(2) casues the function to read the SSPBUF without releasing the clock */
			incoming = i2c_read(2);
		} else {
			output_high(_PIC_LED_GREEN);
			incoming = i2c_read();
			output_low(_PIC_LED_GREEN); 
		}

		if ( 1 == state ) {      
			/* first byte is address */                
			address = incoming;
		} else if ( state >= 2 && 0x80 != state ) {
			/* received byte is data */
		
			/* save MSB and we'll process it on next */
			if ( 2 == state ) {
				lastMSB=incoming;
			} else if ( 3 == state ) {
				/* 16 bit value made of previous byte and this byte */
				write_i2c(address,make16(lastMSB,incoming));
			}
		}
	}


	if ( state >= 0x80 ) {
		/* I2C master is requesting data from us */

#if 1		
		/* this doesn't */
		if ( ! bit_test(address,0) ) {
			/* read 16 bit register (register address half of I2C address) on even address */
			lastValue=map_i2c(address>>1);

			/* send the MSB */
			i2c_write(make8(lastValue,1));
		} else {
			/* send LSB of 16 bit register on odd address */
			i2c_write(make8(lastValue,0));
		}
#else
		/* this works */
		i2c_write(state);
#endif
		

		address++;
	}

	/* reset watchdog timer */
	timers.read_watchdog_seconds=0;
}


