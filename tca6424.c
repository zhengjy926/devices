/**
 ******************************************************************************
 * @file        : tca6424.c
 * @author      : ZJY
 * @version     : V1.0
 * @date        : 2025-02-25
 * @brief       : TCA6424 24-bit I2C I/O expander driver implementation (per TCA6424A datasheet)
 * @attention   : None
 ******************************************************************************
 * @history     :
 * V1.0 : 1. Initial version, implements basic TCA6424 features
 *        2. Supports 24-bit I/O port read/write, single-pin operation,
 *           direction and polarity configuration, and cache synchronization
 ******************************************************************************
 */
/* Includes ------------------------------------------------------------------*/

#include "tca6424.h"
#include <string.h>
#include "errno-base.h"
#include "gpio.h"
#include "bsp_dwt.h"

#define  LOG_TAG             "tca6424"
#define  LOG_LVL             3
#include "log.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

#define TCA6424_CMD_SIZE                 (1U)

/* Private macro -------------------------------------------------------------*/
/** @brief Command byte with auto-increment: after each access, the next port is selected for continuous multi-port transfer */
#define TCA6424_CMD_INPUT_AI             (TCA6424_REG_INPUT_PORT0  | TCA6424_CMD_AI_MASK)
#define TCA6424_CMD_OUTPUT_AI            (TCA6424_REG_OUTPUT_PORT0  | TCA6424_CMD_AI_MASK)
#define TCA6424_CMD_POL_AI               (TCA6424_REG_POL_PORT0    | TCA6424_CMD_AI_MASK)
#define TCA6424_CMD_CFG_AI               (TCA6424_REG_CFG_PORT0    | TCA6424_CMD_AI_MASK)

/* Private variables ---------------------------------------------------------*/

/* Exported variables  -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize a TCA6424 device: bind an I2C adapter, configure RST/INT pins, and synchronize device registers to local caches.
 * @param dev Pointer to device structure, must not be NULL.
 * @param addr 7-bit I2C slave address (for example TCA6424_I2C_ADDR_L / TCA6424_I2C_ADDR_H).
 * @param adapter_name Name of the registered I2C adapter.
 * @param rst_pin GPIO pin id of reset pin; use UINT32_MAX if not present.
 * @param int_pin GPIO pin id of interrupt pin; use UINT32_MAX if not present.
 * @return 0 on success; -EINVAL if parameters are invalid; -ENODEV if adapter not found; other negative values on sync failure.
 */
int tca6424_init(tca6424_t *dev, uint8_t addr, const char *adapter_name,
                 uint32_t rst_pin, uint32_t int_pin)
{
    struct i2c_adapter *adap;
    size_t name_len;

    if (dev == NULL) {
        LOG_E("Invalid parameter: dev is NULL");
        return -EINVAL;
    }

    if (adapter_name == NULL) {
        LOG_E("Invalid parameter: adapter_name is NULL");
        return -EINVAL;
    }

    adap = i2c_find_adapter(adapter_name);
    if (adap == NULL) {
        LOG_E("I2C adapter %s not found", adapter_name);
        return -ENODEV;
    }

    dev->client.addr    = (uint16_t)addr;
    dev->client.flags   = 0U;
    dev->client.adapter = adap;
    name_len = sizeof(dev->client.name) - 1U;
    (void)strncpy(dev->client.name, "tca6424", name_len);
    dev->client.name[name_len] = '\0';

    dev->rst_pin = rst_pin;
    dev->int_pin = int_pin;

    if (dev->rst_pin != UINT32_MAX) {
        gpio_set_mode(dev->rst_pin, PIN_OUTPUT_PP, PIN_PULL_UP);
        gpio_write(dev->rst_pin, 1);
    }

    if (dev->int_pin != UINT32_MAX) {
        gpio_set_mode(dev->int_pin, PIN_INPUT, PIN_PULL_UP);
    }

    (void)memset(dev->out_cache, 0xFF, (size_t)TCA6424_NUM_PORTS);
    (void)memset(dev->in_cache, 0x00, (size_t)TCA6424_NUM_PORTS);
    (void)memset(dev->pol_cache, 0x00, (size_t)TCA6424_NUM_PORTS);
    (void)memset(dev->cfg_cache, 0xFF, (size_t)TCA6424_NUM_PORTS);

    return tca6424_sync_cache(dev);
}

