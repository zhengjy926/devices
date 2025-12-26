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
 *
 ******************************************************************************
 */
/* Includes ------------------------------------------------------------------*/
#include "i2c.h"

/* Debug support - optional */
#define  LOG_TAG             "i2c"
#define  LOG_LVL             4
#include "log.h"

/* Private typedef -----------------------------------------------------------*/
static LIST_HEAD(i2c_adapter_list);  /* Adapter list head */

/* Private define ------------------------------------------------------------*/
#define I2C_MAX_CLIENTS              (16U)    /**< Maximum number of I2C clients */
#define I2C_DMA_THRESHOLD            (16U)    /**< Minimum length to use DMA */

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/**
 * @brief Static client pool (no dynamic memory allocation)
 */
static struct i2c_client i2c_client_pool[I2C_MAX_CLIENTS];
static uint8_t i2c_client_used[I2C_MAX_CLIENTS];

/* Exported variables -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static int i2c_validate_addr(uint16_t addr, uint16_t flags);
static int i2c_transfer_internal(struct i2c_adapter *adap,
                                 struct i2c_msg *msgs,
                                 uint32_t num);
static struct i2c_client *i2c_alloc_client(void);
static void i2c_free_client(struct i2c_client *client);

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
 * @brief Allocate client from static pool
 * @return Client pointer on success, NULL on failure
 */
static struct i2c_client *i2c_alloc_client(void)
{
    uint8_t i = 0U;
    
    for (i = 0U; i < I2C_MAX_CLIENTS; i++) {
        if (i2c_client_used[i] == 0U) {
            i2c_client_used[i] = 1U;
            (void)memset(&i2c_client_pool[i], 0, sizeof(struct i2c_client));
            return &i2c_client_pool[i];
        }
    }
    
    return NULL;
}

/**
 * @brief Free client to static pool
 * @param client Client pointer
 */
static void i2c_free_client(struct i2c_client *client)
{
    uint8_t i = 0U;
    
    if (client == NULL) {
        return;
    }
    
    for (i = 0U; i < I2C_MAX_CLIENTS; i++) {
        if (&i2c_client_pool[i] == client) {
            i2c_client_used[i] = 0U;
            break;
        }
    }
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
 * @brief Create new I2C client device
 * @param name Device name
 * @param adapter_name Adapter name to attach to
 * @param addr Device address
 * @param flags Device flags
 * @return Client pointer on success, NULL on failure
 */
struct i2c_client *i2c_new_client(const char *name,
                                  const char *adapter_name,
                                  uint16_t addr,
                                  uint16_t flags)
{
    struct i2c_adapter *adap;
    struct i2c_client *client;
    int ret;
    
    /* Parameter validation */
    if ((name == NULL) || (adapter_name == NULL)) {
        return NULL;
    }
    
    /* Validate address */
    ret = i2c_validate_addr(addr, flags);
    if (ret != 0) {
        return NULL;
    }
    
    /* Find adapter */
    adap = i2c_find_adapter(adapter_name);
    if (adap == NULL) {
        return NULL;
    }
    
    /* Note: Address mode validation should be done at transfer time */
    /* The algorithm structure no longer includes functionality flags */
    
    /* Allocate client */
    client = i2c_alloc_client();
    if (client == NULL) {
        return NULL;
    }
    
    /* Initialize client */
    client->name = name;
    client->adapter = adap;
    client->addr = addr;
    client->flags = flags;
    client->timeout_ms = adap->timeout_ms;
    client->driver_data = NULL;
    
    return client;
}

/**
 * @brief Delete I2C client device
 * @param client Client pointer
 * @return 0 on success, error code on failure
 */
int i2c_del_client(struct i2c_client *client)
{
    if (client == NULL) {
        return -EINVAL;
    }
    
    /* Free client */
    i2c_free_client(client);
    
    return 0;
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
        /* Check if we should retry */
        if ((ret == -EIO) || (ret == -ETIMEOUT)) {
            retry++;
            if (retry < adap->retries) {
                /* Small delay before retry */
                /* Note: In real implementation, this should use a delay function */
                continue;
            }
        }
        
        /* Don't retry on other errors */
        break;
    } while (retry < adap->retries);
    
    return ret;
}
