/**
  ******************************************************************************
  * @file        : led_core.c
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2025-07-15
  * @brief       : LED核心框架实现
  * @attention   : xxx
  ******************************************************************************
  * @history     :
  *         V1.0 : 
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "led.h"
#include "errno-base.h"

#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Exported variables  -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
int32_t LED_Register(led_t *self, const led_ops_t *ops, void *hw_ctx)
{
    int32_t ret = 0;

    if ((self == NULL) || (ops == NULL) || (hw_ctx == NULL)) {
        return -ERR_INVAL;
    }

    if ((ops->init == NULL) || (ops->on == NULL) || (ops->off == NULL)) {
        return -ERR_INVAL;
    }
    
    memset(self, 0, sizeof(led_t));

    /* 临时挂载 hw_ctx 供 init 使用，init 成功后再绑定 ops */
    self->hw_ctx = hw_ctx;
    ret = ops->init(self);
    if (ret != 0) {
        memset(self, 0, sizeof(led_t));
        return ret;
    }

    self->ops = ops;
    return 0;
}

int32_t LED_On(led_t *self)
{
    int32_t ret = 0;

    if ((self == NULL) || (self->ops == NULL)) {
        return -ERR_INVAL;
    }

    if (self->is_on) {
        return 0;
    }

    ret = self->ops->on(self);
    if (ret == 0) {
        self->is_on = true;
        self->brightness = LED_BRIGHTNESS_MAX;
    } else {
        ret = -ERR_IO;
    }
    return ret;
}

int32_t LED_Off(led_t *self)
{
    int32_t ret = 0;

    if ((self == NULL) || (self->ops == NULL)) {
        return -ERR_INVAL;
    }

    if (!self->is_on) {
        return 0;
    }

    ret = self->ops->off(self);
    if (ret == 0) {
        self->is_on = false;
        self->brightness = 0;
    } else {
        ret = -ERR_IO;
    }
    return ret;
}

int32_t LED_SetBrightness(led_t *self, uint8_t brightness)
{
    int32_t ret = 0;

    if ((self == NULL) || (self->ops == NULL)) {
        return -ERR_INVAL;
    }

    if (self->ops->set_brightness == NULL) {
        return -ERR_NOTSUPP;
    }

    if (brightness > LED_BRIGHTNESS_MAX) {
        return -ERR_INVAL;
    }

    if (brightness == self->brightness) {
        return 0;
    }

    ret = self->ops->set_brightness(self, brightness);
    if (ret == 0) {
        self->brightness = brightness;
        self->is_on = (brightness > 0U);
    } else {
        ret = -ERR_IO;
    }
    return ret;
}

bool LED_IsOn(led_t *self)
{
    if (self == NULL) {
        return false;
    }

    return self->is_on;
}

/* Private functions ---------------------------------------------------------*/

