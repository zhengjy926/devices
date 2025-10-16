/**
  ******************************************************************************
  * @file        : pwm.h
  * @author      : ZJY
  * @version     : V1.0
  * @data        : 2025-05-28
  * @brief       : pwm driver
  * @attattention: None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.add pwm driver
  *
  *
  ******************************************************************************
  */
#ifndef __PWM_H__
#define __PWM_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "sys_def.h"
#include "my_list.h"
#include "mymath.h"

/* Exported define -----------------------------------------------------------*/

/* Exported typedef ----------------------------------------------------------*/
struct pwm_chip;

/**
 * @brief polarity of a PWM signal
 */
enum pwm_polarity {
	PWM_POLARITY_NORMAL,    /**< 开始为高电平且持续占空比时间，剩余时间为低电平 */
	PWM_POLARITY_INVERSED,  /**< 开始为低电平且持续占空比时间，剩余时间为高电平 */
};

/**
 * struct pwm_args - board-dependent PWM arguments
 * @period: reference period
 * @polarity: reference polarity
 *
 * This structure describes board-dependent arguments attached to a PWM
 * device. These arguments are usually retrieved from the PWM lookup table or
 * device tree.
 *
 * Do not confuse this with the PWM state: PWM arguments represent the initial
 * configuration that users want to use on this PWM device rather than the
 * current PWM hardware state.
 */
struct pwm_args {
	uint64_t period;
	enum pwm_polarity polarity;
};

/**
 * @brief state of a PWM channel
 */
struct pwm_state {
	uint32_t period;            /**< PWM period (in nanoseconds) */
	uint32_t duty_cycle;        /**< PWM duty cycle (in nanoseconds) */
	enum pwm_polarity polarity; /**< PWM polarity */
	bool enabled;               /**< PWM enabled status */
	bool usage_power;           /**< If set, the PWM driver is only required to maintain the power
                                  *  output but has more freedom regarding signal form.
                                  *  If supported, the signal can be optimized, for example to
                                  *  improve EMI by phase shifting individual channels.
                                  */
};

/**
 * @brief PWM channel object
 */
struct pwm_device {
	const char *label;          /**< name of the PWM device */
	unsigned long flags;        /**< flags associated with the PWM device */
	unsigned int hwpwm;         /**< per-chip relative index of the PWM device */
	struct pwm_chip *chip;      /**< PWM chip providing this PWM device */
	struct pwm_args args;       /**< PWM arguments */
	struct pwm_state state;     /**< last applied state */
	struct pwm_state last;      /**< last implemented state (for PWM_DEBUG) */
    list_t node;
};

/**
 * @brief PWM capture data
 */
struct pwm_capture {
	uint32_t period;        /**< period of the PWM signal (in nanoseconds) */
	uint32_t duty_cycle;    /**< duty cycle of the PWM signal (in nanoseconds) */
};

/**
 * @brief PWM controller operations
 */
/**
 * @request: optional hook for requesting a PWM
 * @free: optional hook for freeing a PWM
 * @capture: capture and report PWM signal
 * @sizeof_wfhw: size (in bytes) of driver specific waveform presentation
 * @round_waveform_tohw: convert a struct pwm_waveform to driver specific presentation
 * @round_waveform_fromhw: convert a driver specific waveform presentation to struct pwm_waveform
 * @read_waveform: read driver specific waveform presentation from hardware
 * @write_waveform: write driver specific waveform presentation to hardware
 * @apply: atomically apply a new PWM config
 * @get_state: get the current PWM state.
 */
struct pwm_ops {
	int (*capture)(struct pwm_chip *chip, struct pwm_device *pwm,
		       struct pwm_capture *result, unsigned long timeout);
	int (*apply)(struct pwm_chip *chip, struct pwm_device *pwm,
		     const struct pwm_state *state);
	int (*get_state)(struct pwm_chip *chip, struct pwm_device *pwm,
			 struct pwm_state *state);
};

