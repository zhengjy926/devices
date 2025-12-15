/**
  ******************************************************************************
  * @file        : dac.c
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 202x-xx-xx
  * @brief       : DAC设备抽象层实现
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.初始版本
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "dac.h"
#include "errno-base.h"
#include <string.h>

#define  LOG_TAG             "dac"
#define  LOG_LVL             4
#include "log.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

#define DAC_NAME_MAX        16U     /**< DAC设备名称最大长度 */

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

static LIST_HEAD(dac_device_list);  /**< DAC设备链表头 */

/* Exported variables  -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/**
 * @brief 检查DAC设备是否有效
 * @param dac DAC设备指针
 * @return true有效，false无效
 */
static bool _dac_device_valid(const dac_t *dac);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 查找DAC设备
 * @param name 设备名称
 * @return DAC设备指针，失败返回NULL
 */
dac_t* dac_find(const char *name)
{
    dac_t *dev = NULL;
    list_t *node = NULL;

    if (name == NULL) {
        LOG_E("DAC name is NULL");
        return NULL;
    }

    /* 遍历链表查找设备 */
    list_for_each(node, &dac_device_list) {
        dev = list_entry(node, dac_t, node);
        if (dev != NULL && dev->name != NULL && strcmp(dev->name, name) == 0) {
            return dev;
        }
    }

    return NULL;
}

/**
 * @brief 打开DAC设备
 * @param number DAC编号（如0表示DAC1，1表示DAC2）
 * @return DAC设备指针，失败返回NULL
 */
dac_t* dac_open(uint32_t number)
{
    char name[DAC_NAME_MAX];
    dac_t *dev = NULL;

    /* 根据编号生成设备名称 */
    if (number == 0U) {
        (void)strncpy(name, "dac1", DAC_NAME_MAX);
    } else if (number == 1U) {
        (void)strncpy(name, "dac2", DAC_NAME_MAX);
    } else if (number == 2U) {
        (void)strncpy(name, "dac3", DAC_NAME_MAX);
    } else {
        LOG_E("Invalid DAC number: %lu", number);
        return NULL;
    }

    name[DAC_NAME_MAX - 1U] = '\0';

    /* 查找设备 */
    dev = dac_find(name);
    if (dev == NULL) {
        LOG_E("DAC device '%s' not found", name);
        return NULL;
    }

    /* 如果设备未初始化，执行初始化 */
    if (dev->ops != NULL && dev->ops->init != NULL) {
        int32_t ret = dev->ops->init(dev);
        if (ret != 0) {
            LOG_E("Failed to init DAC '%s', ret=%d", name, ret);
            return NULL;
        }
    }

    return dev;
}

/**
 * @brief 关闭DAC设备
 * @param dac DAC设备指针
 */
void dac_close(dac_t *dac)
{
    if (dac == NULL)
    {
        return;
    }

    /* 停止所有通道 */
    if (dac->ops != NULL && dac->ops->stop != NULL)
    {
        uint8_t i;
        for (i = 0U; i < dac->channel_count; i++)
        {
            (void)dac->ops->stop(dac, i);
        }
    }

    /* 反初始化设备 */
    if (dac->ops != NULL && dac->ops->deinit != NULL)
    {
        (void)dac->ops->deinit(dac);
    }
}

/**
 * @brief 启动DAC通道
 * @param dac DAC设备指针
 * @param channel 通道号（从0开始）
 * @return 0成功，<0失败
 */
int32_t dac_start(dac_t *dac, uint8_t channel)
{
    if (dac == NULL)
    {
        LOG_E("DAC device is NULL");
        return -EINVAL;
    }

    if (channel >= dac->channel_count)
    {
        LOG_E("Invalid channel: %u (max: %u)", channel, dac->channel_count);
        return -EINVAL;
    }

    if (dac->ops == NULL || dac->ops->start == NULL)
    {
        LOG_E("DAC '%s' not registered or missing start operation",
              dac->name != NULL ? dac->name : "unknown");
        return -ENODEV;
    }

    return dac->ops->start(dac, channel);
}

/**
 * @brief 停止DAC通道
 * @param dac DAC设备指针
 * @param channel 通道号（从0开始）
 * @return 0成功，<0失败
 */
int32_t dac_stop(dac_t *dac, uint8_t channel)
{
    if (dac == NULL)
    {
        LOG_E("DAC device is NULL");
        return -EINVAL;
    }

    if (channel >= dac->channel_count)
    {
        LOG_E("Invalid channel: %u (max: %u)", channel, dac->channel_count);
        return -EINVAL;
    }

    if (dac->ops == NULL || dac->ops->stop == NULL)
    {
        LOG_E("DAC '%s' not registered or missing stop operation",
              dac->name != NULL ? dac->name : "unknown");
        return -ENODEV;
    }

    return dac->ops->stop(dac, channel);
}

/**
 * @brief 设置DAC输出值
 * @param dac DAC设备指针
 * @param channel 通道号（从0开始）
 * @param value 输出值（原始数字值）
 * @return 0成功，<0失败
 */
