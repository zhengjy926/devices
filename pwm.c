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

#define  LOG_TAG             "main"
#define  LOG_LVL             7
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
/**
 * @brief 设置PWM设备的参数
 * @param pwm: PWM设备指针
 * @param state: 要设置的状态参数
 * @return 成功返回0，失败返回负的错误码
 */
int pwm_set(struct pwm_device *pwm, const struct pwm_state *state)
{
	int err;
	struct pwm_controller *chip;
    
	if (!pwm || !state) {
        LOG_E("pwm or state is null!\r\n");
        return -EINVAL;
    }
		
    
    chip = pwm->chip;
    if (!chip || !chip->ops || !chip->ops->set)
        return -EINVAL;
    
    if (!state->period || state->duty_cycle > state->period)
        return -EINVAL;

	if (state->period == pwm->state.period &&
	    state->duty_cycle == pwm->state.duty_cycle &&
	    state->polarity == pwm->state.polarity)
		return 0;

    err = chip->ops->set(chip, pwm, state);
    if (err)
        return err;
    
    pwm->state = *state;

	return 0;
}

/**
 * @brief 使能PWM输出
 * @param pwm: PWM设备指针
 * @return 成功返回0，失败返回负的错误码
 */
int pwm_enable(struct pwm_device *pwm)
{
    int err;
    struct pwm_controller *chip;
    
    if (!pwm)
        return -EINVAL;
        
    chip = pwm->chip;
    if (!chip || !chip->ops || !chip->ops->enable)
        return -EINVAL;

    if (pwm->enabled)
        return 0;

    err = chip->ops->enable(chip, pwm);
    if (err)
        return err;

    pwm->enabled = true;

    return 0;
}

/**
 * @brief 禁用PWM输出
 * @param pwm: PWM设备指针
 * @return 成功返回0，失败返回负的错误码
 */
int pwm_disable(struct pwm_device *pwm)
{
    int err;
    struct pwm_controller *chip;
    
    if (!pwm)
        return -EINVAL;
        
    chip = pwm->chip;
    if (!chip || !chip->ops || !chip->ops->disable)
        return -EINVAL;

    if (!pwm->enabled)
        return 0;

    err = chip->ops->disable(chip, pwm);
    if (err)
        return err;

    pwm->enabled = false;

    return 0;
}

/**
 * @brief 捕获PWM信号
 * @param pwm: PWM设备指针
 * @param result: 用于存储捕获结果的结构体指针
 * @param timeout: 等待捕获的超时时间（毫秒）
 * @return 成功返回0，失败返回负的错误码
 */
int pwm_capture(struct pwm_device *pwm, struct pwm_capture *result,
		       unsigned long timeout)
{
	struct pwm_controller *chip;
	const struct pwm_ops *ops;
	
	if (!pwm || !result)
	    return -EINVAL;
	    
	chip = pwm->chip;
	if (!chip)
	    return -EINVAL;

	if (!chip->operational)
		return -ENODEV;
		
	ops = chip->ops;
	if (!ops || !ops->capture)
	    return -ENOSYS;

	return ops->capture(chip, pwm, result, timeout);
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
		if (strcmp(dev->name, name) == 0)
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
        
    if (!pwm->name || !pwm->chip)
        return -EINVAL;

    /* 检查是否已存在（可根据label或channel唯一性） */
    list_for_each(node, &pwm_device_list) {
        dev = list_entry(node, struct pwm_device, node);
        if (dev == pwm || (dev->channel == pwm->channel && dev->chip == pwm->chip)) {
            return -EEXIST; // 已存在
        }
    }

    list_node_init(&pwm->node);
    list_add_tail(&pwm->node, &pwm_device_list);

    return 0;
}
