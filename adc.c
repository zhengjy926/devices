/**
  ******************************************************************************
  * @file        : adc.c
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2024-09-26
  * @brief       : xxx
  * @attention   : xxx
  ******************************************************************************
  * @history     :
  *         V1.0 : 
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "adc.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Exported variables  -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
size_t adc_read(device_t *dev, int pos, void *buffer, size_t size)
{
    int32_t result = 0;
    size_t i;
    struct adc_device *adc = (struct adc_device *)dev;
    uint32_t *value = (uint32_t *)buffer;

    for (i = 0; i < size; i += sizeof(int))
    {
        result = adc->ops->convert(adc, pos + i, value);
        if (result != 0)
        {
            return 0;
        }
        value++;
    }

    return i;
}

static int32_t _adc_control(device_t *dev, int cmd, void *args)
{
    int32_t result = -1;
    adc_device_t adc = (struct adc_device *)dev;

    if (cmd == ADC_CMD_ENABLE && adc->ops->enabled)
    {
        result = adc->ops->enabled(adc, (uint32_t)args, true);
    }
    else if (cmd == ADC_CMD_DISABLE && adc->ops->enabled)
    {
        result = adc->ops->enabled(adc, (uint32_t)args, true);
    }
    else if (cmd == ADC_CMD_GET_RESOLUTION && adc->ops->get_resolution && args)
    {
        uint8_t resolution = adc->ops->get_resolution(adc);
        if(resolution != 0)
        {
            *((uint8_t*)args) = resolution;
//            LOG_D("resolution: %d bits", resolution);
            result = 0;
        }
    }
    else if (cmd == ADC_CMD_GET_VREF && adc->ops->get_vref && args)
    {
        int16_t value = adc->ops->get_vref(adc);
        if(value != 0)
        {
            *((int16_t *) args) = value;
            result = 0;
        }
    }

    return result;
}

const static struct device_ops adc_ops =
{
    NULL,
    NULL,
    NULL,
    _adc_read,
    NULL,
    _adc_control,
};

int32_t hw_adc_register(adc_device_t device, const char *name, const struct adc_ops *ops, const void *user_data)
{
    int32_t result = 0;
//    ASSERT(ops != NULL && ops->convert != NULL);

    device->parent.rx_indicate = NULL;
    device->parent.tx_complete = NULL;
    device->parent.ops         = &adc_ops;
    device->ops = ops;
    device->parent.user_data = (void *)user_data;

    result = device_register(&device->parent, name, DEVICE_AFLAG_RW);

    return result;
}

uint32_t adc_read(adc_t dev, int8_t channel);
{
    uint32_t value;

    if (!dev) {
        return 0;
    }

    dev->ops->convert(dev, channel, &value);

    return value;
}

int32_t adc_enable(adc_device_t dev, int8_t channel)
{
    int32_t result = 0;

//    ASSERT(dev);

    if (dev->ops->enabled != NULL)
    {
        result = dev->ops->enabled(dev, channel, true);
    }
    else
    {
        result = -1;
    }

    return result;
}

int32_t adc_disable(adc_device_t dev, int8_t channel)
{
    int32_t result = 0;

//    ASSERT(dev);

    if (dev->ops->enabled != NULL)
    {
        result = dev->ops->enabled(dev, channel, false);
    }
    else
    {
        result = -1;
    }

    return result;
}

int16_t adc_voltage(adc_device_t dev, int8_t channel)
{
    uint32_t value = 0;
    int16_t vref = 0, voltage = 0;
    uint8_t resolution = 0;

//    ASSERT(dev);

    /*get the resolution in bits*/
    if (_adc_control((device_t*) dev, ADC_CMD_GET_RESOLUTION, &resolution) != 0)
    {
        goto _voltage_exit;
    }

    /*get the reference voltage*/
    if (_adc_control((device_t*) dev, ADC_CMD_GET_VREF, &vref) != 0)
    {
        goto _voltage_exit;
    }

    /*read the value and convert to voltage*/
    dev->ops->convert(dev, channel, &value);
    voltage = value * vref / (1 << resolution);

_voltage_exit:
    return voltage;
}

