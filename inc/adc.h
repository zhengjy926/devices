/**
  ******************************************************************************
  * @file        : adc.h
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
#ifndef __ADC_H__
#define __ADC_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "sys_def.h"

/* Exported define -----------------------------------------------------------*/
#define ADC_INTERN_CH_TEMPER     (-1)
#define ADC_INTERN_CH_VREF       (-2)
#define ADC_INTERN_CH_VBAT       (-3)

/* Exported typedef ----------------------------------------------------------*/
struct adc_device;

struct adc_ops
{
    int32_t (*enabled)(struct adc_device *device, int8_t channel, bool enabled);
    int32_t (*convert)(struct adc_device *device, int8_t channel, uint32_t *value);
    uint8_t (*get_resolution)(struct adc_device *device);
    int16_t (*get_vref) (struct adc_device *device);
};

struct adc_device
{
    const struct adc_ops *ops;
};
typedef struct adc_device *adc_t;

typedef enum
{
    ADC_CMD_ENABLE = 0x10,
    ADC_CMD_DISABLE = 0x11,
    ADC_CMD_GET_RESOLUTION = 0x12,  /* get the resolution in bits */
    ADC_CMD_GET_VREF = 0x13,        /* get reference voltage */
} adc_cmd_t;

/* Exported macro ------------------------------------------------------------*/

/* Exported variable prototypes ----------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/
int hw_adc_register(adc_t dev, const char *name, const struct adc_ops *ops, const void *user_data);
adc_t* adc_find(const char *name);
uint32_t adc_read(adc_t dev, int8_t channel);
int adc_enable(adc_t dev, int8_t channel);
int adc_disable(adc_t dev, int8_t channel);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ADC_H__ */

