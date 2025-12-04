/*******************************************************************************
 * i2c.c - Implementación de la librería I2C para dsPIC33FJ32MC204
 * 
 * Implementación completa del módulo I2C con todas las funcionalidades
 * para operación como maestro y esclavo.
 * 
 ******************************************************************************/

#include "i2c.h"
#include <string.h>
#include <stdio.h>

// =============================================================================
// VARIABLES GLOBALES
// =============================================================================

volatile I2C_State_t I2C1_State = I2C_STATE_IDLE;
volatile I2C_State_t I2C2_State = I2C_STATE_IDLE;
volatile bool I2C1_Busy = false;
volatile bool I2C2_Busy = false;
I2C_Config_t I2C1_Config;
I2C_Config_t I2C2_Config;

// Buffers para transmisión/recepción
static uint8_t i2c1_rx_buffer[256];
static uint8_t i2c1_tx_buffer[256];
static uint8_t i2c2_rx_buffer[256];
static uint8_t i2c2_tx_buffer[256];

static uint16_t i2c1_rx_index = 0;
static uint16_t i2c1_tx_index = 0;
static uint16_t i2c2_rx_index = 0;
static uint16_t i2c2_tx_index = 0;

// Callbacks
static I2C_Callback_t i2c1_callback = NULL;
static I2C_Callback_t i2c2_callback = NULL;

// =============================================================================
// FUNCIONES PRIVADAS
// =============================================================================

/**
 * @brief Obtiene puntero al módulo I2C
 */
static volatile uint16_t* _I2C_GetModuleBase(I2C_Module_t module) {
    switch(module) {
        case I2C_MODULE_1: return (volatile uint16_t*)&I2C1CON;
        case I2C_MODULE_2: return (volatile uint16_t*)&I2C2CON;
        default: return (volatile uint16_t*)&I2C1CON;
    }
}

/**
 * @brief Obtiene estado actual del módulo
 */
static volatile I2C_State_t* _I2C_GetState(I2C_Module_t module) {
    switch(module) {
        case I2C_MODULE_1: return &I2C1_State;
        case I2C_MODULE_2: return &I2C2_State;
        default: return &I2C1_State;
    }
}

/**
 * @brief Obtiene flag de busy
 */
static volatile bool* _I2C_GetBusyFlag(I2C_Module_t module) {
    switch(module) {
        case I2C_MODULE_1: return &I2C1_Busy;
        case I2C_MODULE_2: return &I2C2_Busy;
        default: return &I2C1_Busy;
    }
}

/**
 * @brief Obtiene configuración
 */
static I2C_Config_t* _I2C_GetConfig(I2C_Module_t module) {
    switch(module) {
        case I2C_MODULE_1: return &I2C1_Config;
        case I2C_MODULE_2: return &I2C2_Config;
        default: return &I2C1_Config;
    }
}

/**
 * @brief Configura pines I2C
 */
static void _I2C_ConfigurePins(I2C_Module_t module) {
    switch(module) {
        case I2C_MODULE_1:
            // I2C1: SCL1 - RC3, SDA1 - RC4
            ANSELCbits.ANSC3 = 0;  // Digital
            ANSELCbits.ANSC4 = 0;  // Digital
            TRISCbits.TRISC3 = 1;  // Input
            TRISCbits.TRISC4 = 1;  // Input
            // Open-drain configuration
            ODCONCbits.ODCC3 = 1;  // Open-drain
            ODCONCbits.ODCC4 = 1;  // Open-drain
            break;
            
        case I2C_MODULE_2:
            // I2C2: SCL2 - RG2, SDA2 - RG3
            ANSELGbits.ANSG2 = 0;  // Digital
            ANSELGbits.ANSG3 = 0;  // Digital
            TRISGbits.TRISG2 = 1;  // Input
            TRISGbits.TRISG3 = 1;  // Input
            // Open-drain configuration
            ODCONGbits.ODCG2 = 1;  // Open-drain
            ODCONGbits.ODCG3 = 1;  // Open-drain
            break;
    }
}

/**
 * @brief Calcula baud rate register
 */
