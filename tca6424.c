/**
  ******************************************************************************
  * @file        : tca6424.c
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2025-01-XX
  * @brief       : TCA6424 24位I²C I/O扩展器驱动实现
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1. 初始版本，实现TCA6424基本功能
  *                2. 支持24位I/O端口读写
  *                3. 支持单引脚操作
  *                4. 支持配置、输出、极性反转寄存器操作
  *                5. 实现寄存器缓存机制，减少I2C访问
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/

#include "tca6424.h"
#include <string.h>
#include <errno-base.h>

#define  LOG_TAG             "tca6424"
#define  LOG_LVL             4
#include "log.h"

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

/** @brief 最大寄存器数据长度（3个端口） */
#define TCA6424_MAX_REG_LEN             (3U)

/** @brief 最大引脚编号 */
#define TCA6424_MAX_PIN                 (23U)

/**
 * @brief 构造命令字节
 * @param ai 自动递增标志（true=启用，false=禁用）
 * @param reg 寄存器地址（0x00-0x0E）
 * @return 命令字节（bit7=AI，bit0-6=寄存器地址）
 */
#define TCA6424_CMD(ai, reg)   (uint8_t)(((ai) ? 0x80u : 0x00u) | ((reg) & 0x7Fu))

/* Private macro -------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/



/* Exported variables  -------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/



/* Exported functions --------------------------------------------------------*/

/**
 * @brief 初始化TCA6424设备
 * @param dev 设备结构体指针
 * @param addr7 I2C设备地址（7位地址，0x22或0x23）
 * @param adapter_name I2C适配器名称（如"i2c1"）
 * @return 0成功，负数表示错误码
 * @note 初始化后所有引脚默认为输入模式，输出寄存器为0xFF（高电平）
 */
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

    /* 验证I2C地址 */
    if ((addr7 != TCA6424_I2C_ADDR_L) && (addr7 != TCA6424_I2C_ADDR_H)) {
        LOG_E("Invalid I2C address: 0x%02X", addr7);
        return -EINVAL;
    }

    /* 查找I2C适配器 */
    dev->adapter = i2c_find_adapter(adapter_name);
    if (dev->adapter == NULL) {
        LOG_E("Failed to find I2C adapter: %s", adapter_name);
        return -ENODEV;
    }
    
    /* 设置设备地址和标志 */
    dev->addr = addr7;
    dev->flags = 0U;
    
    /* 初始化缓存状态 */
    dev->cache_valid = false;
    (void)memset(dev->out, 0xFFu, sizeof(dev->out));  /* 默认输出为高 */
    (void)memset(dev->cfg, 0xFFu, sizeof(dev->cfg));  /* 默认配置为输入 */
    (void)memset(dev->pol, 0x00u, sizeof(dev->pol));  /* 默认极性正常 */
    
    return 0;
}


/**
 * @brief 读取寄存器（原始访问）
 * @param dev 设备结构体指针
 * @param reg 寄存器地址（0x00-0x0E）
 * @param auto_inc 是否启用自动递增
 * @param rx_buf 接收缓冲区
 * @param len 读取长度（最大3字节）
 * @return 0成功，负数表示错误码
 */
int tca6424_read_reg(tca6424_t *dev, uint8_t reg, bool auto_inc, uint8_t *rx_buf, uint8_t len)
{
    int ret = 0;
    uint8_t cmd = 0u;
    
    /* 参数验证 */
    if ((dev == NULL) || (dev->adapter == NULL) || (rx_buf == NULL)) {
        return -EINVAL;
    }
    
    if ((len == 0u) || (len > TCA6424_MAX_REG_LEN)) {
        LOG_E("Invalid read length: %u", len);
        return -EINVAL;
    }
    
    /* 构造命令字节 */
    cmd = TCA6424_CMD(auto_inc, reg);
    
    /* 执行I2C写后读操作 */
    ret = i2c_write_then_read(dev->adapter, dev->addr, dev->flags,
                              &cmd, 1u, rx_buf, len);
    if (ret < 0) {
        LOG_E("I2C read failed: %d", ret);
        return ret;
    }
    
    return 0;
}

