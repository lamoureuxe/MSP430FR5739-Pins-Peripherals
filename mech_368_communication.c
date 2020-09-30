#include <msp430.h>

/**
 * MECH 368 - Instrumentation and Measurement
 * Mechanical Engineering
 * University of British Columbia
 * Erik Lamoureux
 * Teaching Assistant
 * July 29, 2020
 */

unsigned char RxByte = 0;

volatile unsigned int x;
volatile unsigned int y;
volatile unsigned int z;

volatile unsigned int t;

volatile unsigned int temp_ms5b;
volatile unsigned int temp_ls5b;

volatile unsigned int P1_0_ms5b;
volatile unsigned int P1_0_ls5b;

volatile unsigned int P1_1_ms5b;
volatile unsigned int P1_1_ls5b;

volatile unsigned int P1_2_ms5b;
volatile unsigned int P1_2_ls5b;

volatile unsigned int P1_3_ms5b;
volatile unsigned int P1_3_ls5b;

volatile unsigned int P1_4_ms5b;
volatile unsigned int P1_4_ls5b;

volatile unsigned int P1_5_ms5b;
volatile unsigned int P1_5_ls5b;

volatile unsigned int P3_0_ms5b;
volatile unsigned int P3_0_ls5b;

volatile unsigned int P3_1_ms5b;
volatile unsigned int P3_1_ls5b;

volatile unsigned int P3_2_ms5b;
volatile unsigned int P3_2_ls5b;

volatile unsigned int P3_3_ms5b;
volatile unsigned int P3_3_ls5b;

/**
 * main.c
 */
int main(void)
{
    int i;

    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    // Configure clocks
    CSCTL0 = 0xA500;  // Write password to modify CS registers
    CSCTL1 = DCOFSEL0 + DCOFSEL1;  // DCO = 8 MHz
    CSCTL2 = SELM0 + SELM1 + SELA0 + SELA1 + SELS0 + SELS1; // MCLK = DCO, ACLK = DCO, SMCLK = DCO
    CSCTL3 = DIVM_3 + DIVA_3 + DIVS_3; // MCLK = ACLK = SMCLK = 8MHz/8 = 1MHz

    // Timer TB0.0 enabled
    P2DIR |= BIT5;
    P2SEL0 |= BIT5;
    P2SEL1 &= ~BIT5;
    // Timer B - data rate of 100 Hz
    TB0CCR0 = 10000; // period of timer B: SMLK = 8MHz/8; 1MHz/100Hz = 10000 cycles = 2710;
    TB0CTL = TBSSEL_2 + MC_1; // set SMCLK as source, MC_1 sets up mode MC=01b; no input divide;
    TB0CCTL0 |= CCIE; // enable interrupt

    // Configure ports for UART
    P2SEL0 &= ~(BIT0 + BIT1);
    P2SEL1 |= BIT0 + BIT1;
    // Configure UART0
    UCA0CTLW0 = UCSSEL_1;                    // Run the UART using ACLK
    UCA0MCTLW = UCOS16 + UCBRF_8 + 0x2000;   // Baud rate = 9600 from an 1 MHz clock
    UCA0BRW = 6;
    UCA0IE |= UCRXIE;                       // Enable UART Rx interrupt

    // Enable LED1 and LED2 to be lit based on input
    PJDIR |= BIT0 + BIT1 + BIT2 + BIT3; // PJ.0, PJ.1, PJ.2, PJ.3 as outputs
    P3DIR |= BIT4 + BIT5 + BIT6 + BIT7; // P3.4, P3.5, P3.6, P3.7 as outputs

    // set reference voltage to 2.5V
    REFCTL0 &= ~REFGENBUSY;
    REFCTL0 |= REFVSEL_3 + REFON;

    // Global interrupt enable
    _EINT();

    while (1)
    {
        _NOP();
    }

    return 0;
}

