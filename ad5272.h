/**
 ******************************************************************************
 * @file : ad5272.h
 * @author : ZJY
 * @version : V1.0
 * @date : 2025-01-XX
 * @brief : AD5272数字变阻器I2C驱动头文件
 * @attention : None
 ******************************************************************************
 * @history :
 * V1.0 : 1.初始版本，实现AD5272基本功能
 *
 *
 ******************************************************************************
 */
#ifndef __AD5272_H__
#define __AD5272_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "i2c.h"  /* i2c.h已包含errno-base.h */
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief AD5272设备结构体
 */
typedef struct {
    struct i2c_client *client;  /**< I2C客户端指针 */
    uint16_t max_position;       /**< 最大位置值（1023） */
} ad5272_dev_t;

/* Exported constants --------------------------------------------------------*/

/**
 * @defgroup AD5272_Register_Definitions AD5272寄存器定义
 * @{
 */
#define AD5272_REG_RDAC            (0x00U)  /**< RDAC寄存器地址（读写） */
#define AD5272_REG_CONTROL         (0x01U)  /**< 控制寄存器地址 */
#define AD5272_REG_STATUS          (0x02U)  /**< 状态寄存器地址 */
#define AD5272_REG_50TP_MEMORY     (0x03U)  /**< 50-TP存储器地址 */
/** @} */

/**
 * @defgroup AD5272_Command_Definitions AD5272命令定义
 * @{
 */
#define AD5272_CMD_WRITE_RDAC      (0x00U)  /**< 写RDAC寄存器命令 */
#define AD5272_CMD_READ_RDAC       (0x08U)  /**< 读RDAC寄存器命令 */
#define AD5272_CMD_WRITE_CONTROL   (0x01U)  /**< 写控制寄存器命令 */
#define AD5272_CMD_READ_CONTROL    (0x09U)  /**< 读控制寄存器命令 */
#define AD5272_CMD_WRITE_STATUS    (0x02U)  /**< 写状态寄存器命令（保留） */
#define AD5272_CMD_READ_STATUS     (0x0AU)  /**< 读状态寄存器命令 */
/** @} */

/**
 * @defgroup AD5272_Control_Bits AD5272控制寄存器位定义
 * @{
 */
#define AD5272_CTRL_WP_POS         (0U)     /**< 写保护位位置 */
#define AD5272_CTRL_WP_MASK        (0x01U)  /**< 写保护位掩码 */
#define AD5272_CTRL_SHDN_POS       (1U)     /**< 关断位位置 */
#define AD5272_CTRL_SHDN_MASK      (0x02U)  /**< 关断位掩码 */
/** @} */

/**
 * @defgroup AD5272_Constants AD5272常量定义
 * @{
 */
#define AD5272_MAX_POSITION        (1023U)  /**< 最大位置值（10位） */
#define AD5272_MIN_POSITION        (0U)     /**< 最小位置值 */
#define AD5272_DEFAULT_I2C_ADDR    (0x2FU)  /**< 默认I2C地址 */
#define AD5272_DEFAULT_ADAPTER     "i2c2"   /**< 默认I2C适配器名称 */
/** @} */

/* Exported macros -----------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

int ad5272_init(ad5272_dev_t *dev, uint8_t i2c_addr, const char *adapter_name);
int ad5272_deinit(ad5272_dev_t *dev);
int ad5272_set_resistance(ad5272_dev_t *dev, uint16_t position);
int ad5272_get_resistance(ad5272_dev_t *dev, uint16_t *position);
int ad5272_set_write_protect(ad5272_dev_t *dev, bool enable);
int ad5272_read_status(ad5272_dev_t *dev, uint8_t *status);
int ad5272_shutdown(ad5272_dev_t *dev, bool enable);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __AD5272_H__ */
