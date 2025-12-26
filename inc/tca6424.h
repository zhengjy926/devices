/**
  ******************************************************************************
  * @file        : xxx.h
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 20xx-xx-xx
  * @brief       : 
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.xxx
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

/* Exported types ------------------------------------------------------------*/

/**
 * @brief AD5272设备结构体
 */
typedef struct {
    struct i2c_client *client;   /**< I2C客户端指针 */
    bool     cache_valid;
    uint8_t  out[3];
    uint8_t  cfg[3];
    uint8_t  pol[3];
} tca6424_t;

/* Exported constants --------------------------------------------------------*/

#define TCA6424_I2C_ADDR_L              (0x22U)
#define TCA6424_I2C_ADDR_H              (0x23U)


#define TCA6424_REG_INPUT_PORT0         (0x00u)
#define TCA6424_REG_INPUT_PORT1         (0x01u)
#define TCA6424_REG_INPUT_PORT2         (0x02u)

#define TCA6424_REG_OUTPUT_PORT0        (0x04u)
#define TCA6424_REG_OUTPUT_PORT1        (0x05u)
#define TCA6424_REG_OUTPUT_PORT2        (0x06u)

#define TCA6424_REG_POL_PORT0           (0x08u)
#define TCA6424_REG_POL_PORT1           (0x09u)
#define TCA6424_REG_POL_PORT2           (0x0Au)

#define TCA6424_REG_CFG_PORT0           (0x0Cu)
#define TCA6424_REG_CFG_PORT1           (0x0Du)
#define TCA6424_REG_CFG_PORT2           (0x0Eu)

#define TCA6424_AUTO_INC_POS            (7)

/* Exported macros -----------------------------------------------------------*/



/* Exported variables --------------------------------------------------------*/



/* Exported functions --------------------------------------------------------*/

int tca6424_init(tca6424_t *dev, uint8_t addr7, const char *adapter_name);
int tca6424_refresh_cache(tca6424_t *dev);

/* Raw register access */
int tca6424_read_reg(tca6424_t *dev, uint8_t reg, bool auto_inc, uint8_t *rx_buf, uint8_t len);
int tca6424_write_reg(tca6424_t *dev, uint8_t reg, bool auto_inc, uint8_t *tx_buf, uint8_t len);

/* 24-bit bank access (bit0=P00 ... bit23=P27) */
int tca6424_read_inputs24(tca6424_t *dev, uint32_t *in_bits);
int tca6424_read_outputs24(tca6424_t *dev, uint32_t *out_latch_bits);
int tca6424_write_outputs24(tca6424_t *dev, uint32_t out_latch_bits);

int tca6424_read_config24(tca6424_t *dev, uint32_t *cfg_bits /*1=input*/);
int tca6424_write_config24(tca6424_t *dev, uint32_t cfg_bits /*1=input*/);

int tca6424_read_polarity24(tca6424_t *dev, uint32_t pol_bits /*1=invert*/);
int tca6424_write_polarity24(tca6424_t *dev, uint32_t pol_bits /*1=invert*/);

/* Masked update helpers */
int tca6424_update_outputs24(tca6424_t *dev, uint32_t set_mask, uint32_t clr_mask);
int tca6424_update_config24(tca6424_t *dev, uint32_t set_mask, uint32_t clr_mask);
int tca6424_update_polarity24(tca6424_t *dev, uint32_t set_mask, uint32_t clr_mask);

/* Per-pin convenience */
int tca6424_pin_mode(tca6424_t *dev, uint8_t pin /*0..23*/, bool input);
int tca6424_write_pin(tca6424_t *dev, uint8_t pin /*0..23*/, bool level);
int tca6424_toggle_pin(tca6424_t *dev, uint8_t pin /*0..23*/);
int tca6424_read_pin(tca6424_t *dev, uint8_t pin /*0..23*/, bool *level);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __TCA6424_H__ */
