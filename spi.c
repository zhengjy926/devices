/**
  ******************************************************************************
  * @file        : spi.c
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2025-05-30
  * @brief       : SPI驱动框架实现
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.xxx
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "spi.h"
#include "gpio.h"

#define  DEBUG_TAG                  "spi"
#define  FILE_DEBUG_LEVEL           3
#define  FILE_ASSERT_ENABLED        1
#include "debug.h"

/* Private typedef -----------------------------------------------------------*/
static LIST_HEAD(spi_bus_list);       /* serial list head */
static LIST_HEAD(spi_device_list);    /* serial list head */

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
struct spi_bus* spi_bus_find(const char* bus_name)
{
    struct spi_bus *bus = NULL;
    list_t *node = NULL;
    
    /* parameter check */
    if (bus_name == NULL)
        return NULL;
    
    /* find device by traversing the double linked list */
    list_for_each(node, &spi_bus_list) {
        bus = list_entry(node, struct spi_bus, node);
        if (bus != NULL && strcmp(bus->name, bus_name) == 0) {
            return bus;
        }
    }

    return NULL;
}

int spi_bus_register(struct spi_bus *bus, const char* bus_name, 
                     const struct spi_ops *ops)
{
    if (!bus || !ops || spi_bus_find(bus_name))
        return -EINVAL;

    strncpy(bus->name, bus_name, SPI_NAME_MAX);
    bus->name[SPI_NAME_MAX-1] = '\0';
    bus->ops = ops;
    bus->mode = 0;
    list_add_tail(&bus->node, &spi_bus_list);
    return 0;
}

int spi_device_attach_bus(struct spi_device *dev, const char *dev_name)
{
    ASSERT(dev->cs_pin != 0);
    ASSERT(dev->data_width != 0);
    ASSERT(dev->max_hz != 0);
    
    struct spi_bus *bus;
    
    if (!dev || !dev_name)
        return -EINVAL;

    bus = spi_bus_find(dev_name);
    if (!bus)
        return -EINVAL;

    dev->bus = bus;
    
    gpio_set_mode(dev->cs_pin, PIN_MODE_OUTPUT_PP, PIN_PULL_RESISTOR_UP);
    bus->ops->set_cs(dev, 0);
    
    return 0;
}

int spi_transfer_message(struct spi_device *device, struct spi_message *message)
{
    int ret = 0;
    struct spi_message *msg;
    struct spi_bus *bus;

    if (!device || !message || !device->bus)
        return -EINVAL;

    bus = device->bus;
    if (!bus->ops || !bus->ops->xfer || !bus->ops->configure)
        return -EINVAL;

    /* Configure SPI bus if needed */
    if ( device->mode != bus->mode || device->data_width != bus->data_width \
         || device->max_hz != bus->actual_hz) {
        ret = bus->ops->configure(device);
        if (ret)
            return ret;
        bus->mode = device->mode;
        bus->data_width = device->data_width;
        device->max_hz = bus->actual_hz;
    }
    
    /* Process messages */
    for (msg = message; msg; msg = msg->next) {
        if (!msg->length)
            continue;

        bus->ops->set_cs(device, 1);
        
        ret = bus->ops->xfer(device, msg);
        if (ret < 0)
            break;

        if (msg->cs_change) {
            bus->ops->set_cs(device, 0);
            if (msg->next)
                bus->ops->set_cs(device, 1);
        }
    }
    
    bus->ops->set_cs(device, 0);
    return ret;
}
