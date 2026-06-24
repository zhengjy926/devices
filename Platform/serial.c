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
#define  LOG_LVL             ELOG_LVL_DEBUG
#include "elog.h"

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
serial_t* Serial_Find(const char *name)
{
    serial_t *dev = NULL;
    list_t *node = NULL;
    
    /* Parameter check */
    if (name == NULL) {
        log_e("serial name is NULL");
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
  *         -ERR_INVAL invalid parameter
  *         -ERR_IO I/O error
  */
int32_t Serial_Open(serial_t *port)
{
    int ret = 0;
    
    if (!port) {
        log_e("");
        return -ERR_INVAL;
    }
    
    if (port->opened) {
        return 0;
    }
    
    if (!port->ops || !port->ops->init) {
        return -ERR_INVAL;
    }

    if (!port->rx_buf || port->rx_bufsz == 0 || !port->tx_buf || port->tx_bufsz == 0) {
        return -ERR_INVAL;
    }
    
    /* Initialize hardware */
    ret = port->ops->init(port);
    if (ret) {
        return ret;
    }
    /* Initialize FIFO */
    ret = Kfifo_Init(&port->rx_fifo, port->rx_buf, port->rx_bufsz, 1);
    if (ret) {
        return ret;
    }
    ret = Kfifo_Init(&port->tx_fifo, port->tx_buf, port->tx_bufsz, 1);
    if (ret) {
        return ret;
    }

    /* Initialize configuration with default values */
    struct serial_configure default_cfg = SERIAL_CONFIG_DEFAULT;
    port->config = default_cfg;
    port->opened = 1;
    return 0;
}

void Serial_Close(serial_t *port)
{
    
}

/**
  * @brief  Control serial port parameters
  * @param  port: pointer to serial device
  * @param  cmd: control command
  * @param  arg: control argument
  * @retval 0 on success, negative error code on failure
  */
int32_t Serial_Control(serial_t *port, int cmd, void *arg)
{
    int ret = 0;
    
    if (!port) {
        return -ERR_INVAL;
    }
    
    if (!port->opened) {
        return -ERR_IO;
    }
    
    switch (cmd) {
        case SERIAL_CMD_SET_CONFIG:
        {
            struct serial_configure *config = (struct serial_configure *)arg;
            if (!config) {
                return -ERR_INVAL;
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
                return -ERR_INVAL;
            }
            
            *config = port->config;
            break;
        }
        
        default:
            return -ERR_NOSYS; /* Command not supported */
    }
    
    return ret;
}

/**
  * @brief  Read data from serial port
  * @param  port: pointer to serial device
  * @param  buffer: pointer to buffer
  * @param  size: size of buffer
  * @retval number of bytes read
  *         -ERR_INVAL invalid parameter
  */
int32_t Serial_Read(serial_t *port, void *buffer, size_t size)
{
    int32_t ret = 0;
    
    if(!port || !buffer || size == 0) {
        return -ERR_INVAL;
    }
    if (!port->opened) {
        return -ERR_IO;
    }
    ret = Kfifo_Out(&port->rx_fifo, buffer, size);
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
    size_t len = Kfifo_OutLinear(&port->tx_fifo, &off, Kfifo_Len(&port->tx_fifo));
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
  *         -ERR_INVAL invalid parameter
  *         -ERR_IO I/O error
  */
int32_t Serial_Write(serial_t *port, const void *buffer, size_t size)
{
    int32_t ret = 0;
    if (!port || !buffer || size == 0)
        return -ERR_INVAL;
    
    if (!port->opened)
        return -ERR_IO;
    
    ret = Kfifo_In(&port->tx_fifo, buffer, size);

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
void Serial_TxIsrHook(serial_t *port)
{
    if (!port)
        return;

    /* 跳过已发送的数据 */
    Kfifo_SkipCount(&port->tx_fifo, port->current_tx_len);
    port->current_tx_len = 0;

    /* 如果还有数据，立即启动下一次发送 */
    if (Kfifo_Len(&port->tx_fifo)) {
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
void Serial_RxIsrHook(serial_t *port, const uint8_t *buf, uint16_t size)
{
    if (!port || !buf || size == 0)
        return;

    size_t stored = Kfifo_In(&port->rx_fifo, buf, size);
    if (stored != size) {
        log_w("%s: stored != size", port->name);
    }
}

/**
  * @brief  Register serial device (called by hardware driver)
  * @param  port: pointer to serial device
  * @param  name: name of serial device
  * @retval 0 on success
  *         -ERR_INVAL invalid parameter
  */
int32_t Serial_Register(serial_t *port, const char *name)
{
    if (port == NULL || name == NULL) {
        return -ERR_INVAL;
    }
    
    /* Prevent duplicate names */
    if (Serial_Find(name) != NULL) {
        return -ERR_EXIST;
    }
    
    /* Set device name */
    strncpy(port->name, name, SERIAL_NAME_MAX);
    port->name[SERIAL_NAME_MAX-1] = '\0';
    
    /* Add to device list */
    list_add_tail(&port->node, &serial_list);
    
    return 0;
}


/* Private functions ---------------------------------------------------------*/
