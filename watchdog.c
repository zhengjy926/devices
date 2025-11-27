/**
  ******************************************************************************
  * @file        : watchdog.c
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2025-01-XX
  * @brief       : 看门狗驱动实现
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.添加看门狗驱动实现
  *
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "inc/watchdog.h"
#include "errno-base.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

#define  LOG_TAG             "watchdog"
#define  LOG_LVL             4
#include "log.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define WATCHDOG_MAX_DEVICES    1U    /**< 最大支持的看门狗设备数量 */

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/**
 * @brief 看门狗设备数组
 * @note 静态分配，不使用动态内存
 */
static struct watchdog_device _watchdog_devices[WATCHDOG_MAX_DEVICES];

/**
 * @brief 看门狗设备数量
 */
static uint32_t _watchdog_device_count = 0U;

/* Exported variables  -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
/**
 * @brief 获取看门狗设备的操作函数指针
 * @param dev 看门狗设备指针
 * @return 操作函数指针，失败返回NULL
 */
static const struct watchdog_ops* _get_ops(const struct watchdog_device *dev);

/**
 * @brief 创建看门狗设备
 * @param name 看门狗名称
 * @return 设备指针，失败返回NULL
 */
static struct watchdog_device* _create_device(const char *name);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 为看门狗设备注册操作接口
 * @param dev 看门狗设备指针
 * @param ops 硬件操作函数结构体指针
 * @return 0成功，-EINVAL表示参数无效
 */
int watchdog_register(struct watchdog_device *dev, const struct watchdog_ops *ops)
{
    if (dev == NULL || ops == NULL)
    {
        LOG_E("Invalid parameter: dev=%p, ops=%p", dev, ops);
        return -EINVAL;
    }
    
    /* start是必需的操作 */
    if (ops->start == NULL)
    {
        LOG_E("Missing mandatory operation: start");
        return -EINVAL;
    }
    
    /* 为设备设置操作函数 */
    dev->ops = ops;
    
    LOG_I("Watchdog '%s' registered", dev->name != NULL ? dev->name : "unknown");
    
    return 0;
}

/**
 * @brief 查找看门狗设备
 * @param name 看门狗名称
 * @return 设备指针，未找到则自动创建，失败返回NULL
 */
struct watchdog_device* watchdog_find(const char *name)
{
    uint32_t i;
    struct watchdog_device *dev;
    
    if (name == NULL)
    {
        LOG_E("Invalid parameter: name is NULL");
        return NULL;
    }
    
    /* 查找已存在的设备 */
    for (i = 0U; i < _watchdog_device_count; i++)
    {
        if (_watchdog_devices[i].name != NULL && 
            strcmp(_watchdog_devices[i].name, name) == 0)
        {
            LOG_D("Watchdog '%s' found", name);
            return &_watchdog_devices[i];
        }
    }
    
    /* 未找到，自动创建新设备 */
    dev = _create_device(name);
    if (dev != NULL)
    {
        LOG_D("Watchdog '%s' created", name);
    }
    else
    {
        LOG_E("Failed to create watchdog '%s'", name);
    }
    
    return dev;
}

/**
 * @brief 启动看门狗
 * @param dev 看门狗设备指针
 * @return 0成功，负值表示错误码
 */
int watchdog_start(struct watchdog_device *dev)
{
    const struct watchdog_ops *ops;
    int ret;
    
    if (dev == NULL)
    {
        LOG_E("Invalid parameter: dev is NULL");
        return -EINVAL;
    }
    
    /* 获取操作函数指针 */
    ops = dev->ops;
    if (ops == NULL)
    {
        LOG_E("Watchdog '%s' not registered", dev->name != NULL ? dev->name : "unknown");
        return -ENODEV;
    }
    
    if (ops->start == NULL)
    {
        LOG_E("Watchdog '%s' missing start operation", dev->name != NULL ? dev->name : "unknown");
        return -ENOSYS;
    }
    
    /* 检查是否已启动 */
    if (dev->state == WATCHDOG_STATE_RUNNING)
    {
        LOG_D("Watchdog '%s' already running", dev->name != NULL ? dev->name : "unknown");
        return 0;
    }
    
    /* 调用硬件启动 */
    ret = ops->start(dev);
    if (ret != 0)
    {
        LOG_E("Failed to start watchdog '%s', ret=%d", dev->name != NULL ? dev->name : "unknown", ret);
        return ret;
    }
    
    /* 更新设备状态 */
    dev->state = WATCHDOG_STATE_RUNNING;
    
    LOG_I("Watchdog '%s' started, timeout=%lu ms", 
          dev->name != NULL ? dev->name : "unknown", dev->timeout_ms);
    
    return 0;
}

/**
 * @brief 停止看门狗
 * @param dev 看门狗设备指针
 * @return 0成功，负值表示错误码
 */
