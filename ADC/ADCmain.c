/*******************************************************************************
 * mainADC.c - Ejemplo de uso de la librería ADC
 * 
 * Ejemplo básico que muestra cómo usar la librería ADC
 * para leer diferentes canales y trabajar con los datos.
 * 
 ******************************************************************************/

#include "ADC.h"
#include <stdio.h>

// Callback para interrupciones del ADC
void myADCCallback(uint16_t value) {
    float voltage = ADC_RawToVoltage(value);
    printf("Interrupción ADC: Raw=%d, Voltaje=%.3fV\n", value, voltage);
}

int main(void) {
    // 1. Configurar sistema (clock, etc.)
    // ...
    
    // 2. Configurar ADC
    ADC_Config_t adc_config = ADC_CONFIG_DEFAULT;
    adc_config.mode = ADC_MODE_SINGLE;
    adc_config.sample_rate = 100000; // 100 kHz
    adc_config.interrupt_enable = true;
    adc_config.averaging = ADC_AVG_4;
    
    ADC_Init(&adc_config);
    ADC_SetInterruptCallback(myADCCallback);
    
    // 3. Ejemplo: Leer canal único
    uint16_t raw_value = ADC_ReadSingle(ADC_CHANNEL_0);
    float voltage = ADC_RawToVoltage(raw_value);
    printf("AN0: Raw=%d, Voltage=%.3fV\n", raw_value, voltage);
    
    // 4. Ejemplo: Leer temperatura
    float temp_c = ADC_ReadTemperatureCelsius();
    float temp_f = ADC_ReadTemperatureFahrenheit();
    printf("Temperatura: %.1f°C, %.1f°F\n", temp_c, temp_f);
    
    // 5. Ejemplo: Modo continuo con buffer
    ADC_ConfigureBuffer(true, 16);
    ADC_SelectChannel(ADC_CHANNEL_1);
    
    // Iniciar conversión continua
    adc_config.mode = ADC_MODE_CONTINUOUS;
    ADC_Init(&adc_config);
    ADC_StartConversion();
    
    while (1) {
        // Procesar datos del buffer
        for (uint8_t i = 0; i < 16; i++) {
            uint16_t buffered_value = ADC_GetBufferValue(i);
            // Procesar valor...
        }
        
        // Leer voltaje directamente
        voltage = ADC_ReadVoltage(ADC_CHANNEL_2);
        printf("AN2 Voltage: %.3fV\n", voltage);
        
        // Pequeño delay
        __delay_ms(100);
    }
    
    return 0;
}