/**
 * @brief Perform a hardware reset of TCA6424 through the RST# pin.
 * @param dev Pointer to device structure; if NULL or rst_pin is UINT32_MAX, the function returns immediately.
 */
void tca6424_reset(tca6424_t *dev)
{
    if (dev == NULL) {
        return;
    }
    if (dev->rst_pin == UINT32_MAX) {
        return;
    }
    
    /* Drive RESET pin low for a short pulse to generate a valid reset signal */
    gpio_write(dev->rst_pin, 0);
    bsp_dwt_delay_us(TCA6424_RESET_PULSE_DURATION);
    
    /* Release the RESET pin and wait to ensure internal reset completes */
    gpio_write(dev->rst_pin, 1);
    bsp_dwt_delay_us(TCA6424_TIME_TO_RESET);
}

/**
 * @brief Read input/output/polarity/configuration registers from the device and update local caches.
 * @param dev Pointer to device structure, must not be NULL.
 * @return 0 on success; -EINVAL if dev is NULL; other negative values on I2C read failure.
 */
int tca6424_sync_cache(tca6424_t *dev)
{
    int ret;

    if (dev == NULL) {
        return -EINVAL;
    }

    ret = i2c_read_reg(&dev->client,
                       (uint16_t)TCA6424_CMD_INPUT_AI,
                       TCA6424_CMD_SIZE,
                       dev->in_cache,
                       TCA6424_NUM_PORTS);
    if (ret != (int)TCA6424_NUM_PORTS) {
        return (ret < 0) ? ret : -EIO;
    }

    ret = i2c_read_reg(&dev->client,
                       (uint16_t)TCA6424_CMD_OUTPUT_AI,
                       TCA6424_CMD_SIZE,
                       dev->out_cache,
                       TCA6424_NUM_PORTS);
    if (ret != (int)TCA6424_NUM_PORTS) {
        return (ret < 0) ? ret : -EIO;
    }

    ret = i2c_read_reg(&dev->client,
                       (uint16_t)TCA6424_CMD_POL_AI,
                       TCA6424_CMD_SIZE,
                       dev->pol_cache,
                       TCA6424_NUM_PORTS);
    if (ret != (int)TCA6424_NUM_PORTS) {
        return (ret < 0) ? ret : -EIO;
    }

    ret = i2c_read_reg(&dev->client,
                       (uint16_t)TCA6424_CMD_CFG_AI,
                       TCA6424_CMD_SIZE,
                       dev->cfg_cache,
                       TCA6424_NUM_PORTS);
    if (ret != (int)TCA6424_NUM_PORTS) {
        return (ret < 0) ? ret : -EIO;
    }

    return 0;
}

/**
 * @brief Read input register (8 bits) of a given port and update in_cache.
 * @param dev Pointer to device structure, must not be NULL.
 * @param port Port index 0/1/2.
 * @param val Output pointer for the 8-bit input value of the port.
 * @return 0 on success; -EINVAL on invalid parameters or port >= TCA6424_NUM_PORTS; other negative values on I2C error.
 */
int tca6424_read_port(tca6424_t *dev, uint8_t port, uint8_t *val)
{
    int ret;

    if (dev == NULL || val == NULL) {
        return -EINVAL;
    }
    if (port >= TCA6424_NUM_PORTS) {
        return -EINVAL;
    }

    ret = i2c_read_reg(&dev->client,
                       (uint16_t)(TCA6424_REG_INPUT_PORT0 + port),
                       TCA6424_CMD_SIZE,
                       val,
                       1U);
    if (ret != 1) {
        return (ret < 0) ? ret : -EIO;
    }
    dev->in_cache[port] = *val;
    return 0;
}

/**
 * @brief Write an 8-bit value to the output register of a given port and update out_cache.
 * @param dev Pointer to device structure, must not be NULL.
 * @param port Port index 0/1/2.
 * @param val 8-bit value to be written to the output register.
 * @return 0 on success; -EINVAL on invalid parameters or port >= TCA6424_NUM_PORTS; other negative values on I2C error.
 */