int watchdog_stop(struct watchdog_device *dev)
{
    const struct watchdog_ops *ops;
    int ret;
    
    if (dev == NULL)
    {
        LOG_E("Invalid parameter: dev is NULL");
        return -EINVAL;
    }
    
    /* 检查是否已停止 */
    if (dev->state == WATCHDOG_STATE_STOPPED)
    {
        LOG_D("Watchdog '%s' already stopped", dev->name != NULL ? dev->name : "unknown");
        return 0;
    }
    
    /* 获取操作函数指针 */
    ops = dev->ops;
    if (ops == NULL)
    {
        LOG_E("Watchdog '%s' not registered", dev->name != NULL ? dev->name : "unknown");
        return -ENODEV;
    }
    
    /* stop是可选的，如果不存在则只更新状态 */
    if (ops->stop != NULL)
    {
        ret = ops->stop(dev);
        if (ret != 0)
        {
            LOG_E("Failed to stop watchdog '%s', ret=%d", dev->name != NULL ? dev->name : "unknown", ret);
            return ret;
        }
    }
    else
    {
        LOG_D("Watchdog '%s' stop operation not supported, updating state only", 
              dev->name != NULL ? dev->name : "unknown");
    }
    
    /* 更新设备状态 */
    dev->state = WATCHDOG_STATE_STOPPED;
    
    LOG_I("Watchdog '%s' stopped", dev->name != NULL ? dev->name : "unknown");
    
    return 0;
}

/**
 * @brief 喂狗（刷新看门狗）
 * @param dev 看门狗设备指针
 * @return 0成功，负值表示错误码
 */
int watchdog_ping(struct watchdog_device *dev)
{
    const struct watchdog_ops *ops;
    int ret;
    
    if (dev == NULL)
    {
        LOG_E("Invalid parameter: dev is NULL");
        return -EINVAL;
    }
    
    /* 获取操作函数指针 */
    ops = dev->ops;
    if (ops == NULL)
    {
        LOG_E("Watchdog '%s' not registered", dev->name != NULL ? dev->name : "unknown");
        return -ENODEV;
    }
    
    /* ping是可选的，如果不存在则返回不支持 */
    if (ops->ping == NULL)
    {
        LOG_E("Watchdog '%s' ping operation not supported", dev->name != NULL ? dev->name : "unknown");
        return -ENOSYS;
    }
    
    /* 检查是否已启动 */
    if (dev->state != WATCHDOG_STATE_RUNNING)
    {
        LOG_W("Watchdog '%s' not running, state=%d", 
              dev->name != NULL ? dev->name : "unknown", dev->state);
        return -EINVAL;
    }
    
    /* 调用硬件ping */
    ret = ops->ping(dev);
    if (ret != 0)
    {
        LOG_E("Failed to ping watchdog '%s', ret=%d", dev->name != NULL ? dev->name : "unknown", ret);
    }
    
    return ret;
}

/**
 * @brief 获取看门狗状态
 * @param dev 看门狗设备指针
 * @return 看门狗状态值，如果硬件不支持则返回设备状态
 */
uint32_t watchdog_status(struct watchdog_device *dev)
{
    const struct watchdog_ops *ops;
    
    if (dev == NULL)
    {
        return 0U;
    }
    
    /* 获取操作函数指针 */
    ops = dev->ops;
    if (ops == NULL)
    {
        return (uint32_t)dev->state;
    }
    
    /* status是可选的，如果存在则使用硬件接口 */
    if (ops->status != NULL)
    {
        return ops->status(dev);
    }
    
    /* 否则返回设备状态 */
    return (uint32_t)dev->state;
}

/**
 * @brief 设置看门狗超时时间
 * @param dev 看门狗设备指针
 * @param timeout_ms 超时时间（毫秒）
 * @return 0成功，负值表示错误码
 */
int watchdog_set_timeout(struct watchdog_device *dev, uint32_t timeout_ms)
{
    const struct watchdog_ops *ops;
    uint32_t min_timeout;
    uint32_t max_timeout;
    int ret;
    
    if (dev == NULL)
    {
        LOG_E("Invalid parameter: dev is NULL");
        return -EINVAL;
    }
    
    /* 获取操作函数指针 */
    ops = dev->ops;
    if (ops == NULL)
    {
        LOG_E("Watchdog '%s' not registered", dev->name != NULL ? dev->name : "unknown");
        return -ENODEV;
    }
    
    /* 检查超时时间范围 */
    if (ops->get_min_timeout != NULL)
    {
        min_timeout = ops->get_min_timeout(dev);
        if (timeout_ms < min_timeout)
        {
            LOG_E("Watchdog '%s' timeout %lu ms < min %lu ms", 
                  dev->name != NULL ? dev->name : "unknown", timeout_ms, min_timeout);
            return -EINVAL;
        }
    }
    
    if (ops->get_max_timeout != NULL)
    {
        max_timeout = ops->get_max_timeout(dev);
        if (timeout_ms > max_timeout)
        {
            LOG_E("Watchdog '%s' timeout %lu ms > max %lu ms", 
                  dev->name != NULL ? dev->name : "unknown", timeout_ms, max_timeout);
            return -EINVAL;
        }
    }
    
    /* 保存配置 */
    dev->timeout_ms = timeout_ms;
    
    /* set_timeout是可选的，如果存在则调用硬件接口 */
    if (ops->set_timeout != NULL)
    {
        ret = ops->set_timeout(dev, timeout_ms);
        if (ret != 0)
        {
            LOG_E("Failed to set timeout for watchdog '%s', ret=%d", 
                  dev->name != NULL ? dev->name : "unknown", ret);
            return ret;
        }
    }
    
    LOG_I("Watchdog '%s' timeout set to %lu ms", 
          dev->name != NULL ? dev->name : "unknown", timeout_ms);
    
    return 0;
}

