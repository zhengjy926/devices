/**
  ******************************************************************************
  * @file        : led_static_example.c
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2024-xx-xx
  * @brief       : GPIO LED静态分配使用示例
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.演示静态分配的GPIO LED设备使用方法
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "led_gpio.h"
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* 使用宏定义静态分配的LED设备 */
DEFINE_GPIO_LED(status_led, "PA.5", LED_GPIO_ACTIVE_HIGH, LED_OFF, LED_FULL);
DEFINE_GPIO_LED(error_led, "PB.7", LED_GPIO_ACTIVE_LOW, LED_OFF, LED_FULL);
DEFINE_GPIO_LED(power_led, "PC.13", LED_GPIO_ACTIVE_HIGH, LED_ON, LED_FULL);

/* 也可以直接定义结构体 */
static struct gpio_led_device user_led = {
    .led_cdev = {
        .name = "user_led",
        .brightness = LED_OFF,
        .max_brightness = LED_FULL,
    },
    .gpio_name = "PD.3",
    .active_low = LED_GPIO_ACTIVE_LOW,
};

/* Exported variables  -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  初始化所有LED设备
 * @retval 成功返回0，失败返回负数错误码
 */
int led_static_example_init(void)
{
    int ret;
    
    /* 初始化GPIO LED子系统 */
    ret = gpio_led_init();
    if (ret != 0) {
        printf("Failed to initialize GPIO LED subsystem: %d\n", ret);
        return ret;
    }
    
    /* 注册状态LED */
    ret = gpio_led_register(&status_led);
    if (ret != 0) {
        printf("Failed to register status LED: %d\n", ret);
        return ret;
    }
    
    /* 注册错误LED */
    ret = gpio_led_register(&error_led);
    if (ret != 0) {
        printf("Failed to register error LED: %d\n", ret);
        goto unregister_status;
    }
    
    /* 注册电源LED */
    ret = gpio_led_register(&power_led);
    if (ret != 0) {
        printf("Failed to register power LED: %d\n", ret);
        goto unregister_error;
    }
    
    /* 注册用户LED */
    ret = gpio_led_register(&user_led);
    if (ret != 0) {
        printf("Failed to register user LED: %d\n", ret);
        goto unregister_power;
    }
    
    printf("All LED devices registered successfully\n");
    return 0;
    
unregister_power:
    gpio_led_unregister(&power_led);
unregister_error:
    gpio_led_unregister(&error_led);
unregister_status:
    gpio_led_unregister(&status_led);
    
    return ret;
}

/**
 * @brief  LED控制演示
 * @retval None
 */
void led_static_example_demo(void)
{
    struct led_classdev *led;
    unsigned long delay_on = 500;  /* 500ms */
    unsigned long delay_off = 500; /* 500ms */
    
    printf("LED控制演示开始\n");
    
    /* 方法1：通过名称查找LED设备并控制 */
    led = led_find_by_name("status_led");
    if (led != NULL) {
        printf("控制状态LED亮起\n");
        led_set_brightness(led, LED_FULL);
        
        /* 延时一段时间 */
        /* delay_ms(1000); */
        
        printf("设置状态LED闪烁\n");
        led_blink_set(led, &delay_on, &delay_off);
    }
    
    /* 方法2：直接使用设备结构体控制 */
    printf("控制错误LED亮起\n");
    led_set_brightness(&error_led.led_cdev, LED_FULL);
    
    /* 方法3：控制多个LED */
    led = led_find_by_name("power_led");
    if (led != NULL) {
        printf("电源LED闪烁一次\n");
        led_blink_set_oneshot(led, &delay_on, &delay_off, 0);
    }
    
    led = led_find_by_name("user_led");
    if (led != NULL) {
        printf("用户LED半亮度\n");
        led_set_brightness(led, LED_HALF);
    }
    
    printf("LED控制演示完成\n");
}

/**
 * @brief  注销所有LED设备
 * @retval None
 */
void led_static_example_deinit(void)
{
    printf("注销所有LED设备\n");
    
    /* 注销所有LED设备 */
    gpio_led_unregister(&status_led);
    gpio_led_unregister(&error_led);
    gpio_led_unregister(&power_led);
    gpio_led_unregister(&user_led);
    
    printf("所有LED设备已注销\n");
}

/* Private functions ---------------------------------------------------------*/ 