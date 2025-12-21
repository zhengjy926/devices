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

/* Memory barrier support */
/* Use CMSIS definitions if available, otherwise define inline assembly */
#ifndef __DSB
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || \
    defined(__ARM_ARCH_8M_MAIN__) || defined(__ARM_ARCH_8M_BASE__)
/* Try to use CMSIS compiler header which defines these macros */
#if defined(__GNUC__) || defined(__clang__)
#include "cmsis_gcc.h"
#elif defined(__ICCARM__)
#include "cmsis_iccarm.h"
#elif defined(__ARMCC_VERSION)
#include "cmsis_armclang.h"
#else
/* Fallback: define inline assembly */
#define __DSB() __asm volatile("dsb" ::: "memory")
#define __DMB() __asm volatile("dmb" ::: "memory")
#define __ISB() __asm volatile("isb" ::: "memory")
#endif
#else
/* Fallback: define inline assembly if CMSIS not available */
#define __DSB() __asm volatile("dsb" ::: "memory")
#define __DMB() __asm volatile("dmb" ::: "memory")
#define __ISB() __asm volatile("isb" ::: "memory")
#endif
#endif

/**
 * @defgroup I2C_Flag_Definitions I2C Flag Definitions
 * @{
 */
#define I2C_M_RD            (1U<<0)     /**< Read data, from slave to master */
#define I2C_M_TEN           (1U<<5)      /**< Ten bit chip address */
#define I2C_M_STOP         (1U<<1)     /**< Send stop after this message */
#define I2C_M_NOSTART      (1U<<2)     /**< Don't send a start bit */
#define I2C_M_REV_DIR_ADDR (1U<<3)     /**< Reverse direction of address */
#define I2C_M_IGNORE_NAK   (1U<<4)     /**< Ignore NAK from slave */
#define I2C_M_NO_RD_ACK    (1U<<6)     /**< Skip ACK on reads */
#define I2C_M_RECV_LEN     (1U<<7)     /**< Length will be first received byte */

#define I2C_NAME_MAX        (16U)      /**< Maximum length of I2C adapter name */
/** @} */

/**
 * @defgroup I2C_Functionality_Flags I2C Functionality Flags
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
#define I2C_FUNC_SMBUS_WRITE_BYTE      (1U<<14) /**< SMBus write byte */
#define I2C_FUNC_SMBUS_READ_BYTE_DATA  (1U<<15) /**< SMBus read byte data */
#define I2C_FUNC_SMBUS_WRITE_BYTE_DATA (1U<<16) /**< SMBus write byte data */
#define I2C_FUNC_SMBUS_READ_WORD_DATA  (1U<<17) /**< SMBus read word data */
#define I2C_FUNC_SMBUS_WRITE_WORD_DATA (1U<<18) /**< SMBus write word data */
#define I2C_FUNC_SMBUS_PROC_CALL       (1U<<19) /**< SMBus process call */
#define I2C_FUNC_SMBUS_READ_BLOCK_DATA (1U<<20) /**< SMBus read block data */
#define I2C_FUNC_SMBUS_WRITE_BLOCK_DATA (1U<<21) /**< SMBus write block data */
#define I2C_FUNC_SMBUS_READ_I2C_BLOCK  (1U<<22) /**< SMBus read I2C block */
#define I2C_FUNC_SMBUS_WRITE_I2C_BLOCK (1U<<23) /**< SMBus write I2C block */
#define I2C_FUNC_DMA_SUPPORT           (1U<<24) /**< DMA transfer support */
/** @} */

/* Forward declarations */
struct i2c_msg;
struct i2c_client;
struct i2c_adapter;
struct i2c_algorithm;

/**
 * @brief I2C Message Structure
 * @details Single message descriptor for I2C transfer
 */
struct i2c_msg {
    uint16_t addr;                      /**< Slave address (7-bit or 10-bit) */
    uint16_t flags;                     /**< Message flags (I2C_M_RD, I2C_M_TEN, etc.) */
    uint16_t len;                       /**< Message length in bytes */
    uint8_t *buf;                       /**< Pointer to message data buffer */
    struct list_node msg_list;          /**< Message list node (for multi-message transfers) */
};

/**
 * @brief I2C Client Structure
 * @details Represents an I2C device connected to an adapter
 */
