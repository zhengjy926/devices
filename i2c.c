/**
 ******************************************************************************
 * @file        : i2c.c
 * @author      : ZJY
 * @version     : V1.0
 * @date        : 2025-01-XX
 * @brief       : I2C Driver Framework Implementation (Linux Kernel Style)
 * @attention   : None
 ******************************************************************************
 * @history     :
 *         V1.0 : 1. Complete I2C driver framework with Linux kernel style
 *                2. Thread-safe design for bare-metal and RTOS
 *                3. Compiler optimization compatible
 *                4. Support 7-bit and 10-bit addressing
 *                5. Support DMA transfer
 *                6. Bus hang recovery (i2c_generic_scl_recovery) on -EIO/-ETIMEOUT
 *
 ******************************************************************************
 */
/* Includes ------------------------------------------------------------------*/
#include "i2c.h"
#include "gpio.h"

/* Debug support - optional */
#define  LOG_TAG             "i2c"
#define  LOG_LVL             4
#include "log.h"

/* Private typedef -----------------------------------------------------------*/
static LIST_HEAD(i2c_adapter_list);  /* Adapter list head */

/* Private define ------------------------------------------------------------*/
#define I2C_DMA_THRESHOLD            (16U)    /**< Minimum length to use DMA */
/* Bus recovery: 100kHz half-period = 5us (10^6/100/2 ns -> 5000 ns) */
#define I2C_RECOVERY_DELAY_US        (5U)     /**< Recovery clock half-cycle in us */
#define I2C_RECOVERY_CLK_CNT         (9U)     /**< Number of SCL clock pulses for recovery */

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Exported variables -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static int i2c_validate_addr(uint16_t addr, uint16_t flags);
static int i2c_transfer_internal(struct i2c_adapter *adap,
                                 struct i2c_msg *msgs,
                                 uint32_t num);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Validate I2C address
 * @param addr Address to validate
 * @param flags Address flags
 * @return 0 if valid, -EINVAL if invalid
 */
static int i2c_validate_addr(uint16_t addr, uint16_t flags)
{
    if ((flags & I2C_M_TEN) != 0U) {
        /* 10-bit address: 0x000-0x3FF */
        if (addr > 0x3FFU) {
            return -EINVAL;
        }
    } else {
        /* 7-bit address: 0x08-0x77 (excluding reserved addresses) */
        if ((addr < 0x08U) || (addr > 0x77U)) {
            return -EINVAL;
        }
    }
    
    return 0;
}

/**
 * @brief Internal transfer function
 * @param adap Adapter pointer
 * @param msgs Messages array
 * @param num Number of messages
 * @return Number of messages transferred on success, error code on failure
 */
static int i2c_transfer_internal(struct i2c_adapter *adap,
                                 struct i2c_msg *msgs,
                                 uint32_t num)
{
    int ret = 0;
    int i = 0;
    uint8_t use_dma = 0U;
    uint32_t total_len = 0U;
    
    if (adap->algo->xfer == NULL) {
        LOG_E("I2C level transfers not supported!");
        return -ENOSYS;
    }
    
    /* Validate all messages */
    for (i = 0; i < num; i++) {
        if (msgs[i].buf == NULL) {
            return -EINVAL;
        }
        
        ret = i2c_validate_addr(msgs[i].addr, msgs[i].flags);
        if (ret != 0) {
            return ret;
        }
    }
    
    adap->in_use = 1U;
    
    /* Execute transfer */
    ret = adap->algo->xfer(adap, msgs, num);
    
    adap->in_use = 0U;
    
    return ret;
}

/**
 * @brief Register I2C adapter
 * @param adap Adapter structure pointer
 * @param name Adapter name
 * @param algo Algorithm functions pointer
 * @return 0 on success, error code on failure
 */
int i2c_add_adapter(struct i2c_adapter *adap, 
                    const char *name, 
                    const struct i2c_algorithm *algo)
{
    struct i2c_adapter *found;
    size_t name_len;
    
    /* Parameter validation */
    if ((adap == NULL) || (name == NULL) || (algo == NULL)) {
        return -EINVAL;
    }
    
    /* Check if name already exists */
    found = i2c_find_adapter(name);
    if (found != NULL) {
        return -EEXIST;
    }
    
    /* Validate algo structure */
    if (algo->xfer == NULL) {
        return -EINVAL;
    }
    
    /* Copy name */
    name_len = strlen(name);
    if (name_len >= I2C_NAME_MAX) {
        name_len = I2C_NAME_MAX - 1U;
    }
    (void)memcpy(adap->name, name, name_len);
    adap->name[name_len] = '\0';
    
    /* Set algorithm */
    adap->algo = algo;
    
    /* Initialize list node */
    list_node_init(&adap->node);
    
    /* Initialize configuration cache */
    adap->speed_hz = 100000U;  /* Default 100kHz */
    adap->timeout_ms = 1000U;  /* Default 1 second */
    adap->addr_width = 7U;     /* Default 7-bit addressing */
    adap->in_use = 0U;
    
    /* Add to list */
    list_add_tail(&adap->node, &i2c_adapter_list);
    
    return 0;
}

