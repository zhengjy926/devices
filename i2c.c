/**
 ******************************************************************************
 * @file        : i2c.c
 * @author      : ZJY
 * @version     : V1.0
 * @date        : 2025-02-25
 * @brief       : I2C Driver Framework Implementation (Linux Kernel Style)
 * @attention   : None
 ******************************************************************************
 * @history     :
 *         V1.0 : 1. Complete I2C driver framework with Linux kernel style
 *                2. Thread-safe design for bare-metal and RTOS
 *                3. Compiler optimization compatible
 *                4. Support 7-bit and 10-bit addressing
 *
 ******************************************************************************
 */
/* Includes ------------------------------------------------------------------*/
#include "i2c.h"

#include <string.h>

#include "gpio.h"
#include "errno-base.h"
#include "bsp_dwt.h"

/* Debug support - optional */
#define  LOG_TAG             "i2c"
#define  LOG_LVL             3
#include "log.h"

/* Private typedef -----------------------------------------------------------*/
static LIST_HEAD(i2c_adapter_list);  /* Adapter list head */

/* Private define ------------------------------------------------------------*/
#define I2C_REG_SIZE_8BIT       (1U)
#define I2C_REG_SIZE_16BIT      (2U)

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/


/* Exported variables -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Register an I2C adapter into the framework.
 * @param adap Pointer to a fully initialized i2c_adapter_t, including name, algo and bus_recovery_info.
 * @return 0 on success; -EINVAL if parameters are invalid (missing name, algo or recovery info). On success the adapter is added to the global adapter list.
 */
int i2c_register_adapter(i2c_adapter_t *adap)
{
    struct i2c_bus_recovery_info *bri = adap->bus_recovery_info;
    
	if (!adap->name[0]) {
        LOG_E("i2c adapter has no name!");
        return -EINVAL;
    }
    
    if (adap->algo == NULL) {
        LOG_E("i2c adapter '%s': no algo supplied!", adap->name);
        return -EINVAL;
    }
    
    /* Set default timeout to 1 second if not already set */
    if (adap->timeout == 0) {
        LOG_W("i2c adapter %s: timeout 0, set default 1000", adap->name);
        adap->timeout = 1000;
    }
    
    if (!bri) {
        LOG_E("i2c adapter '%s': no bus_recovery_info supplied!", adap->name);
        return -EINVAL;
    }
    
    list_add_tail(&adap->node, &i2c_adapter_list);
    
    return 0;
}

/**
 * @brief Find a registered I2C adapter by name.
 * @param name Adapter name, must match adap->name used at registration.
 * @return Pointer to i2c_adapter_t if found; NULL if not found or name is NULL.
 */
i2c_adapter_t* i2c_find_adapter(const char *name)
{
    i2c_adapter_t *adap = NULL;
    list_t *node = NULL;
    
    /* Parameter check */
    if (name == NULL) {
        LOG_E("i2c_find_adapter:name is NULL");
        return NULL;
    }
    
    /* Find device by traversing the double linked list */
    list_for_each(node, &i2c_adapter_list) {
        adap = list_entry(node, struct i2c_adapter, node);
        if (adap && strcmp(adap->name, name) == 0) {
            return adap;
        }
    }
    
    return NULL;
}

extern uint32_t HAL_GetTick(void);

/**
 * @brief Perform an I2C transfer sequence on the given adapter (with retry and bus recovery support).
 * @param adap I2C adapter to use.
 * @param msgs Array of messages, each containing slave address, direction, length and buffer.
 * @param num Number of messages in the array.
 * @return On success, returns the number of messages transferred; on failure, negative errno (for example -EINVAL, -EIO). On arbitration loss, bus recovery and retry are attempted automatically.
 */
