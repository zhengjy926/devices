/**
  ******************************************************************************
  * @file        : gpio_key.c
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2024-09-26
  * @brief       : GPIO Key Driver Implementation
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : Implementation of GPIO-based key driver
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "gpio_key.h"
#include "gpio.h"
#include "stimer.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define KEY_EVENT_CB(event)                         \
    do {                                            \
        if (key->cb_func[event])                    \
            key->cb_func[event]((void *)key);       \
    } while (0)

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static stimer_t key_scan_timer = {0};       /* Key scan timer */

/* Exported variables --------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void key_detect(key_t *key);
static uint8_t key_read_pin(const key_t *key);
static void key_scan_timer_callback(void *arg);

/* Exported functions --------------------------------------------------------*/
static uint32_t gpio_key_read_state(struct key_dev *dev)
{
    gpio_key_cfg_t* cfg = (gpio_key_cfg_t*)dev->hw_context;
    
    return gpio_read(cfg->pin_id);
}

int gpio_key_register(key_t *dev, uint8_t id, const gpio_key_cfg_t* cfg, gpio_key_mode_t mode)
{
    if (!cfg)
        return -EINVAL;
    
    if (!dev)
        return -EINVAL;
    
    // GPIO 初始化
    if (mode == GPIO_KEY_MODE_POLL){
        if (cfg->active_low) {
            gpio_set_mode(cfg->pin_id, PIN_INPUT, PIN_PULL_UP);
        } else {
            gpio_set_mode(cfg->pin_id, PIN_INPUT, PIN_PULL_DOWN);
        }
    } else {
        if (cfg->active_low) {
            gpio_attach_irq (cfg->pin_id, PIN_EVENT_FALLING_EDGE, key_irq_handle, NULL);
        } else {
            gpio_attach_irq (cfg->pin_id, PIN_EVENT_RISING_EDGE, key_irq_handle, NULL);
        }
        gpio_irq_enable(cfg->pin_id, 1);
    }
    
    // 填充句柄
    dev->id = id;
    dev->read_state = gpio_key_read_state;
    dev->hw_context = (void*)cfg;       // 存储私有配置
    dev->state = KEY_STATE_NONE;
    dev->debounce_integrator = 0;

    // 调用服务层进行注册
    return key_device_register(dev);
}

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
    key->debounce_time = KEY_DEBOUNCE_TIME;
    key->long_time = KEY_LONG_TIME;
    key->hold_time = KEY_HOLD_TIME;
    key->repeat_time = KEY_REPEAT_TIME;

    /* Get GPIO pin */
    pin = gpio_get(pin_name);
    if (!pin)
        return -EINVAL;

    key->pin = pin;

    /* Initialize GPIO */
    if (key->active_level)
        gpio_set_mode(key->pin , PIN_INPUT, PIN_PULL_DOWN);
    else
        gpio_set_mode(key->pin , PIN_INPUT, PIN_PULL_UP);

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
