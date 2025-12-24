/**
 ******************************************************************************
 * @file : ad5272.c
 * @author : ZJY
 * @version : V1.0
 * @date : 2025-01-XX
 * @brief : AD5272数字变阻器I2C驱动实现
 * @attention : None
 ******************************************************************************
 * @history :
 * V1.0 : 1.初始版本，实现AD5272基本功能
 *
 *
 ******************************************************************************
 */
/* Includes ------------------------------------------------------------------*/
#include "ad5272.h"

#define  LOG_TAG             "ad5272"
#define  LOG_LVL             4
#include "log.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define AD5272_CMD_MASK            (0xF0U)  /**< 命令位掩码（高4位） */
#define AD5272_DATA_HIGH_MASK      (0x0FU)  /**< 数据高4位掩码 */
#define AD5272_DATA_LOW_MASK       (0xFFU)  /**< 数据低8位掩码 */
#define AD5272_RDAC_DATA_MASK      (0x03FFU) /**< RDAC数据掩码（10位） */

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Exported variables -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static int ad5272_write_reg(ad5272_dev_t *dev, uint8_t reg, uint16_t data);
static int ad5272_read_reg(ad5272_dev_t *dev, uint8_t reg, uint16_t *data);
static int ad5272_write_control(ad5272_dev_t *dev, uint8_t ctrl_value);
static int ad5272_read_control(ad5272_dev_t *dev, uint8_t *ctrl_value);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 初始化AD5272设备
 * @param dev 设备结构体指针
 * @param i2c_addr I2C设备地址（7位地址）
 * @param adapter_name I2C适配器名称
 * @return 0成功，负数错误码
 */
int ad5272_init(ad5272_dev_t *dev, uint8_t i2c_addr, const char *adapter_name)
{
    int ret = 0;
    uint8_t status = 0U;
    
    /* 参数验证 */
    if (dev == NULL) {
        LOG_E("Invalid parameter: dev is NULL");
        return -EINVAL;
    }
    
    if (adapter_name == NULL) {
        adapter_name = AD5272_DEFAULT_ADAPTER;
        LOG_W("adapter_name is NULL, use default: %s", adapter_name);
    }
    
    LOG_D("Initializing AD5272, I2C addr: 0x%02X, adapter: %s", i2c_addr, adapter_name);
    
    /* 创建I2C客户端 */
    dev->client = i2c_new_client("ad5272", adapter_name, (uint16_t)i2c_addr, 0U);
    if (dev->client == NULL) {
        LOG_E("Failed to create I2C client for AD5272");
        return -ENODEV;
    }
    
    /* 初始化设备结构体 */
    dev->max_position = AD5272_MAX_POSITION;
    
    /* 注意：初始化时不立即验证设备通信，避免I2C操作影响其他外设（如串口DMA）
     * 设备通信验证可以在首次实际使用时进行
     */
    LOG_I("AD5272 initialized successfully");
    return 0;
}

/**
 * @brief 反初始化AD5272设备
 * @param dev 设备结构体指针
 * @return 0成功，负数错误码
 */
int ad5272_deinit(ad5272_dev_t *dev)
{
    int ret = 0;
    
    /* 参数验证 */
    if (dev == NULL) {
        LOG_E("Invalid parameter: dev is NULL");
        return -EINVAL;
    }
    
    /* 删除I2C客户端 */
    if (dev->client != NULL) {
        LOG_D("Deinitializing AD5272");
        ret = i2c_del_client(dev->client);
        dev->client = NULL;
        if (ret == 0) {
            LOG_I("AD5272 deinitialized successfully");
        } else {
            LOG_W("AD5272 deinit returned error: %d", ret);
        }
    }
    
    return ret;
}

/**
 * @brief 设置阻值位置
 * @param dev 设备结构体指针
 * @param position 位置值（0-1023）
 * @return 0成功，负数错误码
 */
int ad5272_set_resistance(ad5272_dev_t *dev, uint16_t position)
{
    int ret = 0;
    
    /* 参数验证 */
    if (dev == NULL) {
        LOG_E("Invalid parameter: dev is NULL");
        return -EINVAL;
    }
    
    if (dev->client == NULL) {
        LOG_E("Device not initialized");
        return -ENODEV;
    }
    
    if (position > AD5272_MAX_POSITION) {
        LOG_E("Invalid position: %d (max: %d)", position, AD5272_MAX_POSITION);
        return -EINVAL;
    }
    
    LOG_D("Setting resistance position: %d", position);
    
    /* 写入RDAC寄存器 */
    ret = ad5272_write_reg(dev, AD5272_REG_RDAC, position);
    if (ret != 0) {
        LOG_E("Failed to set resistance position: %d", ret);
    } else {
        LOG_D("Resistance position set successfully: %d", position);
    }
    
    return ret;
}