int32_t dac_set_value(dac_t *dac, uint8_t channel, uint32_t value)
{
    uint32_t max_value;

    if (dac == NULL)
    {
        LOG_E("DAC device is NULL");
        return -EINVAL;
    }

    if (channel >= dac->channel_count)
    {
        LOG_E("Invalid channel: %u (max: %u)", channel, dac->channel_count);
        return -EINVAL;
    }

    if (dac->ops == NULL || dac->ops->set_value == NULL)
    {
        LOG_E("DAC '%s' not registered or missing set_value operation",
              dac->name != NULL ? dac->name : "unknown");
        return -ENODEV;
    }

    /* 检查值是否超出范围 */
    max_value = (1UL << dac->resolution_bits) - 1UL;
    if (value > max_value)
    {
        LOG_W("DAC value %lu exceeds max %lu, clamping", value, max_value);
        value = max_value;
    }

    return dac->ops->set_value(dac, channel, value);
}

/**
 * @brief 设置DAC输出电压（毫伏）
 * @param dac DAC设备指针
 * @param channel 通道号（从0开始）
 * @param voltage_mv 输出电压（毫伏）
 * @return 0成功，<0失败
 */
int32_t dac_set_voltage(dac_t *dac, uint8_t channel, uint32_t voltage_mv)
{
    uint32_t value;
    uint32_t max_value;
    uint64_t temp;

    if (dac == NULL)
    {
        LOG_E("DAC device is NULL");
        return -EINVAL;
    }

    if (channel >= dac->channel_count)
    {
        LOG_E("Invalid channel: %u (max: %u)", channel, dac->channel_count);
        return -EINVAL;
    }

    if (dac->ops == NULL || dac->ops->set_voltage != NULL)
    {
        /* 如果硬件支持直接设置电压，使用硬件接口 */
        return dac->ops->set_voltage(dac, channel, voltage_mv);
    }

    /* 否则，通过set_value实现 */
    if (dac->ops == NULL || dac->ops->set_value == NULL)
    {
        LOG_E("DAC '%s' not registered or missing set_value operation",
              dac->name != NULL ? dac->name : "unknown");
        return -ENODEV;
    }

    /* 检查电压是否超出范围 */
    if (voltage_mv > dac->vref_mv)
    {
        LOG_W("DAC voltage %lu mV exceeds vref %lu mV, clamping",
              voltage_mv, dac->vref_mv);
        voltage_mv = dac->vref_mv;
    }

    /* 计算数字值：value = (voltage_mv * max_value) / vref_mv */
    max_value = (1UL << dac->resolution_bits) - 1UL;
    temp = (uint64_t)voltage_mv * max_value;
    value = (uint32_t)(temp / dac->vref_mv);

    return dac->ops->set_value(dac, channel, value);
}

/**
 * @brief 注册DAC设备
 * @param dev DAC设备结构体指针
 * @param name 设备名称
 * @param ops 操作函数指针
 * @param user_data 用户私有数据
 * @return 0成功，<0失败
 */
int32_t hw_dac_register(dac_t *dev, const char *name, const struct dac_ops *ops, void *user_data)
{
    list_t *node = NULL;
    dac_t *existing = NULL;

    if (dev == NULL || name == NULL || ops == NULL)
    {
        LOG_E("Invalid parameter: dev=%p, name=%p, ops=%p", dev, name, ops);
        return -EINVAL;
    }

    /* 检查必需的操作函数 */
    if (ops->set_value == NULL)
    {
        LOG_E("Missing mandatory operation: set_value");
        return -EINVAL;
    }

    /* 检查设备名称是否已存在 */
    list_for_each(node, &dac_device_list)
    {
        existing = list_entry(node, dac_t, node);
        if (existing == dev)
        {
            LOG_E("DAC device already registered");
            return -EEXIST;
        }
        if (existing->name != NULL && strcmp(existing->name, name) == 0)
        {
            LOG_E("DAC name '%s' already exists", name);
            return -EEXIST;
        }
    }

    /* 初始化设备结构 */
    (void)memset(dev, 0, sizeof(dac_t));
    dev->name = name;
    dev->ops = ops;
    dev->priv = user_data;

    /* 设置默认值（如果未设置） */
    if (dev->resolution_bits == 0U)
    {
        dev->resolution_bits = 12U;  /* 默认12位 */
    }
    if (dev->vref_mv == 0U)
    {
        dev->vref_mv = 3300U;  /* 默认3.3V */
    }
    if (dev->channel_count == 0U)
    {
        dev->channel_count = 1U;  /* 默认1个通道 */
    }

    /* 添加到设备链表 */
    list_node_init(&dev->node);
    list_add_tail(&dev->node, &dac_device_list);

    LOG_I("DAC device '%s' registered (resolution=%u bits, vref=%lu mV, channels=%u)",
          name, dev->resolution_bits, dev->vref_mv, dev->channel_count);

    return 0;
}

/* Private functions ---------------------------------------------------------*/
/**
 * @brief 检查DAC设备是否有效
 * @param dac DAC设备指针
 * @return true有效，false无效
 */
static bool _dac_device_valid(const dac_t *dac)
{
    if (dac == NULL)
    {
        return false;
    }

    if (dac->name == NULL || dac->ops == NULL)
    {
        return false;
    }

    if (dac->ops->set_value == NULL)
    {
        return false;
    }

    return true;
}

