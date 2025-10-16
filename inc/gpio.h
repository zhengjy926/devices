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

/**
  Pin Id          | Hardware Resource
  :---------------|:-----------------------
    0 ..  15      | PORTA 0..15
   16 ..  31      | PORTB 0..15
   32 ..  47      | PORTC 0..15
   48 ..  63      | PORTD 0..15
   64 ..  79      | PORTE 0..15
   80 ..  95      | PORTF 0..15
   96 .. 111      | PORTG 0..15
  112 .. 127      | PORTH 0..15
  128 .. 143      | PORTI 0..15
  144 .. 159      | PORTJ 0..15
  160 .. 175      | PORTK 0..15
  176 .. 191      | PORTM 0..15
  192 .. 207      | PORTN 0..15
  208 .. 223      | PORTO 0..15
  224 .. 239      | PORTP 0..15
  240 .. 255      | PORTZ 0..15
*/
/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported define -----------------------------------------------------------*/
#define PIN_IRQ_PIN_NONE                -1

/* Exported typedef ----------------------------------------------------------*/
/**
 * @brief GPIO pin mode configurations
 */
typedef enum {
    PIN_INPUT,                  /**< Configure pin as input */
    PIN_OUTPUT_PP,              /**< Configure pin as push-pull output */
    PIN_OUTPUT_OD               /**< Configure pin as open-drain output */
} pin_mode_t;

/**
 * @brief GPIO pull resistor configurations
 */
typedef enum {
    PIN_PULL_NONE,              /**< No pull-up and pull-down resistor */
    PIN_PULL_UP,                /**< pull-up resistor */
    PIN_PULL_DOWN               /**< pull-down resistor */
} pin_pull_t;


typedef enum {
    PIN_EVENT_RISING_EDGE,    /**< Rising-edge */
    PIN_EVENT_FALLING_EDGE,   /**< Falling-edge */
    PIN_EVENT_EITHER_EDGE     /**< Either edge (rising and falling) */
} pin_event_t;

/**
 * @brief GPIO interrupt handler structure
 */
struct pin_irq_hdr {
    int16_t         pin;         /**< GPIO pin number */
    pin_event_t     event;       /**< Interrupt trigger event */
    void (*hdr)(void *args);     /**< Interrupt handler function */
    void             *args;      /**< Argument passed to the interrupt handler */
};

struct gpio_ops {
    void    (*set_mode)     (uint32_t pin_id, pin_mode_t mode, pin_pull_t pull_resistor);
    void    (*write)        (uint32_t pin_id, uint8_t value);
    uint8_t (*read)         (uint32_t pin_id);
    int     (*attach_irq)   (uint32_t pin_id, pin_event_t event, void (*hdr)(void *args), void *args);
    int     (*detach_irq)   (uint32_t pin_id);
    int     (*irq_enable)   (uint32_t pin_id, uint32_t enabled);
    int     (*get)          (const char *name);
};

/* Exported macro ------------------------------------------------------------*/

/* Exported variable prototypes ----------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/
int     gpio_get        (const char *name);
void    gpio_set_mode   (uint32_t pin_id, pin_mode_t mode, pin_pull_t pull_resistor);
void    gpio_write      (uint32_t pin_id, uint8_t value);
uint8_t gpio_read       (uint32_t pin_id);
int     gpio_attach_irq (uint32_t pin_id, pin_event_t event, void (*hdr)(void *args), void *args);
int     gpio_detach_irq (uint32_t pin_id);
int     gpio_irq_enable (uint32_t pin_id, uint32_t enabled);
int     gpio_register   (const struct gpio_ops *ops);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GPIO_H__ */
