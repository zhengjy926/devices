/**
  ******************************************************************************
  * @file        : spi.h
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2025-01-XX
  * @brief       : SPI Driver Framework Header File (Linux Kernel Style)
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1. Complete refactoring with Linux kernel style
  *                2. Thread-safe design for bare-metal and RTOS
  *                3. Compiler optimization compatible
  *
  ******************************************************************************
  */
#ifndef __SPI_H__
#define __SPI_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "my_list.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/**
 * @defgroup SPI Mode Definitions
 * @{
 */
#define SPI_CPHA            (1U<<0)                  /**< Clock Phase (CPHA) bit definition */
#define SPI_CPOL            (1U<<1)                  /**< Clock Polarity (CPOL) bit definition */

#define SPI_MODE_0          (0U | 0U)                /**< SPI Mode 0: CPOL = 0, CPHA = 0 */
#define SPI_MODE_1          (0U | SPI_CPHA)          /**< SPI Mode 1: CPOL = 0, CPHA = 1 */
#define SPI_MODE_2          (SPI_CPOL | 0U)          /**< SPI Mode 2: CPOL = 1, CPHA = 0 */
#define SPI_MODE_3          (SPI_CPOL | SPI_CPHA)    /**< SPI Mode 3: CPOL = 1, CPHA = 1 */

#define SPI_MODE_LSB        (0U<<2)                  /**< LSB First transmission mode */
#define SPI_MODE_MSB        (1U<<2)                  /**< MSB First transmission mode */

#define SPI_MODE_SW_CS      (0U<<3)                  /**< Software controlled chip select */
#define SPI_MODE_HW_CS      (1U<<3)                  /**< Hardware controlled chip select */

#define SPI_MODE_4WIRE      (0U<<4)                  /**< Standard 4-wire SPI mode */
#define SPI_MODE_3WIRE      (1U<<4)                  /**< 3-wire SPI mode with MOSI/MISO multiplexed */

#define SPI_NAME_MAX        (16U)                    /**< Maximum length of SPI device name */
/** @} */

/* Forward declarations */
struct spi_device;
struct spi_controller;
struct spi_controller_ops;

/**
 * struct spi_delay - SPI delay information
 * @value: Value for the delay
 * @unit: Unit for the delay
 */
struct spi_delay {
#define SPI_DELAY_UNIT_USECS	0
#define SPI_DELAY_UNIT_NSECS	1
#define SPI_DELAY_UNIT_SCK	    2
	uint16_t	value;
	uint8_t	unit;
};

/**
 * @brief SPI Transfer Structure
 * @details Single transfer descriptor, can be linked into a message
 */
struct spi_transfer {
    const void *tx_buf;                /**< Pointer to transmit buffer */
    void *rx_buf;                      /**< Pointer to receive buffer */
    size_t len;                        /**< Length of data to transfer (bytes) */
    unsigned cs_change : 1;            /**< Change chip select state after transfer */
    struct list_node transfer_list;    /**< Message list node */
};

/**
 * @brief SPI Message Structure
 * @details Message queue containing multiple transfers
 */
struct spi_message {
    struct list_node transfers;        /**< Transfer list head */
    struct spi_device *spi;            /**< Associated SPI device */
    int status;                        /**< Transfer status: 0=success, <0=error */
    void *context;                     /**< Context pointer (optional) */
};

/**
 * @brief SPI Device Structure
 * @details Each SPI device must be connected to a controller
 */
struct spi_device {
    const char *name;                  /**< Device name */
    struct spi_controller *controller; /**< Parent controller */
    uint32_t max_speed_hz;             /**< Maximum clock frequency (Hz) */
    uint8_t chip_select;               /**< CS number (hardware CS, 0-15) */
    uint8_t mode;                      /**< SPI configuration flags (bit field):
                                         *   - bit 0: CPHA (Clock Phase)
                                         *   - bit 1: CPOL (Clock Polarity)
                                         *   - bit 2: MSB/LSB (SPI_MODE_MSB/LSB)
                                         *   - bit 3: CS control (SPI_MODE_HW_CS for hardware CS, 0 for software CS)
                                         *   - bit 4: Wire mode (SPI_MODE_3WIRE/4WIRE)
                                         *   Use SPI_MODE_0/1/2/3 for basic mode, combine with other flags as needed */
    uint8_t bits_per_word;             /**< Bits per word (usually 8) */
    size_t cs_pin;                     /**< CS pin (software CS only, valid when mode has SPI_MODE_SW_CS bit set) */
    void *controller_data;             /**< Controller private data */
};

/**
 * @brief SPI Controller Operations Structure
 * @details Hardware-specific operations that must be implemented by BSP layer
 */
struct spi_controller_ops {
    /**
     * @brief Configure SPI controller parameters
     * @param ctrl Controller pointer
     * @param dev Device pointer
     * @return 0 on success, error code on failure
     */
    int (*setup)(struct spi_controller *ctrl, struct spi_device *dev);
    
    /**
     * @brief Set chip select state (software CS)
     * @param ctrl Controller pointer
     * @param dev Device pointer
     * @param enable 1=activate (pull low), 0=release (pull high)
     */
    void (*set_cs)(struct spi_controller *ctrl, struct spi_device *dev, uint8_t enable);
    
    /**
     * @brief Execute transfer
     * @param ctrl Controller pointer
     * @param dev Device pointer
     * @param transfer Transfer descriptor pointer
     * @return Number of bytes transferred on success, error code on failure
     */
    ssize_t (*transfer_one)(struct spi_controller *ctrl, 
                           struct spi_device *dev,
                           struct spi_transfer *transfer);
};

/**
 * @brief SPI Controller Structure
 * @details Controller abstraction, manages configuration and thread safety
 */
struct spi_controller {
    struct list_node node;                      /**< List node */
    char name[SPI_NAME_MAX];                    /**< Controller name */
    const struct spi_controller_ops *ops;       /**< Operation functions */
    void *priv;                                 /**< Private data (points to BSP implementation) */
    uint8_t mode;                               /**< Current mode */
    uint8_t bits_per_word;                      /**< Current bits per word */
    uint32_t max_speed_hz;                      /**< Maximum speed */
    uint32_t actual_speed_hz;                   /**< Actual configured speed */
    struct spi_device *current_device;          /**< Currently configured device */
};

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

int spi_controller_register(struct spi_controller *ctrl, 
                            const char *name, 
                            const struct spi_controller_ops *ops);
struct spi_controller *spi_controller_find(const char *name);
int spi_device_attach(struct spi_device *dev, const char *controller_name);
void spi_message_init(struct spi_message *m);
void spi_message_add_tail(struct spi_transfer *t, struct spi_message *m);
int spi_sync(struct spi_device *dev, struct spi_message *message);

/**
 * @brief Write data to SPI device
 * @param spi SPI device pointer
 * @param buf Data buffer to write
 * @param len Length of data
 * @return Number of bytes written on success, error code on failure
 */
static inline int
spi_write(struct spi_device *spi, const void *buf, size_t len)
{
    struct spi_message m;
    struct spi_transfer t;
    
    if ((spi == NULL) || (buf == NULL) || (len == 0U)) {
        return -22; /* -EINVAL */
    }
    
    spi_message_init(&m);
    
    t.tx_buf = buf;
    t.rx_buf = NULL;
    t.len = len;
    t.cs_change = 0U;
    list_node_init(&t.transfer_list);
    
    spi_message_add_tail(&t, &m);
    
    return spi_sync(spi, &m);
}

/**
 * @brief Read data from SPI device
 * @param spi SPI device pointer
 * @param buf Buffer to store read data
 * @param len Length of data to read
 * @return Number of bytes read on success, error code on failure
 */
static inline int
spi_read(struct spi_device *spi, void *buf, size_t len)
{
    struct spi_message m;
    struct spi_transfer t;
    
    if ((spi == NULL) || (buf == NULL) || (len == 0U)) {
        return -22; /* -EINVAL */
    }
    
    spi_message_init(&m);
    
    t.tx_buf = NULL;
    t.rx_buf = buf;
    t.len = len;
    t.cs_change = 0U;
    list_node_init(&t.transfer_list);
    
    spi_message_add_tail(&t, &m);
    
    return spi_sync(spi, &m);
}

/**
 * @brief Write then read operation
 * @param spi SPI device pointer
 * @param txbuf Buffer containing data to write
 * @param txlen Length of data to write
 * @param rxbuf Buffer to store read data
 * @param rxlen Length of data to read
 * @return Number of bytes transferred on success, error code on failure
 */
static inline int
spi_write_then_read(struct spi_device *spi, 
                    const void *txbuf, size_t txlen, 
                    void *rxbuf, size_t rxlen)
{
    struct spi_message m;
    struct spi_transfer t_tx;
    struct spi_transfer t_rx;
    int ret;
    
    if ((spi == NULL) || (txbuf == NULL) || (rxbuf == NULL) || 
        (txlen == 0U) || (rxlen == 0U)) {
        return -22; /* -EINVAL */
    }
    
    spi_message_init(&m);
    
    /* TX transfer */
    t_tx.tx_buf = txbuf;
    t_tx.rx_buf = NULL;
    t_tx.len = txlen;
    t_tx.cs_change = 0U;
    list_node_init(&t_tx.transfer_list);
    spi_message_add_tail(&t_tx, &m);
    
    /* RX transfer */
    t_rx.tx_buf = NULL;
    t_rx.rx_buf = rxbuf;
    t_rx.len = rxlen;
    t_rx.cs_change = 1U;
    list_node_init(&t_rx.transfer_list);
    spi_message_add_tail(&t_rx, &m);
    
    ret = spi_sync(spi, &m);
    
    return ret;
}

/**
 * @brief Write one byte then read one byte
 * @param spi SPI device pointer
 * @param cmd Command byte to write
 * @return Byte read on success, error code on failure
 */
static inline int
spi_w8r8(struct spi_device *spi, uint8_t cmd)
{
    int status;
    uint8_t result;
    
    if (spi == NULL) {
        return -22; /* -EINVAL */
    }
    
    status = spi_write_then_read(spi, &cmd, 1U, &result, 1U);
    
    if (status < 0) {
        return status;
    }
    
    return (int)result;
}

/**
 * @brief Write one byte then read two bytes
 * @param spi SPI device pointer
 * @param cmd Command byte to write
 * @param result Pointer to store 16-bit result
 * @return 0 on success, error code on failure
 */
static inline int
spi_w8r16(struct spi_device *spi, uint8_t cmd, uint16_t *result)
{
    int status;
    uint16_t temp_result;
    
    if ((spi == NULL) || (result == NULL)) {
        return -22; /* -EINVAL */
    }
    
    status = spi_write_then_read(spi, &cmd, 1U, &temp_result, 2U);
    
    if (status < 0) {
        return status;
    }
    
    *result = temp_result;
    return 0;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SPI_H__ */
