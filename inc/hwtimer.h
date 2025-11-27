/**
  ******************************************************************************
  * @file        : hwtimer.h
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2025-01-XX
  * @brief       : 硬件定时器驱动接口定义
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.添加硬件定时器驱动接口
  *
  *
  ******************************************************************************
  */
#ifndef __HWTIMER_H__
#define __HWTIMER_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "errno-base.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief 定时器模式枚举
 */
typedef enum {
    HWTIMER_MODE_ONESHOT,      /**< 单次触发模式 */
    HWTIMER_MODE_PERIODIC      /**< 周期性触发模式 */
} hwtimer_mode_t;

/**
 * @brief 定时器状态枚举
 */
typedef enum {
    HWTIMER_STATE_STOPPED,     /**< 定时器已停止 */
    HWTIMER_STATE_RUNNING      /**< 定时器运行中 */
} hwtimer_state_t;

/**
 * @brief 定时器回调函数类型
 * @param timer_id 定时器ID
 * @param user_data 用户数据指针
 */
typedef void (*hwtimer_callback_t)(uint32_t timer_id, void *user_data);

/**
 * @brief 定时器配置结构体
 */
struct hwtimer_config {
    uint32_t period_us;         /**< 定时器周期（微秒） */
    hwtimer_mode_t mode;        /**< 定时器模式 */
    hwtimer_callback_t callback;/**< 定时器回调函数 */
    void *user_data;            /**< 用户数据指针 */
};

/**
 * @brief 定时器设备结构体
 */
struct hwtimer_device {
    uint32_t timer_id;           /**< 定时器ID */
    hwtimer_state_t state;       /**< 定时器状态 */
    struct hwtimer_config config;/**< 定时器配置 */
};

/**
 * @brief 硬件定时器操作函数结构体
 */
struct hwtimer_ops {
    /**
     * @brief 初始化定时器硬件
     * @param timer_id 定时器ID
     * @return 0成功，负值表示错误码
     */
    int (*init)(uint32_t timer_id);
    
    /**
     * @brief 反初始化定时器硬件
     * @param timer_id 定时器ID
     * @return 0成功，负值表示错误码
     */
    int (*deinit)(uint32_t timer_id);
    
    /**
     * @brief 启动定时器
     * @param timer_id 定时器ID
     * @param period_us 周期（微秒）
     * @param mode 定时器模式
     * @return 0成功，负值表示错误码
     */
    int (*start)(uint32_t timer_id, uint32_t period_us, hwtimer_mode_t mode);
    
    /**
     * @brief 停止定时器
     * @param timer_id 定时器ID
     * @return 0成功，负值表示错误码
     */
    int (*stop)(uint32_t timer_id);
    
    /**
     * @brief 设置定时器周期
     * @param timer_id 定时器ID
     * @param period_us 周期（微秒）
     * @return 0成功，负值表示错误码
     */
    int (*set_period)(uint32_t timer_id, uint32_t period_us);
    
    /**
     * @brief 获取定时器当前计数值
     * @param timer_id 定时器ID
     * @return 当前计数值，失败返回0
     */
    uint32_t (*get_count)(uint32_t timer_id);
    
    /**
     * @brief 获取定时器最大周期值（微秒）
     * @param timer_id 定时器ID
     * @return 最大周期值（微秒）
     */
    uint32_t (*get_max_period)(uint32_t timer_id);
    
    /**
     * @brief 获取定时器最小周期值（微秒）
     * @param timer_id 定时器ID
     * @return 最小周期值（微秒）
     */
    uint32_t (*get_min_period)(uint32_t timer_id);
    
    /**
     * @brief 获取定时器分辨率（微秒）
     * @param timer_id 定时器ID
     * @return 分辨率（微秒），0表示不支持
     */
    uint32_t (*get_resolution)(uint32_t timer_id);
};

/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

int hwtimer_register(const struct hwtimer_ops *ops);

int hwtimer_init(uint32_t timer_id);

int hwtimer_deinit(uint32_t timer_id);

int hwtimer_config(uint32_t timer_id, const struct hwtimer_config *config);

int hwtimer_start(uint32_t timer_id);

int hwtimer_stop(uint32_t timer_id);

int hwtimer_set_period(uint32_t timer_id, uint32_t period_us);

uint32_t hwtimer_get_count(uint32_t timer_id);

hwtimer_state_t hwtimer_get_state(uint32_t timer_id);

uint32_t hwtimer_get_max_period(uint32_t timer_id);

uint32_t hwtimer_get_min_period(uint32_t timer_id);

uint32_t hwtimer_get_resolution(uint32_t timer_id);

void hwtimer_irq_callback(uint32_t timer_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __HWTIMER_H__ */

