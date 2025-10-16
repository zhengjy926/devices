/**
  ******************************************************************************
  * @file        : led_gpio.h
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2024-xx-xx
  * @brief       : GPIO LED驱动头文件
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.实现基于GPIO的LED驱动后端
  ******************************************************************************
  */
#ifndef __LED_GPIO_H__
#define __LED_GPIO_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "leds.h"

/* Exported define -----------------------------------------------------------*/
/* GPIO LED配置标志 */
#define LED_GPIO_ACTIVE_HIGH        0       /**< LED高电平有效 */
#define LED_GPIO_ACTIVE_LOW         1       /**< LED低电平有效 */

/* Exported typedef ----------------------------------------------------------*/
/**
 * @brief GPIO LED设备结构体
 */
struct gpio_led_device {
    struct led_classdev led_cdev;          /**< LED类设备基础结构 */
    int gpio_pin;                          /**< GPIO引脚号 */
    int active_low;                        /**< 低电平有效标志 */
    const char *gpio_name;                 /**< GPIO引脚名称 */
};

/**
 * @brief GPIO LED配置结构体
 */
struct gpio_led_config {
    const char *name;                      /**< LED名称 */
    const char *gpio_name;                 /**< GPIO引脚名称（如"PA.5"） */
    int active_low;                        /**< 低电平有效标志 */
    enum led_brightness default_brightness; /**< 默认亮度 */
    unsigned int max_brightness;           /**< 最大亮度值 */
};

/* Exported macro ------------------------------------------------------------*/
/**
 * @brief 静态定义GPIO LED设备的宏
 * @param _name LED设备名称
 * @param _gpio_name GPIO引脚名称
 * @param _active_low 是否低电平有效
 * @param _default_brightness 默认亮度
 * @param _max_brightness 最大亮度
 */
#define DEFINE_GPIO_LED(_name, _gpio_name, _active_low, _default_brightness, _max_brightness) \
    struct gpio_led_device _name = { \
        .led_cdev = { \
            .name = #_name, \
            .brightness = _default_brightness, \
            .max_brightness = _max_brightness, \
        }, \
        .gpio_name = _gpio_name, \
        .active_low = _active_low, \
    }

/* Exported variable prototypes ----------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/

/**
 * @brief  注册静态分配的GPIO LED设备到LED核心框架
 * @param  gpio_led: 静态分配的GPIO LED设备指针
 * @retval 成功返回0，失败返回负数错误码
 * @note   设备结构体必须是静态分配的，不会进行内存分配
 *         使用LED核心的统一接口：
 *         - led_set_brightness() 控制LED亮度
 *         - led_blink_set() 设置LED闪烁
 *         - led_find_by_name() 查找LED设备
 */
int gpio_led_register(struct gpio_led_device *gpio_led);

/**
 * @brief  注销GPIO LED设备
 * @param  gpio_led: GPIO LED设备指针
 * @retval None
 * @note   设备会自动从LED核心框架中注销，但不会释放内存
 */
void gpio_led_unregister(struct gpio_led_device *gpio_led);

/**
 * @brief  创建并注册一个GPIO LED设备到LED核心框架（动态分配版本）
 * @param  config: GPIO LED配置结构体指针
 * @retval 成功返回LED设备指针，失败返回NULL
 * @note   使用LED核心的统一接口：
 *         - led_set_brightness() 控制LED亮度
 *         - led_blink_set() 设置LED闪烁
 *         - led_find_by_name() 查找LED设备
 * @deprecated 推荐使用静态分配的gpio_led_register()函数
 */
struct led_classdev *gpio_led_create(const struct gpio_led_config *config);

/**
 * @brief  销毁GPIO LED设备（动态分配版本）
 * @param  led_cdev: LED类设备指针
 * @retval None
 * @note   设备会自动从LED核心框架中注销
 * @deprecated 推荐使用静态分配的gpio_led_unregister()函数
 */
void gpio_led_destroy(struct led_classdev *led_cdev);

/**
 * @brief  初始化GPIO LED子系统
 * @retval 成功返回0，失败返回负数错误码
 * @note   必须在使用任何GPIO LED设备之前调用
 */
int gpio_led_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __LED_GPIO_H__ */
