/*******************************************************************************
 * config.h - Archivo de configuración para dsPIC33FJ32MC204
 * 
 * Autor: Carlos Olivera Ylla
 * Fecha: 04/12/2025
 * 
 * Descripción: Configuración modular del microcontrolador.
 * 
 * USO: Descomenta las opciones que necesites y comenta las que no.
 *      Solo una configuración de cada tipo debe estar activa.
 * 
 ******************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#include <xc.h>

// =============================================================================
// CONFIGURACIÓN DEL SISTEMA - DESCOMENTAR UNA OPCIÓN POR CATEGORÍA
// =============================================================================

// -----------------------------------------------------------------------------
// 1. CONFIGURACIÓN DEL OSCILADOR
// -----------------------------------------------------------------------------

// OPCIÓN A: Oscilador interno máximo con PLL (RECOMENDADA)
#define CONFIG_OSC_INTERNO_PLL

// OPCIÓN B: Oscilador interno sin PLL
// #define CONFIG_OSC_INTERNO_SIMPLE

// OPCIÓN C: Oscilador externo con PLL
// #define CONFIG_OSC_EXTERNO_PLL

// OPCIÓN D: Oscilador externo sin PLL
// #define CONFIG_OSC_EXTERNO_SIMPLE

// -----------------------------------------------------------------------------
// 2. CONFIGURACIÓN DEL WATCHDOG TIMER
// -----------------------------------------------------------------------------

// OPCIÓN A: Watchdog deshabilitado (RECOMENDADO para desarrollo)
#define CONFIG_WDT_OFF

// OPCIÓN B: Watchdog habilitado con tiempo normal
// #define CONFIG_WDT_ON_NORMAL

// OPCIÓN C: Watchdog habilitado con tiempo largo
// #define CONFIG_WDT_ON_LONG

// -----------------------------------------------------------------------------
// 3. CONFIGURACIÓN DE RESET
// -----------------------------------------------------------------------------

// OPCIÓN A: Reset por MCLR habilitado
#define CONFIG_MCLR_ENABLED

// OPCIÓN B: Reset por MCLR deshabilitado (pin como I/O)
// #define CONFIG_MCLR_DISABLED

// -----------------------------------------------------------------------------
// 4. CONFIGURACIÓN DEL BROWN-OUT RESET (BOR)
// -----------------------------------------------------------------------------

// OPCIÓN A: BOR deshabilitado (RECOMENDADO para desarrollo)
#define CONFIG_BOR_OFF

// OPCIÓN B: BOR habilitado en 2.7V
// #define CONFIG_BOR_27V

// OPCIÓN C: BOR habilitado en 2.0V
// #define CONFIG_BOR_20V

// OPCIÓN D: BOR habilitado en 4.2V
// #define CONFIG_BOR_42V

// -----------------------------------------------------------------------------
// 5. CONFIGURACIÓN DE LA PROTECCIÓN DE CÓDIGO
// -----------------------------------------------------------------------------

// OPCIÓN A: Código protegido (RECOMENDADO para producción)
// #define CONFIG_CODE_PROTECT_ON

// OPCIÓN B: Código sin proteger (RECOMENDADO para desarrollo)
#define CONFIG_CODE_PROTECT_OFF

// -----------------------------------------------------------------------------
// 6. CONFIGURACIÓN DE PINES DE DEBUG
// -----------------------------------------------------------------------------

// OPCIÓN A: Debug deshabilitado, pines como I/O (RECOMENDADO)
#define CONFIG_DEBUG_OFF

// OPCIÓN B: Debug habilitado (usa PGD/PGC)
// #define CONFIG_DEBUG_ON

// -----------------------------------------------------------------------------
// 7. CONFIGURACIÓN DEL CLOCK SWITCHING
// -----------------------------------------------------------------------------

// OPCIÓN A: Clock switching deshabilitado
#define CONFIG_CLOCK_SWITCH_OFF

// OPCIÓN B: Clock switching habilitado
// #define CONFIG_CLOCK_SWITCH_ON

// -----------------------------------------------------------------------------
// 8. CONFIGURACIÓN DE LOS PUERTOS
// -----------------------------------------------------------------------------

// Habilitar/deshabilitar puertos específicos para ahorrar energía
// #define CONFIG_PORT_A_ENABLED
#define CONFIG_PORT_B_ENABLED
// #define CONFIG_PORT_C_ENABLED
// #define CONFIG_PORT_D_ENABLED
// #define CONFIG_PORT_E_ENABLED
// #define CONFIG_PORT_F_ENABLED
// #define CONFIG_PORT_G_ENABLED

// =============================================================================
// CONSTANTES DEL SISTEMA
// =============================================================================

// Frecuencias del sistema
#ifdef CONFIG_OSC_INTERNO_PLL
    #define FCY         40000000UL  // 40 MIPS
    #define FOSC        8000000UL   // 8 MHz interno
    #define FPLL        80000000UL  // 80 MHz PLL
    #define FOSC_PRIM   7370000UL   // 7.37 MHz exacto
#elif defined(CONFIG_OSC_INTERNO_SIMPLE)
    #define FCY         7370000UL   // 7.37 MIPS
    #define FOSC        7370000UL   // 7.37 MHz
    #define FPLL        0UL         // No hay PLL
#endif

// Tiempos de delay
#define DELAY_MS(ms)    __delay_ms(ms)
#define DELAY_US(us)    __delay_us(us)

// Estados del sistema
typedef enum {
    SYS_STATE_INIT,
    SYS_STATE_READY,
    SYS_STATE_BUSY,
    SYS_STATE_ERROR,
    SYS_STATE_SLEEP
} System_State_t;

// =============================================================================
// PROTOTIPOS DE FUNCIONES
// =============================================================================

void SYSTEM_Initialize(void);
void SYSTEM_Deinitialize(void);
void SYSTEM_EnterSleep(void);
void SYSTEM_Wakeup(void);
void SYSTEM_Reset(void);
void SYSTEM_EnableInterrupts(void);
void SYSTEM_DisableInterrupts(void);
uint32_t SYSTEM_GetClockFrequency(void);
System_State_t SYSTEM_GetState(void);
void SYSTEM_PrintConfiguration(void);

#endif /* CONFIG_H */