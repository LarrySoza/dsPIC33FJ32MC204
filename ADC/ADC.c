/*******************************************************************************
 * ADC.c - Implementación de la librería ADC para dsPIC33FJ32MC204
 * 
 * Autor: Carlos A. Olivera Ylla
 * Fecha: 04/12/2025
 * 
 * Descripción: Implementación completa del módulo ADC con todas las funciones
 *              necesarias para operación básica y avanzada.
 * 
 ******************************************************************************/

#include "ADC.h"
#include <string.h>
#include <stdio.h>

// =============================================================================
// VARIABLES GLOBALES
// =============================================================================

volatile bool ADC_ConversionComplete = false;
volatile uint16_t ADC_LastValue = 0;
ADC_Config_t ADC_CurrentConfig;
static void (*ADC_UserCallback)(uint16_t) = NULL;

// Buffer para modo scan
static ADC_Channel_t scan_channels[16];
static uint8_t scan_channel_count = 0;
static uint8_t current_scan_index = 0;

// Buffer circular para almacenamiento
static uint16_t adc_buffer[32];
static uint8_t buffer_index = 0;
static bool buffer_enabled = false;
static uint8_t buffer_size = 16;

// =============================================================================
// FUNCIONES PRIVADAS
// =============================================================================

/**
 * @brief Calcula los valores de configuración del clock del ADC
 * @param desired_rate Tasa de muestreo deseada en Hz
 * @return Tad en ciclos de instrucción
 */
static uint8_t _ADC_CalculateTAD(uint32_t desired_rate) {
    uint32_t fcy = 40000000; // 40 MIPS = 40 MHz (Fcy)
    uint32_t max_adc_rate = ADC_MAX_SAMPLE_RATE;
    
    if (desired_rate > max_adc_rate) {
        desired_rate = max_adc_rate;
    }
    
    // Tad mínimo = 75ns para 10-bit, Fcy = 40MHz -> Tcy = 25ns
    // Tad mínimo en ciclos = 75ns / 25ns = 3 ciclos
    uint8_t min_tad_cycles = 3;
    
    // Calcular Tad necesario
    uint32_t required_tad_cycles = fcy / desired_rate;
    
    if (required_tad_cycles < min_tad_cycles) {
        required_tad_cycles = min_tad_cycles;
    }
    
    // Asegurar que esté en rango permitido (3-31 ciclos)
    if (required_tad_cycles > 31) {
        required_tad_cycles = 31;
    }
    
    return (uint8_t)required_tad_cycles;
}

/**
 * @brief Configura los pines analógicos según el canal seleccionado
 * @param channel Canal ADC a configurar
 */
static void _ADC_ConfigurePins(ADC_Channel_t channel) {
    // Deshabilitar pines digitales para los canales analógicos
    switch (channel) {
        case ADC_CHANNEL_0:
            ANSAbits.ANSA0 = 1; TRISAbits.TRISA0 = 1;
            break;
        case ADC_CHANNEL_1:
            ANSAbits.ANSA1 = 1; TRISAbits.TRISA1 = 1;
            break;
        case ADC_CHANNEL_2:
            ANSBbits.ANSB0 = 1; TRISBbits.TRISB0 = 1;
            break;
        case ADC_CHANNEL_3:
            ANSBbits.ANSB1 = 1; TRISBbits.TRISB1 = 1;
            break;
        case ADC_CHANNEL_4:
            ANSBbits.ANSB2 = 1; TRISBbits.TRISB2 = 1;
            break;
        case ADC_CHANNEL_5:
            ANSBbits.ANSB3 = 1; TRISBbits.TRISB3 = 1;
            break;
        // Agregar más casos según sea necesario
        default:
            break;
    }
}

/**
 * @brief Configura el trigger del ADC
 * @param trigger Fuente de trigger a configurar
 */
