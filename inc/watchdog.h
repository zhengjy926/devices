/**
  ******************************************************************************
  * @file        : watchdog.h
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 20xx-xx-xx
  * @brief       : 
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.xxx
  ******************************************************************************
  */
#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported define -----------------------------------------------------------*/
#define DEVICE_CTRL_WDT_GET_TIMEOUT    ((29 * 0x100) + 1) /* get timeout(in seconds) */
#define DEVICE_CTRL_WDT_SET_TIMEOUT    ((29 * 0x100) + 2) /* set timeout(in seconds) */
#define DEVICE_CTRL_WDT_GET_TIMELEFT   ((29 * 0x100) + 3) /* get the left time before reboot(in seconds) */
#define DEVICE_CTRL_WDT_REFRESH        ((29 * 0x100) + 4) /* refresh watchdog */
#define DEVICE_CTRL_WDT_START          ((29 * 0x100) + 5) /* start watchdog */
#define DEVICE_CTRL_WDT_STOP           ((29 * 0x100) + 6) /* stop watchdog */
/* Exported typedef ----------------------------------------------------------*/
struct watchdog_device;
typedef struct watchdog_device watchdog_t;

struct watchdog_ops
{
    int (*init)(watchdog_t *wdt);
    int (*control)(watchdog_t *wdt, int cmd, void *arg);
};

struct watchdog_device
{
    const struct watchdog_ops *ops;
};
/* Exported macro ------------------------------------------------------------*/
/* Exported variable prototypes ----------------------------------------------*/
/* Exported function prototypes ----------------------------------------------*/
int hw_watchdog_register(watchdog_t *wdt,
                         const char *name,
                         uint32_t    flag,
                         void       *data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __WATCHDOG_H__ */

