/**
  ******************************************************************************
  * @file        : hwtimer.c
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2025-01-XX
  * @brief       : 硬件定时器驱动实现
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.添加硬件定时器驱动实现
  *
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "hwtimer.h"
#include "errno-base.h"
#include <assert.h>
#include <stddef.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define HWTIMER_MAX_DEVICES    16U    /**< 最大支持的定时器设备数量 */

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/**
 * @brief 硬件定时器操作函数指针
 * @note 保存硬件特定的定时器操作函数
 */
static const struct hwtimer_ops* _hw_timer_ops = NULL;

/**
 * @brief 定时器设备数组
 * @note 静态分配，不使用动态内存
 */
static struct hwtimer_device _timer_devices[HWTIMER_MAX_DEVICES];

/**
 * @brief 定时器设备数量
 */
static uint32_t _timer_device_count = 0U;

/* Exported variables  -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
/**
 * @brief 查找定时器设备
 * @param timer_id 定时器ID
 * @return 设备指针，未找到返回NULL
 */
static struct hwtimer_device* _find_device(uint32_t timer_id);

/**
 * @brief 创建定时器设备
 * @param timer_id 定时器ID
 * @return 设备指针，失败返回NULL
 */
static struct hwtimer_device* _create_device(uint32_t timer_id);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 注册硬件定时器操作接口
 * @param ops 硬件操作函数结构体指针
 * @return 0成功，-EINVAL表示参数无效
 */
int hwtimer_register(const struct hwtimer_ops *ops)
{
    if (ops == NULL)
    {
        return -EINVAL;
    }
    
    if (ops->init == NULL || ops->start == NULL || ops->stop == NULL)
    {
        return -EINVAL;
    }
    
    _hw_timer_ops = ops;
    return 0;
}

/**
 * @brief 初始化定时器
 * @param timer_id 定时器ID
 * @return 0成功，负值表示错误码
 */
int hwtimer_init(uint32_t timer_id)
{
    struct hwtimer_device *dev;
    int ret;
    
    if (_hw_timer_ops == NULL)
    {
        return -ENODEV;
    }
    
    if (_hw_timer_ops->init == NULL)
    {
        return -ENOSYS;
    }
    
    /* 查找或创建设备 */
    dev = _find_device(timer_id);
    if (dev == NULL)
    {
        dev = _create_device(timer_id);
        if (dev == NULL)
        {
            return -ENOMEM;
        }
    }
    
    /* 如果设备已初始化且正在运行，先停止 */
    if (dev->state == HWTIMER_STATE_RUNNING)
    {
        (void)hwtimer_stop(timer_id);
    }
    
    /* 调用硬件初始化 */
    ret = _hw_timer_ops->init(timer_id);
    if (ret != 0)
    {
        return ret;
    }
    
    /* 初始化设备状态 */
    dev->state = HWTIMER_STATE_STOPPED;
    dev->config.period_us = 0U;
    dev->config.mode = HWTIMER_MODE_PERIODIC;
    dev->config.callback = NULL;
    dev->config.user_data = NULL;
    
    return 0;
}

/**
 * @brief 反初始化定时器
 * @param timer_id 定时器ID
 * @return 0成功，负值表示错误码
 */
int hwtimer_deinit(uint32_t timer_id)
{
    struct hwtimer_device *dev;
    int ret;
    
    if (_hw_timer_ops == NULL)
    {
        return -ENODEV;
    }
    
    if (_hw_timer_ops->deinit == NULL)
    {
        return -ENOSYS;
    }
    
    dev = _find_device(timer_id);
    if (dev == NULL)
    {
        return -ENOENT;
    }
    
    /* 如果定时器正在运行，先停止 */
    if (dev->state == HWTIMER_STATE_RUNNING)
    {
        (void)hwtimer_stop(timer_id);
    }
    
    /* 调用硬件反初始化 */
    ret = _hw_timer_ops->deinit(timer_id);
    if (ret != 0)
    {
        return ret;
    }
    
    /* 清除设备状态 */
    dev->state = HWTIMER_STATE_STOPPED;
    dev->config.period_us = 0U;
    dev->config.mode = HWTIMER_MODE_PERIODIC;
    dev->config.callback = NULL;
    dev->config.user_data = NULL;
    
    return 0;
}

/**
 * @brief 配置定时器
 * @param timer_id 定时器ID
 * @param config 定时器配置结构体指针
 * @return 0成功，负值表示错误码
 */
