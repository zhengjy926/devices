/**
  ******************************************************************************
  * @file        : watchdog.h
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2025-01-XX
  * @brief       : 看门狗驱动接口定义
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.添加看门狗驱动接口
  ******************************************************************************
  */
#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief 看门狗状态枚举
 */
typedef enum {
    WATCHDOG_STATE_STOPPED,    /**< 看门狗已停止 */
    WATCHDOG_STATE_RUNNING     /**< 看门狗运行中 */
} watchdog_state_t;

/**
 * @brief 看门狗设备结构体
 */
struct watchdog_device {
    const char *name;            /**< 看门狗名称 */
    watchdog_state_t state;     /**< 看门狗状态 */
    uint32_t timeout_ms;        /**< 超时时间（毫秒） */
    const struct watchdog_ops *ops; /**< 该设备的操作函数指针 */
};

/**
 * @brief 硬件看门狗操作函数结构体
 */
struct watchdog_ops {
    /* mandatory operations */
    int (*start)(struct watchdog_device *dev);
    /* optional operations */
    int (*stop)(struct watchdog_device *dev);
    int (*ping)(struct watchdog_device *dev);
    uint32_t (*status)(struct watchdog_device *);
    int (*set_timeout)(struct watchdog_device *dev, uint32_t timeout_ms);
    int (*set_pretimeout)(struct watchdog_device *, uint32_t timeout_ms);
    uint32_t (*get_timeout)(const struct watchdog_device *dev);
    uint32_t (*get_max_timeout)(const struct watchdog_device *dev);
    uint32_t (*get_min_timeout)(const struct watchdog_device *dev);
};

/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

int watchdog_register(struct watchdog_device *dev, const struct watchdog_ops *ops);
struct watchdog_device* watchdog_find(const char *name);
int watchdog_set_timeout(struct watchdog_device *dev, uint32_t timeout_ms);
int watchdog_start(struct watchdog_device *dev);
int watchdog_stop(struct watchdog_device *dev);
int watchdog_ping(struct watchdog_device *dev);
uint32_t watchdog_status(struct watchdog_device *dev);
int watchdog_set_pretimeout(struct watchdog_device *dev, uint32_t timeout_ms);
uint32_t watchdog_get_timeout(const struct watchdog_device *dev);
uint32_t watchdog_get_max_timeout(const struct watchdog_device *dev);
uint32_t watchdog_get_min_timeout(const struct watchdog_device *dev);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __WATCHDOG_H__ */




