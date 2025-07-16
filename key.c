/**
  ******************************************************************************
  * @file        : key.c
  * @author      : xxx
  * @version     : V1.0
  * @date        : 2024-xx-xx
  * @brief       : GPIO Key Driver Implementation
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.Implementation of GPIO-based key driver
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "key.h"
#include "gpio.h"
#include "stimer.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define KEY_EVENT_CB(event)                          \
    do {                                            \
        if (key->cb_func[event])                    \
            key->cb_func[event]((void *)key);       \
    } while (0)

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static LIST_HEAD(key_list);                /* Key device list head */
static stimer_t key_scan_timer = {0};       /* Key scan timer */

key_t key[] = {
    {
        .key_name = "key0",
        .pin_name = "PH.3",
        .active_level = 0,
    },
    {
        .key_name = "key1",
        .pin_name = "PH.2",
        .active_level = 0,
    },
    {
        .key_name = "key2",
        .pin_name = "PC.13",
        .active_level = 0,
    },
    {
        .key_name = "wk_up",
        .pin_name = "PA.0",
        .active_level = 1,
    }
};

/* Exported variables --------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void key_detect(key_t *key);
static uint8_t key_read_pin(const key_t *key);
static void key_scan_timer_callback(void *arg);

/* Exported functions --------------------------------------------------------*/
/**
 * @brief  Initialize key hardware
 * @param  key: Key device pointer
 * @param  name: Key name
 * @param  pin: GPIO pin
 * @param  active_level: Active level
 * @retval 0: Success, Others: Failed
 */
static int hw_key_init(key_t *key, const char *key_name, const char *pin_name, uint8_t active_level)
{
    size_t pin;

    if (!key || !key_name || !pin_name)
        return -EINVAL;

    memset(key, 0, sizeof(key_t));
    key->key_name = key_name;
    key->pin_name = pin_name;
    key->active_level = active_level;
    key->state = KEY_STATE_NONE;
    key->last_level = !active_level;
    key->debounce_time = KEY_DEFAULT_DEBOUNCE;    /* Default 2 scan cycles for debounce */
    key->long_time = KEY_DEFAULT_LONG_TIME;       /* Default 100 scan cycles for long press */
    key->hold_time = KEY_DEFAULT_HOLD_TIME;       /* Default 10 scan cycles for long press hold */
    key->repeat_time = KEY_DEFAULT_REPEAT_TIME;   /* Default 20 scan cycles for repeat interval */

    /* Get GPIO pin */
    pin = gpio_get(pin_name);
    if (!pin)
        return -EINVAL;

    key->pin = pin;

    /* Initialize GPIO */
    if (key->active_level)
        gpio_set_mode(key->pin , PIN_MODE_INPUT, PIN_PULL_RESISTOR_DOWN);
    else
        gpio_set_mode(key->pin , PIN_MODE_INPUT, PIN_PULL_RESISTOR_UP);

    return 0;
}

/**
 * @brief  Initialize keys
 * @param  None
 * @retval 0: Success, Others: Failed
 */
int key_init(void)
{
    for (uint32_t i = 0; i < sizeof(key) / sizeof(key[0]); i++) {
        hw_key_init(&key[i], key[i].key_name, key[i].pin_name, key[i].active_level);
    }
    /* Create software timer for periodic key scanning */
    stimer_create(&key_scan_timer, KEY_SCAN_PERIOD_MS,
                    STIMER_AUTO_RELOAD, key_scan_timer_callback, NULL);
    
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
static void key_detect(key_t *key)
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
