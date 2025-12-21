/**
  ******************************************************************************
  * @file        : spi.c
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2025-01-XX
  * @brief       : SPI驱动框架实现 (Linux Kernel Style)
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1. Complete refactoring with Linux kernel style
  *                2. Thread-safe design for bare-metal and RTOS
  *                3. Compiler optimization compatible
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "spi.h"
#include "gpio.h"
#include "errno-base.h"
#include "cmsis_compiler.h"

/* Debug support - optional */
#define  LOG_TAG             "spi"
#define  LOG_LVL             4
#include "log.h"

/* Private typedef -----------------------------------------------------------*/
static LIST_HEAD(spi_controller_list);  /* Controller list head */

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Exported variables -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static inline void spi_controller_lock(struct spi_controller *ctrl);
static inline void spi_controller_unlock(struct spi_controller *ctrl);
static int spi_controller_setup_internal(struct spi_controller *ctrl, 
                                         struct spi_device *dev);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Lock controller (thread safety)
 * @param ctrl Controller pointer
 */
static inline void spi_controller_lock(struct spi_controller *ctrl)
{
    if (ctrl == NULL) {
        return;
    }
    
    if (ctrl->lock != NULL) {
        /* RTOS environment: use mutex */
        ctrl->lock(ctrl->lock_data);
    } else if (ctrl->irq_disable != NULL) {
        /* Bare-metal environment: disable interrupts */
        ctrl->irq_disable();
    }
}

/**
 * @brief Unlock controller (thread safety)
 * @param ctrl Controller pointer
 */
static inline void spi_controller_unlock(struct spi_controller *ctrl)
{
    if (ctrl == NULL) {
        return;
    }
    
    if (ctrl->unlock != NULL) {
        /* RTOS environment: release mutex */
        ctrl->unlock(ctrl->lock_data);
    } else if (ctrl->irq_enable != NULL) {
        /* Bare-metal environment: enable interrupts */
        ctrl->irq_enable();
    }
}

/**
 * @brief Internal setup function (noinline to prevent optimization issues)
 * @param ctrl Controller pointer
 * @param dev Device pointer
 * @return 0 on success, error code on failure
 */
static int __attribute__((noinline))
spi_controller_setup_internal(struct spi_controller *ctrl, 
                               struct spi_device *dev)
{
    int ret;
    uint32_t actual_speed;
    
    if ((ctrl == NULL) || (dev == NULL) || (ctrl->ops == NULL) || 
        (ctrl->ops->setup == NULL)) {
        return -EINVAL;
    }
    
    /* Update configuration cache in critical section */
    spi_controller_lock(ctrl);
    
    /* Call hardware setup */
    ret = ctrl->ops->setup(ctrl, dev);
    if (ret != 0) {
        spi_controller_unlock(ctrl);
        return ret;
    }
    
    /* Read actual speed from controller (may be modified by setup) */
    actual_speed = ctrl->actual_speed_hz;
    
    /* Update cache with device configuration */
    ctrl->mode = dev->mode;
    ctrl->bits_per_word = dev->bits_per_word;
    ctrl->max_speed_hz = dev->max_speed_hz;
    ctrl->actual_speed_hz = actual_speed;
    ctrl->current_device = dev;
    
    /* Memory barrier to ensure writes complete */
    __DSB();
    
    spi_controller_unlock(ctrl);
    
    return 0;
}

/**
 * @brief Register SPI controller
 * @param ctrl Controller structure pointer
 * @param name Controller name
 * @param ops Operation functions pointer
 * @return 0 on success, error code on failure
 */
int spi_controller_register(struct spi_controller *ctrl, 
                            const char *name, 
                            const struct spi_controller_ops *ops)
{
    struct spi_controller *found;
    size_t name_len;
    
    /* Parameter validation */
    if ((ctrl == NULL) || (name == NULL) || (ops == NULL)) {
        return -EINVAL;
    }
    
    /* Check if name already exists */
    found = spi_controller_find(name);
    if (found != NULL) {
        return -EINVAL;
    }
    
    /* Validate ops structure */
    if ((ops->setup == NULL) || (ops->set_cs == NULL) || 
        (ops->transfer_one == NULL)) {
        return -EINVAL;
    }
    
    /* Initialize controller structure */
    (void)memset(ctrl, 0, sizeof(struct spi_controller));
    
    /* Copy name */
    name_len = strlen(name);
    if (name_len >= SPI_NAME_MAX) {
        name_len = SPI_NAME_MAX - 1U;
    }
    (void)memcpy(ctrl->name, name, name_len);
    ctrl->name[name_len] = '\0';
    
    /* Set operations */
    ctrl->ops = ops;
    
    /* Initialize list node */
    list_node_init(&ctrl->node);
    
    /* Initialize configuration cache */
    ctrl->mode = 0xFFU;  /* Invalid value to force first setup */
    ctrl->bits_per_word = 0U;
    ctrl->max_speed_hz = 0U;
    ctrl->actual_speed_hz = 0U;
    ctrl->current_device = NULL;
    
    /* Add to list */
    list_add_tail(&ctrl->node, &spi_controller_list);
    
    return 0;
}

/**
 * @brief Find SPI controller by name
 * @param name Controller name
 * @return Controller pointer on success, NULL on failure
 */
struct spi_controller *spi_controller_find(const char *name)
{
    struct spi_controller *ctrl;
    struct list_node *node;
    
    /* Parameter check */
    if (name == NULL) {
        return NULL;
    }
    
