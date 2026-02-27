/**
 ******************************************************************************
 * @file        : i2c.h
 * @author      : ZJY
 * @version     : V1.0
 * @date        : 2025-02-25
 * @brief       : I2C Driver Framework Header File (Linux Kernel Style)
 * @attention   : None
 ******************************************************************************
 * @history     :
 *         V1.0 : 1. Complete I2C driver framework
 *                2. Thread-safe design for bare-metal and RTOS
 *                3. Compiler optimization compatible
 *                4. Support 7-bit and 10-bit addressing
 *
 ******************************************************************************
 */
#ifndef __I2C_H__
#define __I2C_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stddef.h>
#include <stdint.h>

#include "my_list.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief I2C Message Structure
 * @details Single message descriptor for I2C transfer
 */
typedef struct i2c_msg {
    uint16_t addr;  /**< Slave address (7-bit or 10-bit) */
    uint16_t flags; /**< Message flags (I2C_M_RD, I2C_M_TEN, etc.) */
    uint16_t len;   /**< Message length in bytes */
    uint8_t *buf;   /**< Pointer to message data buffer */
}i2c_msg_t;

/**
 * @brief I2C client descriptor bound to a specific bus (adapter) and 7/10-bit address.
 */
typedef struct i2c_client {
    uint16_t flags;                 /**< flags (I2C_CLIENT_TEN, I2C_CLIENT_SLAVE, etc.) */
    uint16_t addr;                  /**< 7-bit or 10-bit address used on the I2C bus */
    char name[16];                  /**< client name */
    struct i2c_adapter *adapter;    /**< manages the bus segment hosting this I2C device */
}i2c_client_t;

/**
 * @brief I2C bus recovery information: SDA/SCL pins and callbacks before/after recovery.
 */
struct i2c_bus_recovery_info {
    /*
     * This will be called before starting recovery. Platform may configure
     * padmux here for SDA/SCL line or something else they want.
     */
	void (*prepare_recovery)(struct i2c_adapter *adap);
    
    /*
     * This will be called after completing recovery. Platform may configure
     * padmux here for SDA/SCL line or something else they want.
     */
	void (*unprepare_recovery)(struct i2c_adapter *adap);
    
	uint32_t scl_pin_id;    /**< gpio pin id of the SCL line. */
	uint32_t sda_pin_id;    /**< gpio pin id of the SDA line. */
};

/**
 * @brief I2C adapter algorithm: master transfer operations and capability query.
 */
typedef struct i2c_algo {
    int (*master_xfer)(struct i2c_adapter *adap, struct i2c_msg *msgs, uint16_t num);
    int (*master_xfer_atomic)(struct i2c_adapter *adap, struct i2c_msg *msgs, uint16_t num);
	/* To determine what the adapter supports */
	uint32_t (*functionality)(struct i2c_adapter *adap);
}i2c_algo_t;

/**
 * @brief I2C adapter representing one I2C bus, including name, algorithm, timeout, retry and recovery info.
 */
typedef struct i2c_adapter {
    char name[16];                                  /**< Adapter name, used by i2c_find_adapter */
    const struct i2c_algo *algo;                    /**< Transfer algorithm (master_xfer, etc.) */
    struct i2c_bus_recovery_info *bus_recovery_info;/**< Bus recovery info: SDA/SCL pins and callbacks */
    uint32_t timeout;                               /**< Transfer timeout in milliseconds; 0 means use default */
    uint8_t retries;                                /**< Number of retries on arbitration loss or similar errors */
    list_t node;                                    /**< List node to link into global adapter list */
    void *hw_data;                                  /**< Optional low-level hardware private data */
}i2c_adapter_t;

/* Exported constants --------------------------------------------------------*/

/**
 * @defgroup I2C Frequency Modes
 * @{
 */
#define I2C_MAX_STANDARD_MODE_FREQ          (100000)
#define I2C_MAX_FAST_MODE_FREQ		        (400000)
#define I2C_MAX_FAST_MODE_PLUS_FREQ	        (1000000)
#define I2C_MAX_TURBO_MODE_FREQ		        (1400000)
#define I2C_MAX_HIGH_SPEED_MODE_FREQ	    (3400000)
#define I2C_MAX_ULTRA_FAST_MODE_FREQ	    (5000000)
/** @} */

/**
 * @defgroup I2C Message Flag Definitions
 * @{
 */
#define I2C_M_RD            (1U<<0) 
#define I2C_M_TEN           (1U<<1)
/** @} */

/**
 * @defgroup I2C Client Flag Definitions
 * @{
 */
#define I2C_CLIENT_TEN      (1U<<0)     /**< 10-bit addressing */
#define I2C_CLIENT_SLAVE    (1U<<1)     /**< slave mode */
/** @} */

/* Exported macros -----------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

int i2c_register_adapter(i2c_adapter_t *adap);
i2c_adapter_t* i2c_find_adapter(const char *name);

int i2c_transfer(i2c_adapter_t *adap, i2c_msg_t *msgs, uint16_t num);
int i2c_transfer_buffer_flags(const struct i2c_client *client,
                              uint8_t *buf, uint16_t count, uint16_t flags);

static inline int i2c_master_recv(const struct i2c_client *client,
                                  uint8_t *buf, uint16_t count)
{
	return i2c_transfer_buffer_flags(client, buf, count, I2C_M_RD);
};

static inline int i2c_master_send(const struct i2c_client *client,
                                  const uint8_t *buf, uint16_t count)
{
	return i2c_transfer_buffer_flags(client, (uint8_t *)buf, count, 0);
};

int i2c_recovery_bus(struct i2c_adapter *adap);

int i2c_read_reg(const struct i2c_client *client, uint16_t reg, uint16_t reg_size,
                 uint8_t *buf, uint16_t count);


int i2c_write_reg(const struct i2c_client *client, uint16_t reg, uint16_t reg_size,
                  uint8_t *buf, uint16_t count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __I2C_H__ */
