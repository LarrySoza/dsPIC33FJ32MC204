/*******************************************************************************
 * config.c - Implementación de la configuración del sistema
 * 
 * Este archivo configura todos los registros de configuración
 * según las definiciones en config.h
 * 
 ******************************************************************************/

#include "config.h"
#include <stdio.h>
#include <string.h>

// =============================================================================
// VARIABLES GLOBALES
// =============================================================================

static System_State_t system_state = SYS_STATE_INIT;

// =============================================================================
// FUNCIONES PRIVADAS
// =============================================================================

/**
 * @brief Configura el registro FOSCSEL (Oscillator Selection)
 */
static void _ConfigureFOSCSEL(void) {
    FOSCSEL = 0;  // Reset del registro
    
    #ifdef CONFIG_OSC_INTERNO_PLL
        // FNOSC = FRC con PLL (Oscilador interno con PLL)
        FOSCSELbits.FNOSC = 0b001;  // FRC con PLL
        // IESO = 0 (Two-Speed Start-up disabled)
        FOSCSELbits.IESO = 0;
        
    #elif defined(CONFIG_OSC_INTERNO_SIMPLE)
        // FNOSC = FRC (Oscilador interno sin PLL)
        FOSCSELbits.FNOSC = 0b000;  // FRC
        FOSCSELbits.IESO = 0;
        
    #elif defined(CONFIG_OSC_EXTERNO_PLL)
        // FNOSC = PRI con PLL (Oscilador primario con PLL)
        FOSCSELbits.FNOSC = 0b011;  // PRI con PLL
        FOSCSELbits.IESO = 0;
        
    #elif defined(CONFIG_OSC_EXTERNO_SIMPLE)
        // FNOSC = PRI (Oscilador primario sin PLL)
        FOSCSELbits.FNOSC = 0b010;  // PRI
        FOSCSELbits.IESO = 0;
    #endif
}

/**
 * @brief Configura el registro FOSC (Oscillator Configuration)
 */
static void _ConfigureFOSC(void) {
    FOSC = 0;  // Reset del registro
    
    // Configuración común
    #ifdef CONFIG_OSC_INTERNO_PLL
        // FRC con PLL, divide por 2
        // PLL multiplica por 16, divide por 2 = 8x total
    #endif
    
    // OSCOFN = 0 (Clock output disabled)
    FOSCbits.OSCOFN = 0;
    
    // POSCMD = 00 (Primary oscillator disabled for FRC)
    FOSCbits.POSCMD = 0b00;
    
    #ifdef CONFIG_DEBUG_OFF
        // Debug deshabilitado, pines como I/O
        FOSCbits.OSCIOFNC = 0;  // CLKO disabled
    #else
        // Debug habilitado
        FOSCbits.OSCIOFNC = 1;  // CLKO enabled
    #endif
}

/**
 * @brief Configura el registro FWDT (Watchdog Timer)
 */
static void _ConfigureFWDT(void) {
    FWDT = 0;  // Reset del registro
    
    #ifdef CONFIG_WDT_OFF
        // Watchdog deshabilitado
        FWDTbits.WDTPS = 0b01111;  // 1:32,768 (default)
        FWDTbits.WINDIS = 1;       // Window mode disabled
        FWDTbits.FWDTEN = 0;       // Watchdog timer disabled
        
    #elif defined(CONFIG_WDT_ON_NORMAL)
        // Watchdog habilitado, tiempo normal
        FWDTbits.WDTPS = 0b01010;  // 1:1,024 (~1ms @ 32kHz)
        FWDTbits.WINDIS = 1;       // Window mode disabled
        FWDTbits.FWDTEN = 1;       // Watchdog timer enabled
        
    #elif defined(CONFIG_WDT_ON_LONG)
        // Watchdog habilitado, tiempo largo
        FWDTbits.WDTPS = 0b11111;  // 1:2,147,483,648 (~18 horas)
        FWDTbits.WINDIS = 1;
        FWDTbits.FWDTEN = 1;
    #endif
}

/**
 * @brief Configura el registro FPOR (Power-on Reset)
 */
