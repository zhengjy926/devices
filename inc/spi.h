/**
  ******************************************************************************
  * @file        : spi.h
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 20xx-xx-xx
  * @brief       : SPI Driver Framework Header File
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.xxx
  *
  *
  ******************************************************************************
  */
#ifndef __SPI_H__
#define __SPI_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "sys_def.h"
#include "my_list.h"
#include <string.h>
#include "sys_config.h"

/**
 * @defgroup SPI_Mode_Definitions SPI Mode Definitions
 * @{
 */
#define SPI_CPHA            (1<<0)                  /**< Clock Phase (CPHA) bit definition */
#define SPI_CPOL            (1<<1)                  /**< Clock Polarity (CPOL) bit definition */

#define SPI_MODE_0          (0 | 0)                 /**< SPI Mode 0: CPOL = 0, CPHA = 0 */
#define SPI_MODE_1          (0 | SPI_CPHA)          /**< SPI Mode 1: CPOL = 0, CPHA = 1 */
#define SPI_MODE_2          (SPI_CPOL | 0)          /**< SPI Mode 2: CPOL = 1, CPHA = 0 */
#define SPI_MODE_3          (SPI_CPOL | SPI_CPHA)   /**< SPI Mode 3: CPOL = 1, CPHA = 1 */

#define SPI_MODE_LSB        (0<<2)                  /**< LSB First transmission mode */
#define SPI_MODE_MSB        (1<<2)                  /**< MSB First transmission mode */

#define SPI_MODE_SW_CS      (0<<3)                  /**< Software controlled chip select */
#define SPI_MODE_HW_CS      (1<<3)                  /**< Hardware controlled chip select */

#define SPI_MODE_4WIRE      (0<<4)                  /**< Standard 4-wire SPI mode */
#define SPI_MODE_3WIRE      (1<<4)                  /**< 3-wire SPI mode with MOSI/MISO multiplexed */

#define SPI_NAME_MAX        (16)                    /**< Maximum length of SPI device name */
/** @} */

struct spi_device;
struct spi_ops;

/**
 * @brief SPI Message Structure
 */
struct spi_message
{
    const void *send_buf;           /**< Pointer to transmit buffer */
    void *recv_buf;                 /**< Pointer to receive buffer */
    size_t length;                  /**< Length of data to transfer */
    unsigned cs_change : 1;         /**< Change chip select state after transfer */
    struct spi_message *next;       /**< Pointer to next message in queue */
};

/**
 * @brief SPI Bus Structure
 */
struct spi_bus
{
    list_t                 node;               /**< Bus node */
    char                    name[SPI_NAME_MAX]; /**< Bus name */
    uint8_t                 mode;               /**< SPI mode */
    uint8_t                 data_width;         /**< Data width in bits */
    uint32_t                max_hz;             /**< Maximum transfer rate */
    uint32_t                actual_hz;
    const struct spi_ops    *ops;               /**< SPI operation functions */
    void                    *hw_data;           /**< Hardware specific data */
};

/**
 * @brief SPI Device Structure
 * @details Each SPI device must be connected to a virtual bus
 */
struct spi_device
{
    const char      *name;          /**< Device name */
    struct spi_bus  *bus;           /**< Parent bus */
    uint8_t         mode;           /**< Device operating mode */
    uint8_t         data_width;     /**< Data width in bits */
    uint32_t        max_hz;         /**< Maximum clock frequency */
    size_t          cs_pin;         /**< Chip select pin */
};

/**
 * @brief SPI Operations Structure
 */
struct spi_ops
{
    /**
     * @brief Configure SPI device
     * @param device SPI device pointer
     * @return 0 on success, error code on failure
     */
    int (*configure)(struct spi_device *dev);
    
    /**
     * @brief Set chip select state
     * @param spi SPI device pointer
     * @param enable 1 to enable, 0 to disable chip select
     */
    void (*set_cs)(struct spi_device *dev, uint8_t enable);
    
    /**
     * @brief Transfer data
     * @param device SPI device pointer
     * @param message Message structure pointer
     * @return Number of bytes transferred on success, error code on failure
     */
    ssize_t (*xfer)(struct spi_device *dev, struct spi_message *message);
};

/**
 * @brief Attach SPI device to bus
 * @param dev SPI device pointer
 * @param bus_name Bus name
 * @return 0 on success, error code on failure
 */
int spi_device_attach_bus(struct spi_device *dev, const char *bus_name);

/**
 * @brief Transfer SPI message
 * @param device SPI device pointer
 * @param message Message structure pointer
 * @return Number of bytes transferred on success, error code on failure
 */
int spi_transfer_message(struct spi_device *device, struct spi_message *message);

/**
 * @brief Register SPI bus
 * @param bus Bus structure pointer
 * @param bus_name Bus name
 * @param ops Operation functions pointer
 * @return 0 on success, error code on failure
 */
int spi_bus_register(struct spi_bus       *bus,
                     const char           *bus_name, 
                     const struct spi_ops *ops);

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
    struct spi_message   m = {
            .send_buf   = buf,
            .length     = len,
        };

    return spi_transfer_message(spi, &m);
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
    struct spi_message   m = {
            .recv_buf   = buf,
            .length     = len,
        };

    return spi_transfer_message(spi, &m);
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
spi_write_then_read(struct spi_device *spi, const void *txbuf, size_t txlen, void *rxbuf, size_t rxlen)
{
    struct spi_message msg_tx, msg_rx = {0};
    
    msg_tx.send_buf = txbuf;
    msg_tx.recv_buf = NULL;
    msg_tx.length = txlen;
    msg_tx.cs_change = 0;
    msg_tx.next = &msg_rx;
    
    msg_rx.send_buf = NULL;
    msg_rx.recv_buf = rxbuf;
    msg_rx.length = rxlen;
    msg_rx.cs_change = 1;
    msg_rx.next = NULL;

    return spi_transfer_message(spi, &msg_tx);
}

/**
 * @brief Write one byte then read one byte
 * @param spi SPI device pointer
 * @param cmd Command byte to write
 * @return Byte read on success, error code on failure
 */
static inline ssize_t spi_w8r8(struct spi_device *spi, uint8_t cmd)
{
    ssize_t status;
    uint8_t result;

    status = spi_write_then_read(spi, &cmd, 1, &result, 1);

    return (status < 0) ? status : result;
}

/**
 * @brief Write one byte then read two bytes
 * @param spi SPI device pointer
 * @param cmd Command byte to write
 * @return Two bytes read on success, error code on failure
 */
static inline ssize_t spi_w8r16(struct spi_device *spi, uint8_t cmd)
{
    ssize_t status;
    uint16_t result;

    status = spi_write_then_read(spi, &cmd, 1, &result, 2);

    return (status < 0) ? status : result;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SPI_H__ */
