/*
Program performs writing of byte array at certain address in EEPROM after pressing
button1. And reading of this byte array after pressing button2 (writing and reading
is performed starting from specified address 'start_address')
*/
#include <msp430FG4618.h>
    
#define COUNT 10   // amount of bytes to write and read
#define ADDRESS 0  // initial address for writing and reading

    // unsigned char - this is 8-bits integer data type with min. value 0
    // and max. value 255
unsigned char start_address = ADDRESS;  // initial address for writing and reading
                                        // of an array that consists of COUNT bytes
unsigned char byte_array[COUNT] = {0x01, 0x02, 0x03, 0x04, 0x05,\
                                    0x06, 0x07, 0x08, 0x09, 0x0A};
                                    // array of numbers of size COUNT bytes
unsigned char received_byte_array[COUNT] = {0x00, 0x00, 0x00, 0x00,\
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                        // array of COUNT bytes received from EEPROM
                        // (initialized with zeros)
unsigned char mask;     // mask for pressed button pin determination
int i = 0;              // variable for counter
volatile unsigned int j = 0; // variable for counter (is used for delay)

void init(void);     // prototype of initialization function
void write(void);    // prototype of writing to EEPROM function
void read(void);     // prototype of reading from EEPROM function

void main (void) { 
    init();     // call initialization function
}

/* initialization function */
void init(void) {   

    WDTCTL = WDTPW + WDTHOLD;   // stop watchdog timer
    
    P3SEL |= BIT1 + BIT2;   // choose alternative (I2C) function for P3.1 and P3.2
    
    // ports configuration for button1 and button2
    P1DIR &= ~(BIT0 + BIT1);    // input direction for button1 and button2 (P1.0, P1.1)
    P1SEL &= ~(BIT0 + BIT1);    // GPIO mode for button1 and button2 (P1.0, P1.1)
    P1IES &= ~(BIT0 + BIT1);    // interrupt on 0 to 1 transition (pressed) for 
                                // button1 and button2  
                            
    P1IFG &= ~(BIT0 + BIT1);    // to avoid instant interrupt request we reset its flag
                                // for pins P1.0, P1.1 prior to enabling interrupts
    P1IE |= BIT1 + BIT0;        // enable interrupts for P1.0 and P1.1
    
    // initial USCI module configuration
    UCB0CTL1 |= UCSWRST;        // module configuration mode is on
    // master, I2C, synchronous mode
    UCB0CTL0 |= UCMST + UCMODE0 + UCMODE1 + UCSYNC; 
    
    // source SCL frequency is equal to SMCLK = 1 MHz but later it is divided,
    // transmission mode, holding configuration mode
    UCB0CTL1 = UCSSEL1 + UCSSEL0;
    
    UCB0BR0 = 10;               // denominator is 10, i.e. final SCL frequency
                                // is 100 kHz
    UCB0BR1 = 0;                // high 8 bits of denominator
    UCB0I2CSA = 0x50;           // address of slave device (EEPROM) on I2C bus    
    UCB0CTL1 &= ~UCSWRST;       // module configuration mode is off,
                                // module is ready to work
 
    __bis_SR_register(LPM0_bits + GIE); // CPU-off, OSC-on, GIE
}

/* writing to EEPROM function */
void write(void) { 

    // DMA controller configuration
    DMACTL0 = 0x000D;   // DMA0TSEL = 1101 - choice setting UCB0TXIFG flag as
                        // interrupt source for channel DMA0                      
    DMACTL1 = 0x04;     // start of transfer after selection of next CPU command,
                        // priorities of channels remain unchanged, halt of transfer
                        // in case of NMI is forbidden
                        
    // DMA0 channel configuration
    DMA0SA = (void (*)())&byte_array;   // source address (address of array)  
                                        // term (void (*)()) is required for
                                        // transformation of address to 20 bits
                                        // format because DMA0SA is 20 bits length
    DMA0DA = (void (*)())&UCB0TXBUF;    // destination address (address 0x006F)
    DMA0SZ = 1;                         // counter of transfers (single transfer)
    DMA0CTL = 0x03E4;   // single transfer without repeat, destination address
                        // remains unchanged, source address increases by 1, byte
                        // exchange between source and destination, transfer by level,
                        // channel enable flag is reset, channel interrupt generating
                        // flag is reset, flag of channel interrupt enable is set.
                        // flag of suspension of transfers after NMI is reset,
                        // DMAREQ bit is reset
                    
    // creating packets for EEPROM writing
    start_address = ADDRESS;    // start address initialization    
    i = 1;              // variable of write operations counter is initialized with 1
    
    UCB0CTL1 |= UCTR + UCTXSTT; // transmission mode, start condition generating
                                
    while(!(IFG2 & UCB0TXIFG)); // wait for bit UCB0TXIFG set - it means that 
                                // 1st byte of data (address of byte in EEPROM)
                                // can be written in transmission buffer
    UCB0TXBUF = start_address;  // write byte of data that contains low 8 bits of
                                // address of EEPROM cell in transmission buffer
    while(!(IFG2 & UCB0TXIFG)); // wait for bit UCB0TXIFG set - it means that next
                                // byte of data can be written in transmission buffer
    start_address++;            // increase address of EEPROM cell for 1
    DMA0CTL |= DMAEN;           // launch single transfer (more precisely, we enable
                                // but it should launch immediately
}

/* reading from EEPROM function */
void read(void) {
    
    // DMA controller configuration
    DMACTL0 = 0x000C;   // DMA0TSEL = 1101 - choice setting UCB0RXIFG flag as
                        // interrupt source for channel DMA0                       
    DMACTL1 = 0x04;     // start of transfer after selection of next CPU command,
                        // priorities of channels remain unchanged, halt of transfer
                        // in case of NMI is forbidden
    
    // DMA0 channel configuration
    DMA0SA = (void (*)())&UCB0RXBUF;    // source address (address 0x006E)
                                        // term (void (*)()) is required for
                                        // transformation of address to 20 bits
                                        // format because DMA0SA is 20 bits length
    DMA0DA = (void (*)())&received_byte_array;  // destination address
    DMA0SZ = 1;         // counter of transfers (single transfer)
    DMA0CTL = 0x0CE4;   // single transfer without repeat, destination address
                        // increases for 1, source address remains unchanged, byte
                        // exchange between source and destination, transfer by level,
                        // channel enable flag is reset, channel interrupt generating
                        // flag is reset, flag of channel interrupt enable is set.
                        // flag of suspension of transfers after NMI is reset,
                        // DMAREQ bit is reset
                                 
    // creating packets for EEPROM reading 
    start_address = ADDRESS;    // start address initialization    
    i = 1;              // variable of write operations counter is initialized with 1
    
    UCB0CTL1 |= UCTR + UCTXSTT; // transmission mode, start condition generating
    while(!(IFG2 & UCB0TXIFG)); // wait for bit UCB0TXIFG set - it means that 
                                // 1st byte of data (address of byte in EEPROM)
                                // can be written in transmission buffer
    UCB0TXBUF = start_address;  // write byte of data that contains low 8 bits of
                                // address of EEPROM cell in transmission buffer
    while(!(IFG2 & UCB0TXIFG)); // wait for bit UCB0TXIFG set - it means that next
                                // byte of data can be written in transmission buffer   
    UCB0CTL1 &= ~UCTR;          // receicer mode
    UCB0CTL1 |= UCTXSTT;        // start condition generating
    while(!(IFG2 & UCB0RXIFG)); // wait for bit UCB0RXIFG set - it means that 1st
                                // byte of data from EEPROM is read to reception
                                // buffer and MCU acknowledged its reception 
    start_address++;            // increase address of EEPROM cell for 1
    DMA0CTL |= DMAEN;           // launch single transfer (more precisely, we enable
                                // but it should launch immediately   
}

/* Button1 or button2 was pressed */
#pragma vector = PORT1_VECTOR
__interrupt void PORT1_ISR(void) {
    
    mask = P1IFG;               // read state of P1IFG register

    P1IE &= ~(BIT0 + BIT1);     // forbid interrupts from P1.0, P1.1 to avoid
                                // electrical bouncing

    if(mask & 0x01) // button1
        write();    // launch write function
    else            // button2
        read();     // launch read function
    
    for(j = 0; j < 30000; j++); // 30 ms delay to avoid electrical bouncing

    P1IFG &= ~(BIT0 + BIT1);    // reset interrupt flag for P1.0 and P1.1
                                // prior to enabling interrupts
    P1IE |= BIT0 + BIT1;        // again enable interrupts for P1.0 and P1.1

    __bis_SR_register_on_exit(LPM0_bits + GIE); // exit from interupt service
                                                // CPU-off, OSC-on, GIE
}

/* Transfer performed */
#pragma vector = DMA_VECTOR
__interrupt void DMA_ISR(void) {
    
    UCB0CTL1 |= UCTXSTP;    // end condition generating
    IFG2 &= ~(UCB0RXIFG + UCB0TXIFG);   // reset interrupt flags from receiver
                                        // and transmitter buffers                                 
    DMA0CTL &= ~DMAIFG;                 // channel interrupt flag reset
    
    if(i < COUNT){  // if amount of read/write operations is less than COUNT,
                    // then perform next read/write operation
        if(DMACTL0 & BIT0){     // if interrupt source is setting of UCB0TXIFG flag
            UCB0CTL1 |= UCTR + UCTXSTT; // transmission mode, start condition generating
                                        
            while(!(IFG2 & UCB0TXIFG)); // wait for bit UCB0TXIFG set - it means that 
                                        // 1st byte of data (address of byte in EEPROM)
                                        // can be written in transmission buffer            
            UCB0TXBUF = start_address;  // write byte of data that contains low 8 bits of
                                        // address of EEPROM cell in transmission buffer
            while(!(IFG2 & UCB0TXIFG)); // wait for bit UCB0TXIFG set - it means that 
                                        // next byte of data can be written
                                        // in transmission buffer 
        } else {                // if interrupt source is setting of UCB0RXIFG flag
            UCB0CTL1 |= UCTR + UCTXSTT; // transmission mode, start condition generating
                                        
            while(!(IFG2 & UCB0TXIFG)); // wait for bit UCB0TXIFG set - it means that 
                                        // 1st byte of data (address of byte in EEPROM)
                                        // can be written in transmission buffer            
            UCB0TXBUF = start_address;  // write byte of data that contains low 8 bits of
                                        // address of EEPROM cell in transmission buffer
            while(!(IFG2 & UCB0TXIFG)); // wait for bit UCB0TXIFG set - it means that 
                                        // next byte of data can be written
                                        // in transmission buffer   
            UCB0CTL1 &= ~UCTR;          // receiver mode
            UCB0CTL1 |= UCTXSTT;        // start condition generating
            while(!(IFG2 & UCB0RXIFG)); // wait for bit UCB0RXIFG set - it means that 1st
                                        // byte of data from EEPROM is read to reception
                                        // buffer and MCU acknowledged its reception
        }

        start_address++;        // launch single transfer (more precisely, we enable
                                // but it should launch immediately
        i++;            // increase counter of read/write operations variable for 1
    }      
    __bis_SR_register_on_exit(LPM0_bits + GIE); // exit from interupt service
                                                // CPU-off, OSC-on, GIE
}