int hwtimer_config(uint32_t timer_id, const struct hwtimer_config *config)
{
    struct hwtimer_device *dev;
    uint32_t max_period;
    uint32_t min_period;
    
    if (config == NULL)
    {
        return -EINVAL;
    }
    
    if (_hw_timer_ops == NULL)
    {
        return -ENODEV;
    }
    
    dev = _find_device(timer_id);
    if (dev == NULL)
    {
        return -ENOENT;
    }
    
    /* 如果定时器正在运行，不允许修改配置 */
    if (dev->state == HWTIMER_STATE_RUNNING)
    {
        return -EBUSY;
    }
    
    /* 验证周期值 */
    if (config->period_us == 0U)
    {
        return -EINVAL;
    }
    
    /* 检查周期范围（如果硬件支持） */
    if (_hw_timer_ops->get_max_period != NULL)
    {
        max_period = _hw_timer_ops->get_max_period(timer_id);
        if (max_period > 0U && config->period_us > max_period)
        {
            return -EINVAL;
        }
    }
    
    if (_hw_timer_ops->get_min_period != NULL)
    {
        min_period = _hw_timer_ops->get_min_period(timer_id);
        if (min_period > 0U && config->period_us < min_period)
        {
            return -EINVAL;
        }
    }
    
    /* 保存配置 */
    dev->config = *config;
    
    return 0;
}

/**
 * @brief 启动定时器
 * @param timer_id 定时器ID
 * @return 0成功，负值表示错误码
 */
int hwtimer_start(uint32_t timer_id)
{
    struct hwtimer_device *dev;
    int ret;
    
    if (_hw_timer_ops == NULL)
    {
        return -ENODEV;
    }
    
    assert(_hw_timer_ops->start != NULL);
    
    dev = _find_device(timer_id);
    if (dev == NULL)
    {
        return -ENOENT;
    }
    
    /* 检查是否已运行 */
    if (dev->state == HWTIMER_STATE_RUNNING)
    {
        return 0;
    }
    
    /* 检查配置是否有效 */
    if (dev->config.period_us == 0U)
    {
        return -EINVAL;
    }
    
    /* 调用硬件启动函数 */
    ret = _hw_timer_ops->start(timer_id, dev->config.period_us, dev->config.mode);
    if (ret != 0)
    {
        return ret;
    }
    
    /* 更新状态 */
    dev->state = HWTIMER_STATE_RUNNING;
    
    return 0;
}

/**
 * @brief 停止定时器
 * @param timer_id 定时器ID
 * @return 0成功，负值表示错误码
 */
int hwtimer_stop(uint32_t timer_id)
{
    struct hwtimer_device *dev;
    int ret;
    
    if (_hw_timer_ops == NULL)
    {
        return -ENODEV;
    }
    
    assert(_hw_timer_ops->stop != NULL);
    
    dev = _find_device(timer_id);
    if (dev == NULL)
    {
        return -ENOENT;
    }
    
    /* 如果已经停止，直接返回 */
    if (dev->state == HWTIMER_STATE_STOPPED)
    {
        return 0;
    }
    
    /* 调用硬件停止函数 */
    ret = _hw_timer_ops->stop(timer_id);
    if (ret != 0)
    {
        return ret;
    }
    
    /* 更新状态 */
    dev->state = HWTIMER_STATE_STOPPED;
    
    return 0;
}

/**
 * @brief 设置定时器周期
 * @param timer_id 定时器ID
 * @param period_us 周期（微秒）
 * @return 0成功，负值表示错误码
 */
int hwtimer_set_period(uint32_t timer_id, uint32_t period_us)
{
    struct hwtimer_device *dev;
    int ret;
    uint32_t max_period;
    uint32_t min_period;
    
    if (period_us == 0U)
    {
        return -EINVAL;
    }
    
    if (_hw_timer_ops == NULL)
    {
        return -ENODEV;
    }
    
    if (_hw_timer_ops->set_period == NULL)
    {
        return -ENOSYS;
    }
    
    dev = _find_device(timer_id);
    if (dev == NULL)
    {
        return -ENOENT;
    }
    
    /* 检查周期范围（如果硬件支持） */
    if (_hw_timer_ops->get_max_period != NULL)
    {
        max_period = _hw_timer_ops->get_max_period(timer_id);
        if (max_period > 0U && period_us > max_period)
        {
            return -EINVAL;
        }
    }
    
    if (_hw_timer_ops->get_min_period != NULL)
    {
        min_period = _hw_timer_ops->get_min_period(timer_id);
        if (min_period > 0U && period_us < min_period)
        {
            return -EINVAL;
        }
    }
    
    /* 调用硬件设置周期函数 */
    ret = _hw_timer_ops->set_period(timer_id, period_us);
    if (ret != 0)
    {
        return ret;
    }
    
    /* 更新配置 */
    dev->config.period_us = period_us;
    
    return 0;
}

