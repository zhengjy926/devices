/**
  ******************************************************************************
  * @file        : spi_nor.c
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 20xx-xx-xx
  * @brief       : SPI NOR Flash驱动实现
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.xxx
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "spi_nor.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define SPI_NOR_TIMEOUT_MS          1000    /* 操作超时时间(ms) */
#define SPI_NOR_WRITE_TIMEOUT_MS    3000    /* 写操作超时时间(ms) */
#define SPI_NOR_ERASE_TIMEOUT_MS    5000    /* 擦除操作超时时间(ms) */

/* Private macro -------------------------------------------------------------*/
uint32_t HAL_GetTick(void);

#define SPI_NOR_WAIT_TIMEOUT(timeout_ms) \
    uint32_t start = HAL_GetTick(); \
    while((HAL_GetTick() - start) < timeout_ms)

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static int spi_nor_wait_ready(struct spi_nor *nor, uint32_t timeout_ms);
static int spi_nor_write_enable(struct spi_nor *nor);

/* Exported functions --------------------------------------------------------*/

/* 基本电源管理与控制命令 */
int spi_nor_power_down(struct spi_nor *nor)
{
    if (!nor || !nor->spi)
        return -EINVAL;
        
    return spi_w8r8(nor->spi, SPINOR_CMD_POWER_DOWN);
}

int spi_nor_release_power_down(struct spi_nor *nor)
{
    if (!nor || !nor->spi)
        return -EINVAL;
        
    return spi_w8r8(nor->spi, SPINOR_CMD_RELEASE_POWER_DOWN);
}

int spi_nor_software_reset(struct spi_nor *nor)
{
    int ret;
    
    if (!nor || !nor->spi)
        return -EINVAL;
    
    /* 发送复位使能命令 */
    ret = spi_w8r8(nor->spi, SPINOR_CMD_SRSTEN);
    if (ret < 0)
        return ret;
        
    /* 发送复位命令 */
    return spi_w8r8(nor->spi, SPINOR_CMD_SRST);
}

/* 状态寄存器操作 */
int spi_nor_read_sr1(struct spi_nor *nor)
{
    if (!nor || !nor->spi)
        return -EINVAL;
        
    return spi_w8r8(nor->spi, SPINOR_CMD_RDSR);
}

int spi_nor_read_sr2(struct spi_nor *nor)
{
    if (!nor || !nor->spi)
        return -EINVAL;
        
    return spi_w8r8(nor->spi, SPINOR_CMD_RDSR2);
}

int spi_nor_read_sr3(struct spi_nor *nor)
{
    if (!nor || !nor->spi)
        return -EINVAL;
        
    return spi_w8r8(nor->spi, SPINOR_CMD_RDSR3);
}

int spi_nor_write_sr1(struct spi_nor *nor, uint8_t status)
{
    int ret;
    
    if (!nor || !nor->spi)
        return -EINVAL;
    
    ret = spi_nor_write_enable(nor);
    if (ret < 0)
        return ret;
    
    uint8_t cmd[2] = {SPINOR_CMD_WRSR, status};
    ret = spi_write(nor->spi, cmd, sizeof(cmd));
    if (ret < 0)
        return ret;
        
    return spi_nor_wait_ready(nor, SPI_NOR_TIMEOUT_MS);
}

/* 擦除操作 */
int spi_nor_chip_erase(struct spi_nor *nor)
{
    int ret;
    
    if (!nor || !nor->spi)
        return -EINVAL;
    
    ret = spi_nor_write_enable(nor);
    if (ret < 0)
        return ret;
    
    ret = spi_w8r8(nor->spi, SPINOR_CMD_CHIP_ERASE);
    if (ret < 0)
        return ret;
        
    return spi_nor_wait_ready(nor, SPI_NOR_ERASE_TIMEOUT_MS);
}

int spi_nor_sector_erase(struct spi_nor *nor, uint32_t addr)
{
    int ret;
    uint8_t cmd[4];
    
    if (!nor || !nor->spi)
        return -EINVAL;
    
    ret = spi_nor_write_enable(nor);
    if (ret < 0)
        return ret;
    
    cmd[0] = SPINOR_CMD_BE_4K;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;
    
    ret = spi_write(nor->spi, cmd, sizeof(cmd));
    if (ret < 0)
        return ret;
        
    return spi_nor_wait_ready(nor, SPI_NOR_ERASE_TIMEOUT_MS);
}

/* 编程操作 */
int spi_nor_page_program(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data)
{
    int ret;
    uint8_t cmd[4];
    
    if (!nor || !nor->spi || !data)
        return -EINVAL;
    
    if (len > nor->page_size)
        len = nor->page_size;
    
    ret = spi_nor_write_enable(nor);
    if (ret < 0)
        return ret;
    
    cmd[0] = SPINOR_CMD_PP;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;
    
    struct spi_message msg_tx = {
        .send_buf = cmd,
        .length = sizeof(cmd),
        .cs_change = 0,
        .next = NULL
    };
    
    struct spi_message msg_data = {
        .send_buf = data,
        .length = len,
        .cs_change = 1,
        .next = NULL
    };
    
    msg_tx.next = &msg_data;
    
    ret = spi_transfer_message(nor->spi, &msg_tx);
    if (ret < 0)
        return ret;
        
    return spi_nor_wait_ready(nor, SPI_NOR_WRITE_TIMEOUT_MS);
}

/* 读取操作 */
int spi_nor_read_data(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data)
{
    uint8_t cmd[4];
    
    if (!nor || !nor->spi || !data)
        return -EINVAL;
    
    cmd[0] = SPINOR_CMD_READ;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;
    
    return spi_write_then_read(nor->spi, cmd, sizeof(cmd), data, len);
}

/* Private functions ---------------------------------------------------------*/
static int spi_nor_wait_ready(struct spi_nor *nor, uint32_t timeout_ms)
{
    int ret;
    
    SPI_NOR_WAIT_TIMEOUT(timeout_ms) {
        ret = spi_nor_read_sr1(nor);
        if (ret < 0)
            return ret;
            
        if (!(ret & SPINOR_SR1_WIP))
            return SPI_NOR_OK;
    }
    
    return SPI_NOR_ERR_TIMEOUT;
}

static int spi_nor_write_enable(struct spi_nor *nor)
{
    int ret;
    uint8_t cmd = SPINOR_CMD_WREN;
    
    ret = spi_write(nor->spi, &cmd, 1);
    
    if (ret < 0)
        return ret;
    
    return ret;
}