struct i2c_client {
    const char *name;                   /**< Device name */
    struct i2c_adapter *adapter;        /**< Parent adapter */
    uint16_t addr;                      /**< Device address */
    uint16_t flags;                     /**< Device flags */
    uint32_t timeout_ms;                /**< Timeout in milliseconds */
    void *driver_data;                  /**< Driver private data */
};

/**
 * @brief I2C Algorithm Structure
 * @details Hardware-specific operations that must be implemented by BSP layer
 */
struct i2c_algorithm {
    /**
     * @brief Execute I2C transfer (interrupt/polling mode)
     * @param adap Adapter pointer
     * @param msgs Array of messages to transfer
     * @param num Number of messages
     * @return Number of messages transferred on success, error code on failure
     * @note This function may be called from interrupt context, must be reentrant
     */
    int (*master_xfer)(struct i2c_adapter *adap, 
                       struct i2c_msg *msgs, 
                       int num);
    
    /**
     * @brief Execute I2C transfer using DMA (optional)
     * @param adap Adapter pointer
     * @param msgs Array of messages to transfer
     * @param num Number of messages
     * @return Number of messages transferred on success, error code on failure
     * @note This function may be called from interrupt context, must be reentrant
     * @note If NULL, DMA is not supported
     */
    int (*master_xfer_dma)(struct i2c_adapter *adap,
                          struct i2c_msg *msgs,
                          int num);
    
    /**
     * @brief Functionality flags
     * @details Bitmask of I2C_FUNC_* flags indicating supported features
     */
    uint32_t functionality;
};

/**
 * @brief I2C Adapter Structure
 * @details Adapter abstraction, manages configuration and thread safety
 */
struct i2c_adapter {
    struct list_node node;              /**< List node */
    char name[I2C_NAME_MAX];            /**< Adapter name */
    const struct i2c_algorithm *algo;   /**< Algorithm functions */
    void *algo_data;                    /**< Algorithm private data */
    
    /* Configuration cache (volatile to prevent optimization issues) */
    volatile uint32_t speed_hz;         /**< Bus speed in Hz */
    volatile uint32_t timeout_ms;       /**< Timeout in milliseconds */
    volatile uint8_t addr_width;        /**< Address width: 7 or 10 */
    
    /* Thread safety support (optional) */
    void (*lock)(void *lock_data);      /**< Lock function (RTOS environment) */
    void (*unlock)(void *lock_data);    /**< Unlock function (RTOS environment) */
    void *lock_data;                    /**< Lock data (e.g., mutex handle) */
    
    /* Interrupt control support (optional) */
    void (*irq_disable)(void);          /**< Disable interrupts (bare-metal environment) */
    void (*irq_enable)(void);           /**< Enable interrupts (bare-metal environment) */
    
    /* DMA support flags */
    uint8_t dma_supported : 1;           /**< DMA is supported by hardware */
    uint8_t dma_enabled : 1;             /**< DMA is currently enabled */
    
    /* Status flags */
    volatile uint8_t in_use : 1;        /**< Adapter is currently in use */
};

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Register I2C adapter
 * @param adap Adapter structure pointer
 * @param name Adapter name
 * @param algo Algorithm functions pointer
 * @return 0 on success, error code on failure
 */
int i2c_add_adapter(struct i2c_adapter *adap, 
                    const char *name, 
                    const struct i2c_algorithm *algo);

/**
 * @brief Unregister I2C adapter
 * @param adap Adapter structure pointer
 * @return 0 on success, error code on failure
 */
int i2c_del_adapter(struct i2c_adapter *adap);

/**
 * @brief Find I2C adapter by name
 * @param name Adapter name
 * @return Adapter pointer on success, NULL on failure
 */
struct i2c_adapter *i2c_find_adapter(const char *name);

/**
 * @brief Create new I2C client device
 * @param name Device name
 * @param adapter_name Adapter name to attach to
 * @param addr Device address
 * @param flags Device flags
 * @return Client pointer on success, NULL on failure
 */
struct i2c_client *i2c_new_client(const char *name,
                                  const char *adapter_name,
                                  uint16_t addr,
                                  uint16_t flags);

/**
 * @brief Delete I2C client device
 * @param client Client pointer
 * @return 0 on success, error code on failure
 */
int i2c_del_client(struct i2c_client *client);

/**
 * @brief Synchronous I2C message transfer
 * @param client Client pointer
 * @param msgs Array of messages to transfer
 * @param num Number of messages
 * @return Number of messages transferred on success, error code on failure
 */
