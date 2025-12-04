/*******************************************************************************
 * i2cmain.c - Ejemplos de uso de la librería I2C
 * 
 * Este archivo contiene ejemplos prácticos de cómo usar la librería I2C
 * con diferentes dispositivos y configuraciones.
 * 
 ******************************************************************************/

#include "i2c.h"
#include <stdio.h>
#include <string.h>

// =============================================================================
// EJEMPLO 1: CONFIGURACIÓN BÁSICA
// =============================================================================

void ejemplo_configuracion_basica(void) {
    printf("=== Ejemplo 1: Configuración Básica ===\n");
    
    // Configuración como maestro
    I2C_Config_t config = I2C_CONFIG_DEFAULT_MASTER;
    config.module = I2C_MODULE_1;
    config.speed = I2C_SPEED_100KHZ;
    
    // Inicializar I2C
    I2C_Init(&config);
    
    // Imprimir configuración
    I2C_PrintConfig(I2C_MODULE_1);
    
    printf("I2C inicializado correctamente.\n");
}

// =============================================================================
// EJEMPLO 2: ESCANEO DE DISPOSITIVOS I2C
// =============================================================================

void ejemplo_escanear_bus(void) {
    printf("\n=== Ejemplo 2: Escaneo de Bus I2C ===\n");
    
    uint8_t dispositivos[16];
    uint8_t encontrados = 0;
    
    // Escanear bus
    if (I2C_ScanBus(I2C_MODULE_1, dispositivos, 16)) {
        printf("Dispositivos encontrados:\n");
        for (uint8_t i = 0; i < 16 && dispositivos[i] != 0; i++) {
            printf("  - Dirección 0x%02X\n", dispositivos[i]);
            encontrados++;
        }
        printf("Total: %d dispositivos\n", encontrados);
    } else {
        printf("No se encontraron dispositivos I2C.\n");
    }
}

// =============================================================================
// EJEMPLO 3: COMUNICACIÓN CON EEPROM 24LC256
// =============================================================================

#define EEPROM_ADDRESS 0x50  // Dirección de EEPROM 24LC256

void ejemplo_eeprom_24lc256(void) {
    printf("\n=== Ejemplo 3: EEPROM 24LC256 ===\n");
    
    uint8_t direccion = 0x0000;  // Dirección de memoria
    uint8_t dato_escritura = 0xAB;
    uint8_t dato_lectura = 0x00;
    
    // Escribir en EEPROM
    uint8_t buffer_escritura[3] = {
        (direccion >> 8) & 0xFF,   // Dirección alta
        direccion & 0xFF,          // Dirección baja
        dato_escritura             // Dato
    };
    
    if (I2C_WriteData(I2C_MODULE_1, EEPROM_ADDRESS, buffer_escritura, 3)) {
        printf("Dato 0x%02X escrito en dirección 0x%04X\n", 
               dato_escritura, direccion);
        
        // Pequeña espera para escritura
        __delay_ms(10);
        
        // Leer de EEPROM
        uint8_t buffer_direccion[2] = {
            (direccion >> 8) & 0xFF,
            direccion & 0xFF
        };
        
        // Primero escribir dirección
        I2C_WriteData(I2C_MODULE_1, EEPROM_ADDRESS, buffer_direccion, 2);
        
        // Luego leer dato
        I2C_ReadData(I2C_MODULE_1, EEPROM_ADDRESS, &dato_lectura, 1);
        
        printf("Dato leído: 0x%02X\n", dato_lectura);
        
        if (dato_escritura == dato_lectura) {
            printf("✓ Verificación exitosa!\n");
        } else {
            printf("✗ Error en verificación\n");
        }
    } else {
        printf("Error al escribir en EEPROM\n");
    }
}

// =============================================================================
// EJEMPLO 4: SENSOR DE TEMPERATURA LM75
// =============================================================================

#define LM75_ADDRESS 0x48  // Dirección por defecto del LM75
#define LM75_REG_TEMP 0x00 // Registro de temperatura

void ejemplo_sensor_lm75(void) {
    printf("\n=== Ejemplo 4: Sensor LM75 ===\n");
    
    // Leer temperatura del LM75
    uint8_t buffer_temp[2];
    
    if (I2C_ReadData(I2C_MODULE_1, LM75_ADDRESS, buffer_temp, 2)) {
        // Convertir a temperatura (LM75: 11-bit, 0.125°C/LSB)
        int16_t raw_temp = (buffer_temp[0] << 8) | buffer_temp[1];
        raw_temp >>= 5;  // Desplazar bits de relleno
        
        float temperatura = (float)raw_temp * 0.125;
        
        printf("Temperatura LM75: %.2f°C\n", temperatura);
    } else {
        printf("Error al leer sensor LM75\n");
    }
}