/**
 * @brief 获取定时器当前计数值
 * @param timer_id 定时器ID
 * @return 当前计数值，失败返回0
 */
uint32_t hwtimer_get_count(uint32_t timer_id)
{
    if (_hw_timer_ops == NULL)
    {
        return 0U;
    }
    
    if (_hw_timer_ops->get_count == NULL)
    {
        return 0U;
    }
    
    if (_find_device(timer_id) == NULL)
    {
        return 0U;
    }
    
    return _hw_timer_ops->get_count(timer_id);
}

/**
 * @brief 获取定时器状态
 * @param timer_id 定时器ID
 * @return 定时器状态，失败返回HWTIMER_STATE_STOPPED
 */
hwtimer_state_t hwtimer_get_state(uint32_t timer_id)
{
    struct hwtimer_device *dev;
    
    dev = _find_device(timer_id);
    if (dev == NULL)
    {
        return HWTIMER_STATE_STOPPED;
    }
    
    return dev->state;
}

/**
 * @brief 获取定时器最大周期值（微秒）
 * @param timer_id 定时器ID
 * @return 最大周期值（微秒），0表示不支持
 */
uint32_t hwtimer_get_max_period(uint32_t timer_id)
{
    if (_hw_timer_ops == NULL)
    {
        return 0U;
    }
    
    if (_hw_timer_ops->get_max_period == NULL)
    {
        return 0U;
    }
    
    return _hw_timer_ops->get_max_period(timer_id);
}

/**
 * @brief 获取定时器最小周期值（微秒）
 * @param timer_id 定时器ID
 * @return 最小周期值（微秒），0表示不支持
 */
uint32_t hwtimer_get_min_period(uint32_t timer_id)
{
    if (_hw_timer_ops == NULL)
    {
        return 0U;
    }
    
    if (_hw_timer_ops->get_min_period == NULL)
    {
        return 0U;
    }
    
    return _hw_timer_ops->get_min_period(timer_id);
}

/**
 * @brief 获取定时器分辨率（微秒）
 * @param timer_id 定时器ID
 * @return 分辨率（微秒），0表示不支持
 */
uint32_t hwtimer_get_resolution(uint32_t timer_id)
{
    if (_hw_timer_ops == NULL)
    {
        return 0U;
    }
    
    if (_hw_timer_ops->get_resolution == NULL)
    {
        return 0U;
    }
    
    return _hw_timer_ops->get_resolution(timer_id);
}

/**
 * @brief 定时器中断回调函数（由硬件层调用）
 * @param timer_id 定时器ID
 * @note 此函数应在硬件定时器中断服务程序中调用
 */
void hwtimer_irq_callback(uint32_t timer_id)
{
    struct hwtimer_device *dev;
    
    dev = _find_device(timer_id);
    if (dev == NULL)
    {
        return;
    }
    
    /* 调用用户回调函数 */
    if (dev->config.callback != NULL)
    {
        dev->config.callback(timer_id, dev->config.user_data);
    }
    
    /* 如果是单次触发模式，自动停止 */
    if (dev->config.mode == HWTIMER_MODE_ONESHOT)
    {
        dev->state = HWTIMER_STATE_STOPPED;
    }
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 查找定时器设备
 * @param timer_id 定时器ID
 * @return 设备指针，未找到返回NULL
 */
static struct hwtimer_device* _find_device(uint32_t timer_id)
{
    uint32_t i;
    
    for (i = 0U; i < _timer_device_count; i++)
    {
        if (_timer_devices[i].timer_id == timer_id)
        {
            return &_timer_devices[i];
        }
    }
    
    return NULL;
}

/**
 * @brief 创建定时器设备
 * @param timer_id 定时器ID
 * @return 设备指针，失败返回NULL
 */
static struct hwtimer_device* _create_device(uint32_t timer_id)
{
    struct hwtimer_device *dev;
    
    /* 检查是否超过最大数量 */
    if (_timer_device_count >= HWTIMER_MAX_DEVICES)
    {
        return NULL;
    }
    
    /* 获取空闲设备 */
    dev = &_timer_devices[_timer_device_count];
    
    /* 初始化设备 */
    dev->timer_id = timer_id;
    dev->state = HWTIMER_STATE_STOPPED;
    dev->config.period_us = 0U;
    dev->config.mode = HWTIMER_MODE_PERIODIC;
    dev->config.callback = NULL;
    dev->config.user_data = NULL;
    
    _timer_device_count++;
    
    return dev;
}