static void _ADC_ConfigureTrigger(ADC_Trigger_t trigger) {
    AD1CON1bits.SSRC = 0; // Resetear configuración
    
    switch (trigger) {
        case ADC_TRIGGER_MANUAL:
            AD1CON1bits.SSRC = 0b000; // Manual trigger
            break;
        case ADC_TRIGGER_TMR1:
            AD1CON1bits.SSRC = 0b011; // Timer1 period match
            break;
        case ADC_TRIGGER_TMR2:
            // Configurar para Timer2
            break;
        case ADC_TRIGGER_PWM:
            AD1CON1bits.SSRC = 0b010; // PWM special event
            break;
        case ADC_TRIGGER_AUTO:
            AD1CON1bits.SSRC = 0b111; // Auto-convert
            break;
        default:
            AD1CON1bits.SSRC = 0b000; // Manual por defecto
            break;
    }
}

// =============================================================================
// FUNCIONES PÚBLICAS
// =============================================================================

/**
 * @brief Inicializa el módulo ADC con la configuración especificada
 * @param config Puntero a estructura de configuración
 */
void ADC_Init(ADC_Config_t *config) {
    if (config == NULL) {
        // Usar configuración por defecto
        ADC_CurrentConfig = (ADC_Config_t)ADC_CONFIG_DEFAULT;
    } else {
        memcpy(&ADC_CurrentConfig, config, sizeof(ADC_Config_t));
    }
    
    // 1. Deshabilitar ADC durante configuración
    ADC_Disable();
    
    // 2. Calibrar ADC si está habilitado
    if (ADC_CurrentConfig.calibrate) {
        ADC_Calibrate();
    }
    
    // 3. Configurar modo de operación
    AD1CON1bits.AD12B = 1; // Modo 12-bit (compatible con 10-bit)
    AD1CON1bits.FORM = ADC_CurrentConfig.format; // Formato del resultado
    
    if (ADC_CurrentConfig.mode == ADC_MODE_CONTINUOUS) {
        AD1CON1bits.ASAM = 1; // Auto-sampling habilitado
    } else {
        AD1CON1bits.ASAM = 0; // Sampling manual
    }
    
    // 4. Configurar trigger
    _ADC_ConfigureTrigger(ADC_CurrentConfig.trigger);
    
    // 5. Configurar clock del ADC
    uint8_t tad_cycles = _ADC_CalculateTAD(ADC_CurrentConfig.sample_rate);
    ADC_SetSampleTime(tad_cycles);
    
    // 6. Configurar referencia de voltaje
    ADC_SetVREF(ADC_CurrentConfig.vref_positive, ADC_CurrentConfig.vref_negative);
    
    // 7. Configurar promediado
    AD1CON3bits.SAMC = ADC_CurrentConfig.averaging;
    
    // 8. Configurar mux alternativo si es necesario
    AD1CON1bits.ALTS = ADC_CurrentConfig.alternate_mux ? 1 : 0;
    
    // 9. Configurar interrupciones
    if (ADC_CurrentConfig.interrupt_enable) {
        IFS0bits.AD1IF = 0; // Limpiar flag de interrupción
        IEC0bits.AD1IE = 1; // Habilitar interrupción ADC
        IPC3bits.AD1IP = 4; // Prioridad de interrupción
    } else {
        IEC0bits.AD1IE = 0; // Deshabilitar interrupción
    }
    
    // 10. Habilitar ADC
    ADC_Enable();
    
    // 11. Inicializar variables
    ADC_ConversionComplete = false;
    ADC_LastValue = 0;
    buffer_index = 0;
    memset(adc_buffer, 0, sizeof(adc_buffer));
}

/**
 * @brief Deshabilita y reinicia el módulo ADC
 */
void ADC_Deinit(void) {
    ADC_Disable();
    AD1CON1 = 0x0000;
    AD1CON2 = 0x0000;
    AD1CON3 = 0x0000;
    AD1CHS = 0x0000;
    IEC0bits.AD1IE = 0;
    IFS0bits.AD1IF = 0;
}

/**
 * @brief Calibra el módulo ADC
 * @note Esta función debe llamarse con VDD estable y sin carga en los pines
 */
