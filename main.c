/*
 * MSP430G2253 ADC_3GPIO v1.c
 *
 *  Created on: 20/09/2013
 *  	Author: Ant Scranney
 */
#include <msp430.h>
// Global Variables
unsigned int adc[4] = {0};					// This will hold the distance values
unsigned int distance1= 0;
unsigned int distance2 = 0;


char receive = ' ';

//Function Prototypes
void Setup_HW();						// Setup watchdog timer, clockc, ADC ports
void Read_ADC();						// This function reads the ADC and stores the x, y and z values
void UARTSetup();

char* itoa (int value, char *result, int base);
void sendChar(char c);
void sendInt(int num);
void sendString(char str[]);

int main(void)
{
	UARTSetup();
	__enable_interrupt();
	Setup_HW();
	while (1)
	{
		Read_ADC();							// This function reads the ADC and stores the values
	    //__bis_SR_register(CPUOFF + GIE);        // LPM0, ADC10_ISR will force exit
		__delay_cycles(100000);
	}
}

// ADC10 interrupt service routine
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR()
{
  __bic_SR_register_on_exit(CPUOFF);        // Clear CPUOFF bit from 0(SR)
}

void Setup_HW(void)
{
	WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
	ADC10CTL1 = INCH_3 + CONSEQ_1;            // A2/A1/A0, single sequence
	ADC10CTL0 = ADC10SHT_2 + MSC + ADC10ON + ADC10IE;
	ADC10DTC1 = 0x02;                         // 2 conversions
	ADC10AE0 |= 0x18;                         // Disable digital I/O on P1.0 to P1.2
	//ADC10SA = (unsigned int)adc;				// Copies data in ADC10SA to unsigned int adc array
}

void Read_ADC()
{
    ADC10CTL0 &= ~ENC;
    while (ADC10CTL1 & BUSY);               // Wait if ADC10 core is active
    ADC10SA = (unsigned int)adc;			// Copies data in ADC10SA to unsigned int adc array
    ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start
    __delay_cycles(50);

    distance1 = adc[0];						// adc array 0 copied to the variable distance1
    distance2 = adc[2];						// adc array 1 copied to the variable distance2

    sendInt(distance1);
    sendChar(',');
    sendInt(distance2);
	sendChar((char)0x0D);					// Send carriage Return
	sendChar((char)0x0A);					// Send Newline

    //__bis_SR_register(CPUOFF + GIE);        // LPM0, ADC10_ISR will force exit
}


void UARTSetup()
{
	/* Use Calibration values  for 1MHz    Clock   DCO*/
	DCOCTL =   0;
	BCSCTL1    =   CALBC1_1MHZ;
	DCOCTL =   CALDCO_1MHZ;

	/* Configure   Pin Muxing  P1.1    RXD and P1.2    TXD */
	P1SEL  =   BIT1    |   BIT2;
	P1SEL2 =   BIT1    |   BIT2;

	/* Place   UCA0    in  Reset   to  be  configured  */
	UCA0CTL1   =   UCSWRST;

	/* Configure   */
	UCA0CTL1   |=  UCSSEL_2;   //  SMCLK
	UCA0BR0    =   104;    //  1MHz    9600
	UCA0BR1    =   0;  //  1MHz    9600
	UCA0MCTL   =   UCBRS0; //  Modulation  UCBRSx  =   1

	/* Take    UCA0    out of  reset   */
	UCA0CTL1   &=  ~UCSWRST;

	/* Enable  USCI_A0 RX  interrupt   */
	IE2    |=  UCA0RXIE;
}

#pragma vector=USCIAB0RX_VECTOR
__interrupt void    USCI0RX_ISR()
{
	while  (!(IFG2&UCA0TXIFG));    				// USCI_A0 TX  buffer  ready?
	receive  =   UCA0RXBUF;  					// Stored received char in receive
}

void sendString(char str[])
{
	int i=0;									// Set i as a counter variable
	while(str[i] != '\0')
	{
		sendChar(str[i]);						// Sends the string one char at a time
		i++;
	}
}

void sendChar(char c)
{
	while (!(IFG2 & UCA0TXIFG)); 				// USCI_A0 TX buffer ready? (AKA: checks if the char is ready to send)
	UCA0TXBUF = c;								// Send char c
//	return c;
}

void sendInt(int num)
{
	char str[4];

	itoa (num, str, 10);
	sendString(str);
}


char* itoa (int value, char *result, int base)
{
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}
