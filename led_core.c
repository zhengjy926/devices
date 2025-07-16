/**
  ******************************************************************************
  * @file        : led_core.c
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2025-07-15
  * @brief       : xxx
  * @attention   : xxx
  ******************************************************************************
  * @history     :
  *         V1.0 : 
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "leds.h"
#include "minmax.h"
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Exported variables  -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
static int __led_set_brightness(struct led_classdev *led_cdev, unsigned int value)
{
	if (!led_cdev->brightness_set)
		return -ENOTSUPP;

	led_cdev->brightness_set(led_cdev, value);

	return 0;
}

static int __led_set_brightness_blocking(struct led_classdev *led_cdev, unsigned int value)
{
	if (!led_cdev->brightness_set_blocking)
		return -ENOTSUPP;

	return led_cdev->brightness_set_blocking(led_cdev, value);
}

static void led_timer_function(void *arg)
{
    struct led_classdev *led_cdev = arg;
	unsigned long brightness;
	unsigned long delay;

	if (!led_cdev->blink_delay_on || !led_cdev->blink_delay_off) {
		led_set_brightness_nosleep(led_cdev, LED_OFF);
		clear_bit(LED_BLINK_SW, &led_cdev->work_flags);
		return;
	}

	if (test_and_clear_bit(LED_BLINK_ONESHOT_STOP, &led_cdev->work_flags)) {
		clear_bit(LED_BLINK_SW, &led_cdev->work_flags);
		return;
	}

	brightness = led_get_brightness(led_cdev);
	if (!brightness) {
		/* Time to switch the LED on. */
		if (test_and_clear_bit(LED_BLINK_BRIGHTNESS_CHANGE,
					&led_cdev->work_flags))
			brightness = led_cdev->new_blink_brightness;
		else
			brightness = led_cdev->blink_brightness;
		delay = led_cdev->blink_delay_on;
	} else {
		/* Store the current brightness value to be able
		 * to restore it when the delay_off period is over.
		 */
		led_cdev->blink_brightness = brightness;
		brightness = LED_OFF;
		delay = led_cdev->blink_delay_off;
	}

	led_set_brightness_nosleep(led_cdev, brightness);

	/* Return in next iteration if led is in one-shot mode and we are in
	 * the final blink state so that the led is toggled each delay_on +
	 * delay_off milliseconds in worst case.
	 */
	if (test_bit(LED_BLINK_ONESHOT, &led_cdev->work_flags)) {
		if (test_bit(LED_BLINK_INVERT, &led_cdev->work_flags)) {
			if (brightness)
				set_bit(LED_BLINK_ONESHOT_STOP,
					&led_cdev->work_flags);
		} else {
			if (!brightness)
				set_bit(LED_BLINK_ONESHOT_STOP,
					&led_cdev->work_flags);
		}
	}

	osTimerStart(&led_cdev->blink_timer, delay);
}

static void set_brightness_delayed_set_brightness(struct led_classdev *led_cdev,
                                                unsigned int value)
{
	int ret;

	ret = __led_set_brightness(led_cdev, value);
	if (ret == -ENOTSUPP) {
		ret = __led_set_brightness_blocking(led_cdev, value);
		if (ret == -ENOTSUPP)
			/* No back-end support to set a fixed brightness value */
			return;
	}

	/* LED HW might have been unplugged, therefore don't warn */
	if (ret == -ENODEV && led_cdev->flags & LED_UNREGISTERING &&
	    led_cdev->flags & LED_HW_PLUGGABLE)
		return;

	if (ret < 0)
        printf("Setting an LED's brightness failed (%d)\n", ret);
}

static void set_brightness_delayed(struct led_classdev *led_cdev)
{
	if (test_and_clear_bit(LED_BLINK_DISABLE, &led_cdev->work_flags)) {
		led_stop_software_blink(led_cdev);
		set_bit(LED_SET_BRIGHTNESS_OFF, &led_cdev->work_flags);
	}

	/*
	 * Triggers may call led_set_brightness(LED_OFF),
	 * led_set_brightness(LED_FULL) in quick succession to disable blinking
	 * and turn the LED on. Both actions may have been scheduled to run
	 * before this work item runs once. To make sure this works properly
	 * handle LED_SET_BRIGHTNESS_OFF first.
	 */
	if (test_and_clear_bit(LED_SET_BRIGHTNESS_OFF, &led_cdev->work_flags)) {
		set_brightness_delayed_set_brightness(led_cdev, LED_OFF);
		/*
		 * The consecutives led_set_brightness(LED_OFF),
		 * led_set_brightness(LED_FULL) could have been executed out of
		 * order (LED_FULL first), if the work_flags has been set
		 * between LED_SET_BRIGHTNESS_OFF and LED_SET_BRIGHTNESS of this
		 * work. To avoid ending with the LED turned off, turn the LED
		 * on again.
		 */
		if (led_cdev->delayed_set_value != LED_OFF)
			set_bit(LED_SET_BRIGHTNESS, &led_cdev->work_flags);
	}

	if (test_and_clear_bit(LED_SET_BRIGHTNESS, &led_cdev->work_flags))
		set_brightness_delayed_set_brightness(led_cdev, led_cdev->delayed_set_value);

	if (test_and_clear_bit(LED_SET_BLINK, &led_cdev->work_flags)) {
		unsigned long delay_on = led_cdev->delayed_delay_on;
		unsigned long delay_off = led_cdev->delayed_delay_off;

		led_blink_set(led_cdev, &delay_on, &delay_off);
	}
}

