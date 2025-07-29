/**
  ******************************************************************************
  * @file        : key_core.c
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2024-09-26
  * @brief       : xxx
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "key.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static LIST_HEAD(key_list);                 /* Key device list head */

/* Exported variables  -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
int key_device_register(key_t *key)
{
    return 0;
}

int key_init(void)
{
    
    return 0;
}

/**
 * @brief  Start key (Add key to scan list)
 * @param  key: Key device pointer
 * @retval None
 */
void key_start(key_t *key)
{
    if (!key)
        return;
    
    list_add_tail(&key->node, &key_list);
}

/* Private functions ---------------------------------------------------------*/
/**
 * @brief  Key scan timer callback function
 * @param  arg: User parameter
 * @retval None
 */
static void key_scan_timer_callback(void *arg)
{
    
}

/**
 * @brief  Key detection processing
 * @param  key: Key device pointer
 * @retval None
 */
void key_fsm_handle(key_t *key)
{
    uint8_t level = key->current_level;
    
    /* Debounce processing when level changes */
    if (level != key->last_level) {
        if (++key->ticks >= key->debounce_time) {
            key->last_level = level;
            key->ticks = 0;
        }
        return;
    }

    /* Key state machine processing */
    switch (key->state) {
        case KEY_STATE_NONE:  /* Idle state */
            if (level) {
                key->state = KEY_STATE_DOWN;
                key->ticks = 0;
                key->repeat_count = 1;  /* First press, start counting */
                // 
            }
            break;

        case KEY_STATE_DOWN:  /* Pressed state */
            if (!level) {
                /* Short press release */
                key->state = KEY_STATE_UP;
                key->ticks = 0;
                if (key->repeat_count == 1) {
                    /* Trigger UP event on first press release */
                    
                }
            } else if (++key->ticks >= key->long_time) {
                /* Long press time reached */
                key->state = KEY_STATE_LONG;
                key->ticks = 0;
                key->repeat_count = 0;  /* Enter long press state, clear repeat count */
                
            }
            break;

        case KEY_STATE_LONG:  /* Long press state */
            if (!level) {
                /* Long press release */
                key->state = KEY_STATE_NONE;
                key->ticks = 0;
                key->repeat_count = 0;
                
            } else {
                /* Long press hold */
                if (++key->ticks >= key->hold_time) {
                    key->ticks = 0;
                    
                }
            }
            break;

        case KEY_STATE_UP:    /* Released state */
            if (level) {
                /* Press again within repeat interval */
                key->state = KEY_STATE_DOWN;
                key->ticks = 0;
                key->repeat_count++;  /* Increment repeat count */
            } else if (++key->ticks >= key->repeat_time) {
                /* Repeat interval exceeded, repeat ends */
                if (key->repeat_count > 1) {
                    
                }
                key->state = KEY_STATE_NONE;
                key->ticks = 0;
                key->repeat_count = 0;
            }
            break;

        default:
            key->state = KEY_STATE_NONE;
            key->ticks = 0;
            key->repeat_count = 0;
            break;
    }
}
/* Private functions ---------------------------------------------------------*/