/**
 * @brief Unregister I2C adapter
 * @param adap Adapter structure pointer
 * @return 0 on success, error code on failure
 */
int i2c_del_adapter(struct i2c_adapter *adap)
{
    if (adap == NULL) {
        return -EINVAL;
    }
    
    /* Remove from list */
    list_del(&adap->node);
    
    /* Clear structure */
    (void)memset(adap, 0, sizeof(struct i2c_adapter));
    
    return 0;
}

/**
 * @brief Find I2C adapter by name
 * @param name Adapter name
 * @return Adapter pointer on success, NULL on failure
 */
struct i2c_adapter *i2c_find_adapter(const char *name)
{
    struct i2c_adapter *adap;
    struct list_node *node;
    
    /* Parameter check */
    if (name == NULL) {
        return NULL;
    }
    
    /* Find adapter by traversing the list */
    list_for_each_entry(adap, &i2c_adapter_list, node) {
        if (strcmp(adap->name, name) == 0) {
            return adap;
        }
    }
    
    return NULL;
}

/**
 * @brief Check if I2C bus is free (SCL and SDA high)
 * @param adap Adapter pointer
 * @return 0 if bus free, -EBUSY if not
 */
int i2c_generic_bus_free(struct i2c_adapter *adap)
{
    struct i2c_bus_recovery_info *bri;
    uint8_t scl_val;
    uint8_t sda_val;

    if (adap == NULL) {
        return -EINVAL;
    }
    bri = adap->bus_recovery_info;
    if (bri == NULL) {
        return -EINVAL;
    }
    scl_val = gpio_read(bri->scl_pin_id);
    sda_val = gpio_read(bri->sda_pin_id);
    if ((scl_val != 0U) && (sda_val != 0U)) {
        return 0;
    }
    return -EBUSY;
}

/**
 * @brief Generic SCL clock stretching recovery (bus hang recovery)
 * @param adap Adapter pointer (must have bus_recovery_info configured)
 * @return 0 on success, -EBUSY if SCL stuck low
 */
int i2c_generic_scl_recovery(struct i2c_adapter *adap)
{
    struct i2c_bus_recovery_info *bri;
    uint8_t scl;
    int ret;
    uint32_t i;

    if (adap == NULL) {
        return -EINVAL;
    }
    bri = adap->bus_recovery_info;
    if (bri == NULL) {
        return -EINVAL;
    }
    if (bri->recovery_delay_us == NULL) {
        return -EINVAL;
    }

    if (bri->prepare_recovery != NULL) {
        bri->prepare_recovery(adap);
    }

    /* Generate STOP: SCL high then SDA high (SDA follows SCL half-cycle later) */
    scl = 1U;
    gpio_write(bri->scl_pin_id, scl);
    bri->recovery_delay_us(adap, I2C_RECOVERY_DELAY_US);
    gpio_write(bri->sda_pin_id, scl);
    bri->recovery_delay_us(adap, I2C_RECOVERY_DELAY_US / 2U);

    ret = 0;
    for (i = 0U; i < (I2C_RECOVERY_CLK_CNT * 2U); i++) {
        if (scl != 0U) {
            if (gpio_read(bri->scl_pin_id) == 0U) {
                LOG_E("I2C SCL stuck low, exit recovery");
                ret = -EBUSY;
                break;
            }
        }
        scl = (scl == 0U) ? 1U : 0U;
        gpio_write(bri->scl_pin_id, scl);
        if (scl != 0U) {
            bri->recovery_delay_us(adap, I2C_RECOVERY_DELAY_US);
        } else {
            bri->recovery_delay_us(adap, I2C_RECOVERY_DELAY_US / 2U);
        }
        gpio_write(bri->sda_pin_id, scl);
        bri->recovery_delay_us(adap, I2C_RECOVERY_DELAY_US / 2U);

        if (scl != 0U) {
            ret = i2c_generic_bus_free(adap);
            if (ret == 0) {
                break;
            }
        }
    }

    if (bri->unprepare_recovery != NULL) {
        bri->unprepare_recovery(adap);
    }

    return ret;
}

/**
 * @brief Synchronous I2C message transfer
 * @param adap Adapter pointer
 * @param msgs Array of messages to transfer
 * @param num Number of messages
 * @return Number of messages transferred on success, error code on failure
 */
int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, uint32_t num)
{
    int ret = 0;
    uint32_t retry = 0;
    
    /* Parameter validation */
    if ((adap == NULL) || (msgs == NULL) || (num == 0U)) {
        return -EINVAL;
    }
    
    /* Retry on failure */
    do {
        ret = i2c_transfer_internal(adap, msgs, num);
        if (ret >= 0) {
            break;
        }
        /* On bus hang (-EIO/-ETIMEOUT), try bus recovery then retry */
        if ((ret == -EIO) || (ret == -ETIMEOUT)) {
            LOG_W("i2c_transfer: try bus recovery");
            retry++;
            if (retry < adap->retries) {
                if (adap->bus_recovery_info != NULL) {
                    (void)i2c_generic_scl_recovery(adap);
                }
                continue;
            }
        }
        break;
    } while (retry < adap->retries);
    
    return ret;
}
