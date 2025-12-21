/**
  ******************************************************************************
  * @file        : serial.h
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2025-01-27
  * @brief       : Generic serial port driver interface
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.Enhanced interface and improved documentation
  *
  ******************************************************************************
  */
#ifndef __SERIAL_H__
#define __SERIAL_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "kfifo.h"
#include "my_list.h"
#include <stdbool.h>

#if USING_RTOS
    #include "FreeRTOS.h"
    #include "task.h"
    #include "semphr.h"
    
    #define UART_TX_TASK_PRIORITY (configMAX_PRIORITIES - 2)
#endif // USING_RTOS

/* Exported define -----------------------------------------------------------*/
#define BAUD_RATE_2400                  2400
#define BAUD_RATE_4800                  4800
#define BAUD_RATE_9600                  9600
#define BAUD_RATE_19200                 19200
#define BAUD_RATE_38400                 38400
#define BAUD_RATE_57600                 57600
#define BAUD_RATE_115200                115200
#define BAUD_RATE_230400                230400
#define BAUD_RATE_460800                460800
#define BAUD_RATE_500000                500000
#define BAUD_RATE_921600                921600
#define BAUD_RATE_2000000               2000000
#define BAUD_RATE_2500000               2500000
#define BAUD_RATE_3000000               3000000

#define DATA_BITS_5                     5
#define DATA_BITS_6                     6
#define DATA_BITS_7                     7
#define DATA_BITS_8                     8
#define DATA_BITS_9                     9

#define STOP_BITS_1                     0
#define STOP_BITS_2                     1
#define STOP_BITS_3                     2
#define STOP_BITS_4                     3

#define PARITY_NONE                     0
#define PARITY_ODD                      1
#define PARITY_EVEN                     2

#define SERIAL_FLOWCONTROL_CTSRTS       1
#define SERIAL_FLOWCONTROL_NONE         0

/* Serial control commands */
#define SERIAL_CMD_SET_CONFIG           0x01
#define SERIAL_CMD_GET_CONFIG           0x02
#define SERIAL_CMD_FLUSH_RX             0x03
#define SERIAL_CMD_FLUSH_TX             0x04

#define SERIAL_NAME_MAX                 (8)                    /**< Maximum length of SPI device name */

/* Default config for serial_configure structure */
#define SERIAL_CONFIG_DEFAULT                         \
{                                                     \
    .baud_rate = BAUD_RATE_115200,                    \
    .data_bits = DATA_BITS_8,                         \
    .stop_bits = STOP_BITS_1,                         \
    .parity = PARITY_NONE,                            \
    .flowcontrol = SERIAL_FLOWCONTROL_NONE,           \
    .reserved = 0                                     \
}

/* Exported typedef ----------------------------------------------------------*/
/* Forward declaration */
struct serial;

/**
 * @brief Serial configuration structure
 */
struct serial_configure
{
    uint32_t baud_rate;                 ///< Baud rate
    
    uint32_t data_bits      :4;         ///< Data bits
    uint32_t stop_bits      :2;         ///< Stop bits
    uint32_t parity         :2;         ///< Parity
    uint32_t flowcontrol    :1;         ///< Flow control
    uint32_t reserved       :23;         ///< Reserved
};

/**
 * @brief Serial operations function set
 */
typedef struct serial_ops
{
    int (*init)(struct serial *port);                                    ///< Initialize the serial port
    int (*send)(struct serial *port, const void *buf, size_t size);      ///< Send data to the serial port
    int (*start_rx)(struct serial *port);                                ///< Start the receive operation
    int (*configure)(struct serial *port, struct serial_configure *cfg); ///< Configure serial parameters
    bool (*tx_is_busy)(struct serial *port);
} serial_ops_t;

/**
 * @brief Serial device structure
 */
typedef struct serial{
    char name[SERIAL_NAME_MAX];         ///< Serial name
    volatile uint8_t opened;            
    const serial_ops_t *ops;            ///< Low-level operation function pointer
    struct serial_configure config;     ///< Current serial configuration
    size_t rx_bufsz;                    ///< The size of rx buffer
    size_t tx_bufsz;                    ///< The size of tx buffer
    void *rx_buf;                       ///< Pointer to rx Buffer
    void *tx_buf;                       ///< Pointer to tx Buffer
    kfifo_t rx_fifo;                    ///< rx fifo
    kfifo_t tx_fifo;                    ///< tx fifo
    volatile size_t current_tx_len;     ///< The length of current tx data
    list_t node;                        ///< Node of serial list
    void *prv_data;                     ///< Private data
}serial_t;

/* Exported macro ------------------------------------------------------------*/

/* Exported variable prototypes ----------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/
/**
 * @brief Serial device management functions
 */
serial_t* serial_find    (const char *name);
int       serial_open    (serial_t *port);
void      serial_close   (serial_t *port);
int       serial_read    (serial_t *port, void *buffer, size_t size);
int       serial_write   (serial_t *port, const void *buffer, size_t size);
int       serial_control (serial_t *port, int cmd, void *arg);


/**
 * @brief Hardware-specific functions (called by low-level drivers)
 */
int  hw_serial_register     (serial_t *port, const char *name);
void hw_serial_rx_done_isr  (serial_t *port, const uint8_t *buf, uint16_t size);
void hw_serial_tx_done_isr  (serial_t *port);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SERIAL_H__ */