static uint16_t _I2C_CalculateBRG(uint32_t fcy, uint32_t desired_speed) {
    // Fórmula: BRG = (Fcy / (2 * Fscl)) - 2
    // Donde Fscl = desired_speed
    uint32_t brg = (fcy / (2 * desired_speed)) - 2;
    
    // Limitar a valor máximo de 16 bits
    if (brg > 0xFFFF) brg = 0xFFFF;
    if (brg < 2) brg = 2;  // Mínimo según datasheet
    
    return (uint16_t)brg;
}

/**
 * @brief Espera condición I2C
 */
static bool _I2C_WaitCondition(I2C_Module_t module, uint16_t timeout_ms) {
    volatile uint16_t* i2c_con = _I2C_GetModuleBase(module);
    uint32_t timeout_counter = timeout_ms * 1000;  // Aproximado
    
    while ((*i2c_con & 0x1F) != 0 && timeout_counter--) {
        // Verificar errores
        if (*i2c_con & (1 << 9)) {  // I2COV (Overflow)
            *_I2C_GetState(module) = I2C_STATE_OVERRUN;
            return false;
        }
        if (*i2c_con & (1 << 8)) {  // IWCOL (Write collision)
            *_I2C_GetState(module) = I2C_STATE_BUS_COLLISION;
            return false;
        }
    }
    
    if (timeout_counter == 0) {
        *_I2C_GetState(module) = I2C_STATE_TIMEOUT;
        return false;
    }
    
    return true;
}

// =============================================================================
// FUNCIONES PÚBLICAS
// =============================================================================

/**
 * @brief Inicializa el módulo I2C
 */
void I2C_Init(I2C_Config_t *config) {
    if (config == NULL) return;
    
    volatile I2C_Config_t* cfg = _I2C_GetConfig(config->module);
    memcpy((void*)cfg, config, sizeof(I2C_Config_t));
    
    // Configurar pines
    _I2C_ConfigurePins(config->module);
    
    // Obtener puntero a registros
    volatile uint16_t* i2c_con = _I2C_GetModuleBase(config->module);
    volatile uint16_t* i2c_stat = i2c_con + 1;  // I2CxSTAT
    volatile uint16_t* i2c_add = i2c_con + 2;   // I2CxADD
    volatile uint16_t* i2c_msk = i2c_con + 3;   // I2CxMSK
    volatile uint16_t* i2c_brg = i2c_con + 4;   // I2CxBRG
    
    // Deshabilitar módulo durante configuración
    *i2c_con = 0x0000;
    
    // Configurar velocidad (BRG)
    uint32_t fcy = 40000000;  // 40 MHz Fcy para dsPIC33FJ32MC204
    *i2c_brg = _I2C_CalculateBRG(fcy, config->speed);
    
    // Configurar según modo
    switch(config->mode) {
        case I2C_MODE_MASTER:
            // Modo maestro
            *i2c_con = 0x8000;  // I2CEN = 1
            break;
            
        case I2C_MODE_SLAVE_7BIT:
            // Esclavo 7-bit
            *i2c_add = config->slave_address << 1;  // Dirección en 7-bit
            *i2c_msk = 0x0000;  // Sin máscara
            *i2c_con = 0x8000;  // I2CEN = 1
            *i2c_con |= 0x4000; // Slave mode
            break;
            
        case I2C_MODE_SLAVE_10BIT:
            // Esclavo 10-bit
            *i2c_add = config->slave_address;  // Dirección en 10-bit
            *i2c_con = 0x8000;  // I2CEN = 1
            *i2c_con |= 0x4400; // Slave mode + 10-bit address
            break;
            
        default:
            break;
    }
    
    // Configurar SMBus si es necesario
    if (config->smbus_enable) {
        *i2c_con |= 0x0020;  // SMBEN = 1
    }
    
    // Configurar slew rate
    if (config->slew_rate_control) {
        *i2c_con |= 0x0040;  // DISSLW = 0 (slew rate enabled)
    } else {
        *i2c_con |= 0x0080;  // DISSLW = 1 (slew rate disabled)
    }
    
    // Configurar interrupciones
    if (config->interrupt_enable) {
        switch(config->module) {
            case I2C_MODULE_1:
                IFS1bits.I2C1BIF = 0;
                IEC1bits.I2C1BIE = 1;
                IPC7bits.I2C1BIP = 4;
                break;
            case I2C_MODULE_2:
                IFS3bits.I2C2BIF = 0;
                IEC3bits.I2C2BIE = 1;
                IPC14bits.I2C2BIP = 4;
                break;
        }
    }
    
    // Inicializar estado
    *_I2C_GetState(config->module) = I2C_STATE_IDLE;
    *_I2C_GetBusyFlag(config->module) = false;
    
    // Configurar callback
    I2C_SetCallback(config->module, config->callback);
}

