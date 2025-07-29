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

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static LIST_HEAD(gpio_key_list);            /* Key device list head */
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

static void gpio_key_irq_handle(void *args)
{
    key_t* dev = (key_t*)args;
    
    key_fsm_handle(dev);
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
            gpio_attach_irq (cfg->pin_id, PIN_EVENT_EITHER_EDGE, gpio_key_irq_handle, NULL);
        } else {
            gpio_attach_irq (cfg->pin_id, PIN_EVENT_EITHER_EDGE, gpio_key_irq_handle, NULL);
        }
        gpio_irq_enable(cfg->pin_id, 1);
    }
    
    // 填充句柄
    dev->read_state = gpio_key_read_state;
    dev->hw_context = (void*)cfg;       // 存储私有配置
    dev->state = KEY_STATE_NONE;

    // 调用服务层进行注册
    return key_device_register(dev);
}