int tca6424_write_port(tca6424_t *dev, uint8_t port, uint8_t val)
{
    int ret;

    if (dev == NULL) {
        return -EINVAL;
    }
    if (port >= TCA6424_NUM_PORTS) {
        return -EINVAL;
    }

    ret = i2c_write_reg(&dev->client,
                        (uint16_t)(TCA6424_REG_OUTPUT_PORT0 + port),
                        TCA6424_CMD_SIZE,
                        &val,
                        1U);
    if (ret != 1) {
        return (ret < 0) ? ret : -EIO;
    }
    dev->out_cache[port] = val;
    return 0;
}

/**
 * @brief Read all 24 input bits at once (Port0 in bits 0–7, Port1 in bits 8–15, Port2 in bits 16–23) and update in_cache.
 * @param dev Pointer to device structure, must not be NULL.
 * @param val Output 24-bit input state.
 * @return 0 on success; -EINVAL on invalid parameters; other negative values on I2C error.
 */
int tca6424_read_inputs(tca6424_t *dev, uint32_t *val)
{
    int ret;
    uint8_t i;
    uint8_t buf[TCA6424_NUM_PORTS];

    if (dev == NULL || val == NULL) {
        return -EINVAL;
    }

    ret = i2c_read_reg(&dev->client,
                       (uint16_t)TCA6424_CMD_INPUT_AI,
                       TCA6424_CMD_SIZE,
                       buf,
                       TCA6424_NUM_PORTS);
    if (ret != (int)TCA6424_NUM_PORTS) {
        return (ret < 0) ? ret : -EIO;
    }
    for (i = 0U; i < TCA6424_NUM_PORTS; i++) {
        dev->in_cache[i] = buf[i];
    }
    *val = (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) | ((uint32_t)buf[2] << 16);
    return 0;
}

/**
 * @brief Write all 24 output bits at once: bits 0–7/8–15/16–23 go to Port0/1/2 respectively, and update out_cache.
 * @param dev Pointer to device structure, must not be NULL.
 * @param val 24-bit output value.
 * @return 0 on success; -EINVAL on invalid parameters; other negative values on I2C error.
 */
int tca6424_write_outputs(tca6424_t *dev, uint32_t val)
{
    int ret;
    uint8_t i;
    uint8_t buf[TCA6424_NUM_PORTS];

    if (dev == NULL) {
        return -EINVAL;
    }

    buf[0] = (uint8_t)(val & 0xFFU);
    buf[1] = (uint8_t)((val >> 8) & 0xFFU);
    buf[2] = (uint8_t)((val >> 16) & 0xFFU);
    ret = i2c_write_reg(&dev->client,
                        (uint16_t)TCA6424_CMD_OUTPUT_AI,
                        TCA6424_CMD_SIZE,
                        buf,
                        TCA6424_NUM_PORTS);
    if (ret != (int)TCA6424_NUM_PORTS) {
        return (ret < 0) ? ret : -EIO;
    }
    for (i = 0U; i < TCA6424_NUM_PORTS; i++) {
        dev->out_cache[i] = buf[i];
    }
    return 0;
}

/**
 * @brief Read all 24 configuration bits (1=input, 0=output) at once and update cfg_cache.
 * @param dev Pointer to device structure, must not be NULL.
 * @param cfg Output 24-bit configuration value.
 * @return 0 on success; -EINVAL on invalid parameters; other negative values on I2C error.
 */
int tca6424_read_config(tca6424_t *dev, uint32_t *cfg)
{
    int ret;
    uint8_t i;
    uint8_t buf[TCA6424_NUM_PORTS];

    if (dev == NULL || cfg == NULL) {
        return -EINVAL;
    }

    ret = i2c_read_reg(&dev->client,
                       (uint16_t)TCA6424_CMD_CFG_AI,
                       TCA6424_CMD_SIZE,
                       buf,
                       TCA6424_NUM_PORTS);
    if (ret != (int)TCA6424_NUM_PORTS) {
        return (ret < 0) ? ret : -EIO;
    }
    for (i = 0U; i < TCA6424_NUM_PORTS; i++) {
        dev->cfg_cache[i] = buf[i];
    }
    *cfg = (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) | ((uint32_t)buf[2] << 16);
    return 0;
}