/**
 * @brief Deshabilita el módulo I2C
 */
void I2C_Deinit(I2C_Module_t module) {
    volatile uint16_t* i2c_con = _I2C_GetModuleBase(module);
    *i2c_con = 0x0000;  // Deshabilitar módulo
    
    // Deshabilitar interrupciones
    switch(module) {
        case I2C_MODULE_1:
            IEC1bits.I2C1BIE = 0;
            break;
        case I2C_MODULE_2:
            IEC3bits.I2C2BIE = 0;
            break;
    }
    
    // Resetear estado
    *_I2C_GetState(module) = I2C_STATE_IDLE;
    *_I2C_GetBusyFlag(module) = false;
}

/**
 * @brief Genera condición START
 */
bool I2C_Start(I2C_Module_t module) {
    volatile uint16_t* i2c_con = _I2C_GetModuleBase(module);
    volatile bool* busy = _I2C_GetBusyFlag(module);
    
    if (*busy) return false;
    
    *busy = true;
    *_I2C_GetState(module) = I2C_STATE_BUSY;
    
    // Generar condición START
    *i2c_con |= 0x0001;  // SEN = 1
    
    // Esperar a que se complete
    if (!_I2C_WaitCondition(module, _I2C_GetConfig(module)->timeout_ms)) {
        *busy = false;
        return false;
    }
    
    return true;
}

/**
 * @brief Genera condición REPEATED START
 */
bool I2C_Restart(I2C_Module_t module) {
    volatile uint16_t* i2c_con = _I2C_GetModuleBase(module);
    
    // Generar REPEATED START
    *i2c_con |= 0x0002;  // RSEN = 1
    
    // Esperar a que se complete
    return _I2C_WaitCondition(module, _I2C_GetConfig(module)->timeout_ms);
}

/**
 * @brief Genera condición STOP
 */
bool I2C_Stop(I2C_Module_t module) {
    volatile uint16_t* i2c_con = _I2C_GetModuleBase(module);
    volatile bool* busy = _I2C_GetBusyFlag(module);
    
    // Generar condición STOP
    *i2c_con |= 0x0004;  // PEN = 1
    
    // Esperar a que se complete
    if (!_I2C_WaitCondition(module, _I2C_GetConfig(module)->timeout_ms)) {
        return false;
    }
    
    // Marcar como no ocupado
    *busy = false;
    *_I2C_GetState(module) = I2C_STATE_IDLE;
    
    return true;
}

/**
 * @brief Escribe un byte en el bus
 */
bool I2C_WriteByte(I2C_Module_t module, uint8_t data) {
    volatile uint16_t* i2c_con = _I2C_GetModuleBase(module);
    volatile uint16_t* i2c_trn = i2c_con + 5;  // I2CxTRN
    
    // Escribir dato en buffer de transmisión
    *i2c_trn = data;
    
    // Iniciar transmisión
    *i2c_con |= 0x0008;  // RCEN = 0, PEN = 0, SEN = 0, RSEN = 0, ACKEN = 0
    
    // Esperar a que se complete
    if (!_I2C_WaitCondition(module, _I2C_GetConfig(module)->timeout_ms)) {
        return false;
    }
    
    // Verificar ACK
    if (*i2c_con & (1 << 15)) {  // ACKSTAT = 1 (NACK recibido)
        *_I2C_GetState(module) = I2C_STATE_DATA_NACK;
        return false;
    }
    
    return true;
}