#pragma vector = USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
    RxByte = UCA0RXBUF; // Get the new byte from the Rx buffer
    while (!(UCA0IFG & UCTXIFG)); // Wait until the previous Tx is finished
    //UCA0TXBUF = RxByte; // Echo back the received byte

    if (RxByte == 'Z'| RxByte == 'z') // Stop Transmission
    {
        UCA0TXBUF = 0;
        UCA0TXBUF = 0;
        UCA0TXBUF = 0;
        UCA0TXBUF = 0;
    }
    if (RxByte == 'A'| RxByte == 'a') // Accelerometer - 0-3.3V range OR 0-2.5V range
    {
        // Configure port 2.7 for accelerometer
        P2DIR |= BIT7;
        P2OUT |= BIT7;
        P2SEL0 &= ~BIT7;
        P2SEL1 &= ~BIT7;

        // Configure ADC A12, A13, A14
        P3SEL0 |= BIT0 + BIT1 + BIT2;
        P3SEL1 |= BIT0 + BIT1 + BIT2;

        // Configure ADC
        ADC10CTL0 &= ~ADC10ENC;
        ADC10CTL1 |= ADC10SHP + ADC10SSEL_1 + ADC10CONSEQ_0;// ADCCLK = ACLK; sampling timer; single sample mode
        ADC10CTL0 &= ~ADC10SHT_2;
        ADC10CTL0 |= ADC10ON;  // ADC 10 On, sample and hold time = 16 ADC clks
        ADC10CTL2 |= ADC10RES; // 10 bit resolution
        if (RxByte == 'A')
        {
            ADC10MCTL0 |= ADC10SREF_0; // 0-Vcc reference voltage (0-3.3V)
        }
        else if (RxByte == 'a')
        {
            ADC10MCTL0 |= ADC10SREF_1; // 0-Vref reference voltage (0-2.5V)
        }
    }

    if (RxByte == 'B' | RxByte == 'b') // NTC Temperature - 0-3.3V range OR 0-2.5V range
    {
        // Configure ports for NTC temperature sensor power P2.7/A4 high
        P2DIR |= BIT7;
        P2OUT |= BIT7;
        P2SEL0 &= ~BIT7;
        P2SEL1 &= ~BIT7;

        // Configure P1.4 (A4) temperature NTC
        P1DIR |= BIT4;
        P1SEL0 |= BIT4;
        P1SEL1 |= BIT4;

        // Configure ADC
        ADC10CTL0 &= ~ADC10ENC;
        ADC10CTL1 |= ADC10SHP + ADC10SSEL_1 + ADC10CONSEQ_0;// ADCCLK = ACLK; sampling timer; single sample mode
        ADC10CTL0 &= ~ADC10SHT_2;
        ADC10CTL0 |= ADC10ON;  // ADC 10 On, sample and hold time = 16 ADC clks
        ADC10CTL2 |= ADC10RES; // 10 bit resolution
        if (RxByte == 'B')
        {
            ADC10MCTL0 |= ADC10SREF_0; // 0-Vcc reference voltage (0-3.3V)
        }
        else if (RxByte =='b')
        {
            ADC10MCTL0 |= ADC10SREF_1; // 0-Vref reference voltage (0-2.5V)
        }
    }
    if (RxByte == 'C' | RxByte == 'c') // P1.0/A0 - 0-3.3V range OR 0-2.5V range
    {
        P1SEL1 |= BIT0;
        P1SEL0 |= BIT0;

        // Configure ADC
        ADC10CTL0 &= ~ADC10ENC;
        ADC10CTL1 |= ADC10SHP + ADC10SSEL_1 + ADC10CONSEQ_0;// ADCCLK = ACLK; sampling timer; single sample mode
        ADC10CTL0 &= ~ADC10SHT_2;
        ADC10CTL0 |= ADC10ON;  // ADC 10 On, sample and hold time = 16 ADC clks
        ADC10CTL2 |= ADC10RES; // 10 bit resolution
        if (RxByte == 'C')
        {
            ADC10MCTL0 |= ADC10SREF_0; // 0-Vcc reference voltage (0-3.3V)
        }
        else if (RxByte == 'c')
        {
            ADC10MCTL0 |= ADC10SREF_1; // 0-Vref reference voltage (0-2.5V)
        }
    }
    if (RxByte == 'D' | RxByte == 'd') // P1.1/A1 - 0-3.3V range OR 0-2.5V range
    {
        P1SEL1 |= BIT1;
        P1SEL0 |= BIT1;

        // Configure ADC
        ADC10CTL0 &= ~ADC10ENC;
        ADC10CTL1 |= ADC10SHP + ADC10SSEL_1 + ADC10CONSEQ_0;// ADCCLK = ACLK; sampling timer; single sample mode
        ADC10CTL0 &= ~ADC10SHT_2;
        ADC10CTL0 |= ADC10ON;  // ADC 10 On, sample and hold time = 16 ADC clks
        ADC10CTL2 |= ADC10RES; // 10 bit resolution
        if (RxByte == 'D')
        {
            ADC10MCTL0 |= ADC10SREF_0; // 0-Vcc reference voltage (0-3.3V)
        }
        else if (RxByte == 'd')
        {
            ADC10MCTL0 |= ADC10SREF_1; // 0-Vref reference voltage (0-2.5V)
        }
    }
    if (RxByte == 'E' | RxByte == 'e') // P1.2/A2 - 0-3.3V range OR 0-2.5V range
    {
        P1SEL1 |= BIT2;
        P1SEL0 |= BIT2;

        // Configure ADC
        ADC10CTL0 &= ~ADC10ENC;
        ADC10CTL1 |= ADC10SHP + ADC10SSEL_1 + ADC10CONSEQ_0;// ADCCLK = ACLK; sampling timer; single sample mode
        ADC10CTL0 &= ~ADC10SHT_2;
        ADC10CTL0 |= ADC10ON;  // ADC 10 On, sample and hold time = 16 ADC clks
        ADC10CTL2 |= ADC10RES; // 10 bit resolution
        if (RxByte == 'E')
        {
            ADC10MCTL0 |= ADC10SREF_0; // 0-Vcc reference voltage (0-3.3V)
        }
        if (RxByte == 'e')
        {
            ADC10MCTL0 |= ADC10SREF_1; // 0-Vref reference voltage (0-2.5V)
        }
    }
    if (RxByte == 'F' | RxByte == 'f') // P3.0/A12 - 0-3.3V range OR 0-2.5V range
    {
        // Disable accelerometer
        P2DIR &= ~BIT7;
        P2OUT &= ~BIT7;

        P3SEL1 |= BIT0;
        P3SEL0 |= BIT0;

        // Configure ADC
        ADC10CTL0 &= ~ADC10ENC;
        ADC10CTL1 |= ADC10SHP + ADC10SSEL_1 + ADC10CONSEQ_0;// ADCCLK = ACLK; sampling timer; single sample mode
        ADC10CTL0 &= ~ADC10SHT_2;
        ADC10CTL0 |= ADC10ON;  // ADC 10 On, sample and hold time = 16 ADC clks
        ADC10CTL2 |= ADC10RES; // 10 bit resolution
        if (RxByte == 'F')
        {
            ADC10MCTL0 |= ADC10SREF_0; // 0-Vcc reference voltage (0-3.3V)
        }
        else if (RxByte =='f')
        {
            ADC10MCTL0 |= ADC10SREF_1; // 0-Vref reference voltage (0-2.5V)
        }
    }
    if (RxByte == 'G' | RxByte == 'g') // P3.1/A13 - 0-3.3V range OR 0-2.5V range
    {
        // Disable accelerometer
        P2DIR &= ~BIT7;
        P2OUT &= ~BIT7;

        P3SEL1 |= BIT1;
        P3SEL0 |= BIT1;

        // Configure ADC
        ADC10CTL0 &= ~ADC10ENC;
        ADC10CTL1 |= ADC10SHP + ADC10SSEL_1 + ADC10CONSEQ_0;// ADCCLK = ACLK; sampling timer; single sample mode
        ADC10CTL0 &= ~ADC10SHT_2;
        ADC10CTL0 |= ADC10ON;  // ADC 10 On, sample and hold time = 16 ADC clks
        ADC10CTL2 |= ADC10RES; // 10 bit resolution
        if (RxByte =='G')
        {
            ADC10MCTL0 |= ADC10SREF_0; // 0-Vcc reference voltage (0-3.3V)
        }
        else if (RxByte == 'g')
        {
            ADC10MCTL0 |= ADC10SREF_1; // 0-Vref reference voltage (0-2.5V)
        }
    }
    if (RxByte == 'H' | RxByte == 'h') // P3.2/A14 - 0-3.3V range OR 0-2.5V range
    {
        // Disable accelerometer
        P2DIR &= ~BIT7;
        P2OUT &= ~BIT7;

        P3SEL1 |= BIT2;
        P3SEL0 |= BIT2;

        // Configure ADC
        ADC10CTL0 &= ~ADC10ENC;
        ADC10CTL1 |= ADC10SHP + ADC10SSEL_1 + ADC10CONSEQ_0;// ADCCLK = ACLK; sampling timer; single sample mode
        ADC10CTL0 &= ~ADC10SHT_2;
        ADC10CTL0 |= ADC10ON;  // ADC 10 On, sample and hold time = 16 ADC clks
        ADC10CTL2 |= ADC10RES; // 10 bit resolution
        if (RxByte == 'H')
        {
            ADC10MCTL0 |= ADC10SREF_0; // 0-Vcc reference voltage (0-3.3V)
        }
        else if (RxByte =='h')
        {
            ADC10MCTL0 |= ADC10SREF_1; // 0-Vref reference voltage (0-2.5V)
        }
    }
    if (RxByte == 'I' | RxByte == 'i') // P3.3/A15 - 0-3.3V range OR 0-2.5V range
    {
        // Disable accelerometer
        P2DIR &= ~BIT7;
        P2OUT &= ~BIT7;

        P3SEL1 |= BIT3;
        P3SEL0 |= BIT3;

        // Configure ADC
        ADC10CTL0 &= ~ADC10ENC;
        ADC10CTL1 |= ADC10SHP + ADC10SSEL_1 + ADC10CONSEQ_0;// ADCCLK = ACLK; sampling timer; single sample mode
        ADC10CTL0 &= ~ADC10SHT_2;
        ADC10CTL0 |= ADC10ON;  // ADC 10 On, sample and hold time = 16 ADC clks
        ADC10CTL2 |= ADC10RES; // 10 bit resolution
        if (RxByte == 'I')
        {
            ADC10MCTL0 |= ADC10SREF_0; // 0-Vcc reference voltage (0-3.3V)
        }
        else if (RxByte == 'i')
        {
            ADC10MCTL0 |= ADC10SREF_1; // 0-Vref reference voltage (0-2.5V)
        }
    }
    if (RxByte == 'J' | RxByte =='j') // P1.3/A3 - 0-3.3V range OR 0-2.5V range
    {
        P1SEL1 |= BIT3;
        P1SEL0 |= BIT3;

        // Configure ADC
        ADC10CTL0 &= ~ADC10ENC;
        ADC10CTL1 |= ADC10SHP + ADC10SSEL_1 + ADC10CONSEQ_0;// ADCCLK = ACLK; sampling timer; single sample mode
        ADC10CTL0 &= ~ADC10SHT_2;
        ADC10CTL0 |= ADC10ON;  // ADC 10 On, sample and hold time = 16 ADC clks
        ADC10CTL2 |= ADC10RES; // 10 bit resolution
        if (RxByte == 'J')
        {
            ADC10MCTL0 |= ADC10SREF_0; // 0-Vcc reference voltage (0-3.3V)
        }
        else if (RxByte == 'j')
        {
            ADC10MCTL0 |= ADC10SREF_1; // 0-Vref reference voltage (0-2.5V)
        }
    }
    if (RxByte == 'K' | RxByte =='k') // P1.4/A4 - 0-3.3V range OR 0-2.5V range
    {
        P1SEL1 |= BIT4;
        P1SEL0 |= BIT4;

        // Configure ADC
        ADC10CTL0 &= ~ADC10ENC;
        ADC10CTL1 |= ADC10SHP + ADC10SSEL_1 + ADC10CONSEQ_0;// ADCCLK = ACLK; sampling timer; single sample mode
        ADC10CTL0 &= ~ADC10SHT_2;
        ADC10CTL0 |= ADC10ON;  // ADC 10 On, sample and hold time = 16 ADC clks
        ADC10CTL2 |= ADC10RES; // 10 bit resolution
        if (RxByte == 'K')
        {
            ADC10MCTL0 |= ADC10SREF_0; // 0-Vcc reference voltage (0-3.3V)
        }
        else if (RxByte == 'k')
        {
            ADC10MCTL0 |= ADC10SREF_1; // 0-Vref reference voltage (0-2.5V)
        }
    }
    if (RxByte == 'L' | RxByte == 'l') // P1.5/A5 - 0-3.3V range OR 0-2.5V range
    {
        P1SEL1 |= BIT5;
        P1SEL0 |= BIT5;

        // Configure ADC
        ADC10CTL0 &= ~ADC10ENC;
        ADC10CTL1 |= ADC10SHP + ADC10SSEL_1 + ADC10CONSEQ_0;// ADCCLK = ACLK; sampling timer; single sample mode
        ADC10CTL0 &= ~ADC10SHT_2;
        ADC10CTL0 |= ADC10ON;  // ADC 10 On, sample and hold time = 16 ADC clks
        ADC10CTL2 |= ADC10RES; // 10 bit resolution
        if (RxByte == 'L')
        {
            ADC10MCTL0 |= ADC10SREF_0; // 0-Vcc reference voltage (0-3.3V)
        }
        else if (RxByte == 'l')
        {
            ADC10MCTL0 |= ADC10SREF_1; // 0-Vref reference voltage (0-2.5V)
        }
    }
    if (RxByte == '1') // LED1 ON
    {
        PJOUT |= BIT0;
    }
    if (RxByte == '!') // LED1 OFF
    {
        PJOUT &= ~BIT0;
    }
    if (RxByte == '2') // LED2 ON
    {
        PJOUT |= BIT1;
    }
    if (RxByte == '@') // LED2 OFF
    {
        PJOUT &= ~BIT1;
    }
    if (RxByte == '3') // LED3 ON
    {
        PJOUT |= BIT2;
    }
    if (RxByte == '#') // LED3 OFF
    {
        PJOUT &= ~BIT2;
    }
    if (RxByte == '4') // LED4 ON
    {
        PJOUT |= BIT3;
    }
    if (RxByte == '$') // LED4 OFF
    {
        PJOUT &= ~BIT3;
    }
    if (RxByte == '5') // LED5 ON
    {
        P3OUT |= BIT4;
    }
    if (RxByte == '%') // LED5 OFF
    {
        P3OUT &= ~BIT4;
    }
    if (RxByte == '6') // LED6 ON
    {
        P3OUT |= BIT5;
    }
    if (RxByte == '^') // LED6 OFF
    {
        P3OUT &= ~BIT5;
    }
    if (RxByte == '7') // LED7 ON
    {
        P3OUT |= BIT6;
    }
    if (RxByte == '&') // LED7 OFF
    {
        P3OUT &= ~BIT6;
    }
    if (RxByte == '8') // LED8 ON
    {
        P3OUT |= BIT7;
    }
    if (RxByte == '*') // LED8 OFF
    {
        P3OUT &= ~BIT7;
    }
}

