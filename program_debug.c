/*
 This program performs write of single byte of data (0xFF) at address 0x04
 in EEPROM and subsequent read of cell at this address
*/
#include <msp430FG4618.h>
    // unsigned char - this is 8-bits integer data type with min. value 0
    // and max. value 255
unsigned char data_word_address; // word with low 8 bits of address of a
                                 // single-byte EEPROM cell
unsigned char byte_of_data;      // byte of data
unsigned char received_byte;     // byte that was received from EEPROM

void main (void) {
    // initialization
    WDTCTL = WDTPW + WDTHOLD;   // hold watchdog timer
    
    P3SEL |= BIT1 + BIT2;       // select alternative (I2C) function
                                // for P3.1 and P3.2
    
    UCB0CTL1 |= UCSWRST;        // mode of module configuration
    // master, I2C, synchronous mode
    UCB0CTL0 |= UCMST + UCMODE0 + UCMODE1 + UCSYNC; 
    // source SCL frequency is equal to SMCLK = 1 MHz but later it is divided,
    // transmission mode, holding configuration mode
    UCB0CTL1 |= UCSSEL1 + UCSSEL0 + UCTR; 
    // configuration mode, master, I2C, synchronous mode
    UCB0CTL0 |= UCSWRST + UCMST + UCMODE0 + UCMODE1 + UCSYNC;
    
    UCB0BR0 = 0x0A;             // denominator is 10, i.e. final SCL frequency
                                // is 100 kHz
    UCB0BR1 = 0;                // high 8 bits of denominator
    UCB0I2CSA = 0x50;           // address of slave device (EEPROM) on I2C bus    
    UCB0CTL1 &= ~UCSWRST;       // module configuration mode is off,
                                // module is ready to work
   
    data_word_address = 0x04;   // 5th cell of EEPROM
    byte_of_data = 0xFF;        // data is 11111111
    received_byte = 0x00;       // initializing variable
    
    // configuration for transmission
    UCB0CTL1 |= UCTR + UCTXSTT; // transmission mode, start condition generating

    // wait for transmitter availability
    while(UCB0CTL1 & UCTXSTT);  // wait for bit UCTXSTT reset - it means that slave
                                // just acknowledged reception of 1st byte of address

    // byte of data transmission
    UCB0TXBUF = data_word_address;  // write byte of data that contains low 8 bits of
                                    // EEPROM cell address into transmission buffer
    while(UCB0CTL1 & UCTXSTT);  // wait for bit UCTXSTT reset - it means that slave
                                // just acknowledged reception of 2nd byte of address
    UCB0TXBUF = byte_of_data;   // write byte of useful data into transmission buffer
                               
    UCB0CTL1 |= UCTXSTP;        // end condition generating
    while(UCB0CTL1 & UCTXSTT);  // wait for bit UCTXSTT reset - it means that slave
                                // just acknowledged byte of useful data reception
    
    // configuration for reception
    UCB0CTL1 |= UCTR + UCTXSTT; // transmission mode, start condition generating
    while(UCB0CTL1 & UCTXSTT);  // wait for bit UCTXSTT reset - it means that slave
                                // just acknowledged reception of 1st byte of address
    UCB0TXBUF = data_word_address;  // write byte of data that contains low 8 bits of
                                    // EEPROM cell address into transmission buffer
    while(UCB0CTL1 & UCTXSTT);  // wait for bit UCTXSTT reset - it means that slave
                                // just acknowledged reception of 2nd byte of address
    UCB0CTL1 &= ~UCTR;          // receiver mode
    UCB0CTL1 |= UCTXSTT;        // start condition generating

    // wait for receiver availability
    while(UCB0CTL1 & UCTXSTT);  // wait for bit UCTXSTT reset - it means that slave
                                // just acknowledged reception of 1st byte of address

    // data reception
    UCB0CTL1 |= UCTXSTP;        // end condition generating
    received_byte = UCB0RXBUF;  // read byte of useful data from reception buffer
    
    // turning device off
    UCB0CTL1 |= UCSWRST;        // transer USCI module from working mode
                                // to configuration mode
    P3SEL &= ~(BIT1 + BIT2);    // transfer P3.1 и P3.2 to GPIO mode
}