/**
 * @brief Lee un byte del bus
 */
uint8_t I2C_ReadByte(I2C_Module_t module, bool ack) {
    volatile uint16_t* i2c_con = _I2C_GetModuleBase(module);
    volatile uint16_t* i2c_rcv = i2c_con + 6;  // I2CxRCV
    
    // Configurar recepción
    if (ack) {
        *i2c_con &= ~(1 << 4);  // ACKDT = 0 (ACK)
    } else {
        *i2c_con |= (1 << 4);   // ACKDT = 1 (NACK)
    }
    
    // Iniciar recepción
    *i2c_con |= 0x0008;  // RCEN = 1
    
    // Esperar a que se complete
    _I2C_WaitCondition(module, _I2C_GetConfig(module)->timeout_ms);
    
    // Leer dato recibido
    return (uint8_t)(*i2c_rcv & 0x00FF);
}

/**
 * @brief Escribe datos a un dispositivo esclavo
 */
bool I2C_WriteData(I2C_Module_t module, uint8_t address, uint8_t *data, uint8_t length) {
    if (length == 0 || data == NULL) return false;
    
    // Generar START
    if (!I2C_Start(module)) return false;
    
    // Enviar dirección + bit de escritura
    if (!I2C_WriteByte(module, (address << 1) | 0x00)) {
        I2C_Stop(module);
        return false;
    }
    
    // Enviar datos
    for (uint8_t i = 0; i < length; i++) {
        if (!I2C_WriteByte(module, data[i])) {
            I2C_Stop(module);
            return false;
        }
    }
    
    // Generar STOP
    return I2C_Stop(module);
}

/**
 * @brief Lee datos de un dispositivo esclavo
 */
bool I2C_ReadData(I2C_Module_t module, uint8_t address, uint8_t *buffer, uint8_t length) {
    if (length == 0 || buffer == NULL) return false;
    
    // Generar START
    if (!I2C_Start(module)) return false;
    
    // Enviar dirección + bit de lectura
    if (!I2C_WriteByte(module, (address << 1) | 0x01)) {
        I2C_Stop(module);
        return false;
    }
    
    // Leer datos
    for (uint8_t i = 0; i < length; i++) {
        bool ack = (i < length - 1);  // ACK en todos menos el último
        buffer[i] = I2C_ReadByte(module, ack);
    }
    
    // Generar STOP
    return I2C_Stop(module);
}

/**
 * @brief Escribe a un registro específico
 */
bool I2C_WriteRegister(I2C_Module_t module, uint8_t dev_addr, uint8_t reg_addr, uint8_t data) {
    uint8_t buffer[2] = {reg_addr, data};
    return I2C_WriteData(module, dev_addr, buffer, 2);
}

/**
 * @brief Lee de un registro específico
 */
uint8_t I2C_ReadRegister(I2C_Module_t module, uint8_t dev_addr, uint8_t reg_addr) {
    uint8_t value = 0;
    
    // Primero escribir la dirección del registro
    if (!I2C_WriteData(module, dev_addr, &reg_addr, 1)) {
        return 0;
    }
    
    // Luego leer el valor
    I2C_ReadData(module, dev_addr, &value, 1);
    
    return value;
}

/**
 * @brief Escanea el bus I2C buscando dispositivos
 */
bool I2C_ScanBus(I2C_Module_t module, uint8_t *devices, uint8_t max_devices) {
    uint8_t found = 0;
    
    for (uint8_t addr = 1; addr < 127; addr++) {
        if (I2C_CheckDevice(module, addr)) {
            if (devices && found < max_devices) {
                devices[found] = addr;
            }
            found++;
        }
    }
    
    return (found > 0);
}

/**
 * @brief Verifica si un dispositivo responde
 */
bool I2C_CheckDevice(I2C_Module_t module, uint8_t address) {
    // Generar START
    if (!I2C_Start(module)) return false;
    
    // Intentar enviar dirección
    bool success = I2C_WriteByte(module, (address << 1) | 0x00);
    
    // Generar STOP
    I2C_Stop(module);
    
    return success;
}

/**
 * @brief Configura callback para interrupciones
 */