/**
 * struct pwm_chip - abstract a PWM controller
 * @dev: device providing the PWMs
 * @ops: callbacks for this PWM controller
 * @owner: module providing this chip
 * @id: unique number of this PWM chip
 * @npwm: number of PWMs controlled by this chip
 * @of_xlate: request a PWM device given a device tree PWM specifier
 * @atomic: can the driver's ->apply() be called in atomic context
 * @uses_pwmchip_alloc: signals if pwmchip_allow was used to allocate this chip
 * @operational: signals if the chip can be used (or is already deregistered)
 * @nonatomic_lock: mutex for nonatomic chips
 * @atomic_lock: mutex for atomic chips
 * @pwms: array of PWM devices allocated by the framework
 */
struct pwm_chip {
//	struct device dev;
	const struct pwm_ops *ops;
	struct module *owner;
	unsigned int id;
	unsigned int npwm;
	bool atomic;
	bool operational;
//	union {
//		/*
//		 * depending on the chip being atomic or not either the mutex or
//		 * the spinlock is used. It protects .operational and
//		 * synchronizes the callbacks in .ops
//		 */
//		struct mutex nonatomic_lock;
//		spinlock_t atomic_lock;
//	};
    void *hw_data;
};
/* Exported macro ------------------------------------------------------------*/

/* Exported variable prototypes ----------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/
/* PWM consumer APIs */
int pwm_apply(struct pwm_device *pwm, const struct pwm_state *state);
int pwm_get_state_hw(struct pwm_device *pwm, struct pwm_state *state);
int pwm_adjust_config(struct pwm_device *pwm);

int pwm_register_device(struct pwm_device *pwm);

/**
 * pwm_get_state() - retrieve the current PWM state
 * @pwm: PWM device
 * @state: state to fill with the current PWM state
 *
 * The returned PWM state represents the state that was applied by a previous call to
 * pwm_apply_might_sleep(). Drivers may have to slightly tweak that state before programming it to
 * hardware. If pwm_apply_might_sleep() was never called, this returns either the current hardware
 * state (if supported) or the default settings.
 */
static inline void pwm_get_state(const struct pwm_device *pwm,
                                 struct pwm_state *state)
{
	*state = pwm->state;
}

/**
 * @brief 检查PWM设备是否使能
 * @param pwm: PWM设备指针
 * @return true: 已使能; false: 未使能
 */
static inline bool pwm_is_enabled(const struct pwm_device *pwm)
{
	struct pwm_state state;

	pwm_get_state(pwm, &state);

	return state.enabled;
}

/**
 * @brief 获取PWM周期值（纳秒）
 * @param pwm: PWM设备指针
 * @return 周期值（纳秒）
 */
static inline uint64_t pwm_get_period(const struct pwm_device *pwm)
{
	struct pwm_state state;

	pwm_get_state(pwm, &state);

	return state.period;
}

/**
 * @brief 获取PWM占空比值（纳秒）
 * @param pwm: PWM设备指针
 * @return 占空比值（纳秒）
 */
static inline uint64_t pwm_get_duty_cycle(const struct pwm_device *pwm)
{
	struct pwm_state state;

	pwm_get_state(pwm, &state);

	return state.duty_cycle;
}

/**
 * @brief 获取PWM极性
 * @param pwm: PWM设备指针
 * @return PWM极性枚举值
 */
static inline enum pwm_polarity pwm_get_polarity(const struct pwm_device *pwm)
{
	struct pwm_state state;

	pwm_get_state(pwm, &state);

	return state.polarity;
}

static inline void pwm_get_args(const struct pwm_device *pwm,
				struct pwm_args *args)
{
	*args = pwm->args;
}

