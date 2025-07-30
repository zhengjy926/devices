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
#include "stimer.h"
#include "kfifo.h"
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static LIST_HEAD(key_list);                 /* Key device list head */
static stimer_t key_scan_timer = {0};       /* Key scan timer */

key_level_msg_t event_fifo_buf[16] = {0};
kfifo_t event_fifo;
/* Exported variables  -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void key_scan_timer_callback(void *arg);
static void key_fsm_handle(key_t *key);

/* Exported functions --------------------------------------------------------*/
int key_device_register(key_t *key)
{
    memset(key, 0, sizeof(key_t));
    
    key->state = KEY_STATE_NONE;
    key->debounce_time = KEY_DEBOUNCE_TIME;
    key->long_time = KEY_LONG_TIME;
    key->hold_time = KEY_HOLD_TIME;
    key->repeat_time = KEY_REPEAT_TIME;
    
    return 0;
}

int key_init(void)
{
    kfifo_init(&event_fifo, event_fifo_buf, sizeof(event_fifo_buf), sizeof(event_fifo_buf[0]));
    
    stimer_create(&key_scan_timer, KEY_SCAN_PERIOD_MS,
                    STIMER_AUTO_RELOAD, key_scan_timer_callback, NULL);
    return 0;
}

int key_get_event(key_event_msg_t *msg)
{
    return kfifo_out(&event_fifo, msg, 1);
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

    /* Start timer if not started */
    if (stimer_get_status(&key_scan_timer) == 0) {
        stimer_start(&key_scan_timer);
    }
}

/**
 * @brief  Stop key (Remove key from scan list)
 * @param  key: Key device pointer
 * @retval None
 */
void key_stop(key_t *key)
{
    if (!key)
        return;

    list_del(&key->node);
}

/* Private functions ---------------------------------------------------------*/
/**
 * @brief  Key scan timer callback function
 * @param  arg: User parameter
 * @retval None
 */
static void key_scan_timer_callback(void *arg)
{
    list_t *node;
    key_t *key;

    /* Traverse key list */
    for (node = key_list.next; node != &key_list; node = node->next) {
        key = (key_t *)((char *)node - offsetof(key_t, node));
        key_fsm_handle(key);
    }
}

/**
 * @brief  Key detection processing
 * @param  key: Key device pointer
 * @retval None
 */
static void key_fsm_handle(key_t *key)
{
    uint32_t level = key->read_state(key);
    
    /* Debounce processing when level changes */
    if (level != key->last_level) {
        if (++key->ticks >= key->debounce_time) {
            key->last_level = level;
            key->ticks = 0;
        }
        return;
    }

    /* Key state machine processing */
    key_event_msg_t msg;
    msg.id = key->id;
    
    switch (key->state) {
        case KEY_STATE_NONE:  /* Idle state */
            if (level) {
                key->state = KEY_STATE_DOWN;
                key->ticks = 0;
                key->repeat_count = 1;  /* First press, start counting */
                msg.event = KEY_EVENT_DOWN;
                kfifo_in(&event_fifo, &msg, 1);
            }
            break;

        case KEY_STATE_DOWN:  /* Pressed state */
            if (!level) {
                /* Short press release */
                key->state = KEY_STATE_UP;
                key->ticks = 0;
                if (key->repeat_count == 1) {
                    /* Trigger UP event on first press release */
                    msg.event = KEY_EVENT_UP;
                    kfifo_in(&event_fifo, &msg, 1);
                }
            } else if (++key->ticks >= key->long_time) {
                /* Long press time reached */
                key->state = KEY_STATE_LONG;
                key->ticks = 0;
                key->repeat_count = 0;  /* Enter long press state, clear repeat count */
                msg.event = KEY_EVENT_LONG_START;
                kfifo_in(&event_fifo, &msg, 1);
            }
            break;

        case KEY_STATE_LONG:  /* Long press state */
            if (!level) {
                /* Long press release */
                key->state = KEY_STATE_NONE;
                key->ticks = 0;
                key->repeat_count = 0;
                msg.event = KEY_EVENT_LONG_FREE;
                kfifo_in(&event_fifo, &msg, 1);
            } else {
                /* Long press hold */
                if (++key->ticks >= key->hold_time) {
                    key->ticks = 0;
                    msg.event = KEY_EVENT_LONG_HOLD;
                    kfifo_in(&event_fifo, &msg, 1);
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
                    msg.event = KEY_EVENT_REPEAT;
                    kfifo_in(&event_fifo, &msg, 1);
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