int i2c_transfer(i2c_adapter_t *adap, i2c_msg_t *msgs, uint16_t num)
{
    int ret = 0;
    uint8_t try = 0;
    
    if (adap == NULL || msgs == NULL || num == 0) {
        LOG_E("i2c_transfer: adap or msgs or num is invalid!");
        return -EINVAL;
    }
    
    if (adap->algo == NULL || adap->algo->master_xfer == NULL) {
        LOG_E("i2c_transfer: adap->algo or adap->algo->master_xfer is NULL!");
        return -EINVAL;
    }
    
    /* Retry automatically on arbitration loss */
	uint32_t tickstart = HAL_GetTick();
	for (ret = 0, try = 0; try <= adap->retries; try++) {
        ret = adap->algo->master_xfer(adap, msgs, num);
		if (ret >= 0) {
            break;
        } else {
            i2c_recovery_bus(adap);
        }
        
		if (((HAL_GetTick() - tickstart) > adap->timeout) || (adap->timeout == 0U)) {
            i2c_recovery_bus(adap);
            break;
        }
	}
    
    return ret;
}

/**
 * @brief Perform a single-buffer I2C transfer (one message) with given flags.
 * @param client I2C client (adapter + address).
 * @param buf Data buffer; for write, holds data to send; for read, holds received data.
 * @param count Number of bytes to transfer.
 * @param flags Message flags, for example I2C_M_RD for read.
 * @return count on success; negative errno on failure.
 */
int i2c_transfer_buffer_flags(const struct i2c_client *client, uint8_t *buf,
                              uint16_t count, uint16_t flags)
{
	int ret;
	struct i2c_msg msg = {
		.addr = client->addr,
		.flags = flags | (client->flags & I2C_M_TEN),
		.len = count,
		.buf = buf,
	};

	ret = i2c_transfer(client->adapter, &msg, 1);

	/*
	 * If everything went ok (i.e. 1 msg transferred), return #bytes
	 * transferred, else error code.
	 */
	return (ret == 1) ? count : ret;
}


#define RECOVERY_NDELAY		(5U)
#define RECOVERY_CLK_CNT	(9U)

/**
 * @brief Perform I2C bus recovery on SDA/SCL (clock pulses + STOP condition check).
 * @param adap Adapter whose bus_recovery_info provides SDA/SCL pins and prepare/unprepare callbacks.
 * @return 0 on successful recovery; -EBUSY if SCL is stuck low or SDA cannot be released.
 */
int i2c_recovery_bus(struct i2c_adapter *adap)
{
	struct i2c_bus_recovery_info *bri = adap->bus_recovery_info;
	uint8_t i = 0;
    uint8_t scl = 1;
    int ret = 0;
    
	if (bri->prepare_recovery)
		bri->prepare_recovery(adap);
    
    gpio_set_mode(bri->scl_pin_id, PIN_OUTPUT_OD, PIN_PULL_NONE);
    gpio_set_mode(bri->sda_pin_id, PIN_OUTPUT_OD, PIN_PULL_NONE);
    
    gpio_write(bri->scl_pin_id, 1);
    bsp_dwt_delay_us(RECOVERY_NDELAY);
    gpio_write(bri->sda_pin_id, 1);
    bsp_dwt_delay_us(RECOVERY_NDELAY/2);
    
	/*
	 * By this time SCL is high, as we need to give 9 falling-rising edges
	 */
	while (i++ < RECOVERY_CLK_CNT * 2) {
		if (scl) {
			/* SCL shouldn't be low here */
			if (!gpio_read(bri->scl_pin_id)) {
                LOG_E("SCL is stuck low, recovery error, exit recovery!");
				ret = -EBUSY;
				break;
			}
		}

		scl = !scl;
		gpio_write(bri->scl_pin_id, 0);
		/* Creating STOP again, see above */
		if (scl)  {
			/* Honour minimum tsu:sto */
			bsp_dwt_delay_us(RECOVERY_NDELAY);
		} else {
			/* Honour minimum tf and thd:dat */
			bsp_dwt_delay_us(RECOVERY_NDELAY/2);
		}
        
        gpio_write(bri->sda_pin_id, scl);
		bsp_dwt_delay_us(RECOVERY_NDELAY/2);

		if (scl) {
			ret = gpio_read(bri->sda_pin_id) ? 0 : -EBUSY;
			if (ret == 0)
				break;
		}
	}
    
	if (bri->unprepare_recovery)
		bri->unprepare_recovery(adap);
    
    LOG_I("Recovery done!");
    
	return ret;
}

