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
#include "board.h"

#define  LOG_TAG             "ad5272"
#define  LOG_LVL             4
#include "log.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define AD5272_CMD_MASK            (0x3CU)  /**< 命令位掩码（高4位） */
#define AD5272_DATA_HIGH_MASK      (0x03U)  /**< 数据高4位掩码 */
#define AD5272_DATA_LOW_MASK       (0xFFU)  /**< 数据低8位掩码 */
#define AD5272_RDAC_DATA_MASK      (0x03FFU) /**< RDAC数据掩码（10位） */

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Exported variables -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static int ad5272_write_cmd(ad5272_dev_t *dev, uint8_t cmd, uint16_t data);
static int ad5272_read_reg(ad5272_dev_t *dev, uint8_t cmd, uint16_t param, uint16_t *result);
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
    
    /* 创建I2C客户端 */
    dev->client = i2c_new_client("ad5272", adapter_name, (uint16_t)i2c_addr, 0U);
    if (dev->client == NULL) {
        LOG_E("Failed to create I2C client for AD5272");
        return -ENODEV;
    }
    
    /* 初始化设备结构体 */
    dev->max_position = AD5272_MAX_POSITION;
    
    /* 软件复位 */
    ad5272_software_reset(dev);
    
    HAL_Delay(1000);
    
    /* 解锁 RDAC 写入 */
    ad5272_set_control_reg(dev, AD5272_CTRL_RDAC_WRITE_EN | AD5272_CTRL_50TP_PROGRAM_EN | AD5272_CTRL_RESISTOR_PERF_DIS);
    
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
 * @brief Command 0: NOP (No Operation) [cite: 129]
 * 通常用于唤醒 I2C 接口或占位
 */
int ad5272_NOP(ad5272_dev_t *dev)
{
    return ad5272_write_cmd(dev, AD5272_CMD_NOP, 0);
}

/**
 * @brief Command 1: Write RDAC [cite: 129]
 * 写入电阻值代码 (0-1023)
 * 注意: 需要先通过 Cmd 7 设置 C1=1 解锁
 */
int ad5272_set_RDAC(ad5272_dev_t *dev, uint16_t code)
{
    return ad5272_write_cmd(dev, AD5272_CMD_WRITE_RDAC, code);
}

/**
 * @brief Command 2: Read RDAC [cite: 129]
 * 读取当前 RDAC 寄存器的值
 */
int ad5272_get_RDAC(ad5272_dev_t *dev, uint16_t *code)
{
    uint16_t raw_val;
    int ret = ad5272_read_reg(dev, AD5272_CMD_READ_RDAC, 0, &raw_val);
    if (ret == 0) {
        *code = raw_val & 0x3FF; // 有效数据是低 10 位
    }
    return ret;
}

/**
 * @brief Command 3: Store Wiper to 50-TP [cite: 129]
 * 将当前 RDAC 值烧录到 50-TP 存储器中
 * 注意: 耗时约 350ms，且需先通过 Cmd 7 设置 C0=1
 */
int ad5272_store_50TP(ad5272_dev_t *dev)
{
    return ad5272_write_cmd(dev, AD5272_CMD_STORE_50TP, 0);
}

/**
 * @brief Command 4: Software Reset [cite: 129]
 * 将 RDAC 刷新为最近一次 50-TP 存储的值
 */
int ad5272_software_reset(ad5272_dev_t *dev)
{
    return ad5272_write_cmd(dev, AD5272_CMD_RESET, 0);
}

/**
 * @brief Command 5: Read 50-TP Memory [cite: 129]
 * 读取指定地址 (0x00 - 0x32) 的 50-TP 内容
 * Table 15 定义了内存映射 [cite: 179]
 */
int ad5272_read_50TP(ad5272_dev_t *dev, uint8_t mem_addr, uint16_t *val)
{
    // mem_addr 放在数据位的低 6 位 (D5-D0)
    return ad5272_read_reg(dev, AD5272_CMD_READ_50TP, (mem_addr & 0x3F), val);
}

/**
 * @brief Command 6: Read Last Memory Address [cite: 129]
 * 读取最后一次编程的 50-TP 地址，用于判断剩余编程次数
 */
int ad5272_get_last_addr(ad5272_dev_t *dev, uint8_t *addr)
{
    uint16_t raw_val;
    int ret = ad5272_read_reg(dev, AD5272_CMD_READ_LAST_ADDR, 0, &raw_val);
    if (ret == 0) {
        *addr = (uint8_t)(raw_val & 0x3F); // 地址在低 6 位
    }
    return ret;
}

/**
 * @brief Command 7: Write Control Register [cite: 129]
 * 设置控制位 (C0, C1, C2)
 * config: 使用 CTRL_* 宏进行按位或
 */
int ad5272_set_control_reg(ad5272_dev_t *dev, uint8_t config)
{
    // config 对应 Data 位的 D2(C2), D1(C1), D0(C0) [cite: 137]
    return ad5272_write_cmd(dev, AD5272_CMD_WRITE_CTRL, (config & 0x07));
}

/**
 * @brief Command 8: Read Control Register [cite: 132]
 * 读取当前控制寄存器状态 (包括 C3 成功标志位)
 */
int ad5272_get_control_reg(ad5272_dev_t *dev, uint8_t *config)
{
    uint16_t raw_val;
    int ret = ad5272_read_reg(dev, AD5272_CMD_READ_CTRL, 0, &raw_val);
    if (ret == 0) {
        *config = (uint8_t)(raw_val & 0x0F); // 控制寄存器是低 4 位 (C3-C0)
    }
    return ret;
}

/**
 * @brief Command 9: Software Shutdown [cite: 132]
 * enable = true: 进入关断模式 (Terminal A 断开)
 * enable = false: 退出关断模式
 */
int ad5272_set_shutdown(ad5272_dev_t *dev, bool enable)
{
    // D0 = 1 为 Shutdown, D0 = 0 为 Normal [cite: 132]
    return ad5272_write_cmd(dev, AD5272_CMD_SHUTDOWN, enable ? 1 : 0);
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 发送命令（内部函数）
 * @param dev 设备结构体指针
 * @param cmd 命令
 * @param data 数据值（10位）
 * @return 0成功，负数错误码
 */
static int ad5272_write_cmd(ad5272_dev_t *dev, uint8_t cmd, uint16_t data)
{
    int ret = 0;
    uint8_t buffer[2];
    uint16_t packet = ((cmd & 0x0F) << 10) | (data & 0x3FF);

    buffer[0] = (uint8_t)(packet >> 8);
    buffer[1] = (uint8_t)(packet & 0xFF);
    
    /* 发送数据 */
    ret = i2c_master_send(dev->client->adapter, dev->client->addr, dev->client->flags,
                         buffer, 2U);
    if (ret < 0) {
        LOG_E("I2C write failed: %d", ret);
        return -EIO;
    }
    
    return 0;
}


static int ad5272_read_reg(ad5272_dev_t *dev, uint8_t cmd, uint16_t param, uint16_t *result)
{
    int ret;
    uint8_t buffer[2];
    
    // 发送读取请求命令
    ret = ad5272_write_cmd(dev, cmd, param);
    if (ret != 0)
        return ret;
    
    // 读取返回的 2 字节数据
    ret = i2c_master_recv(dev->client->adapter, dev->client->addr, dev->client->flags,
                         buffer, 2U);
    if (ret != 2)
        return -EIO;

    // 组装结果 (高位在前)
    *result = ((uint16_t)buffer[0] << 8) | buffer[1];
    return 0;
}