/**
 * @brief Write all 24 configuration bits at once and update cfg_cache.
 * @param dev Pointer to device structure, must not be NULL.
 * @param cfg 24-bit configuration value (1=input, 0=output).
 * @return 0 on success; -EINVAL on invalid parameters; other negative values on I2C error.
 */
int tca6424_write_config(tca6424_t *dev, uint32_t cfg)
{
    int ret;
    uint8_t i;
    uint8_t buf[TCA6424_NUM_PORTS];

    if (dev == NULL) {
        return -EINVAL;
    }

    buf[0] = (uint8_t)(cfg & 0xFFU);
    buf[1] = (uint8_t)((cfg >> 8) & 0xFFU);
    buf[2] = (uint8_t)((cfg >> 16) & 0xFFU);
    ret = i2c_write_reg(&dev->client,
                        (uint16_t)TCA6424_CMD_CFG_AI,
                        TCA6424_CMD_SIZE,
                        buf,
                        TCA6424_NUM_PORTS);
    if (ret != (int)TCA6424_NUM_PORTS) {
        return (ret < 0) ? ret : -EIO;
    }
    for (i = 0U; i < TCA6424_NUM_PORTS; i++) {
        dev->cfg_cache[i] = buf[i];
    }
    return 0;
}

/**
 * @brief Read all 24 polarity-inversion bits at once and update pol_cache.
 * @param dev Pointer to device structure, must not be NULL.
 * @param pol Output 24-bit polarity value (1=invert that input bit).
 * @return 0 on success; -EINVAL on invalid parameters; other negative values on I2C error.
 */
int tca6424_read_polarity(tca6424_t *dev, uint32_t *pol)
{
    int ret;
    uint8_t i;
    uint8_t buf[TCA6424_NUM_PORTS];

    if (dev == NULL || pol == NULL) {
        return -EINVAL;
    }

    ret = i2c_read_reg(&dev->client,
                       (uint16_t)TCA6424_CMD_POL_AI,
                       TCA6424_CMD_SIZE,
                       buf,
                       TCA6424_NUM_PORTS);
    if (ret != (int)TCA6424_NUM_PORTS) {
        return (ret < 0) ? ret : -EIO;
    }
    for (i = 0U; i < TCA6424_NUM_PORTS; i++) {
        dev->pol_cache[i] = buf[i];
    }
    *pol = (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) | ((uint32_t)buf[2] << 16);
    return 0;
}

/**
 * @brief Write all 24 polarity-inversion bits at once and update pol_cache.
 * @param dev Pointer to device structure, must not be NULL.
 * @param pol 24-bit polarity value (1=invert that input bit).
 * @return 0 on success; -EINVAL on invalid parameters; other negative values on I2C error.
 */
int tca6424_write_polarity(tca6424_t *dev, uint32_t pol)
{
    int ret;
    uint8_t i;
    uint8_t buf[TCA6424_NUM_PORTS];

    if (dev == NULL) {
        return -EINVAL;
    }

    buf[0] = (uint8_t)(pol & 0xFFU);
    buf[1] = (uint8_t)((pol >> 8) & 0xFFU);
    buf[2] = (uint8_t)((pol >> 16) & 0xFFU);
    ret = i2c_write_reg(&dev->client,
                        (uint16_t)TCA6424_CMD_POL_AI,
                        TCA6424_CMD_SIZE,
                        buf,
                        TCA6424_NUM_PORTS);
    if (ret != (int)TCA6424_NUM_PORTS) {
        return (ret < 0) ? ret : -EIO;
    }
    for (i = 0U; i < TCA6424_NUM_PORTS; i++) {
        dev->pol_cache[i] = buf[i];
    }
    return 0;
}

/**
 * @brief Read a single pin level (pin 0–23) by reading its port and extracting the bit.
 * @param dev Pointer to device structure, must not be NULL.
 * @param pin Pin index 0–23.
 * @param level Output 0 or 1.
 * @return 0 on success; -EINVAL on invalid parameters or pin >= TCA6424_NUM_PINS; other negative values on I2C error.
 */
int tca6424_read_pin(tca6424_t *dev, uint8_t pin, uint8_t *level)
{
    uint8_t port;
    uint8_t byte_val;
    int ret;

    if (dev == NULL || level == NULL) {
        return -EINVAL;
    }
    if (pin >= TCA6424_NUM_PINS) {
        return -EINVAL;
    }

    port = TCA6424_PORT(pin);
    ret = tca6424_read_port(dev, port, &byte_val);
    if (ret != 0) {
        return ret;
    }
    *level = (uint8_t)((byte_val >> TCA6424_BIT(pin)) & 1U);
    return 0;
}