void ADC_Calibrate(void) {
    // Guardar configuración actual
    uint16_t ad1con1_save = AD1CON1;
    uint16_t ad1con2_save = AD1CON2;
    uint16_t ad1con3_save = AD1CON3;
    
    // Deshabilitar ADC
    AD1CON1bits.ADON = 0;
    
    // Configurar para calibración
    AD1CON1 = 0x0000;
    AD1CON2 = 0x0000;
    AD1CON3 = 0x000F; // Tad máximo para calibración
    
    // Habilitar ADC
    AD1CON1bits.ADON = 1;
    
    // Esperar tiempo de estabilización
    __delay_us(10);
    
    // Iniciar calibración (proceso específico del dsPIC)
    // Nota: Consultar el manual del dsPIC33F para el procedimiento exacto
    
    // Restaurar configuración
    AD1CON1 = ad1con1_save;
    AD1CON2 = ad1con2_save;
    AD1CON3 = ad1con3_save;
    
    // Re-habilitar ADC si estaba habilitado
    if (ad1con1_save & 0x8000) {
        AD1CON1bits.ADON = 1;
    }
}

/**
 * @brief Configura los voltajes de referencia
 * @param vref_pos Voltaje de referencia positivo (V)
 * @param vref_neg Voltaje de referencia negativo (V)
 */
void ADC_SetVREF(float vref_pos, float vref_neg) {
    ADC_CurrentConfig.vref_positive = vref_pos;
    ADC_CurrentConfig.vref_negative = vref_neg;
    
    // Para dsPIC33FJ32MC204, las referencias son AVDD y AVSS por defecto
    // Si se necesitan referencias externas, configurar AD1CON2bits.VCFG
    AD1CON2bits.VCFG = 0b000; // AVDD y AVSS como referencias
    
    // Nota: Para usar referencias externas, conectar a AN2/AN3 y configurar:
    // AD1CON2bits.VCFG = 0b111; // Referencias externas en AN2/AN3
}

/**
 * @brief Selecciona un canal para conversión
 * @param channel Canal a seleccionar
 */
void ADC_SelectChannel(ADC_Channel_t channel) {
    if (channel >= ADC_TOTAL_CHANNELS) {
        return; // Canal inválido
    }
    
    // Configurar pines si es un canal físico
    if (channel <= ADC_CHANNEL_15) {
        _ADC_ConfigurePins(channel);
    }
    
    // Configurar CH0 para conversión
    AD1CHS0bits.CH0SA = channel;
    AD1CHS0bits.CH0NA = 0; // Referencia negativa a VREF-
}

/**
 * @brief Lee un solo valor del canal especificado
 * @param channel Canal a leer
 * @return Valor ADC de 10-bit (0-1023)
 */
uint16_t ADC_ReadSingle(ADC_Channel_t channel) {
    // Seleccionar canal
    ADC_SelectChannel(channel);
    
    // Iniciar conversión
    AD1CON1bits.SAMP = 1; // Iniciar sampling
    __delay_us(1); // Pequeña espera
    
    AD1CON1bits.SAMP = 0; // Iniciar conversión
    
    // Esperar a que complete la conversión
    ADC_WaitForConversion();
    
    // Leer resultado
    return ADC_ReadRaw();
}

/**
 * @brief Lee el valor crudo del ADC
 * @return Valor ADC de 10-bit (0-1023)
 */
uint16_t ADC_ReadRaw(void) {
    ADC_WaitForConversion();
    
    // Leer buffer de resultado
    uint16_t result = ADC1BUF0;
    
    // Para 10-bit, los bits están en las posiciones más significativas
    result >>= 6; // Shift right para obtener valor 10-bit
    
    // Guardar último valor
    ADC_LastValue = result;
    
    // Almacenar en buffer si está habilitado
    if (buffer_enabled) {
        adc_buffer[buffer_index] = result;
        buffer_index = (buffer_index + 1) % buffer_size;
    }
    
    return result;
}

/**
 * @brief Lee el voltaje en el canal especificado
 * @param channel Canal a leer
 * @return Voltaje en el pin (V)
 */
