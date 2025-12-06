/**********************************************************************
 * FIRExample_reducido.c
 * Versión reducida: sólo lo necesario para ejecutar un FIR pasabajo
 * usando la estructura definida en lowpassexample.s y la señal en
 * inputsignal_square1khz.s
 *
 * Con nombres coincidentes con:
 *  - _square1k                 -> extern fractional square1k[256];
 *  - _lowpassexampleFilter     -> extern FIRStruct lowpassexampleFilter;
 **********************************************************************/

// DSPIC33FJ32MC204 Configuration Bit Settings

// 'C' source line config statements

// FBS
#pragma config BWRP = WRPROTECT_OFF     // Boot Segment Write Protect (Boot Segment may be written)
#pragma config BSS = NO_FLASH           // Boot Segment Program Flash Code Protection (No Boot program Flash segment)

// FGS
#pragma config GWRP = OFF               // General Code Segment Write Protect (User program memory is not write-protected)
#pragma config GSS = OFF                // General Segment Code Protection (User program memory is not code-protected)

// FOSCSEL
#pragma config FNOSC = FRC              // Oscillator Mode (Internal Fast RC (FRC))
#pragma config IESO = ON                // Internal External Switch Over Mode (Start-up device with FRC, then automatically switch to user-selected oscillator source when ready)

// FOSC
#pragma config POSCMD = XT              // Primary Oscillator Source (XT Oscillator Mode)
#pragma config OSCIOFNC = OFF           // OSC2 Pin Function (OSC2 pin has clock out function)
#pragma config IOL1WAY = ON             // Peripheral Pin Select Configuration (Allow Only One Re-configuration)
#pragma config FCKSM = CSECMD           // Clock Switching and Monitor (Clock switching is enabled, Fail-Safe Clock Monitor is disabled)

// FWDT
#pragma config WDTPOST = PS32768        // Watchdog Timer Postscaler (1:32,768)
#pragma config WDTPRE = PR128           // WDT Prescaler (1:128)
#pragma config WINDIS = OFF             // Watchdog Timer Window (Watchdog Timer in Non-Window mode)
#pragma config FWDTEN = OFF             // Watchdog Timer Enable (Watchdog timer enabled/disabled by user software)

// FPOR
#pragma config FPWRT = PWR1             // POR Timer Value (Disabled)
#pragma config ALTI2C = OFF             // Alternate I2C  pins (I2C mapped to SDA1/SCL1 pins)
#pragma config LPOL = ON                // Motor Control PWM Low Side Polarity bit (PWM module low side output pins have active-high output polarity)
#pragma config HPOL = ON                // Motor Control PWM High Side Polarity bit (PWM module high side output pins have active-high output polarity)
#pragma config PWMPIN = ON              // Motor Control PWM Module Pin Mode bit (PWM module pins controlled by PORT register at device Reset)

// FICD
#pragma config ICS = PGD1               // Comm Channel Select (Communicate on PGC1/EMUC1 and PGD1/EMUD1)
#pragma config JTAGEN = OFF             // JTAG Port Enable (JTAG is Disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include "p33Fxxxx.h"
#include "dsp.h"


/* Longitud del bloque (coincide con el número de hword en _square1k) */
#define BLOCK_LENGTH 256

/* Declaraciones externas (coinciden con lo exportado en tus .s) */
extern fractional square1k[BLOCK_LENGTH];       /* _square1k en inputsignal_square1khz.s */
extern FIRStruct lowpassexampleFilter;          /* _lowpassexampleFilter en lowpassexample.s */

fractional FilterOut[BLOCK_LENGTH];             /* Buffer de salida */

/* Programa principal: configura reloj, inicializa y ejecuta el FIR una vez */
int main(void)
{
    /* Configurar PLL como en el ejemplo original (opcional si ya lo tienes) */
    PLLFBD = 38;                 /* M = 40 */
    CLKDIVbits.PLLPOST = 0;      /* N1 = 2 */
    CLKDIVbits.PLLPRE = 0;       /* N2 = 2 */
    OSCTUN = 0;

    /* Deshabilitar Watchdog por software */
    RCONbits.SWDTEN = 0;

    /* Cambio de reloj a Primary Oscillator with PLL */
    __builtin_write_OSCCONH(0x03);
    __builtin_write_OSCCONL(0x01);
    while (OSCCONbits.COSC != 0b011) ;
    while (OSCCONbits.LOCK != 1) ;

    /* Inicializa la línea de retardo del filtro (estado) */
    FIRDelayInit(&lowpassexampleFilter);

    /* Ejecuta FIR pasabajo sobre el bloque de entrada */
    FIR(BLOCK_LENGTH, &FilterOut[0], &square1k[0], &lowpassexampleFilter);

    /* En FilterOut quedan las muestras filtradas; bucle infinito */
    while (1) { /* idle; aquí podrías enviar FilterOut por UART/DAC/DMA */ }

    return 0;
}
