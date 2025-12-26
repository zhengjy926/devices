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
 * @defgroup AD5272_Command_Definitions AD5272命令定义（4位）
 * @{
 */
#define AD5272_CMD_NOP              0x00  // 无操作
#define AD5272_CMD_WRITE_RDAC       0x01  // 写入 RDAC (设置电阻值)
#define AD5272_CMD_READ_RDAC        0x02  // 读取 RDAC
#define AD5272_CMD_STORE_50TP       0x03  // 将 RDAC 存入 50-TP 存储器
#define AD5272_CMD_RESET            0x04  // 软件复位
#define AD5272_CMD_READ_50TP        0x05  // 读取 50-TP 内容
#define AD5272_CMD_READ_LAST_ADDR   0x06  // 读取最后编程地址
#define AD5272_CMD_WRITE_CTRL       0x07  // 写入控制寄存器
#define AD5272_CMD_READ_CTRL        0x08  // 读取控制寄存器
#define AD5272_CMD_SHUTDOWN         0x09  // 软件关断
/** @} */

/**
 * @defgroup AD5272_Control_Bits AD5272控制寄存器（10位）位定义
 * @{
 */
#define AD5272_CTRL_50TP_PROGRAM_EN    0x01 // C0: 允许 50-TP 编程 [cite: 137]
#define AD5272_CTRL_RDAC_WRITE_EN      0x02 // C1: 允许 RDAC 写入 [cite: 137]
#define AD5272_CTRL_RESISTOR_PERF_DIS  0x04 // C2: 禁用电阻性能模式 [cite: 137]
#define AD5272_CTRL_50TP_SUCCESS       0x08 // C3: (只读) 编程成功标志 [cite: 137]
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

int ad5272_NOP(ad5272_dev_t *dev);
int ad5272_set_RDAC(ad5272_dev_t *dev, uint16_t code);
int ad5272_get_RDAC(ad5272_dev_t *dev, uint16_t *code);
int ad5272_store_50TP(ad5272_dev_t *dev);
int ad5272_software_reset(ad5272_dev_t *dev);
int ad5272_read_50TP(ad5272_dev_t *dev, uint8_t mem_addr, uint16_t *val);
int ad5272_get_last_addr(ad5272_dev_t *dev, uint8_t *addr);
int ad5272_set_control_reg(ad5272_dev_t *dev, uint8_t config);
int ad5272_get_control_reg(ad5272_dev_t *dev, uint8_t *config);
int ad5272_set_shutdown(ad5272_dev_t *dev, bool enable);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __AD5272_H__ */
