/**
 ******************************************************************************
 * @file        : i2c.h
 * @author      : ZJY
 * @version     : V1.0
 * @date        : 2025-01-XX
 * @brief       : I2C Driver Framework Header File (Linux Kernel Style)
 * @attention   : None
 ******************************************************************************
 * @history     :
 *         V1.0 : 1. Complete I2C driver framework with Linux kernel style
 *                2. Thread-safe design for bare-metal and RTOS
 *                3. Compiler optimization compatible
 *                4. Support 7-bit and 10-bit addressing
 *                5. Support DMA transfer
 *
 ******************************************************************************
 */
#ifndef __I2C_H__
#define __I2C_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "my_list.h"
#include "errno-base.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "cmsis_compiler.h"

/* Exported types ------------------------------------------------------------*/
#define I2C_NAME_MAX                    (16U)      /**< Maximum length of I2C adapter name */

/* Forward declarations */
struct i2c_msg;
struct i2c_adapter;
struct i2c_algorithm;
struct i2c_bus_recovery_info;

/**
 * @brief I2C Adapter Structure
 * @details Adapter abstraction, manages configuration and thread safety
 */
struct i2c_adapter {
    struct list_node node;              /**< List node */
    char name[I2C_NAME_MAX];            /**< Adapter name */
    const struct i2c_algorithm *algo;   /**< Algorithm functions */
    void *algo_data;                    /**< Algorithm private data */
    
    /* Configuration cache */
    uint32_t speed_hz;         /**< Bus speed in Hz */
    uint32_t timeout_ms;       /**< Timeout in milliseconds */
    uint8_t addr_width;        /**< Address width: 7 or 10 */
    uint8_t retries;           /**< 传输失败的重试次数 */
    
    /* Status flags */
    volatile uint8_t in_use : 1;        /**< Adapter is currently in use */
    struct i2c_bus_recovery_info *bus_recovery_info;
};

/**
 * @brief I2C Message Structure
 * @details Single message descriptor for I2C transfer
 */
struct i2c_msg {
    uint16_t addr;  /**< Slave address (7-bit or 10-bit) */
    uint16_t flags; /**< Message flags (I2C_M_RD, I2C_M_TEN, etc.) */
    uint16_t len;   /**< Message length in bytes */
    uint8_t *buf;   /**< Pointer to message data buffer */
};

/**
 * @brief I2C Algorithm Structure
 * @details Hardware-specific operations that must be implemented by BSP layer
 */
struct i2c_algorithm {
    /* 主模式 I2C 传输 */
    int (*xfer)(struct i2c_adapter *adap, struct i2c_msg *msgs, uint32_t num);
};

struct i2c_bus_recovery_info {
    uint32_t scl_pin_id;
    uint32_t sda_pin_id;
    void (*prepare_recovery)(struct i2c_adapter *adap);
    void (*unprepare_recovery)(struct i2c_adapter *adap);
};

/* Exported constants --------------------------------------------------------*/

/**
 * @defgroup I2C Frequency Modes
 * @{
 */
#define I2C_MAX_STANDARD_MODE_FREQ	    100000
#define I2C_MAX_FAST_MODE_FREQ		    400000
#define I2C_MAX_FAST_MODE_PLUS_FREQ	    1000000
#define I2C_MAX_TURBO_MODE_FREQ		    1400000
#define I2C_MAX_HIGH_SPEED_MODE_FREQ	3400000
#define I2C_MAX_ULTRA_FAST_MODE_FREQ	5000000
/** @} */

/**
 * @defgroup I2C Messege Flag Definitions
 * @{
 */
#define I2C_M_RD            (1U<<0) /**< Read data, from slave to master. If
                                      *  not set, the transaction is interpreted
                                      *  as write.
                                      */