static void _ConfigureFPOR(void) {
    FPOR = 0;  // Reset del registro
    
    // ALTI2C = 0 (I2C in standard mode)
    FPORbits.ALTI2C = 0;
    
    // HPOL = 1 (PWM high-side polarity active high)
    FPORbits.HPOL = 1;
    
    // LPOL = 1 (PWM low-side polarity active high)
    FPORbits.LPOL = 1;
    
    #ifdef CONFIG_BOR_OFF
        // Brown-out reset deshabilitado
        FPORbits.BOREN = 0b00;  // BOR disabled
    #elif defined(CONFIG_BOR_27V)
        // BOR a 2.7V
        FPORbits.BOREN = 0b10;  // BOR enabled at 2.7V
    #elif defined(CONFIG_BOR_20V)
        // BOR a 2.0V
        FPORbits.BOREN = 0b01;  // BOR enabled at 2.0V
    #elif defined(CONFIG_BOR_42V)
        // BOR a 4.2V
        FPORbits.BOREN = 0b11;  // BOR enabled at 4.2V
    #endif
    
    // PWMPIN = 1 (PWM pins controlled by PWM module)
    FPORbits.PWMPIN = 1;
}

/**
 * @brief Configura el registro FICD (In-Circuit Debugger)
 */
static void _ConfigureFICD(void) {
    FICD = 0;  // Reset del registro
    
    #ifdef CONFIG_DEBUG_OFF
        // Debug deshabilitado, pines como I/O
        FICDbits.JTAGEN = 0;  // JTAG disabled
    #else
        // Debug habilitado
        FICDbits.JTAGEN = 1;  // JTAG enabled
    #endif
    
    // ICS = 01 (PGC1/EMUC1 and PGD1/EMUD1)
    FICDbits.ICS = 0b01;
    
    #ifdef CONFIG_CODE_PROTECT_OFF
        // Código sin proteger
        FICDbits.CODEPROT = 0b000;  // Code protection disabled
    #elif defined(CONFIG_CODE_PROTECT_ON)
        // Código protegido
        FICDbits.CODEPROT = 0b111;  // Code protection enabled
    #endif
}

/**
 * @brief Configura el registro FGS (General Segment)
 */
static void _ConfigureFGS(void) {
    FGS = 0;  // Reset del registro
    
    // GWRP = 0 (Flash not write protected)
    FGSbits.GWRP = 0;
    
    // GCP = 0 (Code protection disabled)
    FGSbits.GCP = 0;
}

/**
 * @brief Configura el PLL para máxima velocidad
 */
static void _ConfigurePLL(void) {
    #ifdef CONFIG_OSC_INTERNO_PLL
        // Deshabilitar PLL temporalmente
        CLKDIVbits.PLLEN = 0;
        
        // Configurar PLL para multiplicar por 16
        // Fosc = 7.37 MHz, PLL x16 = 117.92 MHz
        // Luego dividir por 2 para obtener Fcy = 58.96 MHz (pero limitado a 40 MIPS)
        
        // PLLPRE<4:0> (N1): divisor pre-PLL
        // N1 = FRC / Fin - 2
        // Para FRC = 7.37 MHz y Fin deseado = 0.8 MHz (mínimo para PLL)
        // N1 = 7.37 / 0.8 - 2 = 9.2125 - 2 = 7.2125 ≈ 7
        PLLFBD = 43;      // Multiplicador M = 45 (PLLDIV + 2)
        CLKDIVbits.PLLPOST = 0;  // Divisor post-PLL N2 = 2
        
        // Esperar estabilización
        __delay_us(100);
        
        // Habilitar PLL
        CLKDIVbits.PLLEN = 1;
        
        // Esperar que el PLL se bloquee
        while(!OSCCONbits.LOCK);
        
    #endif
}

/**
 * @brief Configura los puertos para ahorro de energía
 */
