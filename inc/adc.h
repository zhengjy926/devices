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
#include <stdint.h>
#include <stdbool.h>

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

typedef struct adc_device
{
    uint32_t resolution_bits;  /* ADC分辨率，比如12表示12-bit */
    uint32_t vref_mv;          /* 参考电压(mV)，用于换算成物理量 */
    const struct adc_ops *ops;
    void *priv;
} adc_t;

/* Exported macro ------------------------------------------------------------*/

/* Exported variable prototypes ----------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/
adc_t* adc_find(const char *name);
/*
 * 打开ADC设备。典型用于做一次性校准/上电准备。
 * 返回0成功，<0失败。
 */
adc_t* adc_open(uint32_t number);

/*
 * 关闭ADC设备。会确保停止采样。
 */
void adc_close(adc_t *adc);

/*
 * 启动连续采样(DMA/中断推FIFO)
 * 返回0成功
 */
int adc_start(adc_t *adc);

/*
 * 停止连续采样
 */
int adc_stop(adc_t *adc);

uint32_t adc_read(adc_t dev, int8_t channel);

int hw_adc_register(adc_t dev, const char *name, const struct adc_ops *ops, const void *user_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ADC_H__ */