int i2c_transfer(struct i2c_client *client, 
                 struct i2c_msg *msgs, 
                 int num);

/**
 * @brief Send data to I2C device (simplified interface)
 * @param client Client pointer
 * @param buf Data buffer to send
 * @param count Number of bytes to send
 * @return Number of bytes sent on success, error code on failure
 */
static inline int
i2c_master_send(struct i2c_client *client, const void *buf, size_t count)
{
    struct i2c_msg msg;
    int ret;
    
    if ((client == NULL) || (buf == NULL) || (count == 0U)) {
        return -EINVAL;
    }
    
    msg.addr = client->addr;
    msg.flags = client->flags;
    msg.len = (uint16_t)count;
    msg.buf = (uint8_t *)buf;
    list_node_init(&msg.msg_list);
    
    ret = i2c_transfer(client, &msg, 1);
    
    if (ret < 0) {
        return ret;
    }
    
    return (int)count;
}

/**
 * @brief Receive data from I2C device (simplified interface)
 * @param client Client pointer
 * @param buf Buffer to store received data
 * @param count Number of bytes to receive
 * @return Number of bytes received on success, error code on failure
 */
static inline int
i2c_master_recv(struct i2c_client *client, void *buf, size_t count)
{
    struct i2c_msg msg;
    int ret;
    
    if ((client == NULL) || (buf == NULL) || (count == 0U)) {
        return -EINVAL;
    }
    
    msg.addr = client->addr;
    msg.flags = client->flags | I2C_M_RD;
    msg.len = (uint16_t)count;
    msg.buf = (uint8_t *)buf;
    list_node_init(&msg.msg_list);
    
    ret = i2c_transfer(client, &msg, 1);
    
    if (ret < 0) {
        return ret;
    }
    
    return (int)count;
}

/**
 * @brief Write then read operation
 * @param client Client pointer
 * @param write_buf Buffer containing data to write
 * @param write_len Length of data to write
 * @param read_buf Buffer to store read data
 * @param read_len Length of data to read
 * @return 0 on success, error code on failure
 */
static inline int
i2c_write_then_read(struct i2c_client *client,
                    const void *write_buf, size_t write_len,
                    void *read_buf, size_t read_len)
{
    struct i2c_msg msgs[2];
    int ret;
    
    if ((client == NULL) || (write_buf == NULL) || (read_buf == NULL) ||
        (write_len == 0U) || (read_len == 0U)) {
        return -EINVAL;
    }
    
    /* Write message */
    msgs[0].addr = client->addr;
    msgs[0].flags = client->flags;
    msgs[0].len = (uint16_t)write_len;
    msgs[0].buf = (uint8_t *)write_buf;
    list_node_init(&msgs[0].msg_list);
    
    /* Read message */
    msgs[1].addr = client->addr;
    msgs[1].flags = client->flags | I2C_M_RD;
    msgs[1].len = (uint16_t)read_len;
    msgs[1].buf = (uint8_t *)read_buf;
    list_node_init(&msgs[1].msg_list);
    
    ret = i2c_transfer(client, msgs, 2);
    
    return ret;
}

/**
 * @brief Write one byte then read one byte
 * @param client Client pointer
 * @param cmd Command byte to write
 * @param result Pointer to store read byte
 * @return 0 on success, error code on failure
 */
static inline int
i2c_w8r8(struct i2c_client *client, uint8_t cmd, uint8_t *result)
{
    int ret;
    uint8_t temp_result;
    
    if ((client == NULL) || (result == NULL)) {
        return -EINVAL;
    }
    
    ret = i2c_write_then_read(client, &cmd, 1U, &temp_result, 1U);
    
    if (ret < 0) {
        return ret;
    }
    
    *result = temp_result;
    return 0;
}

/**
 * @brief Write one byte then read two bytes
 * @param client Client pointer
 * @param cmd Command byte to write
 * @param result Pointer to store 16-bit result
 * @return 0 on success, error code on failure
 */
static inline int
i2c_w8r16(struct i2c_client *client, uint8_t cmd, uint16_t *result)
{
    int ret;
    uint16_t temp_result;
    
    if ((client == NULL) || (result == NULL)) {
        return -EINVAL;
    }
    
    ret = i2c_write_then_read(client, &cmd, 1U, &temp_result, 2U);
    
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
