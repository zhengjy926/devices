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
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
static uint32_t gpio_key_read_state(struct key_dev *dev)
{
    gpio_key_cfg_t* cfg = (gpio_key_cfg_t*)dev->hw_context;
    
    uint32_t level = gpio_read(cfg->pin_id);
    return cfg->active_low ? !level : level;
}


int gpio_key_register(key_t *dev, uint8_t id, gpio_key_cfg_t* cfg)
{
    if (!cfg)
        return -EINVAL;
    
    if (!dev)
        return -EINVAL;
    
    // GPIO 初始化
    if (cfg->active_low) {
        gpio_set_mode(cfg->pin_id, PIN_INPUT, PIN_PULL_UP);
    } else {
        gpio_set_mode(cfg->pin_id, PIN_INPUT, PIN_PULL_DOWN);
    }
    
    key_device_register(dev);
    
    // 填充句柄
    dev->id = id;
    dev->read_state = gpio_key_read_state;
    dev->hw_context = (void*)cfg;       // 存储私有配置

    // 调用服务层进行注册
    return 0;
}