// =============================================================================
// EJEMPLO 5: MODO ESCLAVO
// =============================================================================

// Callback para modo esclavo
void esclavo_callback(I2C_Event_t evento, uint8_t dato) {
    static uint8_t buffer[32];
    static uint8_t index = 0;
    
    switch(evento) {
        case I2C_EVENT_START:
            printf("Esclavo: START recibido\n");
            index = 0;
            break;
            
        case I2C_EVENT_DATA_RECEIVED:
            printf("Esclavo: Dato recibido: 0x%02X\n", dato);
            if (index < 32) {
                buffer[index++] = dato;
            }
            break;
            
        case I2C_EVENT_DATA_REQUESTED:
            printf("Esclavo: Solicitado dato\n");
            // Enviar respuesta
            I2C_PutByte(I2C_MODULE_2, 0xAA);
            break;
            
        case I2C_EVENT_STOP:
            printf("Esclavo: STOP recibido\n");
            printf("Total datos recibidos: %d\n", index);
            break;
            
        default:
            break;
    }
}

void ejemplo_modo_esclavo(void) {
    printf("\n=== Ejemplo 5: Modo Esclavo ===\n");
    
    // Configurar como esclavo
    I2C_Config_t config = I2C_CONFIG_DEFAULT_SLAVE;
    config.module = I2C_MODULE_2;  // Usar módulo 2 como esclavo
    config.slave_address = 0x40;   // Dirección del esclavo
    config.callback = esclavo_callback;
    
    I2C_Init(&config);
    
    printf("Esclavo configurado en dirección 0x%02X\n", config.slave_address);
    printf("Esperando comunicación desde maestro...\n");
    
    // Mantener activo
    while(1) {
        // El esclavo responde mediante interrupciones
        __delay_ms(100);
    }
}

// =============================================================================
// EJEMPLO 6: RUTINAS AVANZADAS
// =============================================================================

void ejemplo_avanzado(void) {
    printf("\n=== Ejemplo 6: Rutinas Avanzadas ===\n");
    
    // 1. Escritura múltiple de registros
    uint8_t dispositivo = 0x68;  // Dirección ejemplo
    uint8_t registros[] = {0x00, 0x01, 0x02, 0x03};
    uint8_t valores[] = {0x10, 0x20, 0x30, 0x40};
    
    for (uint8_t i = 0; i < 4; i++) {
        if (I2C_WriteRegister(I2C_MODULE_1, dispositivo, registros[i], valores[i])) {
            printf("Registro 0x%02X escrito con 0x%02X\n", registros[i], valores[i]);
        }
    }
    
    // 2. Lectura secuencial
    uint8_t buffer_lectura[4];
    if (I2C_ReadData(I2C_MODULE_1, dispositivo, buffer_lectura, 4)) {
        printf("Datos leídos: ");
        for (uint8_t i = 0; i < 4; i++) {
            printf("0x%02X ", buffer_lectura[i]);
        }
        printf("\n");
    }
}

// =============================================================================
// FUNCIÓN PRINCIPAL
// =============================================================================

int main(void) {
    // Inicializar sistema
    // ... (configuración de clock, etc.)
    
    printf("\n========== DEMO LIBRERÍA I2C ==========\n");
    
    // Ejecutar ejemplos
    ejemplo_configuracion_basica();
    
    // Esperar un momento
    __delay_ms(1000);
    
    ejemplo_escanear_bus();
    
    // Solo ejecutar si hay dispositivos conectados
    ejemplo_eeprom_24lc256();
    ejemplo_sensor_lm75();
    
    // Descomentar para probar modo esclavo
    // ejemplo_modo_esclavo();
    
    ejemplo_avanzado();
    
    printf("\n========== FIN DE DEMO ==========\n");
    
    while(1) {
        // Mantener programa activo
    }
    
    return 0;
}

// =============================================================================
// INTERRUPCIONES (si se usan)
// =============================================================================

// Interrupción I2C1
void __attribute__((interrupt, no_auto_psv)) _I2C1Interrupt(void) {
    I2C_ISR_Handler(I2C_MODULE_1);
}

// Interrupción I2C2
void __attribute__((interrupt, no_auto_psv)) _I2C2Interrupt(void) {
    I2C_ISR_Handler(I2C_MODULE_2);
}