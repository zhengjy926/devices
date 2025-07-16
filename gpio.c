/**
  ******************************************************************************
  * @file        : xxxx.c
  * @author      : ZJY
  * @version     : V1.0
  * @data        : 20xx-xx-xx
  * @brief       : 
  * @attattention: None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.xxx
  *
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "gpio.h"
#include <assert.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static struct gpio_ops _hw_pin;
/* Exported variables  -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
void gpio_set_mode(size_t pin_id, size_t mode, size_t pull_resistor)
{
    assert(_hw_pin.set_mode != NULL);
    
    _hw_pin.set_mode(pin_id, mode, pull_resistor);
}

void gpio_write(size_t pin_id, uint8_t value)
{
    assert(_hw_pin.write != NULL);
    
    _hw_pin.write(pin_id, value);
}

uint8_t gpio_read(size_t pin_id)
{
    assert(_hw_pin.read != NULL);
    return _hw_pin.read(pin_id);
}

int gpio_get(const char *name)
{
    if (name == NULL)
        return -EINVAL;
    
    assert(_hw_pin.get != NULL);
    
    return _hw_pin.get(name);
}

int gpio_register(const struct gpio_ops *ops)
{
    if (ops == NULL)
        return -EINVAL;
    
    _hw_pin = *ops;
    
    return 0;
}
/* Private functions ---------------------------------------------------------*/

