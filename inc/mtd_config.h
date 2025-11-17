/**
  ******************************************************************************
  * @file        : mtd_config.h
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2024-11-03
  * @brief       : MTD配置选项
  ******************************************************************************
  */
#ifndef __MTD_CONFIG_H__
#define __MTD_CONFIG_H__

#ifdef __cplusplus
 extern "C" {
#endif

/* MTD功能配置 ----------------------------------------------------------------*/

/**
 * @brief 是否支持OOB（Out-of-Band）操作
 * @note  NAND Flash通常需要此功能，NOR Flash可关闭以节省资源
 */
#define MTD_SUPPORT_OOB                 1

/**
 * @brief 是否支持分区功能
 * @note  支持父子设备的分区管理，关闭可节省RAM和ROM
 */
#define MTD_SUPPORT_PARTITION           1

/**
 * @brief 是否支持ECC统计
 * @note  支持ECC错误统计功能，用于监控Flash健康状态
 */
#define MTD_SUPPORT_ECC_STATS           1

/**
 * @brief 是否支持NAND特性（坏块管理）
 * @note  支持坏块检测和标记功能，NOR Flash不需要
 */
#define MTD_SUPPORT_NAND                1

/**
 * @brief 是否启用参数校验
 * @note  发布版本可关闭以提高性能
 */
#define MTD_ENABLE_PARAM_CHECK          1

/* 地址类型定义 --------------------------------------------------------------*/
typedef uint32_t mtd_addr_t;
#define MTD_ADDR_MAX    UINT32_MAX

#ifdef __cplusplus
}
#endif

#endif /* __MTD_CONFIG_H__ */

