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
#include <string.h>
#include "cmsis_compiler.h"
#include <assert.h>

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
        return NULL;
    }
    
    /* Find device by traversing the double linked list */
    list_for_each(node, &serial_list) {
        dev = list_entry(node, struct serial, node);
        if (dev != NULL && strcmp(dev->name, name) == 0) {
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
int serial_init(serial_t *port)
{
    int ret = 0;
    
    assert(port != NULL);
    
    if (port->flags.initialized) {
        return 0;
    }
    assert(port->ops->init != NULL);
    
    /* Initialize hardware */
    ret = port->ops->init(port);
    if (ret) {
        return ret;
    }
    
    assert(port->rx_buf != NULL && port->rx_bufsz != 0);
    assert(port->tx_buf != NULL && port->tx_bufsz != 0);
    /* Initialize RX FIFO */
    ret = kfifo_init(&port->rx_fifo, port->rx_buf, port->rx_bufsz, 1);
    if (ret) {
        return ret;
    }
    /* Initialize TX FIFO */
    ret = kfifo_init(&port->tx_fifo, port->tx_buf, port->tx_bufsz, 1);
    if (ret) {
        return ret;
    }

    /* Initialize configuration with default values */
    struct serial_configure default_cfg = SERIAL_CONFIG_DEFAULT;
    port->config = default_cfg;

    port->flags.initialized = 1;
    
#if USING_RTOS
    /* Create FreeRTOS synchronization objects */
    port->tx_sem = xSemaphoreCreateBinary();
    if (!port->tx_sem) {
        return -ENOMEM;
    }
    
    port->write_mutex = xSemaphoreCreateMutex();
    if (!port->write_mutex) {
        vSemaphoreDelete(port->tx_sem);
        return -ENOMEM;
    }
    
    port->read_mutex = xSemaphoreCreateMutex();
    if (!port->read_mutex) {
        vSemaphoreDelete(port->tx_sem);
        vSemaphoreDelete(port->write_mutex);
        return -ENOMEM;
    }
    
    /* Create TX task */
    BaseType_t task_ret = xTaskCreate(serial_tx_task, port->name, 256, port, 
                                      UART_TX_TASK_PRIORITY, &port->tx_task);
    if (task_ret != pdPASS) {
        vSemaphoreDelete(port->tx_sem);
        vSemaphoreDelete(port->write_mutex);
        vSemaphoreDelete(port->read_mutex);
        return -ENOMEM;
    }
#endif
    return 0;
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
    
    if (!port->flags.initialized) {
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
    
    if (size == 0)
        return 0;
    
    assert((port != NULL) && (buffer != NULL));
    
    if (!port->flags.initialized) {
        return -EIO;
    }
    
#if USING_RTOS
    // FreeRTOS环境：使用互斥量保护多任务读取
    if (port->read_mutex) {
        xSemaphoreTake(port->read_mutex, portMAX_DELAY);
    }
    
    ret = kfifo_out_locked(&port->rx_fifo, buffer, size);
    
    if (port->read_mutex) {
        xSemaphoreGive(port->read_mutex);
    }
#else
    // 裸机环境：直接读取，无需保护（单生产者-单消费者）
    ret = kfifo_out(&port->rx_fifo, buffer, size);
#endif
    
    return ret;
}

#if USING_RTOS
/**
  * @brief  Serial TX task for FreeRTOS
  * @param  arg: pointer to serial device
  * @retval None
  */
static void serial_tx_task(void *arg)
{
    serial_t *port = (serial_t *)arg;
    
    for(;;)
    {
        // 等待发送触发
        if(xSemaphoreTake(port->tx_sem, portMAX_DELAY) == pdTRUE) {
            // 获取写入互斥量，防止与写入操作冲突
            if (xSemaphoreTake(port->write_mutex, portMAX_DELAY) == pdTRUE) {
                
                taskENTER_CRITICAL();
                if(!kfifo_is_empty(&port->tx_fifo) && !port->flags.tx_busy) {
                    start_transfer(port);
                    taskEXIT_CRITICAL();
                    xSemaphoreGive(port->write_mutex);
                } else {
                    taskEXIT_CRITICAL();
                    xSemaphoreGive(port->write_mutex);
                }
            }
        }
    }
}
#endif

/**
  * @brief  Start transfer operation
  * @param  port: pointer to serial device
  * @retval None
  */
static void start_transfer(serial_t *port)
{
    size_t len = 0;
    size_t off_index = 0;  /* FIFO data offset index */
    uint8_t *data_ptr;
    
    if (!port->ops || !port->ops->send) {
        return;
    }
    
    /* Get pointer to data in FIFO and available length */
    len = kfifo_out_linear(&port->tx_fifo, &off_index, kfifo_len(&port->tx_fifo));
    
    if(len > 0)
    {
        data_ptr = (uint8_t *)port->tx_buf;

        /* Send data using hardware-specific function */
        if (port->ops->send(port, &data_ptr[off_index], len) == 0) {
            port->current_tx_len = len;
        } else {
//            LOG_E("Hard busy!\r\n");
            port->current_tx_len = 0;
        }
        port->flags.tx_busy = 1;
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
    
    if (!port || !buffer || size == 0) {
        return -EINVAL;
    }
    
    if (!port->flags.initialized) {
        return -EIO;
    }
    
#if USING_RTOS
    // FreeRTOS环境：使用互斥量保护多任务写入
    if (port->write_mutex) {
        
        if (xSemaphoreTake(port->write_mutex, portMAX_DELAY) == pdTRUE ) {
            
            taskENTER_CRITICAL();
            ret = kfifo_in(&port->tx_fifo, buffer, size);
//            if (ret != size)
//                LOG_W("tx fifo full\r\n");
            uint8_t flag = port->flags.tx_busy;
            taskEXIT_CRITICAL();
            
            // 如果当前没有发送，触发发送任务
            if (!flag) {
                xSemaphoreGive(port->tx_sem);
            }
            
            xSemaphoreGive(port->write_mutex);
        }
    }
#else
    
    ret = kfifo_in(&port->tx_fifo, buffer, size);
//    if (ret != size)
//        LOG_W("tx fifo full\r\n");

    // 裸机环境：关中断保护全局变量修改
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    // 如果写入成功且当前没有发送，启动发送
    if (!(port->flags.tx_busy)) {
        start_transfer(port);
    }
    __set_PRIMASK(primask);
#endif
    
    return ret;
}

/**
  * @brief  Serial transmit complete callback (called from hardware driver)
  * @param  port: pointer to serial device
  * @retval None
  */
void hw_serial_tx_done_isr(serial_t *port)
{
    if (!port) {
        return;
    }
    
    kfifo_skip_count(&port->tx_fifo, port->current_tx_len);
    port->flags.tx_busy = 0;
    port->current_tx_len = 0;
    
#if USING_RTOS
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // 触发发送任务检查是否还有数据需要发送
    if (port->tx_sem) {
        xSemaphoreGiveFromISR(port->tx_sem, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
#else
    // 裸机环境：直接检查并启动下一次发送
    if (!kfifo_is_empty(&port->tx_fifo)) {
        start_transfer(port);
    }
#endif
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
    if (size == 0) {
        return;
    }

    // 直接写入，无需保护（单生产者）
    size_t stored = kfifo_in(&port->rx_fifo, buf, size);
//    if (stored != size) {
//        LOG_W("rx fifo full!\r\n");
//    }
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