#if USING_RTOS
static void led_worker_thread(void *argument)
{
    struct led_classdev *led_cdev = (struct led_classdev *)argument;

    for (;;) {
        /* 等待信号量，直到有任务需要处理 */
        osSemaphoreAcquire(led_cdev->work_semaphore, osWaitForever);

        /* 处理挂起的任务 */
        set_brightness_delayed(led_cdev);
    }
}
#endif

static void led_set_software_blink(struct led_classdev *led_cdev,
                                   unsigned long delay_on,
                                   unsigned long delay_off)
{
	int current_brightness;

	current_brightness = led_get_brightness(led_cdev);
	if (current_brightness)
		led_cdev->blink_brightness = current_brightness;
	if (!led_cdev->blink_brightness)
		led_cdev->blink_brightness = led_cdev->max_brightness;

	led_cdev->blink_delay_on = delay_on;
	led_cdev->blink_delay_off = delay_off;

	/* never on - just set to off */
	if (!delay_on) {
		led_set_brightness_nosleep(led_cdev, LED_OFF);
		return;
	}

	/* never off - just set to brightness */
	if (!delay_off) {
		led_set_brightness_nosleep(led_cdev, led_cdev->blink_brightness);
		return;
	}

	set_bit(LED_BLINK_SW, &led_cdev->work_flags);
	osTimerStart(&led_cdev->blink_timer, 1);
}


static void led_blink_setup(struct led_classdev *led_cdev,
                             unsigned long *delay_on,
                             unsigned long *delay_off)
{
	if (!test_bit(LED_BLINK_ONESHOT, &led_cdev->work_flags) &&
	    led_cdev->blink_set &&
	    !led_cdev->blink_set(led_cdev, delay_on, delay_off))
		return;

	/* blink with 1 Hz as default if nothing specified */
	if (!*delay_on && !*delay_off)
		*delay_on = *delay_off = 500;

	led_set_software_blink(led_cdev, *delay_on, *delay_off);
}

void led_init_core(struct led_classdev *led_cdev)
{
    /* 工作线程的属性 */
    const osThreadAttr_t led_worker_attr = {
        .name = "led_worker",
        .stack_size = 256, /* 根据需要调整栈大小 */
        .priority = (osPriority_t) osPriorityNormal,
    };

    /* 创建二进制信号量，用于唤醒工作线程
     * max_count=1, initial_count=0 
     */
    led_cdev->work_semaphore = osSemaphoreNew(1, 0, NULL);
    if (led_cdev->work_semaphore == NULL) {
        /* 错误处理：无法创建信号量 */
        return;
    }

    /* 创建工作线程 */
    led_cdev->worker_thread = osThreadNew(led_worker_thread, led_cdev, &led_worker_attr);
    if (led_cdev->worker_thread == NULL) {
        /* 错误处理：无法创建工作线程 */
        return;
    }
    
    led_cdev->blink_timer = osTimerNew(led_timer_function, osTimerPeriodic, led_cdev, NULL);
}

/**
 * led_blink_set - set blinking with software fallback
 * @led_cdev: the LED to start blinking
 * @delay_on: the time it should be on (in ms)
 * @delay_off: the time it should ble off (in ms)
 *
 * This function makes the LED blink, attempting to use the
 * hardware acceleration if possible, but falling back to
 * software blinking if there is no hardware blinking or if
 * the LED refuses the passed values.
 *
 * This function may sleep!
 *
 * Note that if software blinking is active, simply calling
 * led_cdev->brightness_set() will not stop the blinking,
 * use led_set_brightness() instead.
 */
void led_blink_set(struct led_classdev *led_cdev,
                   unsigned long *delay_on,
                   unsigned long *delay_off)
{
    osStatus_t status = osOK;
    
	status = osTimerStop(led_cdev->blink_timer);
    if (status) {
        while(1);
    }

	clear_bit(LED_BLINK_SW, &led_cdev->work_flags);
	clear_bit(LED_BLINK_ONESHOT, &led_cdev->work_flags);
	clear_bit(LED_BLINK_ONESHOT_STOP, &led_cdev->work_flags);

	led_blink_setup(led_cdev, delay_on, delay_off);
}

/**
 * @brief  led_blink_set_oneshot - do a oneshot software blink
 * @param  led_cdev: the LED to start blinking
 * @param  delay_on: the time it should be on (in ms)
 * @param  delay_off: the time it should ble off (in ms)
 * @param  invert: true - blink off first, then on, leaving the led on
 *                 false - blink on first, then off, leaving the led off
 * @retval None
 * @note   This function is guaranteed not to sleep.
 */