/**
 * @brief 写入寄存器（原始访问）
 * @param dev 设备结构体指针
 * @param reg 寄存器地址（0x00-0x0E）
 * @param auto_inc 是否启用自动递增
 * @param tx_buf 发送缓冲区
 * @param len 写入长度（最大3字节）
 * @return 0成功，负数表示错误码
 */
int tca6424_write_reg(tca6424_t *dev, uint8_t reg, bool auto_inc, uint8_t *tx_buf, uint8_t len)
{
    int ret = 0;
    uint8_t frame[1u + TCA6424_MAX_REG_LEN]; /* 1字节命令 + 最多3字节数据 */
    uint8_t i = 0u;
    
    /* 参数验证 */
    if ((dev == NULL) || (dev->adapter == NULL) || (tx_buf == NULL)) {
        return -EINVAL;
    }
    
    if ((len == 0u) || (len > TCA6424_MAX_REG_LEN)) {
        LOG_E("Invalid write length: %u", len);
        return -EINVAL;
    }
    
    /* 构造命令字节 */
    frame[0] = TCA6424_CMD(auto_inc, reg);
    
    /* 复制数据到帧缓冲区 */
    for (i = 0u; i < len; i++) {
        frame[1u + i] = tx_buf[i];
    }
    
    /* 执行I2C写操作 */
    ret = i2c_master_send(dev->adapter, dev->addr, dev->flags, frame, (size_t)(len + 1u));
    if (ret != (int)(len + 1u)) {
        LOG_E("I2C write failed: %d", ret);
        return ret;
    }
    
    return 0;
}

/**
 * @brief 刷新设备缓存（从硬件读取所有寄存器）
 * @param dev 设备结构体指针
 * @return 0成功，负数表示错误码
 * @note 读取输出、配置和极性反转寄存器并更新缓存
 */
