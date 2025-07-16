/**
  ******************************************************************************
  * @file        : xxx.h
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 20xx-xx-xx
  * @brief       : 
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.xxx
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
/* Lower 16 bits reflect status */
#define LED_SUSPENDED		            BIT(0)
#define LED_UNREGISTERING	            BIT(1)

/* Upper 16 bits reflect control information */
#define LED_CORE_SUSPENDRESUME	        BIT(16)
#define LED_SYSFS_DISABLE	            BIT(17)
#define LED_DEV_CAP_FLASH	            BIT(18)
#define LED_HW_PLUGGABLE	            BIT(19)
#define LED_PANIC_INDICATOR	            BIT(20)
#define LED_BRIGHT_HW_CHANGED	        BIT(21)
#define LED_RETAIN_AT_SHUTDOWN	        BIT(22)
#define LED_INIT_DEFAULT_TRIGGER        BIT(23)
#define LED_REJECT_NAME_CONFLICT        BIT(24)
#define LED_MULTI_COLOR		            BIT(25)

#define LED_BLINK_SW			        0
#define LED_BLINK_ONESHOT		        1
#define LED_BLINK_ONESHOT_STOP		    2
#define LED_BLINK_INVERT		        3
#define LED_BLINK_BRIGHTNESS_CHANGE 	4
#define LED_BLINK_DISABLE		        5
/* Brightness off also disables hw-blinking so it is a separate action */
#define LED_SET_BRIGHTNESS_OFF		    6
#define LED_SET_BRIGHTNESS		        7
#define LED_SET_BLINK			        8


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

	struct device		*dev;
	list_t              node;			/* LED Device list */

	unsigned long		blink_delay_on, blink_delay_off;
    osTimerId_t         blink_timer;
	int			        blink_brightness;
	int			        new_blink_brightness;
    
    osSemaphoreId_t     work_semaphore;
    osThreadId_t        worker_thread;
	int			        delayed_set_value;
	unsigned long		delayed_delay_on;
	unsigned long		delayed_delay_off;

	/* Ensures consistent access to the LED class device */
    osMutexId_t         led_access;
};

/* Exported macro ------------------------------------------------------------*/

/* Exported variable prototypes ----------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/
static inline int led_get_brightness(struct led_classdev *led_cdev)
{
	return led_cdev->brightness;
}

void led_blink_set(struct led_classdev *led_cdev,
                    unsigned long *delay_on,
                    unsigned long *delay_off);

void led_blink_set_nosleep(struct led_classdev *led_cdev, 
                            unsigned long delay_on,
                            unsigned long delay_off);

void led_blink_set_oneshot(struct led_classdev *led_cdev,
                           unsigned long *delay_on, unsigned long *delay_off,
                           bool invert);

void led_set_brightness(struct led_classdev *led_cdev, unsigned int brightness);


int led_set_brightness_sync(struct led_classdev *led_cdev, unsigned int value);

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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __LEDS_H__ */

