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
#define  LOG_LVL             ELOG_LVL_DEBUG
#include "elog.h"

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
        log_e("wdg_register_device: wdg is NULL!");
        return -ERR_INVAL;
    }
    
    /* start是必需的操作 */
    if (wdg->ops == NULL || wdg->ops->start == NULL) {
        log_e("wdg_register_device: ops or start is NULL!");
        return -ERR_INVAL;
    }
    
    if (wdg->min_timeout > wdg->max_timeout || wdg->max_timeout == 0) {
        log_e("wdg_register_device: min_timeout or max_timeout is error!");
        return -ERR_INVAL;
    }
    
    if (wdg_find(wdg->name) != NULL) {
        log_e("wdg_register_device: name already exists!");
        return -ERR_EXIST;
    }
    
    list_add_tail(&wdg->node, &wdg_list);
    
    return 0;
}

struct wdg_device *wdg_find(const char *name)
{
    list_t *pos;
    struct wdg_device *wdg;

    if (name == NULL) {
        log_e("wdg_find: name is NULL!");
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
        log_e("wdg_start: wdg is NULL!");
        return -ERR_INVAL;
    }
    
    if (wdg->ops == NULL || wdg->ops->start == NULL) {
        log_e("wdg_start: ops or start is NULL!");
        return -ERR_NOTSUPP;
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
        log_e("wdg_stop: wdg is NULL!");
        return -ERR_INVAL;
    }
    
    if (wdg->ops == NULL || wdg->ops->stop == NULL) {
        log_e("wdg_stop: ops or stop is NULL!");
        return -ERR_NOTSUPP;
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
        log_e("wdg_feed: wdg is NULL!");
        return -ERR_INVAL;
    }
    
    if (wdg->ops == NULL || wdg->ops->feed == NULL) {
        log_e("wdg_feed: ops or feed is NULL!");
        return -ERR_NOTSUPP;
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
        log_e("wdg_set_timeout: wdg is NULL!");
        return -ERR_INVAL;
    }
    
    if (wdg->ops == NULL || wdg->ops->set_timeout == NULL) {
        log_e("wdg_set_timeout: ops or set_timeout is NULL!");
        return -ERR_NOTSUPP;
    }
    
    if (timeout < wdg->min_timeout || timeout > wdg->max_timeout) {
        log_w("wdg_set_timeout: timeout is invalid parameter!");
        return -ERR_INVAL;
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
        log_e("wdg_set_pretimeout: wdg is NULL!");
        return -ERR_INVAL;
    }
    if (wdg->ops == NULL || wdg->ops->set_pretimeout == NULL) {
        log_e("wdg_set_pretimeout: ops or set_pretimeout is NULL!");
        return -ERR_NOTSUPP;
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
        log_e("wdg_get_timeout: wdg or timeout is NULL!");
        return -ERR_INVAL;
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
        log_e("wdg_get_state: wdg or state is NULL!");
        return -ERR_INVAL;
    }
    if (wdg->ops == NULL || wdg->ops->status == NULL) {
        log_e("wdg_get_state: ops or status is NULL!");
        return -ERR_NOTSUPP;
    }
    *state = wdg->ops->status((struct wdg_device *)wdg);
    return 0;
}

/* Private functions ---------------------------------------------------------*/