/**
 * @brief Write a single pin output level (pin 0–23) by modifying only that bit and writing back the port.
 * @param dev Pointer to device structure, must not be NULL.
 * @param pin Pin index 0–23.
 * @param level 0 or 1.
 * @return 0 on success; -EINVAL on invalid parameters or pin >= TCA6424_NUM_PINS; other negative values on I2C error.
 */
int tca6424_write_pin(tca6424_t *dev, uint8_t pin, uint8_t level)
{
    uint8_t port;
    uint8_t mask;
    uint8_t new_val;

    if (dev == NULL) {
        return -EINVAL;
    }
    if (pin >= TCA6424_NUM_PINS) {
        return -EINVAL;
    }

    port = TCA6424_PORT(pin);
    mask = (uint8_t)(1U << TCA6424_BIT(pin));
    new_val = (uint8_t)((dev->out_cache[port] & (uint8_t)~mask) |
                        (level ? mask : 0U));
    return tca6424_write_port(dev, port, new_val);
}

/**
 * @brief Configure the direction of a single pin (input or output) and update cfg_cache.
 * @param dev Pointer to device structure, must not be NULL.
 * @param pin Pin index 0–23.
 * @param dir TCA6424_DIR_INPUT (input) or TCA6424_DIR_OUTPUT (output).
 * @return 0 on success; -EINVAL on invalid parameters or pin >= TCA6424_NUM_PINS; other negative values on I2C error.
 */
int tca6424_set_direction(tca6424_t *dev, uint8_t pin, tca6424_dir_t dir)
{
    uint8_t port;
    uint8_t bit_pos;
    uint8_t mask;
    uint8_t new_cfg;
    int ret;

    if (dev == NULL) {
        return -EINVAL;
    }
    if (pin >= TCA6424_NUM_PINS) {
        return -EINVAL;
    }

    port = TCA6424_PORT(pin);
    bit_pos = TCA6424_BIT(pin);
    mask = (uint8_t)(1U << bit_pos);
    new_cfg = (uint8_t)((dev->cfg_cache[port] & (uint8_t)~mask) |
                        (dir == TCA6424_DIR_INPUT ? mask : 0U));

    ret = i2c_write_reg(&dev->client,
                        (uint16_t)(TCA6424_REG_CFG_PORT0 + port),
                        TCA6424_CMD_SIZE,
                        &new_cfg,
                        1U);
    if (ret != 1) {
        return (ret < 0) ? ret : -EIO;
    }
    dev->cfg_cache[port] = new_cfg;
    return 0;
}

/**
 * @brief Configure whether the input polarity of a single pin is inverted and update pol_cache.
 * @param dev Pointer to device structure, must not be NULL.
 * @param pin Pin index 0–23.
 * @param invert true to invert that input bit; false for normal polarity.
 * @return 0 on success; -EINVAL on invalid parameters or pin >= TCA6424_NUM_PINS; other negative values on I2C error.
 */
int tca6424_set_polarity(tca6424_t *dev, uint8_t pin, bool invert)
{
    uint8_t port;
    uint8_t bit_pos;
    uint8_t mask;
    uint8_t new_pol;
    int ret;

    if (dev == NULL) {
        return -EINVAL;
    }
    if (pin >= TCA6424_NUM_PINS) {
        return -EINVAL;
    }

    port = TCA6424_PORT(pin);
    bit_pos = TCA6424_BIT(pin);
    mask = (uint8_t)(1U << bit_pos);
    new_pol = (uint8_t)((dev->pol_cache[port] & (uint8_t)~mask) |
                        (invert ? mask : 0U));

    ret = i2c_write_reg(&dev->client,
                        (uint16_t)(TCA6424_REG_POL_PORT0 + port),
                        TCA6424_CMD_SIZE,
                        &new_pol,
                        1U);
    if (ret != 1) {
        return (ret < 0) ? ret : -EIO;
    }
    dev->pol_cache[port] = new_pol;
    return 0;
}

/* Private functions ---------------------------------------------------------*/

