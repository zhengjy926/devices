/**
  ******************************************************************************
  * @file        : touch_key.c
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
#include "touch_key.h"
#include "key.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Exported variables  -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
static uint32_t touch_key_read_state(struct key_dev *dev)
{
    
}


int touch_key_register(key_t *dev, uint8_t id, touch_key_cfg_t* cfg)
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

/* Private functions ---------------------------------------------------------*/

