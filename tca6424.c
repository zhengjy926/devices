/**
  ******************************************************************************
  * @file        : xxx.c
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 202x-xx-xx
  * @brief       : xxx
  * @attention   : xxx
  ******************************************************************************
  * @history     :
  *         V1.0 : 
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/

#include "tca6424.h"

#define  LOG_TAG             "tca6424"
#define  LOG_LVL             4
#include "log.h"

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define TCA6424_CMD(ai, reg)   (uint8_t)(((ai) ? 0x80u : 0x00u) | ((reg) & 0x7Fu))

/* Private macro -------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/



/* Exported variables  -------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/



/* Exported functions --------------------------------------------------------*/

int tca6424_init(tca6424_t *dev, uint8_t addr7, const char *adapter_name)
{
    /* 参数验证 */
    if (dev == NULL) {
        LOG_E("Invalid parameter: dev is NULL");
        return -EINVAL;
    }
    
    if (adapter_name == NULL) {
        LOG_E("Invalid parameter: I2C adapter_name is NULL");
        return -EINVAL;
    }
    
    dev->client = i2c_new_client("tca6424",
                                 "i2c1",
                                 TCA6424_I2C_ADDR_H,
                                 0);
    if (dev->client == NULL) {
        LOG_E("Failed to create I2C client for AD5272");
        return -ENODEV;
    }
    
    return 0;
}


int tca6424_read_reg(tca6424_t *dev, uint8_t reg, bool auto_inc, uint8_t *rx_buf, uint8_t len)
{
    int ret = 0;
    uint8_t cmd = TCA6424_CMD(auto_inc, reg);
    
    ret = i2c_write_then_read(dev->client->adapter, TCA6424_I2C_ADDR_H, 0,
                              &cmd, 1,
                              rx_buf, len);
    if (ret != 0) {
        LOG_E("I2C read failed: %d", ret);
        return ret;
    }
    
    return 0;
}

int tca6424_write_reg(tca6424_t *dev, uint8_t reg, bool auto_inc, uint8_t *tx_buf, uint8_t len)
{
    int ret = 0;
    uint8_t cmd = 0;
    
    struct i2c_msg msg[2] = {0};
    
    msg[0].addr = TCA6424_I2C_ADDR_H;
    msg[0].buf = &cmd;
    msg[0].flags = 0;
    msg[0].len = 1;
    
    msg[1].addr = TCA6424_I2C_ADDR_H;
    msg[1].buf = tx_buf;
    msg[1].flags = 0;
    msg[1].len = len;
    
    ret = i2c_transfer(dev->client->adapter, msg, 2);
    if (ret != 1) {
        LOG_E("I2C write failed: %d", ret);
        return ret;
    }
    
    return 0;
}

int tca6424_refresh_cache(tca6424_t *dev)
{
    int ret = 0;
    uint8_t tmp[3] = {0};
    
    ret = tca6424_read_reg(dev, TCA6424_REG_OUTPUT_PORT0, true, tmp, 3u);
    if (ret == 0) {
        dev->out[0] = tmp[0]; 
        dev->out[1] = tmp[1]; 
        dev->out[2] = tmp[2];
        ret = tca6424_read_reg(dev, TCA6424_REG_CFG_PORT0, true, tmp, 3u);
    }
    if (ret == 0) {
        dev->cfg[0] = tmp[0]; 
        dev->cfg[1] = tmp[1]; 
        dev->cfg[2] = tmp[2];
        ret = tca6424_read_reg(dev, TCA6424_REG_POL_PORT0, true, tmp, 3u);
    }
    if (ret == 0) {
        dev->pol[0] = tmp[0]; 
        dev->pol[1] = tmp[1]; 
        dev->pol[2] = tmp[2];
        dev->cache_valid = true;
    }    
    return 0;
}

/* Private functions ---------------------------------------------------------*/

