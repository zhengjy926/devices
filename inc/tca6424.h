/**
  ******************************************************************************
  * @file        : tca6424.h
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2025-01-XX
  * @brief       : TCA6424 24位I²C I/O扩展器驱动头文件
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1. 初始版本，实现TCA6424基本功能
  *                2. 支持24位I/O端口读写
  *                3. 支持单引脚操作
  *                4. 支持配置、输出、极性反转寄存器操作
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
 * @brief TCA6424设备结构体
 * @details 包含I2C适配器指针、设备地址、缓存状态和寄存器缓存
 */
typedef struct {
    struct i2c_adapter *adapter; /**< I2C适配器指针 */
    uint8_t  addr;               /**< I2C设备地址（7位地址） */
    uint16_t flags;              /**< I2C设备标志 */
    bool     cache_valid;        /**< 缓存有效性标志 */
    uint8_t  out[3];             /**< 输出寄存器缓存 [Port0, Port1, Port2] */
    uint8_t  cfg[3];             /**< 配置寄存器缓存 [Port0, Port1, Port2] */
    uint8_t  pol[3];             /**< 极性反转寄存器缓存 [Port0, Port1, Port2] */
} tca6424_t;

/* Exported constants --------------------------------------------------------*/

/** @brief I2C地址定义（ADDR引脚接地时） */
#define TCA6424_I2C_ADDR_L              (0x22U)
/** @brief I2C地址定义（ADDR引脚接VCCP时） */
#define TCA6424_I2C_ADDR_H              (0x23U)

/** @brief 输入端口寄存器地址（只读） */
#define TCA6424_REG_INPUT_PORT0         (0x00u)
#define TCA6424_REG_INPUT_PORT1         (0x01u)
#define TCA6424_REG_INPUT_PORT2         (0x02u)

/** @brief 输出端口寄存器地址（读写） */
#define TCA6424_REG_OUTPUT_PORT0        (0x04u)
#define TCA6424_REG_OUTPUT_PORT1        (0x05u)
#define TCA6424_REG_OUTPUT_PORT2        (0x06u)

/** @brief 极性反转寄存器地址（读写） */
#define TCA6424_REG_POL_PORT0           (0x08u)
#define TCA6424_REG_POL_PORT1           (0x09u)
#define TCA6424_REG_POL_PORT2           (0x0Au)

/** @brief 配置寄存器地址（读写），1=输入，0=输出 */
#define TCA6424_REG_CFG_PORT0           (0x0Cu)
#define TCA6424_REG_CFG_PORT1           (0x0Du)
#define TCA6424_REG_CFG_PORT2           (0x0Eu)

/** @brief 自动递增位位置（命令字节bit7） */
#define TCA6424_AUTO_INC_POS            (7)

/* port: 0..2, bit: 0..7
 * 返回: 0..23
 * 映射: pin = port*8 + bit
 */
#define TCA6424_PIN(port, bit)    ((uint8_t)((uint8_t)(port) * 8u + (uint8_t)(bit)))

/* Exported macros -----------------------------------------------------------*/



/* Exported variables --------------------------------------------------------*/



/* Exported functions --------------------------------------------------------*/

/* 初始化和缓存管理 */
int tca6424_init(tca6424_t *dev, uint8_t addr7, const char *adapter_name);
int tca6424_refresh_cache(tca6424_t *dev);

/* 原始寄存器访问 */
int tca6424_read_reg(tca6424_t *dev, uint8_t reg, bool auto_inc, uint8_t *rx_buf, uint8_t len);
int tca6424_write_reg(tca6424_t *dev, uint8_t reg, bool auto_inc, uint8_t *tx_buf, uint8_t len);

/* 24位端口访问（bit0=P00 ... bit23=P27） */
int tca6424_read_inputs24(tca6424_t *dev, uint32_t *in_bits);
int tca6424_read_outputs24(tca6424_t *dev, uint32_t *out_latch_bits);
int tca6424_write_outputs24(tca6424_t *dev, uint32_t out_latch_bits);
int tca6424_read_polarity24(tca6424_t *dev, uint32_t *pol_bits);
int tca6424_write_polarity24(tca6424_t *dev, uint32_t pol_bits);
int tca6424_read_config24(tca6424_t *dev, uint32_t *cfg_bits);
int tca6424_write_config24(tca6424_t *dev, uint32_t cfg_bits);

/* 掩码更新操作 */
int tca6424_update_outputs24(tca6424_t *dev, uint32_t set_mask, uint32_t clr_mask);
int tca6424_update_config24(tca6424_t *dev, uint32_t set_mask, uint32_t clr_mask);
int tca6424_update_polarity24(tca6424_t *dev, uint32_t set_mask, uint32_t clr_mask);

/* 单引脚操作（pin: 0-23，对应P00-P27） */
int tca6424_pin_mode(tca6424_t *dev, uint8_t pin, bool input);
int tca6424_read_pin(tca6424_t *dev, uint8_t pin, bool *level);
int tca6424_write_pin(tca6424_t *dev, uint8_t pin, bool level);
int tca6424_toggle_pin(tca6424_t *dev, uint8_t pin);
int tca6424_configure_output_pin(tca6424_t *dev, uint8_t pin, bool initial_level);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __TCA6424_H__ */
