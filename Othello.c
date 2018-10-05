/* 
 * File:   Othello.c
 * Author: hirschag & cai
 *
 * Created on May 9, 2016, 8:52 AM
 */

#include <xc.h>
#include <pic16f887.h>
#include "lcd4bit/lcd4bits.h"
#include "i2c/i2c.h"

#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator: High-speed crystal/resonator on RA6/OSC2/CLKOUT and RA7/OSC1/CLKIN)
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

#define DISPFREQ = 3035;

//definitions for the matrix
#define HT16K33_BLINK_CMD       0x80
#define HT16K33_BLINK_DISPLAYON 0x01
#define HT16K33_BLINK_OFF       0
#define HT16K33_BLINK_2HZ       1
#define HT16K33_BLINK_1HZ       2
#define HT16K33_BLINK_HALFHZ    3
#define HT16K33_CMD_BRIGHTNESS  0xE0

#define LED_ON 1
#define LED_OFF 0

#define LED_RED 1
#define LED_YELLOW 2
#define LED_GREEN 3

#define HT16K33_ADDRESS         0x70

const unsigned char KEYS[4][4] = {
                        {'1','2','3','A'},
                        {'4','5','6','B'},
                        {'7','8','9','C'},
                        {'*','0','#','D'}
                        };

unsigned char RedPos[] = {0,0,0,0b00010000,0b00001000,0,0,0};
unsigned char GreenPos[] = {0,0,0,0b00001000,0b00010000,0,0,0};
unsigned char AllPos[] = {0,0,0,0b00011000,0b00011000,0,0,0};

unsigned char color = 0; //this is red, or 'X'

unsigned char LCDStateCount = 0;
unsigned char LCDState = 0;
unsigned char LCDNextState = 0;
unsigned char played = 0;

void interrupt interrupt_handler(void) {
    if(TMR1IF == 1) {
        TMR1IF = 0;
        //about .25 second per overflow
        if(LCDStateCount > 8) {
            LCDNextState = 1;
            LCDStateCount = 0;
        }
        LCDStateCount++;
        
    }
}
/*
 * Pinout:
 * B, keypad
 * A, LCD (4bit mode)
 * D & C, LED matrix
 * 
 * timer 1 at 2 divisions
 * ccp1m3:0 set to 1010: generate software interrupt but leave ccp1 pin is unaffected 
 * This will be the timer for the animation of flipping, no interrupt
 *  
 * ccp2,3:0 is set to 1010: "
 * ccp2 is the one that controls the pointer flicker, if we can
 */

void matrixSetup() {
    I2C_Start();
    i2c_WriteTo(HT16K33_ADDRESS);
    I2C_SendByte(0x21); //set the state
    I2C_Stop();
    I2C_Start();
    i2c_WriteTo(HT16K33_ADDRESS);
    I2C_SendByte(HT16K33_CMD_BRIGHTNESS | 0xf); //set brightness to 15
    I2C_Stop();
    I2C_Start();
    i2c_WriteTo(HT16K33_ADDRESS);
    I2C_SendByte(0x81); //turn off blink
    I2C_Stop();
/*    I2C_Stop();
    
    I2C_Start();
    while(i2c_WriteTo(HT16K33_ADDRESS));
    I2C_SendByte(HT16K33_CMD_BRIGHTNESS | 0xf);
    I2C_Stop();
    
    //no blink
    I2C_Start();
    while (i2c_WriteTo(HT16K33_ADDRESS));
    I2C_SendByte(0x81);
    I2C_Stop();
 */
    

}
void setup(void) {
    lcd_init();
    I2C_Initialize();
    
    matrixSetup();
    
    ANSEL = 0; //setting everything to 
    ANSELH = 0;
    
    TRISB = 0x0F; //setting up for the keypad
    // b7:4 are output, B:3:0 are inputs
    nRBPU = 0; //enable all pull-ups
    //WPUB = 0x0F; //only need pull-ups on 3:0
    
    //outputs for the LED matrix
    TRISD = 0;
    TRISC = 0;
    
    //timer 1 on, PS set to 1:8
    TMR1ON = 1;
    TMR1GE = 0;
    T1CKPS1 = 1; T1CKPS0 = 1;
    TMR1CS = 0; //external clock
    TMR1IE = 0; //no interrupt
    TMR1IF = 0;
    
//    CCP1CON = 0x0A;
//    CCPR1 = TMR1 + 100;
////    CCP2CON = 0x0A;
////    
//    CCP1IF = 0;
//    CCP2IF = 0;
//    CCP1IE = 0; //not used
//    CCP2IE = 0; //unknown for now
    
    SPEN = 1;
    SYNC = 0;
    TXEN = 1; // Enable transmit side of UART
    CREN = 1; // Enable receive side of UART
    BRG16 = 1;
    BRGH = 1;
    SPBRG = 207;
    
    GIE = 1;
    PEIE = 1;
    
}