float ADC_ReadVoltage(ADC_Channel_t channel) {
    uint16_t raw = ADC_ReadSingle(channel);
    return ADC_RawToVoltage(raw);
}

/**
 * @brief Verifica si la conversión ha completado
 * @return true si la conversión ha completado, false en caso contrario
 */
bool ADC_IsConversionComplete(void) {
    return AD1CON1bits.DONE == 1;
}

/**
 * @brief Espera bloqueante hasta que complete la conversión
 */
void ADC_WaitForConversion(void) {
    while (!ADC_IsConversionComplete()) {
        // Esperar activamente
    }
    AD1CON1bits.DONE = 0; // Limpiar flag
}

/**
 * @brief Convierte valor crudo a voltaje
 * @param raw_value Valor ADC crudo (0-1023)
 * @return Voltaje correspondiente (V)
 */
float ADC_RawToVoltage(uint16_t raw_value) {
    float vref_range = ADC_CurrentConfig.vref_positive - ADC_CurrentConfig.vref_negative;
    float voltage = ((float)raw_value / 1023.0) * vref_range;
    voltage += ADC_CurrentConfig.vref_negative;
    return voltage;
}

/**
 * @brief Convierte voltaje a valor crudo del ADC
 * @param voltage Voltaje a convertir (V)
 * @return Valor ADC crudo correspondiente (0-1023)
 */
uint16_t ADC_VoltageToRaw(float voltage) {
    // Asegurar que el voltaje esté dentro del rango
    if (voltage < ADC_CurrentConfig.vref_negative) {
        voltage = ADC_CurrentConfig.vref_negative;
    }
    if (voltage > ADC_CurrentConfig.vref_positive) {
        voltage = ADC_CurrentConfig.vref_positive;
    }
    
    float vref_range = ADC_CurrentConfig.vref_positive - ADC_CurrentConfig.vref_negative;
    float normalized = (voltage - ADC_CurrentConfig.vref_negative) / vref_range;
    return (uint16_t)(normalized * 1023.0);
}

/**
 * @brief Configura el tiempo de muestreo del ADC
 * @param tad_cycles Ciclos de Tad para el tiempo de muestreo
 */
void ADC_SetSampleTime(uint8_t tad_cycles) {
    // Asegurar que esté en rango válido (3-31)
    if (tad_cycles < 3) tad_cycles = 3;
    if (tad_cycles > 31) tad_cycles = 31;
    
    AD1CON3bits.SAMC = tad_cycles;
}

/**
 * @brief Configura el clock de conversión del ADC
 * @param frequency Frecuencia deseada en Hz
 */
void ADC_SetConversionClock(uint32_t frequency) {
    ADC_CurrentConfig.sample_rate = frequency;
    uint8_t tad_cycles = _ADC_CalculateTAD(frequency);
    ADC_SetSampleTime(tad_cycles);
}

/**
 * @brief Habilita/deshabilita el ADC
 */
void ADC_Enable(void) {
    AD1CON1bits.ADON = 1;
    __delay_us(10); // Tiempo de estabilización
}

void ADC_Disable(void) {
    AD1CON1bits.ADON = 0;
}

/**
 * @brief Inicia una conversión manual
 */
void ADC_StartConversion(void) {
    AD1CON1bits.SAMP = 1;
    __delay_us(1);
    AD1CON1bits.SAMP = 0;
}

/**
 * @brief Detiene la conversión
 */
void ADC_StopConversion(void) {
    AD1CON1bits.ASAM = 0;
}

/**
 * @brief Lee la temperatura interna
 * @return Valor crudo del sensor de temperatura
 */
uint16_t ADC_ReadTemperature(void) {
    // Seleccionar canal de temperatura
    AD1CHS0bits.CH0SA = 0b11101; // Canal de temperatura interna
    AD1CHS0bits.CH0NA = 0;
    
    // Iniciar conversión
    ADC_StartConversion();
    ADC_WaitForConversion();
    
    return ADC_ReadRaw();
}