static void _ConfigurePowerSaving(void) {
    // Deshabilitar puertos no utilizados
    
    #ifndef CONFIG_PORT_A_ENABLED
        // Deshabilitar puerto A
        ANSELA = 0x0000;  // Todos digitales
        TRISA = 0xFFFF;   // Todos como entradas
        LATA = 0x0000;    // Salidas en bajo
    #endif
    
    #ifndef CONFIG_PORT_B_ENABLED
        // Deshabilitar puerto B
        ANSELB = 0x0000;
        TRISB = 0xFFFF;
        LATB = 0x0000;
    #endif
    
    #ifndef CONFIG_PORT_C_ENABLED
        // Deshabilitar puerto C
        ANSELC = 0x0000;
        TRISC = 0xFFFF;
        LATC = 0x0000;
    #endif
}

/**
 * @brief Configura el clock switching
 */
static void _ConfigureClockSwitching(void) {
    #ifdef CONFIG_CLOCK_SWITCH_ON
        // Habilitar cambio de clock
        OSCCONbits.COSC = 0b001;  // FRC con PLL como fuente actual
        OSCCONbits.NOSC = 0b001;  // FRC con PLL como fuente nueva
        
        // Habilitar clock switching
        OSCCONbits.OSWEN = 1;
        while(OSCCONbits.OSWEN);  // Esperar que se complete
    #endif
}

// =============================================================================
// FUNCIONES PÚBLICAS
// =============================================================================

/**
 * @brief Inicializa todo el sistema
 */
void SYSTEM_Initialize(void) {
    // 1. Deshabilitar interrupciones globalmente
    SYSTEM_DisableInterrupts();
    
    // 2. Configurar registros de configuración
    _ConfigureFOSCSEL();   // Selección de oscilador
    _ConfigureFOSC();      // Configuración de oscilador
    _ConfigureFWDT();      // Watchdog timer
    _ConfigureFPOR();      // Power-on reset
    _ConfigureFICD();      // In-circuit debugger
    _ConfigureFGS();       // General segment
    
    // 3. Configurar MCLR
    #ifdef CONFIG_MCLR_DISABLED
        // Configurar MCLR como I/O
        ANSELBbits.ANSB5 = 0;   // Digital
        TRISBbits.TRISB5 = 0;   // Salida
        LATBbits.LATB5 = 0;     // En bajo
    #endif
    
    // 4. Configurar PLL si es necesario
    _ConfigurePLL();
    
    // 5. Configurar cambio de clock si está habilitado
    _ConfigureClockSwitching();
    
    // 6. Configurar ahorro de energía
    _ConfigurePowerSaving();
    
    // 7. Configurar interrupciones
    INTCON1bits.NSTDIS = 0;  // Habilitar interrupciones anidadas
    
    // 8. Actualizar estado
    system_state = SYS_STATE_READY;
    
    // 9. Mensaje de inicialización
    printf("Sistema inicializado correctamente.\n");
    printf("Frecuencia: %lu Hz\n", SYSTEM_GetClockFrequency());
}

/**
 * @brief Desinicializa el sistema (para modo bajo consumo)
 */
void SYSTEM_Deinitialize(void) {
    // 1. Deshabilitar todos los periféricos
    // (Agregar desinicialización de periféricos específicos aquí)
    
    // 2. Configurar todos los pines como entradas
    TRISA = 0xFFFF;
    TRISB = 0xFFFF;
    TRISC = 0xFFFF;
    
    // 3. Deshabilitar pull-ups
    CNPU1 = 0x0000;
    CNPU2 = 0x0000;
    
    // 4. Actualizar estado
    system_state = SYS_STATE_SLEEP;
    
    printf("Sistema desinicializado para bajo consumo.\n");
}

/**
 * @brief Entra en modo sleep
 */
void SYSTEM_EnterSleep(void) {
    // 1. Deshabilitar interrupciones
    SYSTEM_DisableInterrupts();
    
    // 2. Configurar para sleep
    OSCCONbits.OSWEN = 0;  // Deshabilitar clock switching
    OSCCONbits.LOCK = 0;   // Desbloquear si es necesario
    
    // 3. Comando sleep
    asm volatile ("pwrsav #0");  // Entrar en modo sleep
    
    // Nota: El código continúa después de un wake-up
}

/**
 * @brief Despierta del modo sleep
 */
