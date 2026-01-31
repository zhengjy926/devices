/**
  ******************************************************************************
  * @file        : wdg.c
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
#include "wdg.h"
#include "errno-base.h"
#include <string.h>

#define  LOG_TAG             "wdg"
#define  LOG_LVL             3
#include "log.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/


/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static LIST_HEAD(wdg_list);

/* Exported variables  -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
int wdg_register_device(struct wdg_device *wdg)
{
    if (wdg == NULL) {
        LOG_E("wdg_register_device: wdg is NULL!");
        return -EINVAL;
    }
    
    /* start是必需的操作 */
    if (wdg->ops == NULL || wdg->ops->start == NULL) {
        LOG_E("wdg_register_device: ops or start is NULL!");
        return -EINVAL;
    }
    
    if (wdg->min_timeout > wdg->max_timeout || wdg->max_timeout == 0) {
        LOG_E("wdg_register_device: min_timeout or max_timeout is error!");
        return -EINVAL;
    }
    
    if (wdg_find(wdg->name) != NULL) {
        LOG_E("wdg_register_device: name already exists!");
        return -EEXIST;
    }
    
    list_add_tail(&wdg->node, &wdg_list);
    
    return 0;
}

struct wdg_device *wdg_find(const char *name)
{
    list_t *pos;
    struct wdg_device *wdg;

    if (name == NULL) {
        LOG_E("wdg_find: name is NULL!");
        return NULL;
    }

    list_for_each(pos, &wdg_list) {
        wdg = list_entry(pos, struct wdg_device, node);
        if (strcmp(wdg->name, name) == 0) {
            return wdg;
        }
    }
    return NULL;
}

/**
 * @brief 启动看门狗
 * @param wdg 看门狗设备指针
 * @return 0成功，负值表示错误码
 */
int wdg_start(struct wdg_device *wdg)
{
    int ret = 0;
    
    if (wdg == NULL) {
        LOG_E("wdg_start: wdg is NULL!");
        return -EINVAL;
    }
    
    if (wdg->ops == NULL || wdg->ops->start == NULL) {
        LOG_E("wdg_start: ops or start is NULL!");
        return -ENOTSUPP;
    }
    
    if (!test_bit(WDOG_HW_RUNNING, &wdg->status)) {
        ret = wdg->ops->start(wdg);
        if (ret == 0) {
            set_bit(WDOG_HW_RUNNING, &wdg->status);
        }
    }
    
    return ret;
}

/**
 * @brief 停止看门狗
 * @param wdg 看门狗设备指针
 * @return 0成功，负值表示错误码
 */
int wdg_stop(struct wdg_device *wdg)
{
    int ret = 0;
    
    if (wdg == NULL) {
        LOG_E("wdg_stop: wdg is NULL!");
        return -EINVAL;
    }
    
    if (wdg->ops == NULL || wdg->ops->stop == NULL) {
        LOG_E("wdg_stop: ops or stop is NULL!");
        return -ENOTSUPP;
    }
    
    if (test_bit(WDOG_HW_RUNNING, &wdg->status)) {
        ret = wdg->ops->stop(wdg);
        if (ret == 0) {
            clear_bit(WDOG_HW_RUNNING, &wdg->status);
        }
    }
    
    return ret;
}

/**
 * @brief 喂狗（刷新看门狗）
 * @param wdg 看门狗设备指针
 * @return 0成功，负值表示错误码
 */
int wdg_feed(struct wdg_device *wdg)
{
    int ret = 0;
    
    if (wdg == NULL) {
        LOG_E("wdg_feed: wdg is NULL!");
        return -EINVAL;
    }
    
    if (wdg->ops == NULL || wdg->ops->feed == NULL) {
        LOG_E("wdg_feed: ops or feed is NULL!");
        return -ENOTSUPP;
    }
    
    return wdg->ops->feed(wdg);
}

/**
 * @brief 设置看门狗超时时间
 * @param wdg 看门狗设备指针
 * @param timeout 超时时间
 * @return 0成功，负值表示错误码
 */
int wdg_set_timeout(struct wdg_device *wdg, uint32_t timeout)
{
    int ret = 0;
    
    if (wdg == NULL) {
        LOG_E("wdg_set_timeout: wdg is NULL!");
        return -EINVAL;
    }
    
    if (wdg->ops == NULL || wdg->ops->set_timeout == NULL) {
        LOG_E("wdg_set_timeout: ops or set_timeout is NULL!");
        return -ENOTSUPP;
    }
    
    if (timeout < wdg->min_timeout || timeout > wdg->max_timeout) {
        LOG_W("wdg_set_timeout: timeout is invalid parameter!");
        return -EINVAL;
    }
    
    return wdg->ops->set_timeout(wdg, timeout);
}

/**
 * @brief 设置看门狗预超时时间
 * @param wdg 看门狗设备指针
 * @param timeout_ms 预超时时间（毫秒）
 * @return 0成功，负值表示错误码
 */
int wdg_set_pretimeout(struct wdg_device *wdg, uint32_t timeout_ms)
{
    if (wdg == NULL) {
        LOG_E("wdg_set_pretimeout: wdg is NULL!");
        return -EINVAL;
    }
    if (wdg->ops == NULL || wdg->ops->set_pretimeout == NULL) {
        LOG_E("wdg_set_pretimeout: ops or set_pretimeout is NULL!");
        return -ENOTSUPP;
    }
    return wdg->ops->set_pretimeout(wdg, timeout_ms);
}

/**
 * @brief 获取看门狗超时时间
 * @param wdg 看门狗设备指针
 * @return 超时时间（秒），失败返回负错误码
 */
int wdg_get_timeout(struct wdg_device *wdg, uint32_t *timeout)
{
    if (wdg == NULL || timeout == NULL) {
        LOG_E("wdg_get_timeout: wdg or timeout is NULL!");
        return -EINVAL;
    }
    
    *timeout = wdg->timeout;
    return 0;
}

/**
 * @brief 获取看门狗状态
 * @param wdg 看门狗设备指针
 * @param state 输出当前状态（见 WDOG_* 位定义）
 * @return 0成功，负值表示错误码
 */
int wdg_get_state(const struct wdg_device *wdg, uint32_t *state)
{
    if (wdg == NULL || state == NULL) {
        LOG_E("wdg_get_state: wdg or state is NULL!");
        return -EINVAL;
    }
    if (wdg->ops == NULL || wdg->ops->status == NULL) {
        LOG_E("wdg_get_state: ops or status is NULL!");
        return -ENOTSUPP;
    }
    *state = wdg->ops->status((struct wdg_device *)wdg);
    return 0;
}

/* Private functions ---------------------------------------------------------*/
