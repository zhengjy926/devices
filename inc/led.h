/**
 ******************************************************************************
 * @file    : led.h
 * @author  : ZJY
 * @version : V1.0
 * @date    : 2026-05-09
 * @brief   : LED 抽象接口（ops + 实例状态）
 * @attention: None
 ******************************************************************************
 * @history :
 * V1.0 : 1. 初始版本
 ******************************************************************************
 */
#ifndef __LED_H__
#define __LED_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Exported types ------------------------------------------------------------*/
typedef struct led_ops led_ops_t;
typedef struct led     led_t;

struct led_ops {
    int32_t (*init)          (led_t* self);
    int32_t (*on)            (led_t* self);
    int32_t (*off)           (led_t* self);
    int32_t (*set_brightness)(led_t* self, uint8_t brightness);
};

struct led {
    const   led_ops_t* ops;
    bool    is_on;
    uint8_t brightness;
    void*   hw_ctx;
};

/* Exported constants --------------------------------------------------------*/
#define LED_BRIGHTNESS_MAX                      (100U)

/* Exported macros -----------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
int32_t LED_Register(led_t *self, const led_ops_t *ops, void *hw_ctx);
int32_t LED_On(led_t *self);
int32_t LED_Off(led_t *self);
int32_t LED_SetBrightness(led_t *self, uint8_t brightness);
bool    LED_IsOn(led_t *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __LED_H__ */
