/**
  ******************************************************************************
  * @copyright   : Copyright To Hangzhou Dinova EP Technology Co.,Ltd
  * @file        : xxxx.c
  * @author      : ZJY
  * @version     : V1.0
  * @data        : 20xx-xx-xx
  * @brief       : 
  * @attattention: None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.xxx
  *
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "watchdog.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Exported variables  -------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
/*
 * This function initializes watchdog
 */
static int watchdog_init(device_t *dev)
{
    watchdog_t *wtd;

//    ASSERT(dev != NULL);
    wtd = (watchdog_t *)dev;
    if (wtd->ops->init)
    {
        return (wtd->ops->init(wtd));
    }

    return (-1);
}

static int watchdog_open(device_t *dev, uint16_t oflag)
{
    return (0);
}

static int watchdog_close(device_t *dev)
{
    watchdog_t *wtd;

//    ASSERT(dev != NULL);
    wtd = (watchdog_t *)dev;

    if (wtd->ops->control(wtd, DEVICE_CTRL_WDT_STOP, NULL) != 0)
    {
//        kprintf(" This watchdog can not be stoped\n");

//        return (-ERROR);
    }

    return (0);
}

static int watchdog_control(device_t *dev,
                                    int              cmd,
                                    void             *args)
{
    watchdog_t *wtd;

//    ASSERT(dev != NULL);
    wtd = (watchdog_t *)dev;

    return (wtd->ops->control(wtd, cmd, args));
}

const static struct device_ops wdt_ops =
{
    watchdog_init,
    watchdog_open,
    watchdog_close,
    NULL,
    NULL,
    watchdog_control,
};

/**
 * This function register a watchdog device
 */
int hw_watchdog_register(struct watchdog_device *wtd,
                                 const char                *name,
                                 uint32_t                flag,
                                 void                      *data)
{
    device_t *device;
//    ASSERT(wtd != NULL);

    device = &(wtd->parent);

    device->rx_indicate = NULL;
    device->tx_complete = NULL;

    device->ops         = &wdt_ops;
    device->user_data   = data;
    
    /* register a character device */
    return device_register(device, name, flag);
}

/* Private functions ---------------------------------------------------------*/