#define I2C_M_TEN           (1U<<1)     /**< Ten bit of slave chip address */
#define I2C_M_NO_RD_ACK     (1U<<2)          /**< In a read message, master ACK/NACK bit is skipped */
#define I2C_M_IGNORE_NAK    (1U<<3)          /**< Treat NACK from client as ACK */
#define I2C_M_NOSTART       (1U<<4)     /**< Skip repeated start sequence */
#define I2C_M_STOP          (1U<<5)     /**< Force a STOP condition after the message */
/** @} */

/**
 * @defgroup The supported function bits of the I2C adapter
 * @{
 */
#define I2C_FUNC_I2C                    (1U<<0)  /**< Plain I2C */
#define I2C_FUNC_10BIT_ADDR             (1U<<1)  /**< 10-bit addresses */
#define I2C_FUNC_PROTOCOL_MANGLING      (1U<<4)  /**< I2C_M_IGNORE_NAK etc. */
#define I2C_FUNC_SMBUS_PEC              (1U<<8)  /**< SMBus PEC */
#define I2C_FUNC_NOSTART                (1U<<9)  /**< I2C_M_NOSTART */
#define I2C_FUNC_SLAVE                  (1U<<10) /**< I2C slave mode */
#define I2C_FUNC_SMBUS_BLOCK_PROC_CALL  (1U<<11) /**< SMBus block process call */
#define I2C_FUNC_SMBUS_QUICK            (1U<<12) /**< SMBus quick command */
#define I2C_FUNC_SMBUS_READ_BYTE        (1U<<13) /**< SMBus read byte */
#define I2C_FUNC_SMBUS_WRITE_BYTE       (1U<<14) /**< SMBus write byte */
#define I2C_FUNC_SMBUS_READ_BYTE_DATA   (1U<<15) /**< SMBus read byte data */
#define I2C_FUNC_SMBUS_WRITE_BYTE_DATA  (1U<<16) /**< SMBus write byte data */
#define I2C_FUNC_SMBUS_READ_WORD_DATA   (1U<<17) /**< SMBus read word data */
#define I2C_FUNC_SMBUS_WRITE_WORD_DATA  (1U<<18) /**< SMBus write word data */
#define I2C_FUNC_SMBUS_PROC_CALL        (1U<<19) /**< SMBus process call */
#define I2C_FUNC_SMBUS_READ_BLOCK_DATA  (1U<<20) /**< SMBus read block data */
#define I2C_FUNC_SMBUS_WRITE_BLOCK_DATA (1U<<21) /**< SMBus write block data */
#define I2C_FUNC_SMBUS_READ_I2C_BLOCK   (1U<<22) /**< SMBus read I2C block */
#define I2C_FUNC_SMBUS_WRITE_I2C_BLOCK  (1U<<23) /**< SMBus write I2C block */
#define I2C_FUNC_DMA_SUPPORT            (1U<<24) /**< DMA transfer support */
/** @} */

/* Exported macros -----------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

int i2c_add_adapter(struct i2c_adapter *adap, 
                    const char *name, 
                    const struct i2c_algorithm *algo);

int i2c_del_adapter(struct i2c_adapter *adap);

struct i2c_adapter *i2c_find_adapter(const char *name);

int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, uint32_t num);

/**
 * @brief Send data to I2C device (simplified interface)
 * @param adap Adapter pointer
 * @param addr Device address
 * @param flags Device flags
 * @param buf Data buffer to send
 * @param count Number of bytes to send
 * @return Number of bytes sent on success, error code on failure
 */
static inline int
i2c_master_send(struct i2c_adapter *adap, uint16_t addr, uint16_t flags,
                const void *buf, size_t count)
{
    struct i2c_msg msg;
    int ret;
    
    if ((adap == NULL) || (buf == NULL) || (count == 0U)) {
        return -EINVAL;
    }
    
    msg.addr = addr;
    msg.flags = flags;
    msg.len = (uint16_t)count;
    msg.buf = (uint8_t *)buf;
    
    ret = i2c_transfer(adap, &msg, 1);
    
    if (ret < 0) {
        return ret;
    }
    
    return (int)count;
}

