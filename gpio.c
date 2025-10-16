/**
  ******************************************************************************
  * @file        : gpio.c
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2024-09-26
  * @brief       : GPIO driver implementation
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
#include "errno-base.h"
#include <assert.h>
#include <stddef.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/**
 * @brief GPIO operations structure instance
 * @note This pointer holds the hardware-specific GPIO operations
 */
static const struct gpio_ops* _hw_pin;

/* Exported variables  -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
/**
 * @brief Set GPIO pin mode and pull resistor
 * @param pin_id Pin identifier
 * @param mode Pin mode (INPUT/OUTPUT_PP/OUTPUT_OD)
 * @param pull_resistor Pull resistor configuration
 * @return None
 */
void gpio_set_mode(uint32_t pin_id, pin_mode_t mode, pin_pull_t pull_resistor)
{
    assert(_hw_pin->set_mode != NULL);
    
    _hw_pin->set_mode(pin_id, mode, pull_resistor);
}

/**
 * @brief Write digital value to GPIO pin
 * @param pin_id Pin identifier
 * @param value Digital value to write (0 or 1)
 * @return None
 */
void gpio_write(uint32_t pin_id, uint8_t value)
{
    assert(_hw_pin->write != NULL);
    
    _hw_pin->write(pin_id, value);
}

/**
 * @brief Read digital value from GPIO pin
 * @param pin_id Pin identifier
 * @return Digital value read from the pin (0 or 1)
 */
uint8_t gpio_read(size_t pin_id)
{
    assert(_hw_pin->read != NULL);
    
    return _hw_pin->read(pin_id);
}

/**
 * @brief Get GPIO pin identifier from name
 * @param name Pin name (e.g., "PA.0")
 * @return Pin identifier or -EINVAL if name is invalid
 */
int gpio_get(const char *name)
{
    if (name == NULL)
        return -EINVAL;
    
    assert(_hw_pin->get != NULL);
    
    return _hw_pin->get(name);
}

/**
 * @brief Attach interrupt handler to GPIO pin
 * @param pin_id Pin identifier
 * @param event Interrupt trigger event (RISING/FALLING/RISING_FALLING)
 * @param hdr Interrupt handler function
 * @param args Argument passed to the interrupt handler
 * @return 0 on success, -ENOSYS if operation is not supported
 */
int gpio_attach_irq(uint32_t pin_id, pin_event_t event, void (*hdr)(void *args), void *args)
{
    assert(_hw_pin != NULL);
    
    if (_hw_pin->attach_irq)
    {
        return _hw_pin->attach_irq(pin_id, event, hdr, args);
    }
    return -ENOSYS;
}

/**
 * @brief Detach interrupt handler from GPIO pin
 * @param pin_id Pin identifier
 * @return 0 on success, -ENOSYS if operation is not supported
 */
int gpio_detach_irq(uint32_t pin_id)
{
    assert(_hw_pin != NULL);
    
    if (_hw_pin->detach_irq)
    {
        return _hw_pin->detach_irq(pin_id);
    }
    return -ENOSYS;
}

/**
 * @brief Enable or disable GPIO interrupt
 * @param pin_id Pin identifier
 * @param enabled 1 to enable, 0 to disable
 * @return 0 on success, -ENOSYS if operation is not supported
 */
int gpio_irq_enable(uint32_t pin_id, uint32_t enabled)
{
    assert(_hw_pin != NULL);
    
    if (_hw_pin->irq_enable)
    {
        return _hw_pin->irq_enable(pin_id, enabled);
    }
    return -ENOSYS;
}

/**
 * @brief Register GPIO operations
 * @param ops Pointer to the GPIO operations structure
 * @return 0 on success, -EINVAL if ops is NULL
 */
int gpio_register(const struct gpio_ops *ops)
{
    if (ops == NULL)
        return -EINVAL;
    
    _hw_pin = ops;
    return 0;
}
/* Private functions ---------------------------------------------------------*/

