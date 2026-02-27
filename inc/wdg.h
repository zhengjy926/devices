/**
  ******************************************************************************
  * @file        : wdg.h
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
#ifndef __WDG_H__
#define __WDG_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "my_list.h"
#include "bitops.h"

/* Exported types ------------------------------------------------------------*/
struct wdg_device;

/**
 * @brief Bit numbers for status flags
 */
#define WDOG_ACTIVE                 (1U << 0)   /* Is the wdg running/active */
#define WDOG_NO_WAY_OUT		        (1U << 1)	/* Is 'nowayout' feature set ? */
#define WDOG_STOP_ON_REBOOT	        (1U << 2)	/* Should be stopped on reboot */
#define WDOG_HW_RUNNING		        (1U << 3)	/* True if HW wdg running */
#define WDOG_STOP_ON_UNREGISTER	    (1U << 4)	/* Should be stopped on unregister */
#define WDOG_NO_PING_ON_SUSPEND	    (1U << 5)	/* Ping worker should be stopped on suspend */

/**
 * @brief The wdg-devices operations
 */
struct wdg_ops {
    /* mandatory operations */
    int (*start)(struct wdg_device *);
    /* optional operations */
    int (*stop)(struct wdg_device *);
    int (*feed)(struct wdg_device *);
    uint32_t (*status)(struct wdg_device *);
    int (*set_timeout)(struct wdg_device *, uint32_t);
    int (*set_pretimeout)(struct wdg_device *, uint32_t);
};

/**
 * @brief 看门狗设备结构体
 */
struct wdg_device {
    const char *name;               /**< 看门狗名称 */
    const struct wdg_ops *ops;      /**< 该设备的操作函数指针 */
    uint32_t timeout;               /**< 超时时间（秒） */
    uint32_t min_timeout;           /**< 最小超时时间（秒） */
	uint32_t max_timeout;           /**< 最大超时时间（秒） */
    void *driver_data;
    uint32_t status;                /**< 当前状态（见 WDOG_* 位定义） */
    list_t node;
};

/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
int  wdg_register_device(struct wdg_device *wdg);
struct wdg_device *wdg_find(const char *name);
int  wdg_start(struct wdg_device *wdg);
int  wdg_stop(struct wdg_device *wdg);
int  wdg_feed(struct wdg_device *wdg);    
int  wdg_set_timeout(struct wdg_device *wdg, uint32_t timeout);
int  wdg_get_timeout(struct wdg_device *wdg, uint32_t *timeout);
int  wdg_set_pretimeout(struct wdg_device *wdg, uint32_t timeout_ms);
int  wdg_get_state(const struct wdg_device *wdg, uint32_t *state);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __WDG_H__ */