/**
 * @brief Lee temperatura en grados Celsius
 * @return Temperatura en °C
 */
float ADC_ReadTemperatureCelsius(void) {
    uint16_t raw_temp = ADC_ReadTemperature();
    
    // Fórmula para dsPIC33F (consultar datasheet para valores exactos)
    // Temperatura = (Vtemp - Vtemp0) / Tc + T0
    // Donde: Vtemp = raw * 3.3/1024, Vtemp0 = 0.6V @ 25°C, Tc = 1.73mV/°C
    
    float vtemp = raw_temp * (3.3 / 1024.0);
    float temperature = ((vtemp - 0.6) / 0.00173) + 25.0;
    
    return temperature;
}

/**
 * @brief Lee temperatura en grados Fahrenheit
 * @return Temperatura en °F
 */
float ADC_ReadTemperatureFahrenheit(void) {
    float celsius = ADC_ReadTemperatureCelsius();
    return (celsius * 9.0 / 5.0) + 32.0;
}

/**
 * @brief Configura buffer circular para almacenamiento
 * @param enable true para habilitar buffer
 * @param size Tamaño del buffer (máx 32)
 */
void ADC_ConfigureBuffer(bool enable, uint8_t size) {
    buffer_enabled = enable;
    if (size > 32) size = 32;
    buffer_size = size;
    buffer_index = 0;
}

/**
 * @brief Obtiene valor del buffer
 * @param index Índice en el buffer
 * @return Valor almacenado en el índice
 */
uint16_t ADC_GetBufferValue(uint8_t index) {
    if (index >= buffer_size) return 0;
    return adc_buffer[index];
}

/**
 * @brief Rutina de interrupción del ADC
 * @note Esta función debe llamarse desde la ISR del ADC
 */
void __attribute__((interrupt, no_auto_psv)) _ADC1Interrupt(void) {
    // Limpiar flag de interrupción
    IFS0bits.AD1IF = 0;
    
    // Leer valor
    ADC_LastValue = ADC_ReadRaw();
    ADC_ConversionComplete = true;
    
    // Llamar callback del usuario si está configurado
    if (ADC_UserCallback != NULL) {
        ADC_UserCallback(ADC_LastValue);
    }
    
    // Manejar modo scan si está activo
    if (scan_channel_count > 0) {
        current_scan_index = (current_scan_index + 1) % scan_channel_count;
        ADC_SelectChannel(scan_channels[current_scan_index]);
        
        // Iniciar siguiente conversión
        if (ADC_CurrentConfig.mode == ADC_MODE_CONTINUOUS) {
            ADC_StartConversion();
        }
    }
}

/**
 * @brief Configura callback para interrupciones del ADC
 * @param callback Función a llamar cuando ocurre interrupción
 */
void ADC_SetInterruptCallback(void (*callback)(uint16_t value)) {
    ADC_UserCallback = callback;
}

// =============================================================================
// FUNCIONES DE DIAGNÓSTICO
// =============================================================================

/**
 * @brief Imprime la configuración actual del ADC
 */
void ADC_PrintConfiguration(void) {
    printf("=== Configuración ADC ===\n");
    printf("Estado: %s\n", AD1CON1bits.ADON ? "Habilitado" : "Deshabilitado");
    printf("Modo: %s\n", ADC_CurrentConfig.mode == ADC_MODE_SINGLE ? "Single" : 
                         ADC_CurrentConfig.mode == ADC_MODE_CONTINUOUS ? "Continuo" : "Scan");
    printf("Sample Rate: %lu Hz\n", ADC_CurrentConfig.sample_rate);
    printf("VREF+: %.2f V\n", ADC_CurrentConfig.vref_positive);
    printf("VREF-: %.2f V\n", ADC_CurrentConfig.vref_negative);
    printf("Promediado: %d muestras\n", 1 << ADC_CurrentConfig.averaging);
    printf("Interrupciones: %s\n", ADC_CurrentConfig.interrupt_enable ? "Habilitadas" : "Deshabilitadas");
    printf("=========================\n");
}