/*
 Serial stuff in case things don't work;
 */
void OutChar(unsigned char outchr) {
    while (TXIF == 0);
    TXREG = outchr;
}

unsigned char InChar(void) {
    while (RCIF == 0);
    return RCREG;
}

void OutCharMsg(const char *string) {
    while(*string != 0)
        OutChar(*string++);
}

/*
 * This function will check to make sure that all positions are currently valid
 */
unsigned char validateAll(void) {
    for(unsigned char i = 0; i<sizeof(AllPos); i++) {
        if(GreenPos[i] ^ RedPos[i] != AllPos[i])
            return 0;
    }
    return 1;
}

//checks to see if a position by bit manipulation
unsigned char checkPositionFree(unsigned char row, unsigned char col, unsigned char colorArray[]) {
    char temp = 0x80>>col;
    temp ^= colorArray[row];
    temp &= colorArray[row];
    temp = temp == colorArray[row];
    return temp;
}

void drawMatrix(void) {
    I2C_Start();
    RD1 = 1;
    i2c_WriteTo(HT16K33_ADDRESS);
    I2C_SendByte(0x00);
    for(char temp = 0; temp < 8; temp++) {
        I2C_SendByte(GreenPos[temp]);
        I2C_SendByte(RedPos[temp]);
    }
    RD1 = 0;
    I2C_Stop();
}
void sendMatrix(char PointerR, char PointerC) {
    OutCharMsg(" 12345678");
    OutChar(0x0A);
    OutChar(0x0D);
    for(char r = 0; r<8; r++) {
        OutChar(r + 0x31);
        for(char c = 0; c<8; c++) {
            if(r == PointerR && c == PointerC) {
                OutChar('!');
                continue;
            }
            if(checkPositionFree(r,c,AllPos) == 1) {
                OutChar('.');
            } else {
                if(checkPositionFree(r,c,GreenPos) == 1) {
                    if (checkPositionFree(r,c,RedPos)) {
                        OutChar('?');
                    } else {
                        OutChar('X');
                    }
                } else {
                    OutChar('O');
                }
            }
        }
        OutChar(0x0A);
        OutChar(0x0D);
        
    }
    OutChar(0x0A);
    OutChar(0x0D);
}

/*
 * This function will check all of the buttons on the keypad and return 1 if a 
 * button is pressed, otherwise will return 0
 */
unsigned char getKeypadKey(void) {
    unsigned char temp = 0xE0;
    unsigned char temp2 = 0;
    unsigned char col = 0;
    while(temp != 0xF0) {
        PORTB = temp;
        temp2 = PORTB & 0x0F;
        switch(temp2) {
            case 0b00001110:
                return KEYS[3][3-col];
            case 0b00001101:
                return KEYS[2][3-col];
            case 0b00001011:
                return KEYS[1][3-col];
            case 0b00000111:
                return KEYS[0][3-col];
            default:
                break;
        }
        col++;
        temp = ((temp & 0xF0) << 1) | 0x10;
    }
    return 0;
}

/*
 * This function will return the character at the position defined in the key
 * matrix
 */
unsigned char getKeypadPressed(void) {
    unsigned char temp = 0b11100000;
    unsigned char temp2 = 0;
    //unsigned char temp = 0;
    
    while((temp & 0xF0) != 0xF0) {
        PORTB = temp;
        temp2 = PORTB & 0x0F;
        if(temp2 != 0x0F) {
            return 1;
        }
        temp = ((temp & 0xF0) << 1) | 0x10;
    }
    return 0;
}