/**
 * @brief 设置看门狗预超时时间
 * @param dev 看门狗设备指针
 * @param timeout_ms 预超时时间（毫秒）
 * @return 0成功，负值表示错误码
 */
int watchdog_set_pretimeout(struct watchdog_device *dev, uint32_t timeout_ms)
{
    const struct watchdog_ops *ops;
    
    if (dev == NULL)
    {
        return -EINVAL;
    }
    
    /* 获取操作函数指针 */
    ops = dev->ops;
    if (ops == NULL)
    {
        return -ENODEV;
    }
    
    /* set_pretimeout是可选的，如果不存在则返回不支持 */
    if (ops->set_pretimeout == NULL)
    {
        return -ENOSYS;
    }
    
    /* 调用硬件接口 */
    return ops->set_pretimeout(dev, timeout_ms);
}

/**
 * @brief 获取看门狗超时时间
 * @param dev 看门狗设备指针
 * @return 超时时间（毫秒），失败返回0
 */
uint32_t watchdog_get_timeout(const struct watchdog_device *dev)
{
    const struct watchdog_ops *ops;
    
    if (dev == NULL)
    {
        return 0U;
    }
    
    /* 获取操作函数指针 */
    ops = dev->ops;
    if (ops == NULL)
    {
        return dev->timeout_ms;
    }
    
    /* get_timeout是可选的，如果存在则使用硬件接口 */
    if (ops->get_timeout != NULL)
    {
        return ops->get_timeout(dev);
    }
    
    /* 否则返回保存的配置值 */
    return dev->timeout_ms;
}

/**
 * @brief 获取看门狗状态
 * @param dev 看门狗设备指针
 * @return 看门狗状态
 */
watchdog_state_t watchdog_get_state(const struct watchdog_device *dev)
{
    if (dev == NULL)
    {
        return WATCHDOG_STATE_STOPPED;
    }
    
    return dev->state;
}

/**
 * @brief 获取看门狗最大超时时间
 * @param dev 看门狗设备指针
 * @return 最大超时时间（毫秒）
 */
uint32_t watchdog_get_max_timeout(const struct watchdog_device *dev)
{
    const struct watchdog_ops *ops;
    
    if (dev == NULL)
    {
        return 0U;
    }
    
    /* 获取操作函数指针 */
    ops = dev->ops;
    if (ops == NULL)
    {
        return 0U;
    }
    
    /* get_max_timeout是可选的 */
    if (ops->get_max_timeout == NULL)
    {
        return 0U;
    }
    
    return ops->get_max_timeout(dev);
}

/**
 * @brief 获取看门狗最小超时时间
 * @param dev 看门狗设备指针
 * @return 最小超时时间（毫秒）
 */
uint32_t watchdog_get_min_timeout(const struct watchdog_device *dev)
{
    const struct watchdog_ops *ops;
    
    if (dev == NULL)
    {
        return 0U;
    }
    
    /* 获取操作函数指针 */
    ops = dev->ops;
    if (ops == NULL)
    {
        return 0U;
    }
    
    /* get_min_timeout是可选的 */
    if (ops->get_min_timeout == NULL)
    {
        return 0U;
    }
    
    return ops->get_min_timeout(dev);
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 创建看门狗设备
 * @param name 看门狗名称
 * @return 设备指针，失败返回NULL
 */
static struct watchdog_device* _create_device(const char *name)
{
    struct watchdog_device *dev;
    
    if (name == NULL)
    {
        LOG_E("Invalid parameter: name is NULL");
        return NULL;
    }
    
    /* 检查是否超过最大设备数量 */
    if (_watchdog_device_count >= WATCHDOG_MAX_DEVICES)
    {
        LOG_E("Maximum watchdog devices (%u) reached", WATCHDOG_MAX_DEVICES);
        return NULL;
    }
    
    /* 获取空闲设备 */
    dev = &_watchdog_devices[_watchdog_device_count];
    
    /* 初始化设备 */
    dev->name = name;
    dev->state = WATCHDOG_STATE_STOPPED;
    dev->timeout_ms = 0U;
    dev->ops = NULL;  /* 初始化为NULL，需要调用watchdog_register注册 */
    
    /* 增加设备计数 */
    _watchdog_device_count++;
    
    LOG_D("Watchdog device '%s' created (total: %u)", name, _watchdog_device_count);
    
    return dev;
}