/**
 * @brief 读取当前阻值位置
 * @param dev 设备结构体指针
 * @param position 输出位置值指针（0-1023）
 * @return 0成功，负数错误码
 */
int ad5272_get_resistance(ad5272_dev_t *dev, uint16_t *position)
{
    int ret = 0;
    uint16_t rdac_value = 0U;
    
    /* 参数验证 */
    if ((dev == NULL) || (position == NULL)) {
        LOG_E("Invalid parameter: dev or position is NULL");
        return -EINVAL;
    }
    
    if (dev->client == NULL) {
        LOG_E("Device not initialized");
        return -ENODEV;
    }
    
    /* 读取RDAC寄存器 */
    ret = ad5272_read_reg(dev, AD5272_REG_RDAC, &rdac_value);
    if (ret == 0) {
        *position = rdac_value & AD5272_RDAC_DATA_MASK;
        LOG_D("Read resistance position: %d", *position);
    } else {
        LOG_E("Failed to read resistance position: %d", ret);
    }
    
    return ret;
}

/**
 * @brief 设置写保护
 * @param dev 设备结构体指针
 * @param enable true使能写保护，false禁用写保护
 * @return 0成功，负数错误码
 */
int ad5272_set_write_protect(ad5272_dev_t *dev, bool enable)
{
    int ret = 0;
    uint8_t ctrl_value = 0U;
    
    /* 参数验证 */
    if (dev == NULL) {
        LOG_E("Invalid parameter: dev is NULL");
        return -EINVAL;
    }
    
    if (dev->client == NULL) {
        LOG_E("Device not initialized");
        return -ENODEV;
    }
    
    LOG_D("Setting write protect: %s", enable ? "enable" : "disable");
    
    /* 读取当前控制寄存器值 */
    ret = ad5272_read_control(dev, &ctrl_value);
    if (ret != 0) {
        LOG_E("Failed to read control register: %d", ret);
        return ret;
    }
    
    /* 设置或清除写保护位 */
    if (enable) {
        ctrl_value |= AD5272_CTRL_WP_MASK;
    } else {
        ctrl_value &= (uint8_t)(~AD5272_CTRL_WP_MASK);
    }
    
    /* 写回控制寄存器 */
    ret = ad5272_write_control(dev, ctrl_value);
    if (ret != 0) {
        LOG_E("Failed to set write protect: %d", ret);
    } else {
        LOG_D("Write protect %s successfully", enable ? "enabled" : "disabled");
    }
    
    return ret;
}

/**
 * @brief 读取状态寄存器
 * @param dev 设备结构体指针
 * @param status 输出状态值指针
 * @return 0成功，负数错误码
 */
int ad5272_read_status(ad5272_dev_t *dev, uint8_t *status)
{
    int ret = 0;
    uint16_t reg_value = 0U;
    
    /* 参数验证 */
    if ((dev == NULL) || (status == NULL)) {
        LOG_E("Invalid parameter: dev or status is NULL");
        return -EINVAL;
    }
    
    if (dev->client == NULL) {
        LOG_E("Device not initialized");
        return -ENODEV;
    }
    
    /* 读取状态寄存器 */
    ret = ad5272_read_reg(dev, AD5272_REG_STATUS, &reg_value);
    if (ret == 0) {
        *status = (uint8_t)(reg_value & 0xFFU);
        LOG_D("Read status register: 0x%02X", *status);
    } else {
        LOG_E("Failed to read status register: %d", ret);
    }
    
    return ret;
}

/**
 * @brief 设置关断模式
 * @param dev 设备结构体指针
 * @param enable true进入关断模式，false退出关断模式
 * @return 0成功，负数错误码
 */
