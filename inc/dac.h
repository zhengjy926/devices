/**
  ******************************************************************************
  * @file        : dac.h
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 20xx-xx-xx
  * @brief       : DAC设备抽象层接口定义
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.初始版本
  ******************************************************************************
  */
#ifndef __DAC_H__
#define __DAC_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/

#include "my_list.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported define -----------------------------------------------------------*/

/* Exported typedef ----------------------------------------------------------*/

struct dac_device;

/**
 * @brief DAC操作函数结构体
 */
struct dac_ops
{
    /**
     * @brief 初始化DAC设备
     * @param device DAC设备指针
     * @return 0成功，<0失败
     */
    int32_t (*init)(struct dac_device *device);

    /**
     * @brief 反初始化DAC设备
     * @param device DAC设备指针
     * @return 0成功，<0失败
     */
    int32_t (*deinit)(struct dac_device *device);

    /**
     * @brief 启动DAC通道
     * @param device DAC设备指针
     * @param channel 通道号（从0开始）
     * @return 0成功，<0失败
     */
    int32_t (*start)(struct dac_device *device, uint8_t channel);

    /**
     * @brief 停止DAC通道
     * @param device DAC设备指针
     * @param channel 通道号（从0开始）
     * @return 0成功，<0失败
     */
    int32_t (*stop)(struct dac_device *device, uint8_t channel);

    /**
     * @brief 设置DAC输出值
     * @param device DAC设备指针
     * @param channel 通道号（从0开始）
     * @param value 输出值（原始数字值，范围取决于分辨率）
     * @return 0成功，<0失败
     */
    int32_t (*set_value)(struct dac_device *device, uint8_t channel, uint32_t value);

    /**
     * @brief 设置DAC输出电压（毫伏）
     * @param device DAC设备指针
     * @param channel 通道号（从0开始）
     * @param voltage_mv 输出电压（毫伏）
     * @return 0成功，<0失败
     */
    int32_t (*set_voltage)(struct dac_device *device, uint8_t channel, uint32_t voltage_mv);

    /**
     * @brief 获取DAC分辨率（位数）
     * @param device DAC设备指针
     * @return 分辨率位数（如12表示12-bit）
     */
    uint8_t (*get_resolution)(struct dac_device *device);

    /**
     * @brief 获取参考电压（毫伏）
     * @param device DAC设备指针
     * @return 参考电压（毫伏）
     */
    uint32_t (*get_vref)(struct dac_device *device);
};

/**
 * @brief DAC设备结构体
 */
typedef struct dac_device
{
    const char *name;              /**< 设备名称 */
    uint8_t resolution_bits;       /**< DAC分辨率 */
    uint32_t vref_mv;              /**< 参考电压(mV)，用于换算成物理量 */
    uint8_t channel_count;         /**< 通道数量 */
    const struct dac_ops *ops;     /**< 操作函数指针 */
    list_t node;                   /**< 链表节点 */
    void *priv;                    /**< 私有数据指针，用于存储硬件相关数据 */
} dac_t;

/* Exported macro ------------------------------------------------------------*/

/* Exported variable prototypes ----------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/

dac_t*  dac_find(const char *name);
dac_t*  dac_open(uint32_t number);
void    dac_close(dac_t *dac);
int32_t dac_start(dac_t *dac, uint8_t channel);
int32_t dac_stop(dac_t *dac, uint8_t channel);
int32_t dac_set_value(dac_t *dac, uint8_t channel, uint32_t value);
int32_t dac_set_voltage(dac_t *dac, uint8_t channel, uint32_t voltage_mv);
int32_t hw_dac_register(dac_t *dev, const char *name, const struct dac_ops *ops, void *user_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DAC_H__ */