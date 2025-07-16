# GPIO LED静态分配使用指南

## 概述

GPIO LED驱动现在支持两种设备分配方式：
1. **静态分配**（推荐）：设备结构体在编译时分配，不使用动态内存
2. **动态分配**：运行时动态分配内存，适用于设备数量不确定的情况

## 静态分配的优势

- **内存效率**：不需要malloc/free，避免内存碎片
- **实时性**：没有动态内存分配的不确定性
- **安全性**：编译时确定内存布局，减少运行时错误
- **嵌入式友好**：符合嵌入式系统的设计原则

## 使用方法

### 1. 使用宏定义（推荐）

最简单的方式是使用`DEFINE_GPIO_LED`宏：

```c
#include "led_gpio.h"

/* 定义静态LED设备 */
DEFINE_GPIO_LED(status_led, "PA.5", LED_GPIO_ACTIVE_HIGH, LED_OFF, LED_FULL);
DEFINE_GPIO_LED(error_led, "PB.7", LED_GPIO_ACTIVE_LOW, LED_OFF, LED_FULL);
```

宏参数说明：
- `status_led`: LED设备变量名
- `"PA.5"`: GPIO引脚名称
- `LED_GPIO_ACTIVE_HIGH`: 高电平有效（或LED_GPIO_ACTIVE_LOW）
- `LED_OFF`: 默认亮度
- `LED_FULL`: 最大亮度

### 2. 直接定义结构体

也可以直接定义设备结构体：

```c
static struct gpio_led_device user_led = {
    .led_cdev = {
        .name = "user_led",
        .brightness = LED_OFF,
        .max_brightness = LED_FULL,
    },
    .gpio_name = "PD.3",
    .active_low = LED_GPIO_ACTIVE_LOW,
};
```

### 3. 注册设备

在系统初始化时注册LED设备：

```c
int led_system_init(void)
{
    int ret;
    
    /* 初始化GPIO LED子系统 */
    ret = gpio_led_init();
    if (ret != 0) {
        return ret;
    }
    
    /* 注册LED设备 */
    ret = gpio_led_register(&status_led);
    if (ret != 0) {
        return ret;
    }
    
    ret = gpio_led_register(&error_led);
    if (ret != 0) {
        gpio_led_unregister(&status_led);
        return ret;
    }
    
    return 0;
}
```

### 4. 控制LED

使用LED核心框架的统一接口控制LED：

```c
void led_control_demo(void)
{
    struct led_classdev *led;
    unsigned long delay_on = 500, delay_off = 500;
    
    /* 方法1：通过名称查找并控制 */
    led = led_find_by_name("status_led");
    if (led != NULL) {
        led_set_brightness(led, LED_FULL);      /* 点亮LED */
        led_blink_set(led, &delay_on, &delay_off); /* 设置闪烁 */
    }
    
    /* 方法2：直接使用设备结构体 */
    led_set_brightness(&error_led.led_cdev, LED_HALF);
    
    /* 方法3：单次闪烁 */
    led = led_find_by_name("user_led");
    if (led != NULL) {
        led_blink_set_oneshot(led, &delay_on, &delay_off, 0);
    }
}
```

### 5. 注销设备

系统关闭时注销设备：

```c
void led_system_deinit(void)
{
    gpio_led_unregister(&status_led);
    gpio_led_unregister(&error_led);
    gpio_led_unregister(&user_led);
}
```

## API参考

### 静态分配接口

- `DEFINE_GPIO_LED(name, gpio, active_low, default_brightness, max_brightness)` - 定义LED设备宏
- `int gpio_led_register(struct gpio_led_device *gpio_led)` - 注册LED设备
- `void gpio_led_unregister(struct gpio_led_device *gpio_led)` - 注销LED设备

### LED核心控制接口

- `led_set_brightness(led_cdev, brightness)` - 设置LED亮度
- `led_blink_set(led_cdev, &delay_on, &delay_off)` - 设置LED闪烁
- `led_blink_set_oneshot(led_cdev, &delay_on, &delay_off, invert)` - 单次闪烁
- `led_find_by_name(name)` - 通过名称查找LED设备

### 动态分配接口（兼容性）

- `gpio_led_create(config)` - 创建LED设备（动态分配）
- `gpio_led_destroy(led_cdev)` - 销毁LED设备（动态分配）

## 最佳实践

1. **优先使用静态分配**：对于固定数量的LED设备
2. **集中管理**：在一个模块中定义所有LED设备
3. **错误处理**：注册失败时要正确回滚已注册的设备
4. **命名规范**：使用有意义的LED名称，便于查找
5. **资源清理**：系统关闭时注销所有设备

## 注意事项

- 静态设备结构体必须是全局变量或静态变量
- 注册顺序要考虑依赖关系和错误处理
- GPIO引脚名称格式为"Px.y"（如"PA.5"）
- 编译错误`'sys_def.h' file not found`不影响功能，是头文件路径问题 