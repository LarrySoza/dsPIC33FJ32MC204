/*******************************************************************************
 * i2c.h - Librería I2C para dsPIC33FJ32MC204
 * 
 * Autor: Carlos Olivera Vila
 * Fecha: 04/12/2025
 * 
 * Descripción: Implementación completa del módulo I2C (I²C) con soporte para
 *              maestro y esclavo, operaciones síncronas y asíncronas.
 * 
 * Características:
 * - Soporte maestro/esclavo
 * - Clock de 100kHz y 400kHz
 * - Transmisiones síncronas y asíncronas
 * - Buffer para recepción
 * - Timeout y manejo de errores
 * - Compatible con SMBus/PMBus
 * 
 ******************************************************************************/

#ifndef I2C_H
#define I2C_H

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

// =============================================================================
// DEFINICIONES DE CONFIGURACIÓN
// =============================================================================

// Módulo I2C a usar (I2C1 o I2C2)
typedef enum {
    I2C_MODULE_1 = 1,
    I2C_MODULE_2 = 2
} I2C_Module_t;

// Modos de operación
typedef enum {
    I2C_MODE_MASTER,      // Modo maestro
    I2C_MODE_SLAVE,       // Modo esclavo
    I2C_MODE_SLAVE_7BIT,  // Esclavo 7-bit
    I2C_MODE_SLAVE_10BIT  // Esclavo 10-bit
} I2C_Mode_t;

// Velocidades estándar I2C
typedef enum {
    I2C_SPEED_100KHZ = 100000,    // Standard mode
    I2C_SPEED_400KHZ = 400000,    // Fast mode
    I2C_SPEED_1MHZ   = 1000000    // Fast mode plus
} I2C_Speed_t;

// Direcciones I2C especiales
#define I2C_GENERAL_CALL_ADDRESS  0x00
#define I2C_START_BYTE            0x01
#define I2C_CBUS_ADDRESS          0x02
#define I2C_DCBUS_ADDRESS         0x03
#define I2C_RESERVED_ADDRESS      0x04
#define I2C_HS_MODE_CODE          0x05
#define I2C_SET_SPEED             0x06

// Estados del bus I2C
typedef enum {
    I2C_STATE_IDLE,           // Bus libre
    I2C_STATE_BUSY,           // Bus ocupado
    I2C_STATE_ERROR,          // Error detectado
    I2C_STATE_TIMEOUT,        // Timeout ocurrido
    I2C_STATE_ADDR_NACK,      // ACK negativo en dirección
    I2C_STATE_DATA_NACK,      // ACK negativo en dato
    I2C_STATE_ARB_LOST,       // Pérdida de arbitraje
    I2C_STATE_BUS_COLLISION,  // Colisión en bus
    I2C_STATE_OVERRUN,        // Overrun de buffer
    I2C_STATE_SUCCESS         // Operación exitosa
} I2C_State_t;

// Eventos I2C (para callbacks)
typedef enum {
    I2C_EVENT_START,          // Condición START detectada
    I2C_EVENT_RESTART,        // Condición REPEATED START
    I2C_EVENT_STOP,           // Condición STOP detectada
    I2C_EVENT_ADDR_RECEIVED,  // Dirección recibida
    I2C_EVENT_DATA_RECEIVED,  // Dato recibido
    I2C_EVENT_DATA_REQUESTED, // Maestro solicita dato
    I2C_EVENT_ACK_SENT,       // ACK enviado
    I2C_EVENT_NACK_SENT,      // NACK enviado
    I2C_EVENT_ERROR           // Error detectado
} I2C_Event_t;

// Callback function type
typedef void (*I2C_Callback_t)(I2C_Event_t event, uint8_t data);

// Estructura de configuración
typedef struct {
    I2C_Module_t module;      // Módulo I2C a usar
    I2C_Mode_t mode;          // Modo de operación
    I2C_Speed_t speed;        // Velocidad del bus
    uint8_t slave_address;    // Dirección esclavo (7-bit)
    bool general_call_enable; // Habilitar general call
    bool slew_rate_control;   // Control de slew rate
    bool smbus_enable;        // Habilitar protocolo SMBus
    uint16_t timeout_ms;      // Timeout en milisegundos
    bool interrupt_enable;    // Habilitar interrupciones
    I2C_Callback_t callback;  // Función de callback
} I2C_Config_t;