void I2C_SetCallback(I2C_Module_t module, I2C_Callback_t callback) {
    switch(module) {
        case I2C_MODULE_1:
            i2c1_callback = callback;
            break;
        case I2C_MODULE_2:
            i2c2_callback = callback;
            break;
    }
}

/**
 * @brief Handler de interrupción I2C
 */
void I2C_ISR_Handler(I2C_Module_t module) {
    volatile uint16_t* i2c_con = _I2C_GetModuleBase(module);
    volatile uint16_t* i2c_stat = i2c_con + 1;
    I2C_Callback_t callback = NULL;
    
    // Obtener callback
    switch(module) {
        case I2C_MODULE_1:
            callback = i2c1_callback;
            IFS1bits.I2C1BIF = 0;  // Limpiar flag
            break;
        case I2C_MODULE_2:
            callback = i2c2_callback;
            IFS3bits.I2C2BIF = 0;  // Limpiar flag
            break;
    }
    
    if (callback == NULL) return;
    
    // Determinar evento
    if (*i2c_stat & (1 << 15)) {  // Start bit detectado
        callback(I2C_EVENT_START, 0);
    }
    else if (*i2c_stat & (1 << 14)) {  // Stop bit detectado
        callback(I2C_EVENT_STOP, 0);
    }
    else if (*i2c_stat & (1 << 13)) {  // Datos recibidos
        volatile uint16_t* i2c_rcv = i2c_con + 6;
        uint8_t data = (uint8_t)(*i2c_rcv & 0x00FF);
        callback(I2C_EVENT_DATA_RECEIVED, data);
    }
    else if (*i2c_stat & (1 << 12)) {  // Solicitud de datos
        callback(I2C_EVENT_DATA_REQUESTED, 0);
    }
}

/**
 * @brief Imprime configuración actual
 */
void I2C_PrintConfig(I2C_Module_t module) {
    I2C_Config_t* cfg = _I2C_GetConfig(module);
    
    printf("\n=== Configuración I2C%d ===\n", module);
    printf("Modo: %s\n", cfg->mode == I2C_MODE_MASTER ? "Maestro" : 
                         cfg->mode == I2C_MODE_SLAVE_7BIT ? "Esclavo 7-bit" : 
                         "Esclavo 10-bit");
    printf("Velocidad: %lu Hz\n", cfg->speed);
    printf("Dirección esclavo: 0x%02X\n", cfg->slave_address);
    printf("Timeout: %d ms\n", cfg->timeout_ms);
    printf("General Call: %s\n", cfg->general_call_enable ? "Habilitado" : "Deshabilitado");
    printf("SMBus: %s\n", cfg->smbus_enable ? "Habilitado" : "Deshabilitado");
    printf("Interrupciones: %s\n", cfg->interrupt_enable ? "Habilitadas" : "Deshabilitadas");
    printf("==========================\n");
}

/**
 * @brief Espera a que el bus esté libre
 */
bool I2C_WaitIdle(I2C_Module_t module, uint16_t timeout_ms) {
    volatile uint16_t* i2c_stat = _I2C_GetModuleBase(module) + 1;
    uint32_t timeout_counter = timeout_ms * 1000;
    
    while ((*i2c_stat & (1 << 14)) && timeout_counter--) {
        // Esperar activamente
    }
    
    return (timeout_counter > 0);
}

/**
 * @brief Obtiene el último error
 */
I2C_State_t I2C_GetLastError(I2C_Module_t module) {
    return *_I2C_GetState(module);
}

/**
 * @brief Limpia errores
 */
void I2C_ClearErrors(I2C_Module_t module) {
    *_I2C_GetState(module) = I2C_STATE_IDLE;
    
    // Limpiar flags de error en el módulo
    volatile uint16_t* i2c_con = _I2C_GetModuleBase(module);
    *i2c_con &= ~((1 << 9) | (1 << 8));  // Limpiar I2COV e IWCOL
}

/**
 * @brief Verifica si el bus está ocupado
 */
bool I2C_IsBusy(I2C_Module_t module) {
    return *_I2C_GetBusyFlag(module);
}