int tca6424_refresh_cache(tca6424_t *dev)
{
    int ret = 0;
    uint8_t tmp[TCA6424_MAX_REG_LEN] = {0u};
    
    if (dev == NULL) {
        return -EINVAL;
    }
    
    /* 读取输出寄存器 */
    ret = tca6424_read_reg(dev, TCA6424_REG_OUTPUT_PORT0, true, tmp, TCA6424_MAX_REG_LEN);
    if (ret == 0) {
        dev->out[0] = tmp[0]; 
        dev->out[1] = tmp[1]; 
        dev->out[2] = tmp[2];
        
        /* 读取配置寄存器 */
        ret = tca6424_read_reg(dev, TCA6424_REG_CFG_PORT0, true, tmp, TCA6424_MAX_REG_LEN);
    }
    
    if (ret == 0) {
        dev->cfg[0] = tmp[0]; 
        dev->cfg[1] = tmp[1]; 
        dev->cfg[2] = tmp[2];
        
        /* 读取极性反转寄存器 */
        ret = tca6424_read_reg(dev, TCA6424_REG_POL_PORT0, true, tmp, TCA6424_MAX_REG_LEN);
    }
    
    if (ret == 0) {
        dev->pol[0] = tmp[0]; 
        dev->pol[1] = tmp[1]; 
        dev->pol[2] = tmp[2];
        dev->cache_valid = true;
    } else {
        dev->cache_valid = false;
    }
    
    return ret;
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 将3个字节打包为24位值
 * @param b 字节数组 [Port0, Port1, Port2]
 * @return 24位值（bit0-7=Port0, bit8-15=Port1, bit16-23=Port2）
 */
static uint32_t pack_u24(const uint8_t b[3])
{
    return ((uint32_t)b[0]) |
           ((uint32_t)b[1] << 8u) |
           ((uint32_t)b[2] << 16u);
}

/**
 * @brief 将24位值解包为3个字节
 * @param v 24位值
 * @param b 字节数组（输出）[Port0, Port1, Port2]
 */
static void unpack_u24(uint32_t v, uint8_t b[3])
{
    b[0] = (uint8_t)(v & 0xFFu);
    b[1] = (uint8_t)((v >> 8u) & 0xFFu);
    b[2] = (uint8_t)((v >> 16u) & 0xFFu);
}

/**
 * @brief 验证引脚编号
 * @param pin 引脚编号
 * @return 0有效，-EINVAL无效
 */
static int validate_pin(uint8_t pin)
{
    return (pin <= TCA6424_MAX_PIN) ? 0 : -EINVAL;
}

/**
 * @brief 获取引脚对应的位掩码
 * @param pin 引脚编号（0-23）
 * @return 位掩码（1 << pin）
 */
static uint32_t bit_of_pin(uint8_t pin)
{
    return (uint32_t)1u << (uint32_t)pin;
}

/**
 * @brief 读取24位输入端口（bit0=P00 ... bit23=P27）
 * @param dev 设备结构体指针
 * @param in_bits 输入位状态（输出参数）
 * @return 0成功，负数表示错误码
 * @note 输入端口寄存器反映引脚的实际电平，无论引脚配置为输入还是输出
 */
int tca6424_read_inputs24(tca6424_t *dev, uint32_t *in_bits)
{
    int rc = 0;
    uint8_t b[TCA6424_MAX_REG_LEN] = {0u};
    
    if ((dev == NULL) || (in_bits == NULL)) {
        return -EINVAL;
    }

    rc = tca6424_read_reg(dev, TCA6424_REG_INPUT_PORT0, true, b, TCA6424_MAX_REG_LEN);
    if (rc == 0) {
        *in_bits = pack_u24(b);
    }
    
    return rc;
}

/**
 * @brief 读取24位输出锁存器
 * @param dev 设备结构体指针
 * @param out_latch_bits 输出锁存器状态（输出参数）
 * @return 0成功，负数表示错误码
 * @note 读取的是输出锁存器的值，不是引脚的实际电平
 */
int tca6424_read_outputs24(tca6424_t *dev, uint32_t *out_latch_bits)
{
    int rc = 0;
    uint8_t b[TCA6424_MAX_REG_LEN] = {0u};
    
    if ((dev == NULL) || (out_latch_bits == NULL)) {
        return -EINVAL;
    }

    rc = tca6424_read_reg(dev, TCA6424_REG_OUTPUT_PORT0, true, b, TCA6424_MAX_REG_LEN);
    if (rc == 0) {
        *out_latch_bits = pack_u24(b);
    }
    
    return rc;
}

/**
 * @brief 写入24位输出端口
 * @param dev 设备结构体指针
 * @param out_latch_bits 输出位状态（bit0=P00 ... bit23=P27）
 * @return 0成功，负数表示错误码
 * @note 只对配置为输出的引脚有效
 */
int tca6424_write_outputs24(tca6424_t *dev, uint32_t out_latch_bits)
{
    int rc = 0;
    uint8_t b[TCA6424_MAX_REG_LEN] = {0u};
    
    if (dev == NULL) {
        return -EINVAL;
    }
    
    unpack_u24(out_latch_bits, b);
    rc = tca6424_write_reg(dev, TCA6424_REG_OUTPUT_PORT0, true, b, TCA6424_MAX_REG_LEN);
    
    if (rc == 0) {
        dev->out[0] = b[0];
        dev->out[1] = b[1];
        dev->out[2] = b[2];
        dev->cache_valid = true;
    }
    
    return rc;
}

/**
 * @brief 读取24位配置寄存器
 * @param dev 设备结构体指针
 * @param cfg_bits 配置位（输出参数，1=输入，0=输出）
 * @return 0成功，负数表示错误码
 */
int tca6424_read_config24(tca6424_t *dev, uint32_t *cfg_bits)
{
    int rc = 0;
    uint8_t b[TCA6424_MAX_REG_LEN] = {0u};
    
    if ((dev == NULL) || (cfg_bits == NULL)) {
        return -EINVAL;
    }

    rc = tca6424_read_reg(dev, TCA6424_REG_CFG_PORT0, true, b, TCA6424_MAX_REG_LEN);
    if (rc == 0) {
        *cfg_bits = pack_u24(b);
    }
    
    return rc;
}

/**
 * @brief 写入24位配置寄存器
 * @param dev 设备结构体指针
 * @param cfg_bits 配置位（1=输入，0=输出）
 * @return 0成功，负数表示错误码
 * @note 上电默认所有引脚为输入（0xFF）
 */
int tca6424_write_config24(tca6424_t *dev, uint32_t cfg_bits)
{
    int rc = 0;
    uint8_t b[TCA6424_MAX_REG_LEN] = {0u};
    
    if (dev == NULL) {
        return -EINVAL;
    }
    
    unpack_u24(cfg_bits, b);
    rc = tca6424_write_reg(dev, TCA6424_REG_CFG_PORT0, true, b, TCA6424_MAX_REG_LEN);
    
    if (rc == 0) {
        dev->cfg[0] = b[0];
        dev->cfg[1] = b[1];
        dev->cfg[2] = b[2];
        dev->cache_valid = true;
    }
    
    return rc;
}

/**
 * @brief 读取24位极性反转寄存器
 * @param dev 设备结构体指针
 * @param pol_bits 极性反转位（输出参数，1=反转，0=正常）
 * @return 0成功，负数表示错误码
 * @note 只对配置为输入的引脚有效
 */
int tca6424_read_polarity24(tca6424_t *dev, uint32_t *pol_bits)
{
    int rc = 0;
    uint8_t b[TCA6424_MAX_REG_LEN] = {0u};
    
    if ((dev == NULL) || (pol_bits == NULL)) {
        return -EINVAL;
    }
    
    rc = tca6424_read_reg(dev, TCA6424_REG_POL_PORT0, true, b, TCA6424_MAX_REG_LEN);
    if (rc == 0) {
        *pol_bits = pack_u24(b);
    }
    
    return rc;
}

/**
 * @brief 写入24位极性反转寄存器
 * @param dev 设备结构体指针
 * @param pol_bits 极性反转位（1=反转，0=正常）
 * @return 0成功，负数表示错误码
 * @note 只对配置为输入的引脚有效，上电默认为0x00（正常极性）
 */
int tca6424_write_polarity24(tca6424_t *dev, uint32_t pol_bits)
{
    int rc = 0;
    uint8_t b[TCA6424_MAX_REG_LEN] = {0u};
    
    if (dev == NULL) {
        return -EINVAL;
    }
    
    unpack_u24(pol_bits, b);
    rc = tca6424_write_reg(dev, TCA6424_REG_POL_PORT0, true, b, TCA6424_MAX_REG_LEN);
    
    if (rc == 0) {
        dev->pol[0] = b[0];
        dev->pol[1] = b[1];
        dev->pol[2] = b[2];
        dev->cache_valid = true;
    }
    
    return rc;
}

/**
 * @brief 应用掩码更新24位寄存器值
 * @param regv 寄存器值数组（输入输出参数）
 * @param set_mask 置1的位掩码
 * @param clr_mask 清0的位掩码（优先级高于set_mask）
 * @note 如果某位在set_mask和clr_mask中都设置，则clr_mask优先
 */
static void apply_mask_u24(uint8_t regv[3], uint32_t set_mask, uint32_t clr_mask)
{
    uint32_t v = 0u;
    
    v = pack_u24(regv);
    v |= set_mask;
    v &= ~clr_mask;
    unpack_u24(v, regv);
}

/**
 * @brief 更新24位输出端口（掩码操作）
 * @param dev 设备结构体指针
 * @param set_mask 置1的位掩码
 * @param clr_mask 清0的位掩码（优先级高于set_mask）
 * @return 0成功，负数表示错误码
 * @note 使用缓存机制，减少I2C读取操作
 */
int tca6424_update_outputs24(tca6424_t *dev, uint32_t set_mask, uint32_t clr_mask)
{
    int rc = 0;
    uint8_t b[TCA6424_MAX_REG_LEN] = {0u};
    
    if (dev == NULL) {
        return -EINVAL;
    }

    /* 如果缓存无效，先读取当前值 */
    if (!dev->cache_valid) {
        rc = tca6424_read_reg(dev, TCA6424_REG_OUTPUT_PORT0, true, b, TCA6424_MAX_REG_LEN);
        if (rc != 0) {
            return rc;
        }
        dev->out[0] = b[0];
        dev->out[1] = b[1];
        dev->out[2] = b[2];
        dev->cache_valid = true;
    }

    /* 从缓存加载，应用掩码 */
    b[0] = dev->out[0];
    b[1] = dev->out[1];
    b[2] = dev->out[2];
    apply_mask_u24(b, set_mask, clr_mask);

    /* 写入新值 */
    rc = tca6424_write_reg(dev, TCA6424_REG_OUTPUT_PORT0, true, b, TCA6424_MAX_REG_LEN);
    if (rc == 0) {
        dev->out[0] = b[0];
        dev->out[1] = b[1];
        dev->out[2] = b[2];
    }

    return rc;
}

/**
 * @brief 更新24位配置寄存器（掩码操作）
 * @param dev 设备结构体指针
 * @param set_mask 置1的位掩码（设为输入）
 * @param clr_mask 清0的位掩码（设为输出，优先级高于set_mask）
 * @return 0成功，负数表示错误码
 * @note 使用缓存机制，减少I2C读取操作
 */
int tca6424_update_config24(tca6424_t *dev, uint32_t set_mask, uint32_t clr_mask)
{
    int rc = 0;
    uint8_t b[TCA6424_MAX_REG_LEN] = {0u};
    
    if (dev == NULL) {
        return -EINVAL;
    }

    /* 如果缓存无效，先读取当前值 */
    if (!dev->cache_valid) {
        rc = tca6424_read_reg(dev, TCA6424_REG_CFG_PORT0, true, b, TCA6424_MAX_REG_LEN);
        if (rc != 0) {
            return rc;
        }
        dev->cfg[0] = b[0];
        dev->cfg[1] = b[1];
        dev->cfg[2] = b[2];
        dev->cache_valid = true;
    }

    /* 从缓存加载，应用掩码 */
    b[0] = dev->cfg[0];
    b[1] = dev->cfg[1];
    b[2] = dev->cfg[2];
    apply_mask_u24(b, set_mask, clr_mask);

    /* 写入新值 */
    rc = tca6424_write_reg(dev, TCA6424_REG_CFG_PORT0, true, b, TCA6424_MAX_REG_LEN);
    if (rc == 0) {
        dev->cfg[0] = b[0];
        dev->cfg[1] = b[1];
        dev->cfg[2] = b[2];
    }
    
    return rc;
}

/**
 * @brief 更新24位极性反转寄存器（掩码操作）
 * @param dev 设备结构体指针
 * @param set_mask 置1的位掩码（启用反转）
 * @param clr_mask 清0的位掩码（禁用反转，优先级高于set_mask）
 * @return 0成功，负数表示错误码
 * @note 使用缓存机制，减少I2C读取操作
 */
int tca6424_update_polarity24(tca6424_t *dev, uint32_t set_mask, uint32_t clr_mask)
{
    int rc = 0;
    uint8_t b[TCA6424_MAX_REG_LEN] = {0u};
    
    if (dev == NULL) {
        return -EINVAL;
    }

    /* 如果缓存无效，先读取当前值 */
    if (!dev->cache_valid) {
        rc = tca6424_read_reg(dev, TCA6424_REG_POL_PORT0, true, b, TCA6424_MAX_REG_LEN);
        if (rc != 0) {
            return rc;
        }
        dev->pol[0] = b[0];
        dev->pol[1] = b[1];
        dev->pol[2] = b[2];
        dev->cache_valid = true;
    }

    /* 从缓存加载，应用掩码 */
    b[0] = dev->pol[0];
    b[1] = dev->pol[1];
    b[2] = dev->pol[2];
    apply_mask_u24(b, set_mask, clr_mask);

    /* 写入新值 */
    rc = tca6424_write_reg(dev, TCA6424_REG_POL_PORT0, true, b, TCA6424_MAX_REG_LEN);
    if (rc == 0) {
        dev->pol[0] = b[0];
        dev->pol[1] = b[1];
        dev->pol[2] = b[2];
    }
    
    return rc;
}

/**
 * @brief 设置引脚方向
 * @param dev 设备结构体指针
 * @param pin 引脚编号（0-23，对应P00-P27）
 * @param input true=输入，false=输出
 * @return 0成功，负数表示错误码
 */
int tca6424_pin_mode(tca6424_t *dev, uint8_t pin, bool input)
{
    uint32_t m = 0u;
    
    if (dev == NULL) {
        return -EINVAL;
    }
    
    if (validate_pin(pin) != 0) {
        return -EINVAL;
    }

    m = bit_of_pin(pin);
    /* 配置寄存器：1=输入，0=输出 */
    return input ? tca6424_update_config24(dev, m, 0u)   /* 置1 => 输入 */
                 : tca6424_update_config24(dev, 0u, m);  /* 清0 => 输出 */
}

/**
 * @brief 写入引脚电平
 * @param dev 设备结构体指针
 * @param pin 引脚编号（0-23，对应P00-P27）
 * @param level 引脚电平（true=高，false=低）
 * @return 0成功，负数表示错误码
 * @note 只对配置为输出的引脚有效
 */
int tca6424_write_pin(tca6424_t *dev, uint8_t pin, bool level)
{
    uint32_t m = 0u;
    
    if (dev == NULL) {
        return -EINVAL;
    }
    
    if (validate_pin(pin) != 0) {
        return -EINVAL;
    }

    m = bit_of_pin(pin);
    return level ? tca6424_update_outputs24(dev, m, 0u)   /* 置1 => 高电平 */
                 : tca6424_update_outputs24(dev, 0u, m);  /* 清0 => 低电平 */
}

/**
 * @brief 翻转引脚电平
 * @param dev 设备结构体指针
 * @param pin 引脚编号（0-23，对应P00-P27）
 * @return 0成功，负数表示错误码
 * @note 只对配置为输出的引脚有效
 */
int tca6424_toggle_pin(tca6424_t *dev, uint8_t pin)
{
    int rc = 0;
    uint32_t out = 0u;
    uint32_t m = 0u;

    if (dev == NULL) {
        return -EINVAL;
    }
    
    if (validate_pin(pin) != 0) {
        return -EINVAL;
    }

    m = bit_of_pin(pin);

    /* 读取当前输出值 */
    rc = tca6424_read_outputs24(dev, &out);
    if (rc != 0) {
        return rc;
    }
    
    /* 翻转对应位 */
    out ^= m;
    return tca6424_write_outputs24(dev, out);
}

/**
 * @brief 读取引脚电平
 * @param dev 设备结构体指针
 * @param pin 引脚编号（0-23，对应P00-P27）
 * @param level 引脚电平（输出参数）
 * @return 0成功，负数表示错误码
 * @note 读取的是输入端口寄存器，反映引脚的实际电平
 */
int tca6424_read_pin(tca6424_t *dev, uint8_t pin, bool *level)
{
    int rc = 0;
    uint32_t inb = 0u;

    if ((dev == NULL) || (level == NULL)) {
        return -EINVAL;
    }
    
    if (validate_pin(pin) != 0) {
        return -EINVAL;
    }

    rc = tca6424_read_inputs24(dev, &inb);
    if (rc != 0) {
        return rc;
    }

    *level = ((inb & bit_of_pin(pin)) != 0u);
    return 0;
}

/**
 * @brief 配置输出引脚（先设置输出值，再切换为输出模式，避免毛刺）
 * @param dev 设备结构体指针
 * @param pin 引脚编号（0-23，对应P00-P27）
 * @param initial_level 初始输出电平
 * @return 0成功，负数表示错误码
 * @note 此函数先设置输出锁存器，再切换为输出模式，可避免切换时的毛刺
 */
int tca6424_configure_output_pin(tca6424_t *dev, uint8_t pin, bool initial_level)
{
    int rc = 0;

    if (dev == NULL) {
        return -EINVAL;
    }
    
    if (validate_pin(pin) != 0) {
        return -EINVAL;
    }

    /* 步骤1：预加载输出锁存器（减少从输入切换到输出时的毛刺） */
    rc = tca6424_write_pin(dev, pin, initial_level);
    if (rc != 0) {
        return rc;
    }

    /* 步骤2：切换为输出模式（配置寄存器bit清0） */
    return tca6424_pin_mode(dev, pin, false);
}

/* Private functions ---------------------------------------------------------*/

