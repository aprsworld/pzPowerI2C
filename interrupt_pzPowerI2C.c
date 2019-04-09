#int_timer2
void isr_timer2() {
	timers.now_millisecond=1;
}



#if 0
unsigned int8 buffer[256];
// SSP ISR implements I2C slave interface
// Controls communication with the master 
// Transfers register data to/from buffer
#INT_SSP
void  SSP_isr(void) 
{
// global buffer provides both incoming and outgoing data
   unsigned int8 incoming, state;
   static unsigned int8 address;
   state = i2c_isr_state();
//   i2cState = state;


   if(state <= 0x80) // Master is sending data
   {  
      if(state == 0x80)
         incoming = i2c_read(2); //read the SSPBUF without releasing the clock
      else
         incoming = i2c_read(1);
      if(state == 1)                      //First received byte is address
         address = incoming;
      else if((state >= 2) && (state != 0x80)) {   //Received byte is data
//         if (address==CMD_REG) newCmd = true;
         buffer[address] = incoming;
         address++;
      }
   }
   if(state >= 0x80)  //Master is requesting data
   {  
      //i2c_write(buffer[address]);
		i2c_write(address);
      address++;
   }
}
#endif


#if 1
unsigned int8 address;

#INT_SSP
void ssp_interrupt () {
	unsigned int8 incoming, state;
	static int16 lastValue;


	state = i2c_isr_state();

	if(state <= 0x80) {                      
		// Master is sending data
		if ( state == 0x80 ) {
			incoming = i2c_read(2); //Passing 2 as parameter, causes the function to read the SSPBUF without releasing the clock
		} else {
			incoming = i2c_read();
		}

		if ( state == 1 ) {                      
			// First received byte is address
			address = incoming;
		} else if(state >= 2 && state != 0x80) {
			// Received byte is data
			// buffer[address++] = incoming;
			address++;
		}
	}


	if ( state >= 0x80 ) {
		//Master is requesting data
		
		if ( ! bit_test(address,0) ) {
			/* read 16 bit register on even address */
			lastValue=map_i2c(address>>1);

			 i2c_write(make8(lastValue,1));
			//i2c_write(0);
		} else {
			/* send other byte of 16 bit register on odd address */
			i2c_write(make8(lastValue,0));
			//i2c_write(1);
		}

		address++;
	}

	/* reset watchdog timer */
	current.watchdog_seconds=0;
}
#endif


#if 0
int16 data_to_send=0xdead;


void i2c_interrupt() { 
	int state; 
	int writeBuffer[2]; 
	int readBuffer[2]; 
//	static int16 address=0;

	state = i2c_isr_state(); 

	if ( state==0 ) { 
		//Address match received with R/W bit clear, perform i2c_read( ) to read the I2C address. 
		i2c_read(); 
	} else if ( state==0x80 ) {
		//Address match received with R/W bit set; perform i2c_read( ) to read the I2C address, 
		//and use i2c_write( ) to pre-load the transmit buffer for the next transaction (next I2C read performed by master will read this byte). 
		i2c_read(2); 
		address=0;
	}

	if ( state>=0x80 ) {
		//Master is waiting for data 
		writeBuffer[0] = (data_to_send & 0xFF); //LSB first 
		writeBuffer[1] = ((data_to_send & 0xFF00)>>8);  //MSB secound 

		i2c_write(writeBuffer[state - 0x80]); //Write appropriate byte, based on how many have already been written 

		if ((state-0x80)==2) { 
//			data_sent=make16(writeBuffer[1],writeBuffer[0]); 
//			printf("\nFull data sent: %ld\n",data_sent); 
		} 
	} else if ( state>0 ) {
		//Master has sent data; read. 

		readBuffer[state - 1] = i2c_read(); //LSB first and MSB secound 

		if (state==2) { 
//			data_received=make16(readBuffer[1],readBuffer[0]); 
//			printf("\nFull data received: %ld\n",data_received); 
		} 
	} 
} 
#endif

