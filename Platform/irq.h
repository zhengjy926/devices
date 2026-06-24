/**
  ******************************************************************************
  * @file        : xxx.h
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
#ifndef __IRQ_H__
#define __IRQ_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported define -----------------------------------------------------------*/

/* Exported typedef ----------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported variable prototypes ----------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/
void     interrupt_enter(void);
void     interrupt_leave(void);
uint8_t  interrupt_get_nest(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __IRQ_H__ */

