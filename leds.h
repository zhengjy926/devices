/**
  ******************************************************************************
  * @file        : leds.h
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 20xx-xx-xx
  * @brief       : LED设备头文件
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.实现LED设备框架
  ******************************************************************************
  */
#ifndef __LEDS_H__
#define __LEDS_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "sys_def.h"
#include "bitops.h"

#if USING_RTOS
    #include "cmsis_os2.h"
#endif

/* Exported define -----------------------------------------------------------*/
#define LED_BLINK_SW			        0       /**< Blinking using software methods (timer + workqueue) */
#define LED_BLINK_ONESHOT		        1       /**< Make a oneshot blink */
#define LED_BLINK_ONESHOT_STOP		    2       /**< The oneshot blink has been completed and is ready to stop */
#define LED_BLINK_INVERT		        3       /**< oneshot blink go off first and then go on (invert sequence) */
#define LED_BLINK_BRIGHTNESS_CHANGE 	4       /**< The brightness needs to be changed when blinking. */
#define LED_BLINK_DISABLE		        5       /**< Disable blink */
#define LED_SET_BRIGHTNESS_OFF		    6       /**< Request to set the brightness to LED_OFF and stop blinking */
#define LED_SET_BRIGHTNESS		        7       /**< Set the brightness in delayed_set_value */
#define LED_SET_BLINK			        8       /**< Request to set blinking */

/* Exported typedef ----------------------------------------------------------*/
enum led_brightness {
	LED_OFF		= 0,
	LED_ON		= 1,
	LED_HALF	= 127,
	LED_FULL	= 255,
};

struct led_classdev {
	const char		*name;
	unsigned int    brightness;
	unsigned int    max_brightness;
	unsigned int    color;
	int			    flags;
    
	/* set_brightness_work / blink_timer flags, atomic, private. */
	unsigned long   work_flags;

	/* Set LED brightness level
	 * Must not sleep. Use brightness_set_blocking for drivers
	 * that can sleep while setting brightness.
	 */
	void		(*brightness_set)(struct led_classdev *led_cdev,
                                enum led_brightness brightness);
	/*
	 * Set LED brightness level immediately - it can block the caller for
	 * the time required for accessing a LED device register.
	 */
	int (*brightness_set_blocking)(struct led_classdev *led_cdev,
				       enum led_brightness brightness);
	/* Get LED brightness level */
	enum led_brightness (*brightness_get)(struct led_classdev *led_cdev);

	/*
	 * Activate hardware accelerated blink, delays are in milliseconds
	 * and if both are zero then a sensible default should be chosen.
	 * The call should adjust the timings in that case and if it can't
	 * match the values specified exactly.
	 * Deactivate blinking again when the brightness is set to LED_OFF
	 * via the brightness_set() callback.
	 * For led_blink_set_nosleep() the LED core assumes that blink_set
	 * implementations, of drivers which do not use brightness_set_blocking,
	 * will not sleep. Therefor if brightness_set_blocking is not set
	 * this function must not sleep!
	 */
	int		(*blink_set)(struct led_classdev *led_cdev,
				     unsigned long *delay_on,
				     unsigned long *delay_off);
                     
	list_t              node;			    /* LED Device list */

	unsigned long		blink_delay_on, blink_delay_off;
	int			        blink_brightness;
	int			        new_blink_brightness;

#if USING_RTOS                     
    osTimerId_t         blink_timer;
    osSemaphoreId_t     work_semaphore;
    osThreadId_t        worker_thread;
	int			        delayed_set_value;
	unsigned long		delayed_delay_on;
	unsigned long		delayed_delay_off;
    osMutexId_t         led_access;         /* Ensures consistent access to the LED class device */
#endif
};

/* Exported macro ------------------------------------------------------------*/

/* Exported variable prototypes ----------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/
static inline int led_get_brightness(struct led_classdev *led_cdev)
{
	return led_cdev->brightness;
}

void led_set_brightness(struct led_classdev *led_cdev, unsigned int brightness);
int led_set_brightness_sync(struct led_classdev *led_cdev, unsigned int value);

void led_blink_set(struct led_classdev *led_cdev,
                    unsigned long *delay_on,
                    unsigned long *delay_off);

void led_blink_set_nosleep(struct led_classdev *led_cdev, 
                            unsigned long delay_on,
                            unsigned long delay_off);

void led_blink_set_oneshot(struct led_classdev *led_cdev,
                           unsigned long *delay_on, unsigned long *delay_off,
                           bool invert);

void led_init_core(struct led_classdev *led_cdev);
void led_stop_software_blink(struct led_classdev *led_cdev);
void led_set_brightness_nopm(struct led_classdev *led_cdev, unsigned int value);
void led_set_brightness_nosleep(struct led_classdev *led_cdev, unsigned int value);
               
/**
 * led_update_brightness - update LED brightness
 * @led_cdev: the LED to query
 *
 * Get an LED's current brightness and update led_cdev->brightness
 * member with the obtained value.
 *
 * Returns: 0 on success or negative error value on failure
 */
int led_update_brightness(struct led_classdev *led_cdev);

/* LED设备注册和管理的统一接口 */
int led_classdev_register(struct led_classdev *led_cdev);
void led_classdev_unregister(struct led_classdev *led_cdev);
struct led_classdev *led_find_by_name(const char *name);
int led_subsystem_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __LEDS_H__ */