void led_blink_set_oneshot(struct led_classdev *led_cdev,
                            unsigned long *delay_on,
                            unsigned long *delay_off,
                            bool invert)
{
	if (test_bit(LED_BLINK_ONESHOT, &led_cdev->work_flags) &&
	     osTimerIsRunning(&led_cdev->blink_timer))
		return;

	set_bit(LED_BLINK_ONESHOT, &led_cdev->work_flags);
	clear_bit(LED_BLINK_ONESHOT_STOP, &led_cdev->work_flags);

	if (invert)
		set_bit(LED_BLINK_INVERT, &led_cdev->work_flags);
	else
		clear_bit(LED_BLINK_INVERT, &led_cdev->work_flags);

	led_blink_setup(led_cdev, delay_on, delay_off);
}

void led_blink_set_nosleep(struct led_classdev *led_cdev, unsigned long delay_on,
			   unsigned long delay_off)
{
	/* If necessary delegate to a work queue task. */
	if (led_cdev->blink_set && led_cdev->brightness_set_blocking) {
		led_cdev->delayed_delay_on = delay_on;
		led_cdev->delayed_delay_off = delay_off;
		set_bit(LED_SET_BLINK, &led_cdev->work_flags);
		osSemaphoreRelease(led_cdev->work_semaphore); /* 唤醒工作线程 */
		return;
	}

	led_blink_set(led_cdev, &delay_on, &delay_off);
}

void led_stop_software_blink(struct led_classdev *led_cdev)
{
	osTimerStop(&led_cdev->blink_timer);
	led_cdev->blink_delay_on = 0;
	led_cdev->blink_delay_off = 0;
	clear_bit(LED_BLINK_SW, &led_cdev->work_flags);
}

/**
 * led_set_brightness - set LED brightness
 * @led_cdev: the LED to set
 * @brightness: the brightness to set it to
 *
 * Set an LED's brightness, and, if necessary, cancel the
 * software blink timer that implements blinking when the
 * hardware doesn't. This function is guaranteed not to sleep.
 */
void led_set_brightness(struct led_classdev *led_cdev, unsigned int brightness)
{
	/*
	 * If software blink is active, delay brightness setting
	 * until the next timer tick.
	 */
	if (test_bit(LED_BLINK_SW, &led_cdev->work_flags)) {
		/*
		 * If we need to disable soft blinking delegate this to the
		 * work queue task to avoid problems in case we are called
		 * from hard irq context.
		 */
		if (!brightness) {
			set_bit(LED_BLINK_DISABLE, &led_cdev->work_flags);
			osSemaphoreRelease(led_cdev->work_semaphore); /* 唤醒工作线程 */
		} else {
			set_bit(LED_BLINK_BRIGHTNESS_CHANGE, &led_cdev->work_flags);
			led_cdev->new_blink_brightness = brightness;
		}
		return;
	}

	led_set_brightness_nosleep(led_cdev, brightness);
}

void led_set_brightness_nopm(struct led_classdev *led_cdev, unsigned int value)
{
	/* Use brightness_set op if available, it is guaranteed not to sleep */
	if (!__led_set_brightness(led_cdev, value))
		return;

	/*
	 * Brightness setting can sleep, delegate it to a work queue task.
	 * value 0 / LED_OFF is special, since it also disables hw-blinking
	 * (sw-blink disable is handled in led_set_brightness()).
	 * To avoid a hw-blink-disable getting lost when a second brightness
	 * change is done immediately afterwards (before the work runs),
	 * it uses a separate work_flag.
	 */
	led_cdev->delayed_set_value = value;
	/* Ensure delayed_set_value is seen before work_flags modification */
	__DMB();

	if (value)
		set_bit(LED_SET_BRIGHTNESS, &led_cdev->work_flags);
	else {
		clear_bit(LED_SET_BRIGHTNESS, &led_cdev->work_flags);
		clear_bit(LED_SET_BLINK, &led_cdev->work_flags);
		set_bit(LED_SET_BRIGHTNESS_OFF, &led_cdev->work_flags);
	}

	osSemaphoreRelease(led_cdev->work_semaphore);
}

void led_set_brightness_nosleep(struct led_classdev *led_cdev, unsigned int value)
{
	led_cdev->brightness = min(value, led_cdev->max_brightness);

	if (led_cdev->flags & LED_SUSPENDED)
		return;

	led_set_brightness_nopm(led_cdev, led_cdev->brightness);
}

/**
 * led_set_brightness_sync - set LED brightness synchronously
 * @led_cdev: the LED to set
 * @value: the brightness to set it to
 *
 * Set an LED's brightness immediately. This function will block
 * the caller for the time required for accessing device registers,
 * and it can sleep.
 *
 * Returns: 0 on success or negative error value on failure
 */
int led_set_brightness_sync(struct led_classdev *led_cdev, unsigned int value)
{
	if (led_cdev->blink_delay_on || led_cdev->blink_delay_off)
		return -EBUSY;

	led_cdev->brightness = min(value, led_cdev->max_brightness);

	if (led_cdev->flags & LED_SUSPENDED)
		return 0;

	return __led_set_brightness_blocking(led_cdev, led_cdev->brightness);
}
/* Private functions ---------------------------------------------------------*/

