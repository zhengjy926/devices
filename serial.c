/**
  ******************************************************************************
  * @file        : serial.c
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2025-01-27
  * @brief       : Generic serial port driver implementation
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.Enhanced error handling and improved structure
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "serial.h"
#include "errno-base.h"
#include <string.h>
#include "cmsis_compiler.h"

#define  LOG_TAG             "serial"
#define  LOG_LVL             4
#include "log.h"

/* Private variables ---------------------------------------------------------*/
LIST_HEAD(serial_list);                    /* serial list head */

/* Private function prototypes -----------------------------------------------*/
#if USING_RTOS
    static void serial_tx_task(void *arg);
#endif

static void start_transfer(serial_t *port);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Find serial device by name
  * @param  name: name of serial device
  * @retval pointer to serial device
  *         NULL if not found
  */
serial_t* serial_find(const char *name)
{
    serial_t *dev = NULL;
    list_t *node = NULL;
    
    /* Parameter check */
    if (name == NULL) {
        LOG_E("serial name is NULL");
        return NULL;
    }
    
    /* Find device by traversing the double linked list */
    list_for_each(node, &serial_list) {
        dev = list_entry(node, struct serial, node);
        if (dev && strcmp(dev->name, name) == 0) {
            return dev;
        }
    }
    
    return NULL;
}

/**
  * @brief  Initialize serial port
  * @param  port: pointer to serial device
  * @retval 0 on success
  *         -EINVAL invalid parameter
  *         -EIO I/O error
  */
int serial_open(serial_t *port)
{
    int ret = 0;
    
    if (!port) {
        LOG_E("");
        return -EINVAL;
    }
    
    if (port->opened) {
        return 0;
    }
    
    if (!port->ops || !port->ops->init) {
        return -EINVAL;
    }

    if (!port->rx_buf || port->rx_bufsz == 0 || !port->tx_buf || port->tx_bufsz == 0) {
        return -EINVAL;
    }
    
    /* Initialize hardware */
    ret = port->ops->init(port);
    if (ret) {
        return ret;
    }
    /* Initialize FIFO */
    ret = kfifo_init(&port->rx_fifo, port->rx_buf, port->rx_bufsz, 1);
    if (ret) {
        return ret;
    }
    ret = kfifo_init(&port->tx_fifo, port->tx_buf, port->tx_bufsz, 1);
    if (ret) {
        return ret;
    }

    /* Initialize configuration with default values */
    struct serial_configure default_cfg = SERIAL_CONFIG_DEFAULT;
    port->config = default_cfg;
    port->opened = 1;
    return 0;
}

void serial_close(serial_t *port)
{
    
}

/**
  * @brief  Control serial port parameters
  * @param  port: pointer to serial device
  * @param  cmd: control command
  * @param  arg: control argument
  * @retval 0 on success, negative error code on failure
  */
int serial_control(serial_t *port, int cmd, void *arg)
{
    int ret = 0;
    
    if (!port) {
        return -EINVAL;
    }
    
    if (!port->opened) {
        return -EIO;
    }
    
    switch (cmd) {
        case SERIAL_CMD_SET_CONFIG:
        {
            struct serial_configure *config = (struct serial_configure *)arg;
            if (!config) {
                return -EINVAL;
            }
            
            /* Apply new configuration if hardware configure function is available */
            if (port->ops && port->ops->configure) {
                ret = port->ops->configure(port, config);
                if (ret != 0) {
                    return ret;
                }
                /* Update configuration */
                port->config = *config;
            }
            break;
        }
        
        case SERIAL_CMD_GET_CONFIG:
        {
            struct serial_configure *config = (struct serial_configure *)arg;
            if (!config) {
                return -EINVAL;
            }
            
            *config = port->config;
            break;
        }
        
        default:
            return -ENOSYS; /* Command not supported */
    }
    
    return ret;
}

/**
  * @brief  Read data from serial port
  * @param  port: pointer to serial device
  * @param  buffer: pointer to buffer
  * @param  size: size of buffer
  * @retval number of bytes read
  *         -EINVAL invalid parameter
  */
ssize_t serial_read(serial_t *port, void *buffer, size_t size)
{
    ssize_t ret = 0;
    
    if(!port || !buffer || size == 0) {
        return -EINVAL;
    }
    if (!port->opened) {
        return -EIO;
    }
    ret = kfifo_out(&port->rx_fifo, buffer, size);
    return ret;
}

/**
  * @brief  Start transfer operation
  * @param  port: pointer to serial device
  * @retval None
  */
static void start_transfer(serial_t *port)
{
    if (!port || !port->ops || !port->ops->send) {
        return;
    }
    
    size_t off = 0;
    size_t len = kfifo_out_linear_locked(&port->tx_fifo, &off, kfifo_len(&port->tx_fifo));
    if (len == 0)
        return;

    uint8_t *data_ptr = (uint8_t*)port->tx_fifo.data;
    int hw_ret = port->ops->send(port, &data_ptr[off], len);
    if (hw_ret == 0) {
        port->current_tx_len = len;
    } else {
        port->current_tx_len = 0;
    }
}

/**
  * @brief  Write data to serial port
  * @param  port: pointer to serial device
  * @param  buffer: pointer to buffer
  * @param  size: size of buffer
  * @retval number of bytes written
  *         -EINVAL invalid parameter
  *         -EIO I/O error
  */
ssize_t serial_write(serial_t *port, const void *buffer, size_t size)
{
    ssize_t ret = 0;
    if (!port || !buffer || size == 0)
        return -EINVAL;
    
    if (!port->opened)
        return -EIO;
    
    ret = kfifo_in(&port->tx_fifo, buffer, size);

    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    bool is_busy = port->ops->tx_is_busy(port);
    __set_PRIMASK(primask);
    
    if (is_busy == false) {
        start_transfer(port);
    }
    return ret;
}

/**
  * @brief  Serial transmit complete callback (called from hardware driver)
  * @param  port: pointer to serial device
  * @retval None
  */
void hw_serial_tx_done_isr(serial_t *port)
{
    if (!port)
        return;

    /* 跳过已发送的数据 */
    kfifo_skip_count(&port->tx_fifo, port->current_tx_len);
    port->current_tx_len = 0;

    /* 如果还有数据，立即启动下一次发送 */
    if (!kfifo_is_empty(&port->tx_fifo)) {
        start_transfer(port);
    }
}

/**
  * @brief  Serial receive complete callback (called from hardware driver)
  * @param  port: pointer to serial device
  * @param  buf: pointer to received data buffer
  * @param  size: size of received data
  * @retval None
  */
void hw_serial_rx_done_isr(serial_t *port, const uint8_t *buf, uint16_t size)
{
    if (!port || !buf || size == 0)
        return;

    size_t stored = kfifo_in(&port->rx_fifo, buf, size);
    if (stored != size) {
        // LOG
    }
}

/**
  * @brief  Register serial device (called by hardware driver)
  * @param  port: pointer to serial device
  * @param  name: name of serial device
  * @retval 0 on success
  *         -EINVAL invalid parameter
  */
int hw_serial_register(serial_t *port, const char *name)
{
    if (port == NULL || name == NULL) {
        return -EINVAL;
    }
    
    /* Prevent duplicate names */
    if (serial_find(name) != NULL) {
        return -EEXIST;
    }
    
    /* Set device name */
    strncpy(port->name, name, SERIAL_NAME_MAX);
    port->name[SERIAL_NAME_MAX-1] = '\0';
    
    /* Add to device list */
    list_add_tail(&port->node, &serial_list);
    
    return 0;
}


/* Private functions ---------------------------------------------------------*/