void SYSTEM_Wakeup(void) {
    // El wake-up es automático por interrupción
    // Solo actualizar estado
    system_state = SYS_STATE_READY;
}

/**
 * @brief Resetea el sistema (software reset)
 */
void SYSTEM_Reset(void) {
    // Software reset
    asm volatile ("reset");
}

/**
 * @brief Habilita interrupciones globales
 */
void SYSTEM_EnableInterrupts(void) {
    // Habilitar interrupciones multi-vector
    INTCON2bits.GIE = 1;  // Global Interrupt Enable
    
    // Configurar prioridad por defecto
    IPC0 = 0x0000;
    IPC1 = 0x0000;
    IPC2 = 0x0000;
    IPC3 = 0x0000;
    IPC4 = 0x0000;
    
    printf("Interrupciones habilitadas.\n");
}

/**
 * @brief Deshabilita interrupciones globales
 */
void SYSTEM_DisableInterrupts(void) {
    INTCON2bits.GIE = 0;  // Global Interrupt Disable
}

/**
 * @brief Obtiene la frecuencia del clock actual
 */
uint32_t SYSTEM_GetClockFrequency(void) {
    #ifdef CONFIG_OSC_INTERNO_PLL
        return FCY;  // 40 MHz
    #elif defined(CONFIG_OSC_INTERNO_SIMPLE)
        return FOSC_PRIM;  // 7.37 MHz
    #else
        return 0;
    #endif
}

/**
 * @brief Obtiene el estado actual del sistema
 */
System_State_t SYSTEM_GetState(void) {
    return system_state;
}

/**
 * @brief Imprime la configuración actual del sistema
 */
void SYSTEM_PrintConfiguration(void) {
    printf("\n=== CONFIGURACIÓN DEL SISTEMA ===\n");
    
    // Oscilador
    #ifdef CONFIG_OSC_INTERNO_PLL
        printf("Oscilador: Interno con PLL (40 MIPS)\n");
    #elif defined(CONFIG_OSC_INTERNO_SIMPLE)
        printf("Oscilador: Interno simple (7.37 MIPS)\n");
    #endif
    
    // Watchdog
    #ifdef CONFIG_WDT_OFF
        printf("Watchdog: DESHABILITADO\n");
    #elif defined(CONFIG_WDT_ON_NORMAL)
        printf("Watchdog: HABILITADO (tiempo normal)\n");
    #elif defined(CONFIG_WDT_ON_LONG)
        printf("Watchdog: HABILITADO (tiempo largo)\n");
    #endif
    
    // BOR
    #ifdef CONFIG_BOR_OFF
        printf("BOR: DESHABILITADO\n");
    #else
        printf("BOR: HABILITADO\n");
    #endif
    
    // Protección de código
    #ifdef CONFIG_CODE_PROTECT_OFF
        printf("Protección de código: DESHABILITADA\n");
    #else
        printf("Protección de código: HABILITADA\n");
    #endif
    
    // Debug
    #ifdef CONFIG_DEBUG_OFF
        printf("Debug: DESHABILITADO\n");
    #else
        printf("Debug: HABILITADO\n");
    #endif
    
    // MCLR
    #ifdef CONFIG_MCLR_ENABLED
        printf("MCLR: HABILITADO\n");
    #else
        printf("MCLR: DESHABILITADO (pin como I/O)\n");
    #endif
    
    // Puertos habilitados
    printf("Puertos activos: ");
    #ifdef CONFIG_PORT_A_ENABLED
        printf("A ");
    #endif
    #ifdef CONFIG_PORT_B_ENABLED
        printf("B ");
    #endif
    #ifdef CONFIG_PORT_C_ENABLED
        printf("C ");
    #endif
    printf("\n");
    
    // Frecuencia actual
    printf("Frecuencia: %lu Hz\n", SYSTEM_GetClockFrequency());
    printf("Estado: %s\n", 
        system_state == SYS_STATE_READY ? "Listo" :
        system_state == SYS_STATE_BUSY ? "Ocupado" :
        system_state == SYS_STATE_ERROR ? "Error" : "Inicializando");
    
    printf("===============================\n");
}