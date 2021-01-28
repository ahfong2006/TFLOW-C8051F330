/////////////////////////////////////
//  Generated Initialization File  //
/////////////////////////////////////

#include "c8051f330.h"

// Peripheral specific initialization functions,
// Called from the Init_Device() function
void PCA_Init()
{
    PCA0CN    = 0x40;
    PCA0CPM0  = 0x49;
    PCA0CPL0  = 0xFF;
    PCA0CPL2  = 0xFF;
    PCA0MD    |= 0x40;
    PCA0CPH0  = 0xFF;
}

void Timer_Init()
{
    TCON      = 0x45;
    TMOD      = 0x21;
    TH1       = 0x96;
    TMR2RLL   = 0x06;
    TMR2RLH   = 0xF8;
    TMR2L     = 0x06;
    TMR2H     = 0xF8;
    TMR3CN    = 0x04;
    TMR3RLL   = 0x89;
    TMR3RLH   = 0xF3;
}

void UART_Init()
{
    SCON0     = 0x30;
}

void SPI_Init()
{
    SPI0CFG   = 0x40;
    SPI0CN    = 0x01;
    SPI0CKR   = 0x01;
}

void ADC_Init()
{
    AMX0P     = 0x02;
    AMX0N     = 0x11;
    ADC0CN    = 0x85;
}

void DAC_Init()
{
    IDA0CN    = 0xF0;
}

void Voltage_Reference_Init()
{
    REF0CN    = 0x03;
}

void Port_IO_Init()
{
    // P0.0  -  Skipped,     Open-Drain, Analog
    // P0.1  -  Skipped,     Open-Drain, Analog
    // P0.2  -  Skipped,     Open-Drain, Analog
    // P0.3  -  Skipped,     Push-Pull,  Digital
    // P0.4  -  TX0 (UART0), Open-Drain, Digital
    // P0.5  -  RX0 (UART0), Open-Drain, Digital
    // P0.6  -  Skipped,     Open-Drain, Digital
    // P0.7  -  Skipped,     Open-Drain, Digital

    // P1.0  -  SCK  (SPI0), Push-Pull,  Digital
    // P1.1  -  MISO (SPI0), Open-Drain, Digital
    // P1.2  -  MOSI (SPI0), Push-Pull,  Digital
    // P1.3  -  Unassigned,  Push-Pull,  Digital
    // P1.4  -  Unassigned,  Push-Pull,  Digital
    // P1.5  -  Unassigned,  Open-Drain, Digital
    // P1.6  -  Unassigned,  Open-Drain, Digital
    // P1.7  -  Unassigned,  Open-Drain, Digital

    P0MDIN    = 0xF8;
    P0MDOUT   = 0x08;
    P1MDOUT   = 0x1D;
    P0SKIP    = 0xCF;
    XBR0      = 0x03;
    XBR1      = 0xC0;
}

void Oscillator_Init()
{
    OSCICN    = 0x83;
}

void Interrupts_Init()
{
    IE        = 0xB2;
    EIE1      = 0x18;
    IT01CF    = 0xE6;
}

// Initialization function for device,
// Call Init_Device() from your main program
void Init_Device(void)
{
    PCA_Init();
    Timer_Init();
    UART_Init();
    SPI_Init();
    ADC_Init();
    DAC_Init();
    Voltage_Reference_Init();
    Port_IO_Init();
    Oscillator_Init();
    Interrupts_Init();
}