unsigned char legalPlay(unsigned char row, unsigned char col, unsigned char ownColor[], unsigned char otherColor[]) {
    signed char r;
    signed char c;
    unsigned char nextR;
    unsigned char nextC;
    unsigned char line;
   
    if (row < 8 && col < 8 && checkPositionFree(row, col, AllPos)) {
        //legal = 0;
        for(r = -1; r<2; r++) {
            for(c = -1; c<2; c++) {
                nextR = row + r;
                nextC = col + c;
                //sendMatrix(nextR, nextC);
                line = 0;
                while(checkPositionFree(nextR,nextC,AllPos) == 0) {
                    
                    if(!(nextR > 7 && nextC > 7)) { //legal, both are unsigned, 0 is min, -1 is big
                        if((checkPositionFree(nextR, nextC, otherColor) == 0) && (checkPositionFree(nextR,nextC,ownColor) == 1)) { //is other color and it is taken
                            line = 1;
                        } else {
                            if(line == 1) {
                                return 1; //could be return 1
                            } else {
                                break;
                            }
                        }
                    }
                    nextR = nextR + r;
                    nextC = nextC + c;   
                    //sendMatrix(nextR, nextC);
                }
            }
        }
    }
    return 0; //could be return 0 
}

void drawFrom(unsigned char row, unsigned char col, char r, char c, unsigned char ownColor[], unsigned char otherColor[]) {
    do {
        ownColor[row] |= 0x80 >> col;
        otherColor[row] ^= 0x80 >> col;
        row += r;
        col += c;
        //sendMatrix(row,col);
    } while(checkPositionFree(row, col, ownColor));
}

void playPeice(unsigned char row, unsigned char col, unsigned char ownColor[], unsigned char otherColor[]) {
    signed char r;
    signed char c;
    unsigned char nextR;
    unsigned char nextC;
    unsigned char line;
   
    if (row < 8 && col < 8 && checkPositionFree(row, col, AllPos)) {
        ownColor[row] |= 0x80 >> col;
        
        for(r = -1; r<2; r++) {
            for(c = -1; c<2; c++) {
                nextR = row + r;
                nextC = col + c;
                //sendMatrix(nextR, nextC);
                line = 0;
                while(checkPositionFree(nextR,nextC,AllPos) == 0) {
                    
                    if(!(nextR > 7 && nextC > 7)) { //legal, both are unsigned, 0 is min, -1 is big
                        if((checkPositionFree(nextR, nextC, otherColor) == 0) && (checkPositionFree(nextR,nextC,ownColor) == 1)) { //is other color and it is taken
                            line = 1;
                        } else {
                            if(line == 1) {
                                drawFrom(row + r,col + c,r,c,ownColor,otherColor); //could be return 1
                            } else {
                                break;
                            }
                        }
                    }
                    nextR = nextR + r;
                    nextC = nextC + c;   
                    //sendMatrix(nextR, nextC);
                }
            }
        }
        AllPos[row] |= 0x80 >> col;        
    }
}

unsigned char checkFullBoard(void) {
    for(unsigned char i = 0; i < 8; i++) {
        if(AllPos[i] != 0xFF)
            return 0;
    }
    return 1;
}

unsigned char countColor(unsigned char color[]) {
    unsigned char count = 0;
    unsigned char temp = 0;
    for(unsigned char i = 0; i < 8; i++) {
        temp = color[i];
        while(temp != 0) {
            count += temp & 0x01;
            temp = temp >> 1;
        }
    }
    return count;
}

void displayNuber(unsigned char num) {
    lcd_putch(0x30 + num/10);
    lcd_putch(0x30 + num%10);
}