/**
 * pwm_init_state() - prepare a new state to be applied with pwm_apply_might_sleep()
 * @pwm: PWM device
 * @state: state to fill with the prepared PWM state
 *
 * This functions prepares a state that can later be tweaked and applied
 * to the PWM device with pwm_apply_might_sleep(). This is a convenient function
 * that first retrieves the current PWM state and the replaces the period
 * and polarity fields with the reference values defined in pwm->args.
 * Once the function returns, you can adjust the ->enabled and ->duty_cycle
 * fields according to your needs before calling pwm_apply_might_sleep().
 *
 * ->duty_cycle is initially set to zero to avoid cases where the current
 * ->duty_cycle value exceed the pwm_args->period one, which would trigger
 * an error if the user calls pwm_apply_might_sleep() without adjusting ->duty_cycle
 * first.
 */
static inline void pwm_init_state(const struct pwm_device *pwm,
				  struct pwm_state *state)
{
	struct pwm_args args;

	/* First get the current state. */
	pwm_get_state(pwm, state);

	/* Then fill it with the reference config */
	pwm_get_args(pwm, &args);

	state->period = args.period;
	state->polarity = args.polarity;
	state->duty_cycle = 0;
	state->usage_power = false;
}

/**
 * pwm_get_relative_duty_cycle - 计算相对占空比（四舍五入）
 * @state: PWM 状态
 * @scale: 缩放基准（如 100 表示百分比，top 值表示硬件计数上限）
 *
 * 返回值：0 ~ scale 的整数
 */
static inline uint32_t pwm_get_relative_duty_cycle(const struct pwm_state *state,
                                                    uint32_t scale)
{
    if (!state->period || state->duty_cycle > state->period)
        return 0;

    /* 用 64 位分子避免 scale 较大时溢出 */
    uint64_t num = (uint64_t)state->duty_cycle * scale;
    return (uint32_t)((num + (state->period >> 1)) / state->period);
}

/**
 * pwm_set_relative_duty_cycle() - Set a relative duty cycle value
 * @state: PWM state to fill
 * @duty_cycle: relative duty cycle value
 * @scale: scale in which @duty_cycle is expressed
 *
 * This functions converts a relative into an absolute duty cycle (expressed
 * in nanoseconds), and puts the result in state->duty_cycle.
 *
 * For example if you want to configure a 50% duty cycle, call:
 *
 * pwm_init_state(pwm, &state);
 * pwm_set_relative_duty_cycle(&state, 50, 100);
 * pwm_apply_might_sleep(pwm, &state);
 *
 * Returns: 0 on success or ``-EINVAL`` if @duty_cycle and/or @scale are
 * inconsistent (@scale == 0 or @duty_cycle > @scale)
 */
static inline int pwm_set_relative_duty_cycle(struct pwm_state *state,
                                               uint32_t duty_cycle, uint32_t scale)
{
    if (!scale || duty_cycle > scale) {
        return -EINVAL;
    }
    
    /* 手写四舍五入版本 */
    uint64_t num = (uint64_t)duty_cycle * state->period;
    state->duty_cycle = (num + (scale >> 1)) / scale;
    
    return 0;
}

/**
 * pwm_enable() - start a PWM output toggling
 * @pwm: PWM device
 *
 * Returns: 0 on success or a negative error code on failure.
 */
static inline int pwm_enable(struct pwm_device *pwm)
{
	struct pwm_state state;

	if (!pwm)
		return -EINVAL;

	pwm_get_state(pwm, &state);
	if (state.enabled)
		return 0;

	state.enabled = true;
	return pwm_apply(pwm, &state);
}

/**
 * pwm_disable() - stop a PWM output toggling
 * @pwm: PWM device
 */
static inline void pwm_disable(struct pwm_device *pwm)
{
	struct pwm_state state;

	if (!pwm)
		return;

	pwm_get_state(pwm, &state);
	if (!state.enabled)
		return;

	state.enabled = false;
	pwm_apply(pwm, &state);
}

/**
 * pwm_might_sleep() - is pwm_apply_atomic() supported?
 * @pwm: PWM device
 *
 * Returns: false if pwm_apply_atomic() can be called from atomic context.
 */
static inline bool pwm_might_sleep(struct pwm_device *pwm)
{
	return !pwm->chip->atomic;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PWM_H__ */
