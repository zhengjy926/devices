/**
  ******************************************************************************
  * @file        : led_gpio.c
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2024-xx-xx
  * @brief       : GPIO LED驱动实现
  * @attattention: None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.实现基于GPIO的LED驱动后端
  *
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "led_gpio.h"
#include "gpio.h"
#include "errno-base.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static int32_t LED_GPIO_Init(led_t *self);
static int32_t LED_GPIO_On(led_t *self);
static int32_t LED_GPIO_Off(led_t *self);
static int32_t LED_GPIO_SetLevel(led_t *self, bool on);

static const led_ops_t gpio_led_ops = {
    .init           = LED_GPIO_Init,
    .on             = LED_GPIO_On,
    .off            = LED_GPIO_Off,
    .set_brightness = NULL,
};

/* Exported variables --------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
int32_t LED_GPIO_Register(led_t *self, led_gpio_ctx_t *ctx,
                          const char *name, bool active_level)
{
    uint8_t pin_id;
    int32_t ret;

    if ((self == NULL) || (ctx == NULL) || (name == NULL)) {
        return -ERR_INVAL;
    }

    ret = GPIO_GetPinId(name, &pin_id);
    if (ret != 0) {
        return ret;
    }

    /* 仅填充硬件上下文 */
    ctx->pin_id       = pin_id;
    ctx->active_level = active_level;

    /* 交由抽象层进行统一初始化流程 */
    return LED_Register(self, &gpio_led_ops, ctx);
}

/* Private functions ---------------------------------------------------------*/
static int32_t LED_GPIO_SetLevel(led_t *self, bool on)
{
    const led_gpio_ctx_t *ctx = (const led_gpio_ctx_t *)self->hw_ctx;
    uint8_t active = ctx->active_level ? 1U : 0U;
    uint8_t level = on ? active : (uint8_t)(1U - active);
    
    GPIO_Write(ctx->pin_id, level);
    return 0;
}

static int32_t LED_GPIO_Init(led_t *self)
{
    led_gpio_ctx_t *ctx = (led_gpio_ctx_t *)self->hw_ctx;
    PIN_Pull_e pull = ctx->active_level ? PIN_PULL_DOWN : PIN_PULL_UP;
    
    GPIO_SetMode(ctx->pin_id, PIN_OUTPUT_PP, pull);
    
    return LED_GPIO_SetLevel(self, false); /* 默认熄灭 */
}

static int32_t LED_GPIO_On(led_t *self)
{
    return LED_GPIO_SetLevel(self, true);
}

static int32_t LED_GPIO_Off(led_t *self)
{
    return LED_GPIO_SetLevel(self, false);
}



