/**
  ******************************************************************************
  * @file        : gpio.c
  * @author      : ZJY
  * @version     : V1.0
  * @data        : 2024-09-26
  * @brief       : 
  * @attention   : None
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
static const struct gpio_ops* _hw_pin;
/* Exported variables  -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
void gpio_set_mode(size_t pin_id, PIN_MODE mode, PIN_PULL_RESISTOR pull_resistor)
{
    assert(_hw_pin->set_mode != NULL);
    
    _hw_pin->set_mode(pin_id, mode, pull_resistor);
}

void gpio_write(size_t pin_id, uint8_t value)
{
    assert(_hw_pin->write != NULL);
    
    _hw_pin->write(pin_id, value);
}

uint8_t gpio_read(size_t pin_id)
{
    assert(_hw_pin->read != NULL);
    
    return _hw_pin->read(pin_id);
}

int gpio_get(const char *name)
{
    if (name == NULL)
        return -EINVAL;
    
    assert(_hw_pin->get != NULL);
    
    return _hw_pin->get(name);
}

int gpio_attach_irq(size_t pin_id, uint32_t mode, void (*hdr)(void *args), void *args)
{
    assert(_hw_pin != NULL);
    
    if (_hw_pin->attach_irq)
    {
        return _hw_pin->attach_irq(pin_id, mode, hdr, args);
    }
    return -ENOSYS;
}

int gpio_detach_irq(size_t pin_id)
{
    assert(_hw_pin != NULL);
    
    if (_hw_pin->detach_irq)
    {
        return _hw_pin->detach_irq(pin_id);
    }
    return -ENOSYS;
}

int gpio_irq_enable(size_t pin_id, uint32_t enabled)
{
    assert(_hw_pin != NULL);
    
    if (_hw_pin->irq_enable)
    {
        return _hw_pin->irq_enable(pin_id, enabled);
    }
    return -ENOSYS;
}

int gpio_register(const struct gpio_ops *ops)
{
    if (ops == NULL)
        return -EINVAL;
    
    _hw_pin = ops;
    return 0;
}
/* Private functions ---------------------------------------------------------*/