int ad5272_shutdown(ad5272_dev_t *dev, bool enable)
{
    int ret = 0;
    uint8_t ctrl_value = 0U;
    
    /* 参数验证 */
    if (dev == NULL) {
        LOG_E("Invalid parameter: dev is NULL");
        return -EINVAL;
    }
    
    if (dev->client == NULL) {
        LOG_E("Device not initialized");
        return -ENODEV;
    }
    
    LOG_D("Setting shutdown mode: %s", enable ? "enable" : "disable");
    
    /* 读取当前控制寄存器值 */
    ret = ad5272_read_control(dev, &ctrl_value);
    if (ret != 0) {
        LOG_E("Failed to read control register: %d", ret);
        return ret;
    }
    
    /* 设置或清除关断位 */
    if (enable) {
        ctrl_value |= AD5272_CTRL_SHDN_MASK;
    } else {
        ctrl_value &= (uint8_t)(~AD5272_CTRL_SHDN_MASK);
    }
    
    /* 写回控制寄存器 */
    ret = ad5272_write_control(dev, ctrl_value);
    if (ret != 0) {
        LOG_E("Failed to set shutdown mode: %d", ret);
    } else {
        LOG_D("Shutdown mode %s successfully", enable ? "enabled" : "disabled");
    }
    
    return ret;
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 写寄存器（内部函数）
 * @param dev 设备结构体指针
 * @param reg 寄存器地址
 * @param data 数据值（10位）
 * @return 0成功，负数错误码
 */
static int ad5272_write_reg(ad5272_dev_t *dev, uint8_t reg, uint16_t data)
{
    int ret = 0;
    uint8_t cmd_byte = 0U;
    uint8_t data_byte = 0U;
    uint8_t tx_buf[2U] = {0U, 0U};
    
    if (dev == NULL) {
        return -EINVAL;
    }
    
    if (dev->client == NULL) {
        return -ENODEV;
    }
    
    /* 构建命令字节：高4位为命令，低4位为数据高4位 */
    cmd_byte = (uint8_t)((reg << 4U) | ((data >> 8U) & AD5272_DATA_HIGH_MASK));
    data_byte = (uint8_t)(data & AD5272_DATA_LOW_MASK);
    
    /* 准备发送缓冲区 */
    tx_buf[0U] = cmd_byte;
    tx_buf[1U] = data_byte;
    
    LOG_D("Writing register 0x%02X, data: 0x%04X (cmd: 0x%02X, data: 0x%02X)", 
          reg, data, cmd_byte, data_byte);
    
    /* 发送数据 */
    ret = i2c_master_send(dev->client, tx_buf, 2U);
    if (ret < 0) {
        LOG_E("I2C write failed: %d", ret);
        return -EIO;
    }
    
    return 0;
}

/**
 * @brief 读寄存器（内部函数）
 * @param dev 设备结构体指针
 * @param reg 寄存器地址
 * @param data 输出数据值指针（10位）
 * @return 0成功，负数错误码
 */
static int ad5272_read_reg(ad5272_dev_t *dev, uint8_t reg, uint16_t *data)
{
    int ret = 0;
    uint8_t cmd_byte[2] = {0};
    uint8_t rx_buf[2U] = {0U, 0U};
    uint16_t reg_value = 0U;
    
    if ((dev == NULL) || (data == NULL)) {
        return -EINVAL;
    }
    
    if (dev->client == NULL) {
        return -ENODEV;
    }
    
    /* 构建读命令字节：高4位为读命令，低4位为0 */
    cmd_byte[0] = (uint8_t)((reg << 4U) | 0x08U);
    
    LOG_D("Reading register 0x%02X (cmd: 0x%02X)", reg, cmd_byte);
    
    /* 先写命令字节，再读2字节数据 */
    ret = i2c_write_then_read(dev->client, &cmd_byte, 2U, rx_buf, 2U);
    if (ret < 0) {
        LOG_E("I2C read failed: %d", ret);
        return -EIO;
    }
    
    /* 组合数据：高字节的低4位 + 低字节 */
    reg_value = (uint16_t)(((uint16_t)rx_buf[0U] & AD5272_DATA_HIGH_MASK) << 8U);
    reg_value |= (uint16_t)rx_buf[1U];
    
    *data = reg_value;
    
    LOG_D("Read register 0x%02X, data: 0x%04X", reg, *data);
    
    return 0;
}

/**
 * @brief 写控制寄存器（内部函数）
 * @param dev 设备结构体指针
 * @param ctrl_value 控制寄存器值
 * @return 0成功，负数错误码
 */
static int ad5272_write_control(ad5272_dev_t *dev, uint8_t ctrl_value)
{
    int ret = 0;
    
    if (dev == NULL) {
        return -EINVAL;
    }
    
    /* 控制寄存器是8位的，但使用16位接口 */
    ret = ad5272_write_reg(dev, AD5272_REG_CONTROL, (uint16_t)ctrl_value);
    
    return ret;
}

/**
 * @brief 读控制寄存器（内部函数）
 * @param dev 设备结构体指针
 * @param ctrl_value 输出控制寄存器值指针
 * @return 0成功，负数错误码
 */
static int ad5272_read_control(ad5272_dev_t *dev, uint8_t *ctrl_value)
{
    int ret = 0;
    uint16_t reg_value = 0U;
    
    if ((dev == NULL) || (ctrl_value == NULL)) {
        return -EINVAL;
    }
    
    /* 读取控制寄存器 */
    ret = ad5272_read_reg(dev, AD5272_REG_CONTROL, &reg_value);
    if (ret == 0) {
        *ctrl_value = (uint8_t)(reg_value & 0xFFU);
    }
    
    return ret;
}
