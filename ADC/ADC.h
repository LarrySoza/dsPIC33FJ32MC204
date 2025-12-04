/*******************************************************************************
 * ADC.h - Librería para el módulo ADC del dsPIC33FJ32MC204
 * 
 * Autor: Carlos A. Olivera Ylla
 * Fecha: 04/12/2025
 * 
 * Descripción: Librería para configuración y uso del ADC de 10-bit, 16 canales
 *              del dsPIC33FJ32MC204. Optimizada para muestreo rápido (hasta 1.1 Msps).
 * 
 ******************************************************************************/

#ifndef ADC_H
#define ADC_H

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// DEFINICIONES DE CONFIGURACIÓN
// =============================================================================

// Frecuencia del ADC (máximo 1.1 Msps para 10-bit)
#define ADC_MAX_SAMPLE_RATE 1100000UL

// Canales ADC disponibles (AN0-AN15)
typedef enum {
    ADC_CHANNEL_0 = 0,    // AN0
    ADC_CHANNEL_1,        // AN1
    ADC_CHANNEL_2,        // AN2
    ADC_CHANNEL_3,        // AN3
    ADC_CHANNEL_4,        // AN4
    ADC_CHANNEL_5,        // AN5
    ADC_CHANNEL_6,        // AN6
    ADC_CHANNEL_7,        // AN7
    ADC_CHANNEL_8,        // AN8
    ADC_CHANNEL_9,        // AN9
    ADC_CHANNEL_10,       // AN10
    ADC_CHANNEL_11,       // AN11
    ADC_CHANNEL_12,       // AN12
    ADC_CHANNEL_13,       // AN13
    ADC_CHANNEL_14,       // AN14
    ADC_CHANNEL_15,       // AN15
    ADC_CHANNEL_TEMP,     // Sensor de temperatura interno
    ADC_CHANNEL_DAC1,     // Salida DAC1
    ADC_CHANNEL_FVR,      // Fixed Voltage Reference
    ADC_TOTAL_CHANNELS    // Total de canales disponibles
} ADC_Channel_t;

// Modos de conversión
typedef enum {
    ADC_MODE_SINGLE,      // Conversión única
    ADC_MODE_CONTINUOUS,  // Conversión continua
    ADC_MODE_SCAN,        // Modo scan
    ADC_MODE_MUX_SAMPLE   // Muestreo multiplexado
} ADC_Mode_t;

// Formatos de resultado
typedef enum {
    ADC_FORMAT_INTEGER,   // Formato entero (10-bit en 16-bit)
    ADC_FORMAT_FRACTIONAL // Formato fraccionario
} ADC_Format_t;

// Fuentes de trigger
typedef enum {
    ADC_TRIGGER_MANUAL,   // Trigger manual
    ADC_TRIGGER_TMR1,     // Timer1
    ADC_TRIGGER_TMR2,     // Timer2
    ADC_TRIGGER_TMR3,     // Timer3
    ADC_TRIGGER_PWM,      // PWM
    ADC_TRIGGER_EXT_INT,  // Interrupción externa
    ADC_TRIGGER_AUTO      // Auto-sampling
} ADC_Trigger_t;

// Número de muestras para promediado
typedef enum {
    ADC_AVG_NONE = 0,     // Sin promediado
    ADC_AVG_2 = 1,        // 2 muestras
    ADC_AVG_4 = 2,        // 4 muestras
    ADC_AVG_8 = 3,        // 8 muestras
    ADC_AVG_16 = 4,       // 16 muestras
    ADC_AVG_32 = 5        // 32 muestras
} ADC_Average_t;

// Estructura de configuración del ADC
typedef struct {
    ADC_Mode_t mode;              // Modo de operación
    ADC_Format_t format;          // Formato del resultado
    ADC_Trigger_t trigger;        // Fuente de trigger
    ADC_Average_t averaging;      // Nivel de promediado
    uint32_t sample_rate;         // Tasa de muestreo deseada (Hz)
    float vref_positive;          // Voltaje de referencia positivo (V)
    float vref_negative;          // Voltaje de referencia negativo (V)
    bool interrupt_enable;        // Habilitar interrupciones
    bool auto_sample;             // Auto-sampling habilitado
    bool alternate_mux;           // Usar mux alternativo
    bool calibrate;               // Realizar calibración al inicio
} ADC_Config_t;

// Configuración por defecto
#define ADC_CONFIG_DEFAULT { \
    .mode = ADC_MODE_SINGLE, \
    .format = ADC_FORMAT_INTEGER, \
    .trigger = ADC_TRIGGER_MANUAL, \
    .averaging = ADC_AVG_NONE, \
    .sample_rate = 100000, \
    .vref_positive = 3.3, \
    .vref_negative = 0.0, \
    .interrupt_enable = false, \
    .auto_sample = false, \
    .alternate_mux = false, \
    .calibrate = true \
}

// =============================================================================
// PROTOTIPOS DE FUNCIONES
// =============================================================================

// Inicialización y configuración
void ADC_Init(ADC_Config_t *config);
void ADC_Deinit(void);
void ADC_Calibrate(void);
void ADC_SetVREF(float vref_pos, float vref_neg);

// Control del ADC
void ADC_StartConversion(void);
void ADC_StopConversion(void);
void ADC_Enable(void);
void ADC_Disable(void);

// Selección de canal
void ADC_SelectChannel(ADC_Channel_t channel);
void ADC_ScanChannels(ADC_Channel_t *channels, uint8_t count);
void ADC_EnableChannels(uint16_t channel_mask);

// Lectura de datos
uint16_t ADC_ReadSingle(ADC_Channel_t channel);
uint16_t ADC_ReadRaw(void);
float ADC_ReadVoltage(ADC_Channel_t channel);
bool ADC_IsConversionComplete(void);
void ADC_WaitForConversion(void);

// Funciones avanzadas
void ADC_SetSampleTime(uint8_t tad_cycles);
void ADC_SetConversionClock(uint32_t frequency);
void ADC_EnableDMA(bool enable);
void ADC_SetInterruptCallback(void (*callback)(uint16_t value));

// Lectura especializada
uint16_t ADC_ReadTemperature(void);
float ADC_ReadTemperatureCelsius(void);
float ADC_ReadTemperatureFahrenheit(void);
uint16_t ADC_ReadVDD(void);
float ADC_ReadVDDVoltage(void);

// Configuración de buffer
void ADC_ConfigureBuffer(bool enable, uint8_t size);
uint16_t ADC_GetBufferValue(uint8_t index);

// Funciones utilitarias
float ADC_RawToVoltage(uint16_t raw_value);
uint16_t ADC_VoltageToRaw(float voltage);
void ADC_PrintConfiguration(void);

// =============================================================================
// VARIABLES GLOBALES (externas)
// =============================================================================
extern volatile bool ADC_ConversionComplete;
extern volatile uint16_t ADC_LastValue;
extern ADC_Config_t ADC_CurrentConfig;

#endif /* ADC_H */