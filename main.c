//comments

/*pins for a logic analyzer*/
//TMR0->RC3->ch3(the channel 3 is a logic analyzer channel)
//capture->RC4->ch1
//meander generator->RC5->ch5
    //RC2 is a capture pin(->RC5->ch5)
//TMR1->RC7->ch7   

//pragma
// CONFIG
#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable bit (BOR disabled)
#pragma config LVP = OFF        // Low-Voltage (Single-Supply) In-Circuit Serial Programming Enable bit (RB3 is digital I/O, HV on MCLR must be used for programming)
#pragma config CPD = OFF        // Data EEPROM Memory Code Protection bit (Data EEPROM code protection off)
#pragma config WRT = OFF        // Flash Program Memory Write Enable bits (Write protection off; all program memory may be written to by EECON control)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)

//include
#include <xc.h>

//define
#define _XTAL_FREQ 20000000
#define WHEEL_CIRCUMFERENCE 1.413 //meters
//structs
struct wheel{
    char N_number;
    char current_period_calculated;
    unsigned int TMR1_overflow_counter;
    unsigned int TMR1_last_value;//TMR1 register value
};

//function prototypes
unsigned long T_computing(struct wheel * w0);


//global varaibales
unsigned int GLOBAL_TMR1_overflow_counter=0;
struct wheel w1={0, 1, 0 ,0};
unsigned long TTTT;
float V=0; //for speed

int main(void)
{   
    //config
    //PORTC
    TRISCbits.TRISC7=0;
    TRISCbits.TRISC5=0; //0-as output
    TRISCbits.TRISC4=0;
    TRISCbits.TRISC3=0;
    
    //TMR0
        //prescaler 1:4
        OPTION_REGbits.PSA=0;//0 = Prescaler is assigned to the Timer0 module
        OPTION_REGbits.PS2=0; //001 for 1:4
        OPTION_REGbits.PS1=0;
        OPTION_REGbits.PS0=1;
        
        //clock select
        OPTION_REGbits.T0CS=0;//0 = Internal instruction cycle clock (CLKOUT)
        
        //interrupts
        INTCONbits.TMR0IF=0;
        INTCONbits.TMR0IE=0;
    
    //TMR1
    //T1CON
       //prescaler 1:1
       T1CONbits.T1CKPS1=0; 
       T1CONbits.T1CKPS0=0;
       
       T1CONbits.T1OSCEN=1; //1 = Oscillator is enabled
       T1CONbits.TMR1CS=0; //0 = Internal clock (FOSC/4)  
       T1CONbits.TMR1ON=1;
       
    //interrupt
       PIR1bits.TMR1IF=0; //reset the TMR1 interrupt flag
       PIE1bits.TMR1IE=0; //disable TMR1 interrupts
        
    //capture module
        //capture pin
        TRISCbits.TRISC2=1; //1-as input RC2
        //CCP1CON
        CCP1CON=0b00000101; //0101 = Capture mode, every rising edge
        //interrupts
        PIR1bits.CCP1IF=0; //reset the capture interrupt flag
        PIE1bits.CCP1IE=1; //enable capture interrupts
    
    //other things
        INTCONbits.PEIE=1; //1 = Enables all unmasked peripheral interrupts
        INTCONbits.GIE=1; //enable global interrupts
        
    #define MEANDER_DELAY 7000 //for 12.53 ms
    unsigned long g=0;
    while(1)
    {
        
/*meander generator*/
//RC2 as input <- RC5
    PORTCbits.RC5=1;        
    g=MEANDER_DELAY;    //=1700
    while(g){--g;}

    PORTCbits.RC5=0;
    g=MEANDER_DELAY;    //=1700
    while(g){--g;}

/*speed computing*/        
    //T_computing
    if(!w1.current_period_calculated)
    {
        w1.current_period_calculated=1; //period is calculated
        if(w1.N_number==2)
        {
            TTTT=T_computing(&w1);//us
            w1.N_number=0;
            V=WHEEL_CIRCUMFERENCE/TTTT; //m/us
            V*=1000000;                 //m/s
            V*=3.6;                     //km/h
        }
    }
    
    
    }
    return 0;
}

//interrupt
void interrupt something(void)
{
    //TMR0
    if(INTCONbits.TMR0IF && INTCONbits.TMR0IE)
    {
        PORTCbits.RC3=0;
        INTCONbits.TMR0IF=0; //reset the TMR0 interrupt flag
        INTCONbits.TMR0IE=0; //dead zone off
        
        PIR1bits.CCP1IF=0; //reset the capture interrupt flag
        PIE1bits.CCP1IE=1; //dead zone
    }
    //TMR1
    if(PIR1bits.TMR1IF && PIE1bits.TMR1IE)
    {
        PIR1bits.TMR1IF=0; //reset a TMR1 interrupt flag
        ++GLOBAL_TMR1_overflow_counter;
        PORTCbits.RC7=!PORTCbits.RC7; //for a logic analyzer
    }
    //capture RC2
    if(PIR1bits.CCP1IF && PIE1bits.CCP1IE)
    {
        //TMR0
        PORTCbits.RC3=1;
        INTCONbits.TMR0IF=0;
        TMR0=12;
        INTCONbits.TMR0IE=1;
        
        //w1
        w1.current_period_calculated=0; //calculate me!
        w1.TMR1_overflow_counter=GLOBAL_TMR1_overflow_counter;
        w1.TMR1_last_value=CCPR1;//TMR1 register value
        ++w1.N_number;
        
        //TMR1
        PIR1bits.TMR1IF=0; //reset the TMR1 interrupt flag
        PIE1bits.TMR1IE=1; //1 - enable TMR1 interrupts
        TMR1=0;
        GLOBAL_TMR1_overflow_counter=0;
        
        
        //capture
        PIR1bits.CCP1IF=0; //reset the capture interrupt flag
        PIE1bits.CCP1IE=0; //dead zone
        PORTCbits.RC4=!PORTCbits.RC4;
    }
}
//functions
unsigned long T_computing(struct wheel * w0)
{
    unsigned long T;
    //T=((unsigned long)w0->TMR1_overflow_counter*65535*0.2)+((unsigned long)w0->TMR1_last_value*0.2);
    //65535*0.2 -> 
    T=(((unsigned long)w0->TMR1_overflow_counter)*13107)+(((unsigned long)w0->TMR1_last_value)/5);
    return T;//us
}