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
static void key_fsm_handle(key_t *key);

/* Exported functions --------------------------------------------------------*/
int key_device_register(key_t *key)
{
    key->id
    
    return 0;
}

/**
 * @brief  Find key by name
 * @param  name: Key name
 * @retval Key device pointer
 */
key_t *key_find(const char *name)
{
    for (uint32_t i = 0; i < sizeof(key) / sizeof(key[0]); i++) {
        if (strcmp(key[i].key_name, name) == 0)
            return &key[i];
    }
    return NULL;
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

/**
 * @brief  Attach key event callback function
 * @param  key: Key device pointer
 * @param  event: Event type
 * @param  cb_func: Callback function
 * @retval None
 */
void key_attach(key_t *key, enum key_event event, key_callback cb_func)
{
    if (!key || event >= KEY_EVENT_NUM)
        return;

    key->cb_func[event] = cb_func;
}

/**
 * @brief  Detach key event callback function
 * @param  key: Key device pointer
 * @param  event: Event type
 * @retval None
 */
void key_detach(key_t *key, enum key_event event)
{
    if (!key || event >= KEY_EVENT_NUM)
        return;

    key->cb_func[event] = NULL;
}

void key_irq_handle(void *args)
{
    key_t* dev = (key_t*)args;
    
    key_fsm_handle(dev);
}

/* Private functions ---------------------------------------------------------*/
/**
 * @brief  Read key pin level
 * @param  key: Key device pointer
 * @retval 1: High level, 0: Low level
 */
static uint8_t key_read_pin(const key_t *key)
{
    uint8_t level = gpio_read(key->pin);
    return key->active_level ? level : !level;
}

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
        key_detect(key);
    }
}

/**
 * @brief  Key detection processing
 * @param  key: Key device pointer
 * @retval None
 */
static void key_fsm_handle(key_t *key)
{
    uint8_t level = key_read_pin(key);

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
            KEY_EVENT_CB(KEY_EVENT_DOWN);
        }
        break;

    case KEY_STATE_DOWN:  /* Pressed state */
        if (!level) {
            /* Short press release */
            key->state = KEY_STATE_UP;
            key->ticks = 0;
            if (key->repeat_count == 1) {
                /* Trigger UP event on first press release */
                KEY_EVENT_CB(KEY_EVENT_UP);
            }
        } else if (++key->ticks >= key->long_time) {
            /* Long press time reached */
            key->state = KEY_STATE_LONG;
            key->ticks = 0;
            key->repeat_count = 0;  /* Enter long press state, clear repeat count */
            KEY_EVENT_CB(KEY_EVENT_LONG_START);
        }
        break;

    case KEY_STATE_LONG:  /* Long press state */
        if (!level) {
            /* Long press release */
            key->state = KEY_STATE_NONE;
            key->ticks = 0;
            key->repeat_count = 0;
            KEY_EVENT_CB(KEY_EVENT_LONG_FREE);
        } else {
            /* Long press hold */
            if (++key->ticks >= key->hold_time) {
                key->ticks = 0;
                KEY_EVENT_CB(KEY_EVENT_LONG_HOLD);
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
                KEY_EVENT_CB(KEY_EVENT_REPEAT);
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

