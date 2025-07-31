/**
  ******************************************************************************
  * @file        : gpio_key.h
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2024-09-26
  * @brief       : GPIO Key Driver Implementation
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : Implementation of GPIO-based key driver
  ******************************************************************************
  */
#ifndef __GPIO_KEY_H__
#define __GPIO_KEY_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "key.h"

/* Exported define -----------------------------------------------------------*/

/* Exported typedef ----------------------------------------------------------*/
/* 轮询式GPIO按键的私有配置结构体 */ 
typedef struct {
    uint16_t      pin_id;                   // GPIO pin 引脚编号
    uint8_t       active_low;               // 是否为低电平有效
} gpio_key_cfg_t;

typedef enum {
    GPIO_KEY_MODE_POLL,
    GPIO_KEY_MODE_IRQ
} gpio_key_mode_t;

/* Exported macro ------------------------------------------------------------*/

/* Exported variable prototypes ----------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/
int gpio_key_register(key_t *dev, uint8_t id, gpio_key_cfg_t* cfg);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GPIO_KEY_H__ */

