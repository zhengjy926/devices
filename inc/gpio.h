/**
  ******************************************************************************
  * @file        : gpio.h
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2024-09-26
  * @brief       : GPIO (General Purpose Input/Output) driver interface
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.xxx
  *
  *
  ******************************************************************************
  */
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported define -----------------------------------------------------------*/

/* Exported typedef ----------------------------------------------------------*/
/**
 * @brief GPIO pin mode configurations
 */
typedef enum {
    PIN_INPUT,                  /**< Configure pin as input */
    PIN_OUTPUT_PP,              /**< Configure pin as push-pull output */
    PIN_OUTPUT_OD               /**< Configure pin as open-drain output */
} PIN_Mode_e;

/**
 * @brief GPIO pull resistor configurations
 */
typedef enum {
    PIN_PULL_NONE,              /**< No pull-up and pull-down resistor */
    PIN_PULL_UP,                /**< pull-up resistor */
    PIN_PULL_DOWN               /**< pull-down resistor */
} PIN_Pull_e;


typedef enum {
    PIN_EVENT_RISING_EDGE,    /**< Rising-edge */
    PIN_EVENT_FALLING_EDGE,   /**< Falling-edge */
    PIN_EVENT_EITHER_EDGE     /**< Either edge (rising and falling) */
} PIN_Event_e;

/**
 * @brief GPIO interrupt handler structure
 */
struct pin_irq_hdr {
    int16_t         pin;         /**< GPIO pin number */
    PIN_Event_e     event;       /**< Interrupt trigger event */
    void (*hdr)(void *args);     /**< Interrupt handler function */
    void             *args;      /**< Argument passed to the interrupt handler */
};

struct gpio_ops {
    int32_t (*set_mode)     (uint8_t pin_id, PIN_Mode_e mode, PIN_Pull_e pull_resistor);
    int32_t (*write)        (uint8_t pin_id, uint8_t value);
    int32_t (*read)         (uint8_t pin_id, uint8_t *value);
    int32_t (*attach_irq)   (uint8_t pin_id, PIN_Event_e event, void (*hdr)(void *args), void *args);
    int32_t (*detach_irq)   (uint8_t pin_id);
    int32_t (*irq_enable)   (uint8_t pin_id, uint32_t enabled);
    int32_t (*get_pin_id)   (const char *name, uint8_t *pin_id);
};

/* Exported macro ------------------------------------------------------------*/

/* Exported variable prototypes ----------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/
int32_t GPIO_GetPinId     (const char *name, uint8_t *pin_id);
int32_t GPIO_SetMode      (uint8_t pin_id, PIN_Mode_e mode, PIN_Pull_e pull_resistor);
int32_t GPIO_Write        (uint8_t pin_id, uint8_t value);
int32_t GPIO_Read         (uint8_t pin_id, uint8_t *value);
int32_t GPIO_AttachIrq    (uint8_t pin_id, PIN_Event_e event, void (*hdr)(void *args), void *args);
int32_t GPIO_DetachIrq    (uint8_t pin_id);
int32_t GPIO_IrqEnable    (uint8_t pin_id, uint32_t enabled);
int32_t GPIO_Register     (const struct gpio_ops *ops);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GPIO_H__ */
