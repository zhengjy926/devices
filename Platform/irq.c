/**
  ******************************************************************************
  * @file        : xxxx.c
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 20xx-xx-xx
  * @brief       :
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.xxx
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "irq.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
volatile uint32_t interrupt_nest = 0;
/* Exported variables  -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
/**
 * @brief This function will be invoked by BSP, when enter interrupt service routine
 *
 * @note Please don't invoke this routine in application
 *
 * @see interrupt_leave
 */
void interrupt_enter(void)
{
    atomic_add(&(interrupt_nest), 1);
}

/**
 * @brief This function will be invoked by BSP, when leave interrupt service routine
 *
 * @note Please don't invoke this routine in application
 *
 * @see interrupt_enter
 */
void interrupt_leave(void)
{
    atomic_sub(&(interrupt_nest), 1);
}

/**
 * @brief This function will return the nest of interrupt.
 *
 * User application can invoke this function to get whether current
 * context is interrupt context.
 *
 * @return the number of nested interrupts.
 */
uint8_t interrupt_get_nest(void)
{
    uint8_t ret;
    base_t level;

    __disable_irq();
    ret = atomic_load(&interrupt_nest);
    __enable_irq();

    return ret;
}
/* Private functions ---------------------------------------------------------*/