/**
 * @brief Read data from an I2C device register.
 * @param client I2C client (adapter + 7-bit addr).
 * @param reg Register address (8- or 16-bit according to reg_size).
 * @param reg_size Register address size in bytes: 1 or 2 (16-bit is MSB first on wire).
 * @param buf Buffer to store read data.
 * @param count Number of bytes to read.
 * @return On success: count; on failure: negative errno (e.g. -EINVAL, -EIO).
 */
int i2c_read_reg(const struct i2c_client *client, uint16_t reg, uint16_t reg_size,
                 uint8_t *buf, uint16_t count)
{
	struct i2c_msg msg[2];
	uint8_t reg_buf[2];
	int ret;

	if (client == NULL || client->adapter == NULL || buf == NULL) {
		LOG_E("i2c_read_reg: invalid client or buf");
		return -EINVAL;
	}
	if (reg_size != I2C_REG_SIZE_8BIT && reg_size != I2C_REG_SIZE_16BIT) {
		LOG_E("i2c_read_reg: reg_size must be 1 or 2");
		return -EINVAL;
	}
	if (count == 0U) {
		return 0;
	}

	/* Build register address bytes (MSB first for 16-bit) */
	if (reg_size == I2C_REG_SIZE_16BIT) {
		reg_buf[0] = (uint8_t)(reg >> 8);
		reg_buf[1] = (uint8_t)(reg & 0xFFU);
	} else {
		reg_buf[0] = (uint8_t)(reg & 0xFFU);
	}

	msg[0].addr  = client->addr;
	msg[0].flags = client->flags & I2C_M_TEN;
	msg[0].len   = reg_size;
	msg[0].buf   = reg_buf;

	msg[1].addr  = client->addr;
	msg[1].flags = I2C_M_RD | (client->flags & I2C_M_TEN);
	msg[1].len   = count;
	msg[1].buf   = buf;

	ret = i2c_transfer(client->adapter, msg, 2);
	return (ret == 2) ? (int)count : ret;
}

/**
 * @brief Write data to an I2C device register (two messages: reg then data).
 * @param client I2C client (adapter + 7-bit addr).
 * @param reg Register address (8- or 16-bit according to reg_size).
 * @param reg_size Register address size in bytes: 1 or 2 (16-bit is MSB first on wire).
 * @param buf Data to write; may be NULL only if count is 0.
 * @param count Number of data bytes to write.
 * @return On success: count (or 0 if count is 0); on failure: negative errno.
 */
int i2c_write_reg(const struct i2c_client *client, uint16_t reg, uint16_t reg_size,
                  uint8_t *buf, uint16_t count)
{
	struct i2c_msg msg[2];
	uint8_t reg_buf[2];
	int ret;

	if (client == NULL || client->adapter == NULL) {
		LOG_E("i2c_write_reg: invalid client");
		return -EINVAL;
	}
	if (reg_size != I2C_REG_SIZE_8BIT && reg_size != I2C_REG_SIZE_16BIT) {
		LOG_E("i2c_write_reg: reg_size must be 1 or 2");
		return -EINVAL;
	}
	if (count > 0U && buf == NULL) {
		LOG_E("i2c_write_reg: buf is NULL with count > 0");
		return -EINVAL;
	}

	/* Register address (MSB first for 16-bit) */
	if (reg_size == I2C_REG_SIZE_16BIT) {
		reg_buf[0] = (uint8_t)(reg >> 8);
		reg_buf[1] = (uint8_t)(reg & 0xFFU);
	} else {
		reg_buf[0] = (uint8_t)(reg & 0xFFU);
	}

	msg[0].addr  = client->addr;
	msg[0].flags = client->flags & I2C_M_TEN;
	msg[0].len   = reg_size;
	msg[0].buf   = reg_buf;

	if (count == 0U) {
		ret = i2c_transfer(client->adapter, msg, 1);
		return (ret == 1) ? 0 : ret;
	}

	msg[1].addr  = client->addr;
	msg[1].flags = client->flags & I2C_M_TEN;
	msg[1].len   = count;
	msg[1].buf   = buf;

	ret = i2c_transfer(client->adapter, msg, 2);
	return (ret == 2) ? (int)count : ret;
}

/* Private functions ---------------------------------------------------------*/