    /* Find controller by traversing the list */
    list_for_each_entry(ctrl, &spi_controller_list, node) {
        if (strcmp(ctrl->name, name) == 0) {
            return ctrl;
        }
    }
    
    return NULL;
}

/**
 * @brief Attach SPI device to controller
 * @param dev Device pointer
 * @param controller_name Controller name
 * @return 0 on success, error code on failure
 */
int spi_device_attach(struct spi_device *dev, const char *controller_name)
{
    struct spi_controller *ctrl;
    
    /* Parameter validation */
    if ((dev == NULL) || (controller_name == NULL)) {
        return -EINVAL;
    }
    
    /* Validate device parameters */
    if ((dev->bits_per_word == 0U) || (dev->max_speed_hz == 0U)) {
        return -EINVAL;
    }
    
    /* Find controller */
    ctrl = spi_controller_find(controller_name);
    if (ctrl == NULL) {
        return -EINVAL;
    }
    
    /* Validate controller ops */
    if ((ctrl->ops == NULL) || (ctrl->ops->set_cs == NULL)) {
        return -EINVAL;
    }
    
    /* Attach device to controller */
    dev->controller = ctrl;
    
    /* Initialize CS pin if software CS is used */
    if ((dev->mode & SPI_MODE_HW_CS) == 0U) {
        gpio_set_mode(dev->cs_pin, PIN_OUTPUT_PP, PIN_PULL_UP);
        /* Set CS to inactive state */
        ctrl->ops->set_cs(ctrl, dev, 0U);
    }
    
    return 0;
}

/**
 * @brief Initialize SPI message
 * @param m Message pointer
 */
void spi_message_init(struct spi_message *m)
{
    if (m == NULL) {
        return;
    }
    
    list_node_init(&m->transfers);
    m->spi = NULL;
    m->status = 0;
    m->context = NULL;
}

/**
 * @brief Add transfer to message tail
 * @param t Transfer pointer
 * @param m Message pointer
 */
void spi_message_add_tail(struct spi_transfer *t, struct spi_message *m)
{
    if ((t == NULL) || (m == NULL)) {
        return;
    }
    
    list_add_tail(&t->transfer_list, &m->transfers);
}

/**
 * @brief Synchronous SPI message transfer
 * @param dev Device pointer
 * @param message Message pointer
 * @return 0 on success, error code on failure
 */
int spi_sync(struct spi_device *dev, struct spi_message *message)
{
    struct spi_controller *ctrl;
    struct spi_transfer *transfer;
    struct spi_transfer *transfer_next;
    int ret;
    uint8_t cs_active;
    uint8_t need_setup;
    
    /* Parameter validation */
    if ((dev == NULL) || (message == NULL)) {
        return -EINVAL;
    }
    
    ctrl = dev->controller;
    if ((ctrl == NULL) || (ctrl->ops == NULL)) {
        return -EINVAL;
    }
    
    if ((ctrl->ops->setup == NULL) || (ctrl->ops->set_cs == NULL) || 
        (ctrl->ops->transfer_one == NULL)) {
        return -EINVAL;
    }
    
    /* Initialize message status */
    message->spi = dev;
    message->status = 0;
    
    /* Check if setup is needed */
    spi_controller_lock(ctrl);
    
    need_setup = 0U;
    if ((ctrl->current_device != dev) ||
        (ctrl->mode != dev->mode) ||
        (ctrl->bits_per_word != dev->bits_per_word) ||
        (ctrl->max_speed_hz != dev->max_speed_hz)) {
        need_setup = 1U;
    }
    
    /* Compiler barrier to prevent reordering */
    asm volatile("" ::: "memory");
    
    spi_controller_unlock(ctrl);
    
    /* Setup controller if needed */
    if (need_setup != 0U) {
        ret = spi_controller_setup_internal(ctrl, dev);
        if (ret != 0) {
            message->status = ret;
            return ret;
        }
    }
    
    /* Process transfers */
    cs_active = 0U;
    ret = 0;
    
    {
        struct spi_transfer *transfer_next;
        uint8_t has_next;
        
        list_for_each_entry_safe(transfer, transfer_next, &message->transfers, transfer_list) {
            
            /* Check if there's a next transfer before processing current one */
            has_next = (uint8_t)((&transfer_next->transfer_list != &message->transfers) ? 1U : 0U);
            
            /* Skip zero-length transfers */
            if (transfer->len == 0U) {
                continue;
            }
            
            /* Activate CS if not already active */
            if (cs_active == 0U) {
                ctrl->ops->set_cs(ctrl, dev, 1U);
                cs_active = 1U;
                /* Memory barrier after CS activation */
                __DMB();
            }
            
            /* Execute transfer */
            ret = (int)ctrl->ops->transfer_one(ctrl, dev, transfer);
            if (ret < 0) {
                message->status = ret;
                break;
            }
            
            /* Handle CS change */
            if (transfer->cs_change != 0U) {
                /* Deactivate CS */
                ctrl->ops->set_cs(ctrl, dev, 0U);
                cs_active = 0U;
                /* Memory barrier after CS deactivation */
                __DMB();
                
                /* Reactivate CS for next transfer if exists */
                if (has_next != 0U) {
                    ctrl->ops->set_cs(ctrl, dev, 1U);
                    cs_active = 1U;
                    __DMB();
                }
            }
        }
    }
    
    /* Ensure CS is deactivated */
    if (cs_active != 0U) {
        ctrl->ops->set_cs(ctrl, dev, 0U);
        __DMB();
    }
    
    message->status = ret;
    return ret;
}
