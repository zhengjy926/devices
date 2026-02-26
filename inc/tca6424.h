/**
 ******************************************************************************
 * @file        : tca6424.h
 * @author      : ZJY
 * @version     : V1.0
 * @date        : 2025-02-25
 * @brief       : TCA6424 24-bit I²C I/O expander driver header file
 * @attention   : None
 ******************************************************************************
 * @history     :
 * V1.0 : 1. Initial version, implements basic TCA6424 functions
 *        2. Supports 24-bit I/O port read/write and single-pin operations
 *        3. Supports configuration, output, and polarity inversion registers
 ******************************************************************************
 */
#ifndef __TCA6424_H__
#define __TCA6424_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/

#include "i2c.h"
#include <stdbool.h>
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/
/** @brief TCA6424 device descriptor: I2C client, RST/INT pins, and input/output/polarity/config caches */
typedef struct tca6424_device {
    i2c_client_t client;         /**< I2C client (bus + address), must not be NULL */
    uint32_t rst_pin;            /**< RESET# GPIO pin id (active low); use UINT32_MAX if not present */
    uint32_t int_pin;            /**< Optional INT# GPIO pin id; use UINT32_MAX if not present */
    uint8_t out_cache[3];        /**< Output cache for three ports */
    uint8_t in_cache[3];         /**< Input cache for three ports */
    uint8_t pol_cache[3];        /**< Polarity inversion cache */
    uint8_t cfg_cache[3];        /**< Configuration cache */
} tca6424_t;

/* Exported constants --------------------------------------------------------*/

/**
 * @defgroup TCA6424 I2C Address Definitions
 * @{
 */
#define TCA6424_I2C_ADDR_L              (0x22U) /**< I2C address when ADDR pin is tied to GND */
#define TCA6424_I2C_ADDR_H              (0x23U) /**< I2C address when ADDR pin is tied to VCCP */
/**
 * @}
 */

/**
 * @defgroup TCA6424 Register Address Definitions
 * @{
 */

/**
 * @defgroup TCA6424 Input Port Register (read-only) Addresses
 * @{
 */
#define TCA6424_REG_INPUT_PORT0         (0x00U)
#define TCA6424_REG_INPUT_PORT1         (0x01U)
#define TCA6424_REG_INPUT_PORT2         (0x02U)
/**
 * @}
 */

/**
 * @defgroup TCA6424 Output Port Register (read/write) Addresses
 * @{
 */
#define TCA6424_REG_OUTPUT_PORT0        (0x04U)
#define TCA6424_REG_OUTPUT_PORT1        (0x05U)
#define TCA6424_REG_OUTPUT_PORT2        (0x06U)
/**
 * @}
 */

/**
 * @defgroup TCA6424 Polarity Inversion Register (read/write) Addresses
 * @{
 */
#define TCA6424_REG_POL_PORT0           (0x08U)
#define TCA6424_REG_POL_PORT1           (0x09U)
#define TCA6424_REG_POL_PORT2           (0x0AU)
/**
 * @}
 */

/**
 * @defgroup TCA6424 Configuration Register (read/write) Addresses
 * @{
 */
#define TCA6424_REG_CFG_PORT0           (0x0CU)
#define TCA6424_REG_CFG_PORT1           (0x0DU)
#define TCA6424_REG_CFG_PORT2           (0x0EU)
/**
 * @}
 */

/**
 * @}
 */


#define TCA6424_NUM_PORTS               (3U)
#define TCA6424_PINS_PER_PORT           (8U)
#define TCA6424_NUM_PINS                (24U)

/** Command byte bit 7: auto-increment (after read/write, move to next port) */
#define TCA6424_CMD_AI_BIT              (7U)
#define TCA6424_CMD_AI_MASK             (0x80U)

/* port: 0..2, bit: 0..7 → pin 0..23 */
#define TCA6424_PIN(port, bit)          ((uint8_t)((uint8_t)(port) * 8u + (uint8_t)(bit)))
#define TCA6424_PORT(pin)               ((uint8_t)((uint8_t)(pin) / 8u))
#define TCA6424_BIT(pin)                ((uint8_t)((uint8_t)(pin) % 8u))

#define TCA6424_RESET_PULSE_DURATION    (1U) /**< RESET# low pulse width in microseconds */
#define TCA6424_TIME_TO_RESET           (1U) /**< Time to wait after releasing RESET#, in microseconds */

/* Exported macros -----------------------------------------------------------*/



/* Exported variables --------------------------------------------------------*/



/** Pin direction in configuration register: 1=input, 0=output */
typedef enum {
    TCA6424_DIR_OUTPUT = 0,
    TCA6424_DIR_INPUT  = 1
} tca6424_dir_t;

/* Exported functions --------------------------------------------------------*/
int tca6424_init(tca6424_t *dev, uint8_t addr, const char *adapter_name,
                 uint32_t rst_pin, uint32_t int_pin);
void tca6424_reset(tca6424_t *dev);
int tca6424_sync_cache(tca6424_t *dev);

int tca6424_read_port(tca6424_t *dev, uint8_t port, uint8_t *val);
int tca6424_write_port(tca6424_t *dev, uint8_t port, uint8_t val);

int tca6424_read_pin(tca6424_t *dev, uint8_t pin, uint8_t *level);
int tca6424_write_pin(tca6424_t *dev, uint8_t pin, uint8_t level);

int tca6424_read_inputs(tca6424_t *dev, uint32_t *val);
int tca6424_write_outputs(tca6424_t *dev, uint32_t val);

int tca6424_read_config(tca6424_t *dev, uint32_t *cfg);
int tca6424_write_config(tca6424_t *dev, uint32_t cfg);

int tca6424_read_polarity(tca6424_t *dev, uint32_t *pol);
int tca6424_write_polarity(tca6424_t *dev, uint32_t pol);

int tca6424_set_direction(tca6424_t *dev, uint8_t pin, tca6424_dir_t dir);
int tca6424_set_polarity(tca6424_t *dev, uint8_t pin, bool invert);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __TCA6424_H__ */