void main(void) {
    unsigned char row = 0;
    unsigned char col = 0;
    for(unsigned int temp = 0; temp < 3000; temp++); //delay for lcd initialization
    setup();
    LCDNextState = 1;
    //introduction!
    lcd_puts("Welcome to ");
    lcd_goto(0x40);
    lcd_puts("Othello!");
    DelayMs(5000);
    lcd_clear();
    lcd_puts("Press any key");
    lcd_goto(0x40);
    lcd_puts("to start!");
    while(!getKeypadPressed());
    DelayMs(10);
    while(getKeypadPressed());
    TMR1IE = 1;
    drawMatrix();
    OutChar(0x0C); //new form, clears the serial console.
    OutCharMsg("! is pointer, . is empty, green is 'O', red is 'X'");
    OutChar(0x0A);
    OutChar(0x0D);
    sendMatrix(-1,-1);
    while(!checkFullBoard()) {
        if(LCDNextState == 1) {
            LCDNextState = 0;
            lcd_clear();
            switch(LCDState) {
                case 0:
                    if(color == 0) {
                        lcd_puts("Red's Turn");
                    } else {
                        lcd_puts("Green's Turn");
                    }
                    LCDState++;
                    break;
                case 1:
                    lcd_puts("Press key 1-8");
                    lcd_goto(0x40);
                    lcd_puts("to enter move");
                    LCDState++;
                    break;
                case 4:
                    lcd_puts("Red: ");
                    displayNuber(countColor(RedPos));
                    lcd_goto(0x40);
                    lcd_puts("Green: ");
                    displayNuber(countColor(GreenPos));
                    LCDState++;
                    break;
                case 5:
                    lcd_puts("Total Played:");
                    lcd_goto(0x40);
                    displayNuber(countColor(AllPos) - 4);
                    LCDState = 0;
                    break;
                case 2:
                    lcd_puts("Press * to");
                    lcd_goto(0x40);
                    lcd_puts("select move");
                    LCDState++;
                    break;
                case 3:
                    lcd_puts("Row first");
                    lcd_goto(0x40);
                    lcd_puts("Col second");
                    LCDState++;
                    break;
                default:
                    LCDState = 0;
            }
           
        }
        if(getKeypadPressed()) {
            row = 0; col = 0;
            row = getKeypadKey() - 0x30;
            while(getKeypadPressed());
            if(!(row >= 1 && row <= 8)) {
                lcd_clear();
                lcd_puts("Please select");
                lcd_goto(0x40);
                lcd_puts("keys 1-8");
                DelayMs(2000);
            } else {
                lcd_clear();
                lcd_puts("Row: ");
                lcd_putch(row + 0x30);
                lcd_goto(0x40);
                lcd_puts("Col: ");
                while(!getKeypadPressed());
                DelayMs(10);
                col = getKeypadKey() - 0x30;
                while(getKeypadPressed());
                if(!(col >= 1 && col <= 8)) {
                    lcd_clear();
                    lcd_puts("Please select");
                    lcd_goto(0x40);
                    lcd_puts("keys 1-8");
                    DelayMs(2000);
                } else {
                    lcd_putch(col + 0x30);
                    while(getKeypadKey() != '*');
                    while(getKeypadPressed());
                    row--;
                    col--;
                    sendMatrix(row,col);
                    if(color == 0) {
                        if(legalPlay(row,col,RedPos,GreenPos)) {
                            color = 1;
                            playPeice(row,col,RedPos,GreenPos);
                            sendMatrix(-1,-1);
                        } else {
                            lcd_clear();
                            lcd_puts("Invalid Play");
                        }
                    } else {
                        if(legalPlay(row,col,GreenPos,RedPos)) {
                            color = 0;
                            playPeice(row,col,GreenPos,RedPos);
                            sendMatrix(-1,-1);
                        } else {
                            lcd_clear();
                            lcd_puts("Invalid Play");
                        }
                    }
                    DelayMs(3000);
                    drawMatrix();
                    OutChar(0x0C); //new form, clears the serial console.
                    OutCharMsg("! is pointer, . is empty, green is 'O', red is 'X'");
                    OutChar(0x0A);
                    OutChar(0x0D);
                    sendMatrix(-1,-1);
                    
                }
            }
           
        }        
        
    }
    while(1) {
        if(LCDNextState == 1) {
            LCDNextState = 0;
            lcd_clear();
            switch(LCDState) {
                case 0:
                    lcd_puts("Game over");
                    LCDState++;
                    break;
                case 1:
                    lcd_puts("Red: ");
                    displayNuber(countColor(RedPos));
                    lcd_goto(0x40);
                    lcd_puts("Green: ");
                    displayNuber(countColor(GreenPos));
                    LCDState++;
                    break;
                case 2:
                    if(countColor(RedPos) > countColor(GreenPos)) {
                        lcd_puts("Red Wins!");
                    }
                    else if(countColor(RedPos) < countColor(GreenPos)) {
                        lcd_puts("Green Wins!");
                    } else {
                        lcd_puts("Tie!");
                    }
                    LCDState++;
                    break;
                case 3:
                    lcd_puts("Reset to Restart");
                    LCDState = 0;
                    break;
                default:
                    LCDState = 0;
            }
        }
    }
    
}



