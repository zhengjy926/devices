/**
  ******************************************************************************
  * @file        : led_gpio.h
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2024-xx-xx
  * @brief       : GPIO LED驱动头文件
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.实现基于GPIO的LED驱动后端
  ******************************************************************************
  */
#ifndef __LED_GPIO_H__
#define __LED_GPIO_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "led.h"

/* Exported define -----------------------------------------------------------*/

/* Exported typedef ----------------------------------------------------------*/
/**
 * @brief GPIO LED设备结构体
 */
typedef struct {
    uint8_t  pin_id;                     /**< GPIO 引脚编号 */
    bool     active_level;               /**< 点亮电平：true=高电平点亮，false=低电平点亮 */
} led_gpio_ctx_t;

/* Exported macro ------------------------------------------------------------*/

/* Exported variable prototypes ----------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/
int32_t LED_GPIO_Register(led_t *self, led_gpio_ctx_t *ctx,
                          const char *name, bool active_level);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __LED_GPIO_H__ */
