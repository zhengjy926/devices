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
#include <stddef.h>

#define  LOG_TAG             "gpio"
#define  LOG_LVL             ELOG_LVL_DEBUG
#include "elog.h"

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
 * @brief Get GPIO pin identifier from name
 * @param name Pin name (e.g., "PA0")
 * @param pin_id Pointer to store the pin identifier
 * @return 0 on success, negative errno on failure
 */
int32_t GPIO_GetPinId(const char *name, uint8_t *pin_id)
{
    if ((name == NULL) || (pin_id == NULL)) {
        return -ERR_INVAL;
    }

    if ((_hw_pin == NULL) || (_hw_pin->get_pin_id == NULL))
    {
        return -ERR_NOSYS;
    }

    return _hw_pin->get_pin_id(name, pin_id);
}

/**
 * @brief Set GPIO pin mode and pull resistor
 * @param pin_id Pin identifier
 * @param mode Pin mode (INPUT/OUTPUT_PP/OUTPUT_OD)
 * @param pull_resistor Pull resistor configuration
 * @return None
 */
int32_t GPIO_SetMode(uint8_t pin_id, PIN_Mode_e mode, PIN_Pull_e pull_resistor)
{
    if ((_hw_pin == NULL) || (_hw_pin->set_mode == NULL))
    {
        return -ERR_INVAL;
    }

    return _hw_pin->set_mode(pin_id, mode, pull_resistor);
}

/**
 * @brief Write digital value to GPIO pin
 * @param pin_id Pin identifier
 * @param value Digital value to write (0 or 1)
 * @return None
 */
int32_t GPIO_Write(uint8_t pin_id, uint8_t value)
{
    if ((_hw_pin == NULL) || (_hw_pin->write == NULL))
    {
        return -ERR_INVAL;
    }

    return _hw_pin->write(pin_id, value);
}

/**
 * @brief Read digital value from GPIO pin
 * @param pin_id Pin identifier
 * @return Digital value read from the pin (0 or 1)
 */
int32_t GPIO_Read(uint8_t pin_id, uint8_t *value)
{
    if (value == NULL) {
        return -ERR_INVAL;
    }

    if ((_hw_pin == NULL) || (_hw_pin->read == NULL))
    {
        return -ERR_INVAL;
    }

    return _hw_pin->read(pin_id, value);
}

/**
 * @brief Attach interrupt handler to GPIO pin
 * @param pin_id Pin identifier
 * @param event Interrupt trigger event (RISING/FALLING/RISING_FALLING)
 * @param hdr Interrupt handler function
 * @param args Argument passed to the interrupt handler
 * @return 0 on success, -ERR_NOSYS if operation is not supported
 */
int32_t GPIO_AttachIrq(uint8_t pin_id, PIN_Event_e event, void (*hdr)(void *args), void *args)
{
    if ((_hw_pin == NULL) || (_hw_pin->attach_irq == NULL))
    {
        return -ERR_NOSYS;
    }
    return _hw_pin->attach_irq(pin_id, event, hdr, args);
}

/**
 * @brief Detach interrupt handler from GPIO pin
 * @param pin_id Pin identifier
 * @return 0 on success, -ERR_NOSYS if operation is not supported
 */
int32_t GPIO_DetachIrq(uint8_t pin_id)
{
    if ((_hw_pin == NULL) || (_hw_pin->detach_irq == NULL))
    {
        return -ERR_NOSYS;
    }
    return _hw_pin->detach_irq(pin_id);
}

/**
 * @brief Enable or disable GPIO interrupt
 * @param pin_id Pin identifier
 * @param enabled 1 to enable, 0 to disable
 * @return 0 on success, -ERR_NOSYS if operation is not supported
 */
int32_t GPIO_IrqEnable(uint8_t pin_id, uint32_t enabled)
{
    if ((_hw_pin == NULL) || (_hw_pin->irq_enable == NULL))
    {
        return -ERR_NOSYS;
    }
    return _hw_pin->irq_enable(pin_id, enabled);
}

/**
 * @brief Register GPIO operations
 * @param ops Pointer to the GPIO operations structure
 * @return 0 on success, -ERR_INVAL if ops is NULL
 */
int32_t GPIO_Register(const struct gpio_ops *ops)
{
    if (ops == NULL)
        return -ERR_INVAL;
    
    _hw_pin = ops;
    return 0;
}
/* Private functions ---------------------------------------------------------*/