/**
 * @brief Receive data from I2C device (simplified interface)
 * @param adap Adapter pointer
 * @param addr Device address
 * @param flags Device flags
 * @param buf Buffer to store received data
 * @param count Number of bytes to receive
 * @return Number of bytes received on success, error code on failure
 */
static inline int
i2c_master_recv(struct i2c_adapter *adap, uint16_t addr, uint16_t flags,
                void *buf, size_t count)
{
    struct i2c_msg msg;
    int ret;
    
    if ((adap == NULL) || (buf == NULL) || (count == 0U)) {
        return -EINVAL;
    }
    
    msg.addr = addr;
    msg.flags = flags | I2C_M_RD;
    msg.len = (uint16_t)count;
    msg.buf = (uint8_t *)buf;
    
    ret = i2c_transfer(adap, &msg, 1);
    
    if (ret < 0) {
        return ret;
    }
    
    return (int)count;
}

/**
 * @brief Write then read operation
 * @param adap Adapter pointer
 * @param addr Device address
 * @param flags Device flags
 * @param write_buf Buffer containing data to write
 * @param write_len Length of data to write
 * @param read_buf Buffer to store read data
 * @param read_len Length of data to read
 * @return 0 on success, error code on failure
 */
static inline int
i2c_write_then_read(struct i2c_adapter *adap, uint16_t addr, uint16_t flags,
                    const void *write_buf, size_t write_len,
                    void *read_buf, size_t read_len)
{
    struct i2c_msg msgs[2];
    int ret;
    
    if ((adap == NULL) || (write_buf == NULL) || (read_buf == NULL) ||
        (write_len == 0U) || (read_len == 0U)) {
        return -EINVAL;
    }
    
    /* Write message */
    msgs[0].addr = addr;
    msgs[0].flags = flags;
    msgs[0].len = (uint16_t)write_len;
    msgs[0].buf = (uint8_t *)write_buf;
    
    /* Read message */
    msgs[1].addr = addr;
    msgs[1].flags = flags | I2C_M_RD;
    msgs[1].len = (uint16_t)read_len;
    msgs[1].buf = (uint8_t *)read_buf;
    
    ret = i2c_transfer(adap, msgs, 2);
    
    if (ret < 0) {
        return ret;
    }
    
    return 0;
}

/**
 * @brief Write one byte then read one byte
 * @param adap Adapter pointer
 * @param addr Device address
 * @param flags Device flags
 * @param cmd Command byte to write
 * @param result Pointer to store read byte
 * @return 0 on success, error code on failure
 */
static inline int
i2c_w8r8(struct i2c_adapter *adap, uint16_t addr, uint16_t flags,
         uint8_t cmd, uint8_t *result)
{
    int ret;
    uint8_t temp_result;
    
    if ((adap == NULL) || (result == NULL)) {
        return -EINVAL;
    }
    
    ret = i2c_write_then_read(adap, addr, flags, &cmd, 1U, &temp_result, 1U);
    
    if (ret < 0) {
        return ret;
    }
    
    *result = temp_result;
    return 0;
}

/**
 * @brief Write one byte then read two bytes
 * @param adap Adapter pointer
 * @param addr Device address
 * @param flags Device flags
 * @param cmd Command byte to write
 * @param result Pointer to store 16-bit result
 * @return 0 on success, error code on failure
 */
static inline int
i2c_w8r16(struct i2c_adapter *adap, uint16_t addr, uint16_t flags,
          uint8_t cmd, uint16_t *result)
{
    int ret;
    uint16_t temp_result;
    
    if ((adap == NULL) || (result == NULL)) {
        return -EINVAL;
    }
    
    ret = i2c_write_then_read(adap, addr, flags, &cmd, 1U, &temp_result, 2U);
    
    if (ret < 0) {
        return ret;
    }
    
    *result = temp_result;
    return 0;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __I2C_H__ */