// Configuración por defecto (maestro 100kHz)
#define I2C_CONFIG_DEFAULT_MASTER { \
    .module = I2C_MODULE_1, \
    .mode = I2C_MODE_MASTER, \
    .speed = I2C_SPEED_100KHZ, \
    .slave_address = 0x00, \
    .general_call_enable = false, \
    .slew_rate_control = true, \
    .smbus_enable = false, \
    .timeout_ms = 1000, \
    .interrupt_enable = false, \
    .callback = NULL \
}

// Configuración por defecto (esclavo)
#define I2C_CONFIG_DEFAULT_SLAVE { \
    .module = I2C_MODULE_1, \
    .mode = I2C_MODE_SLAVE_7BIT, \
    .speed = I2C_SPEED_100KHZ, \
    .slave_address = 0x40, \
    .general_call_enable = true, \
    .slew_rate_control = true, \
    .smbus_enable = false, \
    .timeout_ms = 1000, \
    .interrupt_enable = true, \
    .callback = NULL \
}

// =============================================================================
// PROTOTIPOS DE FUNCIONES
// =============================================================================

// Inicialización y configuración
void I2C_Init(I2C_Config_t *config);
void I2C_Deinit(I2C_Module_t module);
void I2C_Reset(I2C_Module_t module);
void I2C_Enable(I2C_Module_t module);
void I2C_Disable(I2C_Module_t module);
bool I2C_IsBusy(I2C_Module_t module);

// Control del bus
bool I2C_Start(I2C_Module_t module);
bool I2C_Restart(I2C_Module_t module);
bool I2C_Stop(I2C_Module_t module);
bool I2C_WaitIdle(I2C_Module_t module, uint16_t timeout_ms);

// Operaciones maestro
bool I2C_WriteByte(I2C_Module_t module, uint8_t data);
uint8_t I2C_ReadByte(I2C_Module_t module, bool ack);
bool I2C_SendAck(I2C_Module_t module);
bool I2C_SendNack(I2C_Module_t module);

// Transmisiones completas
bool I2C_WriteData(I2C_Module_t module, uint8_t address, uint8_t *data, uint8_t length);
bool I2C_ReadData(I2C_Module_t module, uint8_t address, uint8_t *buffer, uint8_t length);
bool I2C_WriteRegister(I2C_Module_t module, uint8_t dev_addr, uint8_t reg_addr, uint8_t data);
uint8_t I2C_ReadRegister(I2C_Module_t module, uint8_t dev_addr, uint8_t reg_addr);

// Funciones esclavo
void I2C_SetSlaveAddress(I2C_Module_t module, uint8_t address);
void I2C_EnableGeneralCall(I2C_Module_t module, bool enable);
uint8_t I2C_GetReceivedAddress(I2C_Module_t module);
bool I2C_DataReady(I2C_Module_t module);
uint8_t I2C_GetByte(I2C_Module_t module);
void I2C_PutByte(I2C_Module_t module, uint8_t data);

// Funciones avanzadas
bool I2C_ScanBus(I2C_Module_t module, uint8_t *devices, uint8_t max_devices);
bool I2C_CheckDevice(I2C_Module_t module, uint8_t address);
void I2C_SetTimeout(I2C_Module_t module, uint16_t timeout_ms);
I2C_State_t I2C_GetLastError(I2C_Module_t module);
void I2C_ClearErrors(I2C_Module_t module);

// Buffer y colas
bool I2C_WriteBuffer(I2C_Module_t module, uint8_t address, uint8_t *data, uint16_t length);
bool I2C_ReadBuffer(I2C_Module_t module, uint8_t address, uint8_t *buffer, uint16_t length);
uint16_t I2C_GetRxBufferCount(I2C_Module_t module);
uint16_t I2C_GetTxBufferCount(I2C_Module_t module);

// Interrupciones y callbacks
void I2C_SetCallback(I2C_Module_t module, I2C_Callback_t callback);
void I2C_EnableInterrupts(I2C_Module_t module, bool enable);
void I2C_ISR_Handler(I2C_Module_t module);

// Utilitarias
void I2C_PrintConfig(I2C_Module_t module);
void I2C_PrintStatus(I2C_Module_t module);
uint32_t I2C_CalculateBaudRate(uint32_t fcy, uint32_t desired_speed);
void I2C_DelayUs(uint16_t microseconds);

// =============================================================================
// VARIABLES GLOBALES
// =============================================================================
extern volatile I2C_State_t I2C1_State;
extern volatile I2C_State_t I2C2_State;
extern volatile bool I2C1_Busy;
extern volatile bool I2C2_Busy;
extern I2C_Config_t I2C1_Config;
extern I2C_Config_t I2C2_Config;

#endif /* I2C_H */