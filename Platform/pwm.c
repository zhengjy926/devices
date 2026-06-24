/**
  ******************************************************************************
  * @copyright   : Copyright To Hangzhou Dinova EP Technology Co.,Ltd
  * @file        : pwm.c
  * @author      : ZJY
  * @version     : V1.0
  * @data        : 2025-05-28
  * @brief       : PWM设备管理实现
  * @attattention: None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.add pwm driver
  *
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "pwm.h"
#include <string.h>

#define  LOG_TAG             "pwm"
#define  LOG_LVL             4
#include "log.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
LIST_HEAD(pwm_device_list);	/**< 全局 PWM 设备链表头 */
/* Exported variables  -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
static bool pwm_state_valid(const struct pwm_state *state)
{
	/*
	 * For a disabled state all other state description is irrelevant and
	 * and supposed to be ignored. So also ignore any strange values and
	 * consider the state ok.
	 */
	if (!state->enabled)
		return true;

	if (!state->period)
		return false;

	if (state->duty_cycle > state->period)
		return false;

	return true;
}

static int __pwm_apply(struct pwm_device *pwm, const struct pwm_state *state)
{
	struct pwm_chip *chip;
	const struct pwm_ops *ops;
	int err;

	if (!pwm || !state)
		return -EINVAL;

	if (!pwm_state_valid(state)) {
		/*
		 * Allow to transition from one invalid state to another.
		 * This ensures that you can e.g. change the polarity while
		 * the period is zero. (This happens on stm32 when the hardware
		 * is in its poweron default state.) This greatly simplifies
		 * working with the sysfs API where you can only change one
		 * parameter at a time.
		 */
		if (!pwm_state_valid(&pwm->state)) {
			pwm->state = *state;
			return 0;
		}

		return -EINVAL;
	}

	chip = pwm->chip;
	ops = chip->ops;

	if (state->period == pwm->state.period &&
	    state->duty_cycle == pwm->state.duty_cycle &&
	    state->polarity == pwm->state.polarity &&
	    state->enabled == pwm->state.enabled &&
	    state->usage_power == pwm->state.usage_power)
		return 0;

    err = ops->apply(chip, pwm, state);
    if (err)
        return err;

    pwm->state = *state;

	return 0;
}

/**
 * pwm_apply_atomic() - apply a new state to a PWM device from atomic context
 * Not all PWM devices support this function, check with pwm_might_sleep().
 * @pwm: PWM device
 * @state: new state to apply
 *
 * Returns: 0 on success, or a negative errno
 * Context: Any
 */
int pwm_apply(struct pwm_device *pwm, const struct pwm_state *state)
{
	struct pwm_chip *chip = pwm->chip;

	if (!chip->operational)
		return -ENODEV;

	return __pwm_apply(pwm, state);
}

/**
 * pwm_get_state_hw() - get the current PWM state from hardware
 * @pwm: PWM device
 * @state: state to fill with the current PWM state
 *
 * Similar to pwm_get_state() but reads the current PWM state from hardware
 * instead of the requested state.
 *
 * Returns: 0 on success or a negative error code on failure.
 * Context: May sleep.
 */
int pwm_get_state_hw(struct pwm_device *pwm, struct pwm_state *state)
{
	struct pwm_chip *chip = pwm->chip;
	const struct pwm_ops *ops = chip->ops;
	int ret = -ENOTSUPP;

	if (!chip->operational)
		return -ENODEV;

    if (ops->get_state) {
		ret = ops->get_state(chip, pwm, state);
	}

	return ret;
}


/**
 * @brief 根据名称查找PWM设备
 * @param name: 设备名称
 * @return 成功返回设备指针，失败返回NULL
 */
struct pwm_device* pwm_get(const char *name)
{
	list_t *node;
	struct pwm_device *dev;

	if (!name)
		return NULL;

	list_for_each(node, &pwm_device_list) {
		dev = list_entry(node, struct pwm_device, node);
		if (strcmp(dev->label, name) == 0)
			return dev;
	}
	return NULL;
}

/**
 * @brief  添加一个PWM设备到全局链表
 * @param  pwm 要添加的pwm_device指针
 * @retval 成功返回0，失败返回负的错误码
 */
int pwm_register_device(struct pwm_device *pwm)
{
    list_t *node;
    struct pwm_device *dev;

    if (!pwm)
        return -EINVAL;
        
    if (!pwm->label || !pwm->chip)
        return -EINVAL;

    /* 检查是否已存在 */
    list_for_each(node, &pwm_device_list) {
        dev = list_entry(node, struct pwm_device, node);
        if (dev == pwm || (dev->hwpwm == pwm->hwpwm && dev->chip == pwm->chip)) {
            return -EEXIST; // 已存在
        }
    }

    list_node_init(&pwm->node);
    list_add_tail(&pwm->node, &pwm_device_list);
    return 0;
}
