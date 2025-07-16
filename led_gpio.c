/**
  ******************************************************************************
  * @file        : led_gpio.c
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2024-xx-xx
  * @brief       : GPIO LED驱动实现
  * @attattention: None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.实现基于GPIO的LED驱动后端
  *
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "led_gpio.h"
#include "gpio.h"
#include <string.h>
#include <stdlib.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Exported variables  -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void gpio_led_brightness_set_impl(struct led_classdev *led_cdev, enum led_brightness brightness);
static enum led_brightness gpio_led_brightness_get_impl(struct led_classdev *led_cdev);
static int gpio_led_hw_init(struct gpio_led_device *gpio_led);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  GPIO LED亮度设置实现函数（LED核心框架回调）
 * @param  led_cdev: LED类设备指针
 * @param  brightness: 亮度值
 * @retval None
 */
static void gpio_led_brightness_set_impl(struct led_classdev *led_cdev, enum led_brightness brightness)
{
    struct gpio_led_device *gpio_led;
    int gpio_value;
    
    if (led_cdev == NULL) {
        return;
    }
    
    /* 通过container_of获取GPIO LED设备结构体 */
    gpio_led = container_of(led_cdev, struct gpio_led_device, led_cdev);
    
    /* 根据亮度值确定GPIO输出状态 */
    if (brightness > LED_OFF) {
        gpio_value = gpio_led->active_low ? PIN_LOW : PIN_HIGH;
    } else {
        gpio_value = gpio_led->active_low ? PIN_HIGH : PIN_LOW;
    }
    
    /* 设置GPIO输出 */
    gpio_write(gpio_led->gpio_pin, gpio_value);
}

/**
 * @brief  GPIO LED亮度获取实现函数（LED核心框架回调）
 * @param  led_cdev: LED类设备指针
 * @retval 当前亮度值
 */
static enum led_brightness gpio_led_brightness_get_impl(struct led_classdev *led_cdev)
{
    struct gpio_led_device *gpio_led;
    int gpio_value;
    
    if (led_cdev == NULL) {
        return LED_OFF;
    }
    
    gpio_led = container_of(led_cdev, struct gpio_led_device, led_cdev);
    
    /* 读取GPIO状态 */
    gpio_value = gpio_read(gpio_led->gpio_pin);
    
    /* 根据active_low标志判断LED状态 */
    if (gpio_led->active_low) {
        return (gpio_value == PIN_LOW) ? LED_ON : LED_OFF;
    } else {
        return (gpio_value == PIN_HIGH) ? LED_ON : LED_OFF;
    }
}

/**
 * @brief  初始化GPIO LED硬件
 * @param  gpio_led: GPIO LED设备指针
 * @retval 成功返回0，失败返回负数错误码
 */
static int gpio_led_hw_init(struct gpio_led_device *gpio_led)
{
    int gpio_pin;
    
    if (gpio_led == NULL || gpio_led->gpio_name == NULL) {
        return -EINVAL;
    }
    
    /* 获取GPIO引脚号 */
    gpio_pin = gpio_get(gpio_led->gpio_name);
    if (gpio_pin < 0) {
        return gpio_pin;
    }
    
    gpio_led->gpio_pin = gpio_pin;
    
    /* 配置GPIO为输出模式 */
    gpio_set_mode(gpio_pin, PIN_MODE_OUTPUT_PP, PIN_PULL_RESISTOR_NONE);
    
    return 0;
}

/**
 * @brief  注册静态分配的GPIO LED设备到LED核心框架
 * @param  gpio_led: 静态分配的GPIO LED设备指针
 * @retval 成功返回0，失败返回负数错误码
 */
