/*******************************************************************************
 * config_basica.h - Configuración básica predefinida
 * 
 * Incluye este archivo si quieres usar la configuración básica recomendada
 * sin tener que descomentar opciones manualmente.
 * 
 ******************************************************************************/

#ifndef CONFIG_BASICA_H
#define CONFIG_BASICA_H

// =============================================================================
// CONFIGURACIÓN BÁSICA RECOMENDADA
// =============================================================================

// 1. Oscilador interno con PLL (40 MIPS)
#define CONFIG_OSC_INTERNO_PLL

// 2. Watchdog deshabilitado (para desarrollo)
#define CONFIG_WDT_OFF

// 3. MCLR habilitado
#define CONFIG_MCLR_ENABLED

// 4. BOR deshabilitado
#define CONFIG_BOR_OFF

// 5. Código sin proteger (para desarrollo)
#define CONFIG_CODE_PROTECT_OFF

// 6. Debug deshabilitado (pines como I/O)
#define CONFIG_DEBUG_OFF

// 7. Clock switching deshabilitado
#define CONFIG_CLOCK_SWITCH_OFF

// 8. Solo puerto B habilitado (ajustar según necesidades)
#define CONFIG_PORT_B_ENABLED

#endif /* CONFIG_BASICA_H */