# pragma vector = TIMER0_B0_VECTOR
__interrupt void TIEMR0_B0_ISR(void)
{

    if (RxByte == 'A' | RxByte == 'a') // Accelerometer
    {
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = 255;

        ADC10CTL0 &= ~ADC10ENC;
        ADC10MCTL0 = ADC10INCH_12;
        ADC10CTL0 |= ADC10ENC + ADC10SC;
        while(!(ADC10CTL1 & ADC10BUSY)==0);
        t = ADC10MEM0;
        x = ADC10MEM0 >> 2; // bit shift by two to get a 8 bit number
        if (x == 255)
        {
            x = 254;
        }
        else if (x > 255)
        {
            x = 254;
        }

        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = x;

        ADC10CTL0 &= ~ADC10ENC;
        ADC10MCTL0 = ADC10INCH_13;
        ADC10CTL0 |= ADC10ENC + ADC10SC;
        while(!(ADC10CTL1 & ADC10BUSY)==0);
        y = ADC10MEM0 >> 2;
        if (y == 255)
        {
            y = 254;
        }
        else if (y > 255)
        {
            y = 254;
        }

        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = y;

        ADC10CTL0 &= ~ADC10ENC;
        ADC10MCTL0 = ADC10INCH_14;
        ADC10CTL0 |= ADC10ENC + ADC10SC;
        while(!(ADC10CTL1 & ADC10BUSY)==0);
        z = ADC10MEM0 >> 2;
        if (z == 255)
        {
            z = 254;
        }
        else if (z > 255)
        {
            z = 254;
        }

        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = z;
    }
    if (RxByte == 'B' | RxByte == 'b') //NTC Temperature
    {
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = 255;

        ADC10CTL0 &= ~ADC10ENC;
        ADC10MCTL0 = ADC10INCH_4;
        ADC10CTL0 |= ADC10ENC + ADC10SC;

        while(!(ADC10CTL1 & ADC10BUSY)==0);
        t = ADC10MEM0;
        temp_ms5b = ADC10MEM0 >> 5; // most significant 5 bit is a right shift of 5
        if (temp_ms5b == 255)
        {
            temp_ms5b = 254;
        }
        else if (temp_ms5b > 255)
        {
            temp_ms5b = 254;
        }

        while(!(ADC10CTL1 & ADC10BUSY)==0);
        temp_ls5b = ADC10MEM0 & 0x1F; // least significant 5 bit is the right-most
        // 5 bits. 0x1F = 0000011111 to only output right-most 5 bits.
        if (temp_ls5b == 255)
        {
            temp_ls5b = 254;
        }
        else if (temp_ls5b > 255)
        {
            temp_ls5b = 254;
        }

        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = temp_ms5b;
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = temp_ls5b;
        //while (!(UCA0IFG & UCTXIFG));
        //UCA0TXBUF = 0;
    }
    if (RxByte == 'C' | RxByte == 'c') // P1.0/A0
    {
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = 255;

        ADC10CTL0 &= ~ADC10ENC;
        ADC10MCTL0 = ADC10INCH_0;
        ADC10CTL0 |= ADC10ENC + ADC10SC;

        while(!(ADC10CTL1 & ADC10BUSY)==0);
        P1_0_ms5b = ADC10MEM0 >> 5; // most significant 5 bit is a right shift of 5
        if (P1_0_ms5b == 255)
        {
            P1_0_ms5b = 254;
        }
        else if (P1_0_ms5b > 255)
        {
            P1_0_ms5b = 254;
        }

        while(!(ADC10CTL1 & ADC10BUSY)==0);
        P1_0_ls5b = ADC10MEM0 & 0x1F; // least significant 5 bit is the right-most
        // 5 bits. 0x1F = 0000011111 to only output right-most 5 bits.
        if (P1_0_ls5b == 255)
        {
            P1_0_ls5b = 254;
        }
        else if (P1_0_ls5b > 255)
        {
            P1_0_ls5b = 254;
        }

        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = P1_0_ms5b;
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = P1_0_ls5b;
        //while (!(UCA0IFG & UCTXIFG));
        //UCA0TXBUF = 0;
    }
    if (RxByte == 'D' | RxByte == 'd') // P1.1/A1
    {
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = 255;

        ADC10CTL0 &= ~ADC10ENC;
        ADC10MCTL0 = ADC10INCH_1;
        ADC10CTL0 |= ADC10ENC + ADC10SC;

        while(!(ADC10CTL1 & ADC10BUSY)==0);
        P1_1_ms5b = ADC10MEM0 >> 5; // most significant 5 bit is a right shift of 5
        if (P1_1_ms5b == 255)
        {
            P1_1_ms5b = 254;
        }
        else if (P1_1_ms5b > 255)
        {
            P1_1_ms5b = 254;
        }

        while(!(ADC10CTL1 & ADC10BUSY)==0);
        P1_1_ls5b = ADC10MEM0 & 0x1F; // least significant 5 bit is the right-most
        // 5 bits. 0x1F = 0000011111 to only output right-most 5 bits.
        if (P1_1_ls5b == 255)
        {
            P1_1_ls5b = 254;
        }
        else if (P1_1_ls5b > 255)
        {
            P1_1_ls5b = 254;
        }

        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = P1_1_ms5b;
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = P1_1_ls5b;
        //while (!(UCA0IFG & UCTXIFG));
        //UCA0TXBUF = 0;
    }
    if (RxByte == 'E' | RxByte == 'e') // P1.2/A2
    {
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = 255;

        ADC10CTL0 &= ~ADC10ENC;
        ADC10MCTL0 = ADC10INCH_2;
        ADC10CTL0 |= ADC10ENC + ADC10SC;

        while(!(ADC10CTL1 & ADC10BUSY)==0);
        P1_2_ms5b = ADC10MEM0 >> 5; // most significant 5 bit is a right shift of 5
        if (P1_2_ms5b == 255)
        {
            P1_2_ms5b = 254;
        }
        else if (P1_2_ms5b > 255)
        {
            P1_2_ms5b = 254;
        }

        while(!(ADC10CTL1 & ADC10BUSY)==0);
        P1_2_ls5b = ADC10MEM0 & 0x1F; // least significant 5 bit is the right-most
        // 5 bits. 0x1F = 0000011111 to only output right-most 5 bits.
        if (P1_2_ls5b == 255)
        {
            P1_2_ls5b = 254;
        }
        else if (P1_2_ls5b > 255)
        {
            P1_2_ls5b = 254;
        }

        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = P1_2_ms5b;
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = P1_2_ls5b;
        //while (!(UCA0IFG & UCTXIFG));
        //UCA0TXBUF = 0;
    }
    if (RxByte == 'F' | RxByte == 'f') // P3.0/A12
    {
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = 255;

        ADC10CTL0 &= ~ADC10ENC;
        ADC10MCTL0 = ADC10INCH_12;
        ADC10CTL0 |= ADC10ENC + ADC10SC;

        while(!(ADC10CTL1 & ADC10BUSY)==0);
        P3_0_ms5b = ADC10MEM0 >> 5; // most significant 5 bit is a right shift of 5
        if (P3_0_ms5b == 255)
        {
            P3_0_ms5b = 254;
        }
        else if (P3_0_ms5b > 255)
        {
            P3_0_ms5b = 254;
        }

        while(!(ADC10CTL1 & ADC10BUSY)==0);
        P3_0_ls5b = ADC10MEM0 & 0x1F; // least significant 5 bit is the right-most
        // 5 bits. 0x1F = 0000011111 to only output right-most 5 bits.
        if (P3_0_ls5b == 255)
        {
            P3_0_ls5b = 254;
        }
        else if (P3_0_ls5b > 255)
        {
            P3_0_ls5b = 254;
        }

        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = P3_0_ms5b;
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = P3_0_ls5b;
        //while (!(UCA0IFG & UCTXIFG));
        //UCA0TXBUF = 0;
    }
    if (RxByte == 'G' | RxByte == 'g') // P3.1/A13
    {
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = 255;

        ADC10CTL0 &= ~ADC10ENC;
        ADC10MCTL0 = ADC10INCH_13;
        ADC10CTL0 |= ADC10ENC + ADC10SC;

        while(!(ADC10CTL1 & ADC10BUSY)==0);
        P3_1_ms5b = ADC10MEM0 >> 5; // most significant 5 bit is a right shift of 5
        if (P3_1_ms5b == 255)
        {
            P3_1_ms5b = 254;
        }
        else if (P3_1_ms5b > 255)
        {
            P3_1_ms5b = 254;
        }

        while(!(ADC10CTL1 & ADC10BUSY)==0);
        P3_1_ls5b = ADC10MEM0 & 0x1F; // least significant 5 bit is the right-most
        // 5 bits. 0x1F = 0000011111 to only output right-most 5 bits.
        if (P3_1_ls5b == 255)
        {
            P3_1_ls5b = 254;
        }
        else if (P3_1_ls5b > 255)
        {
            P3_1_ls5b = 254;
        }

        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = P3_1_ms5b;
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = P3_1_ls5b;
        //while (!(UCA0IFG & UCTXIFG));
        //UCA0TXBUF = 0;
    }

    if (RxByte == 'H' | RxByte == 'h') // P3.2/A14
    {
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = 255;

        ADC10CTL0 &= ~ADC10ENC;
        ADC10MCTL0 = ADC10INCH_14;
        ADC10CTL0 |= ADC10ENC + ADC10SC;

        while(!(ADC10CTL1 & ADC10BUSY)==0);
        P3_2_ms5b = ADC10MEM0 >> 5; // most significant 5 bit is a right shift of 5
        if (P3_2_ms5b == 255)
        {
            P3_2_ms5b = 254;
        }
        else if (P3_2_ms5b > 255)
        {
            P3_2_ms5b = 254;
        }

        while(!(ADC10CTL1 & ADC10BUSY)==0);
        P3_2_ls5b = ADC10MEM0 & 0x1F; // least significant 5 bit is the right-most
        // 5 bits. 0x1F = 0000011111 to only output right-most 5 bits.
        if (P3_2_ls5b == 255)
        {
            P3_2_ls5b = 254;
        }
        else if (P3_2_ls5b > 255)
        {
            P3_2_ls5b = 254;
        }

        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = P3_2_ms5b;
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = P3_2_ls5b;
        //while (!(UCA0IFG & UCTXIFG));
        //UCA0TXBUF = 0;
    }
    if (RxByte == 'I' | RxByte == 'i') // P3.3/A15
    {
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = 255;

        ADC10CTL0 &= ~ADC10ENC;
        ADC10MCTL0 = ADC10INCH_15;
        ADC10CTL0 |= ADC10ENC + ADC10SC;

        while(!(ADC10CTL1 & ADC10BUSY)==0);
        P3_3_ms5b = ADC10MEM0 >> 5; // most significant 5 bit is a right shift of 5
        if (P3_3_ms5b == 255)
        {
            P3_3_ms5b = 254;
        }
        else if (P3_3_ms5b > 255)
        {
            P3_3_ms5b = 254;
        }

        while(!(ADC10CTL1 & ADC10BUSY)==0);
        P3_3_ls5b = ADC10MEM0 & 0x1F; // least significant 5 bit is the right-most
        // 5 bits. 0x1F = 0000011111 to only output right-most 5 bits.
        if (P3_3_ls5b == 255)
        {
            P3_3_ls5b = 254;
        }
        else if (P3_3_ls5b > 255)
        {
            P3_3_ls5b = 254;
        }

        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = P3_3_ms5b;
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = P3_3_ls5b;
        //while (!(UCA0IFG & UCTXIFG));
        //UCA0TXBUF = 0;
    }
    if (RxByte == 'J' | RxByte == 'j') // P1.3/A3
    {
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = 255;

        ADC10CTL0 &= ~ADC10ENC;
        ADC10MCTL0 = ADC10INCH_3;
        ADC10CTL0 |= ADC10ENC + ADC10SC;

        while(!(ADC10CTL1 & ADC10BUSY)==0);
        P1_3_ms5b = ADC10MEM0 >> 5; // most significant 5 bit is a right shift of 5
        if (P1_3_ms5b == 255)
        {
            P1_3_ms5b = 254;
        }
        else if (P1_3_ms5b > 255)
        {
            P1_3_ms5b = 254;
        }

        while(!(ADC10CTL1 & ADC10BUSY)==0);
        P1_3_ls5b = ADC10MEM0 & 0x1F; // least significant 5 bit is the right-most
        // 5 bits. 0x1F = 0000011111 to only output right-most 5 bits.
        if (P1_3_ls5b == 255)
        {
            P1_3_ls5b = 254;
        }
        else if (P1_3_ls5b > 255)
        {
            P1_3_ls5b = 254;
        }

        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = P1_3_ms5b;
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = P1_3_ls5b;
        //while (!(UCA0IFG & UCTXIFG));
        //UCA0TXBUF = 0;
    }
    if (RxByte == 'K' | RxByte == 'k') // P1.4/A4
    {
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = 255;

        ADC10CTL0 &= ~ADC10ENC;
        ADC10MCTL0 = ADC10INCH_4;
        ADC10CTL0 |= ADC10ENC + ADC10SC;

        while(!(ADC10CTL1 & ADC10BUSY)==0);
        P1_4_ms5b = ADC10MEM0 >> 5; // most significant 5 bit is a right shift of 5
        if (P1_4_ms5b == 255)
        {
            P1_4_ms5b = 254;
        }
        else if (P1_4_ms5b > 255)
        {
            P1_4_ms5b = 254;
        }

        while(!(ADC10CTL1 & ADC10BUSY)==0);
        P1_4_ls5b = ADC10MEM0 & 0x1F; // least significant 5 bit is the right-most
        // 5 bits. 0x1F = 0000011111 to only output right-most 5 bits.
        if (P1_4_ls5b == 255)
        {
            P1_4_ls5b = 254;
        }
        else if (P1_4_ls5b > 255)
        {
            P1_4_ls5b = 254;
        }

        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = P1_4_ms5b;
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = P1_4_ls5b;
        //while (!(UCA0IFG & UCTXIFG));
        //UCA0TXBUF = 0;
    }
    if (RxByte == 'L' | RxByte == 'l') // P1.5/A5
    {
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = 255;

        ADC10CTL0 &= ~ADC10ENC;
        ADC10MCTL0 = ADC10INCH_5;
        ADC10CTL0 |= ADC10ENC + ADC10SC;

        while(!(ADC10CTL1 & ADC10BUSY)==0);
        P1_5_ms5b = ADC10MEM0 >> 5; // most significant 5 bit is a right shift of 5
        if (P1_5_ms5b == 255)
        {
            P1_5_ms5b = 254;
        }
        else if (P1_5_ms5b > 255)
        {
            P1_5_ms5b = 254;
        }

        while(!(ADC10CTL1 & ADC10BUSY)==0);
        P1_5_ls5b = ADC10MEM0 & 0x1F; // least significant 5 bit is the right-most
        // 5 bits. 0x1F = 0000011111 to only output right-most 5 bits.
        if (P1_5_ls5b == 255)
        {
            P1_5_ls5b = 254;
        }
        else if (P1_5_ls5b > 255)
        {
            P1_5_ls5b = 254;
        }

        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = P1_5_ms5b;
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = P1_5_ls5b;
        //while (!(UCA0IFG & UCTXIFG));
        //UCA0TXBUF = 0;
    }

    TB0CCTL1 &= ~CCIFG;             // reset timer flag

}
