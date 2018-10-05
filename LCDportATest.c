/* 
 * File:   LCDportATest.c
 * Author: hirschag
 *
 * Created on May 10, 2016, 7:54 PM
 */

#include <xc.h>
#include "lcd4bit/lcd4bits.h"

#pragma config FOSC = INTRC_NOCLKOUT        // Oscillator Selection bits (HS oscillator: High-speed crystal/resonator on RA6/OSC2/CLKOUT and RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = ON       // RE3/MCLR pin function select bit (RE3/MCLR pin function is MCLR)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR21V   // Brown-out Reset Selection bit (Brown-out Reset set to 2.1V)
#pragma config WRT = OFF
/*
 * 
 */
void main(void) {
//    ANSEL = 0; //setting everything to 
//    ANSELH = 0;
    lcd_init();
    TRISD = 0;
    
    while(1) {
        RD0 = !RD0;
        lcd_puts("This is a test");
        lcd_goto(0x40);
        lcd_puts("This is another");
        DelayMs(500);
        lcd_clear();
        DelayMs(250);
    }
}

