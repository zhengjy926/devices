/**
  ******************************************************************************
  * @file        : led_example.c
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2024-xx-xx
  * @brief       : LED使用示例
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.演示统一LED接口的使用
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "led_gpio.h"
#include "leds.h"

/* 示例：GPIO LED配置 */
static const struct gpio_led_config led_configs[] = {
    {
        .name = "status_led",
        .gpio_name = "PA.5",           /* GPIO引脚名称 */
        .active_low = LED_GPIO_ACTIVE_LOW,
        .default_brightness = LED_OFF,
        .max_brightness = LED_FULL,
    },
    {
        .name = "power_led", 
        .gpio_name = "PC.13",
        .active_low = LED_GPIO_ACTIVE_HIGH,
        .default_brightness = LED_ON,
        .max_brightness = LED_FULL,
    },
};

/* 私有变量 */
static struct led_classdev *status_led = NULL;
static struct led_classdev *power_led = NULL;

/**
 * @brief  初始化LED示例
 * @retval 成功返回0，失败返回负数
 */
int led_example_init(void)
{
    int ret;
    
    /* 1. 初始化LED子系统（包含GPIO LED驱动） */
    ret = gpio_led_init();
    if (ret != 0) {
        return ret;
    }
    
    /* 2. 创建GPIO LED设备 */
    status_led = gpio_led_create(&led_configs[0]);
    if (status_led == NULL) {
        return -1;
    }
    
    power_led = gpio_led_create(&led_configs[1]);
    if (power_led == NULL) {
        gpio_led_destroy(status_led);
        return -1;
    }
    
    return 0;
}

/**
 * @brief  LED示例演示
 * @retval None
 */
void led_example_demo(void)
{
    struct led_classdev *led;
    unsigned long delay_on = 500;
    unsigned long delay_off = 500;
    
    /* 使用LED核心框架的统一接口 */
    
    /* 1. 通过名称查找LED设备 */
    led = led_find_by_name("status_led");
    if (led != NULL) {
        /* 控制LED亮度 */
        led_set_brightness(led, LED_FULL);   /* 点亮 */
        
        /* 延时 */
        osDelay(1000);
        
        led_set_brightness(led, LED_OFF);    /* 熄灭 */
    }
    
    /* 2. 直接使用已知的LED设备指针 */
    if (power_led != NULL) {
        /* 设置LED闪烁 */
        led_blink_set(power_led, &delay_on, &delay_off);
        
        /* 延时 */
        osDelay(5000);
        
        /* 停止闪烁并设置为常亮 */
        led_set_brightness(power_led, LED_ON);
    }
    
    /* 3. 一次性闪烁 */
    led = led_find_by_name("status_led");
    if (led != NULL) {
        unsigned long on_time = 200;
        unsigned long off_time = 200;
        
        /* 闪烁3次后保持熄灭状态 */
        for (int i = 0; i < 3; i++) {
            led_blink_set_oneshot(led, &on_time, &off_time, false);
            osDelay(500);
        }
    }
}

/**
 * @brief  清理LED示例
 * @retval None
 */
void led_example_cleanup(void)
{
    /* 销毁LED设备（会自动从核心框架注销） */
    if (status_led != NULL) {
        gpio_led_destroy(status_led);
        status_led = NULL;
    }
    
    if (power_led != NULL) {
        gpio_led_destroy(power_led);
        power_led = NULL;
    }
}

/* 
 * 使用总结：
 * 
 * 1. 设备创建：使用具体驱动的创建函数（如gpio_led_create）
 * 2. 设备控制：使用LED核心框架的统一接口
 *    - led_set_brightness()：控制亮度
 *    - led_blink_set()：设置闪烁
 *    - led_blink_set_oneshot()：一次性闪烁
 *    - led_find_by_name()：按名称查找设备
 * 3. 设备销毁：使用具体驱动的销毁函数（如gpio_led_destroy）
 * 
 * 优势：
 * - 统一的操作接口，无论什么类型的LED
 * - 自动的设备管理（注册/注销）
 * - 内置的软件闪烁支持
 * - 线程安全的操作
 */ 