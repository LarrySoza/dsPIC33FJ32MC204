/*******************************************************************************
 * main.c - Ejemplo de uso de la configuración modular
 * 
 * Demuestra cómo usar los archivos de configuración
 * 
 ******************************************************************************/

#include "config.h"
#include <stdio.h>

int main(void) {
    // 1. Inicializar sistema con configuración seleccionada
    SYSTEM_Initialize();
    
    // 2. Mostrar configuración actual
    SYSTEM_PrintConfiguration();
    
    // 3. Ejemplo: Uso de delays con la frecuencia configurada
    printf("\nProbando delays...\n");
    
    LED1 = 1;  // Suponiendo que hay un LED en algún pin
    DELAY_MS(500);  // 500ms delay
    
    LED1 = 0;
    DELAY_MS(500);
    
    // 4. Ejemplo: Cambiar a modo bajo consumo
    printf("Entrando en modo bajo consumo...\n");
    
    // Desinicializar periféricos
    SYSTEM_Deinitialize();
    
    // Entrar en sleep
    // SYSTEM_EnterSleep();  // Descomentar para usar
    
    // 5. Mantener programa activo
    while(1) {
        // Tu código aquí
        // Ejemplo: LED parpadeante
        LED1 = !LED1;
        DELAY_MS(1000);
    }
    
    return 0;
}

// =============================================================================
// FUNCIONES DE INTERRUPCIÓN
// =============================================================================

// Interrupción por defecto (para debugging)
void __attribute__((interrupt, auto_psv)) _DefaultInterrupt(void) {
    // Manejar interrupciones no atendidas
    system_state = SYS_STATE_ERROR;
    
    // Limpiar flags de interrupción
    IFS0 = 0x0000;
    IFS1 = 0x0000;
    IFS2 = 0x0000;
    
    // Recuperarse del error
    SYSTEM_Reset();
}