int gpio_led_register(struct gpio_led_device *gpio_led)
{
    int ret;
    
    if (gpio_led == NULL) {
        return -EINVAL;
    }
    
    /* 初始化GPIO硬件 */
    ret = gpio_led_hw_init(gpio_led);
    if (ret != 0) {
        return ret;
    }
    
    /* 设置LED类设备的回调函数 */
    gpio_led->led_cdev.brightness_set = gpio_led_brightness_set_impl;
    gpio_led->led_cdev.brightness_get = gpio_led_brightness_get_impl;
    gpio_led->led_cdev.flags = 0;
    
    /* 注册到LED核心框架 */
    ret = led_classdev_register(&gpio_led->led_cdev);
    if (ret != 0) {
        return ret;
    }
    
    /* 设置初始亮度 - 使用LED核心框架的统一接口 */
    led_set_brightness(&gpio_led->led_cdev, gpio_led->led_cdev.brightness);
    
    return 0;
}

/**
 * @brief  注销GPIO LED设备
 * @param  gpio_led: GPIO LED设备指针
 * @retval None
 */
void gpio_led_unregister(struct gpio_led_device *gpio_led)
{
    if (gpio_led == NULL) {
        return;
    }
    
    /* 从LED核心框架注销 */
    led_classdev_unregister(&gpio_led->led_cdev);
    
    /* 注意：静态分配的设备不需要释放内存 */
}

/**
 * @brief  创建并注册一个GPIO LED设备到LED核心框架（动态分配版本）
 * @param  config: GPIO LED配置结构体指针
 * @retval 成功返回LED设备指针，失败返回NULL
 */
struct led_classdev *gpio_led_create(const struct gpio_led_config *config)
{
    struct gpio_led_device *gpio_led;
    int gpio_pin;
    int ret = 0;
    
    if (config == NULL || config->name == NULL || config->gpio_name == NULL) {
        return NULL;
    }
    
    /* 获取GPIO引脚号 */
    gpio_pin = gpio_get(config->gpio_name);
    if (gpio_pin < 0) {
        return NULL;
    }
    
    /* 分配设备结构体 */
    gpio_led = (struct gpio_led_device *)malloc(sizeof(struct gpio_led_device));
    if (gpio_led == NULL) {
        return NULL;
    }
    
    memset(gpio_led, 0, sizeof(struct gpio_led_device));
    
    /* 配置GPIO为输出模式 */
    gpio_set_mode(gpio_pin, PIN_MODE_OUTPUT_PP, PIN_PULL_RESISTOR_NONE);
    
    /* 初始化GPIO LED设备 */
    gpio_led->gpio_pin = gpio_pin;
    gpio_led->active_low = config->active_low;
    gpio_led->gpio_name = config->gpio_name;
    
    /* 初始化LED类设备 */
    gpio_led->led_cdev.name = config->name;
    gpio_led->led_cdev.brightness = config->default_brightness;
    gpio_led->led_cdev.max_brightness = config->max_brightness;
    gpio_led->led_cdev.brightness_set = gpio_led_brightness_set_impl;
    gpio_led->led_cdev.brightness_get = gpio_led_brightness_get_impl;
    gpio_led->led_cdev.flags = 0;
    
    /* 注册到LED核心框架 */
    ret = led_classdev_register(&gpio_led->led_cdev);
    if (ret != 0) {
        free(gpio_led);
        return NULL;
    }
    
    /* 设置初始亮度 - 使用LED核心框架的统一接口 */
    led_set_brightness(&gpio_led->led_cdev, config->default_brightness);
    
    return &gpio_led->led_cdev;
}

/**
 * @brief  销毁GPIO LED设备（动态分配版本）
 * @param  led_cdev: LED类设备指针
 * @retval None
 */
void gpio_led_destroy(struct led_classdev *led_cdev)
{
    struct gpio_led_device *gpio_led;
    
    if (led_cdev == NULL) {
        return;
    }
    
    gpio_led = container_of(led_cdev, struct gpio_led_device, led_cdev);
    
    /* 从LED核心框架注销 */
    led_classdev_unregister(led_cdev);
    
    /* 释放内存 */
    free(gpio_led);
}

/**
 * @brief  初始化GPIO LED子系统
 * @retval 成功返回0，失败返回负数错误码
 */
int gpio_led_init(void)
{
    /* GPIO LED子系统依赖LED核心框架，只需要确保LED子系统已初始化 */
    return led_subsystem_init();
}

/* Private functions ---------------------------------------------------------*/

