/**
 ******************************************************************************
 * @file        : wdg_test.h
 * @author      : ZJY
 * @version     : V1.0
 * @date        : 2025-01-XX
 * @brief       : 看门狗驱动逻辑测试
 * @attention   : None
 ******************************************************************************
 * @history     :
 *         V1.0 : 1.添加看门狗框架与接口逻辑测试
 ******************************************************************************
 */
#ifndef __WDG_TEST_H__
#define __WDG_TEST_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/
/**
 * @brief 看门狗测试结果结构体
 */
typedef struct {
    uint32_t total_tests;   /**< 总用例数 */
    uint32_t passed_tests;  /**< 通过数 */
    uint32_t failed_tests;  /**< 失败数 */
} wdg_test_result_t;

/* Exported functions --------------------------------------------------------*/
/**
 * @brief 运行全部看门狗逻辑测试（仅框架层，使用 mock 设备，不依赖硬件）
 * @param result 输出测试结果，可为 NULL
 * @return 0 全部通过，负值表示有失败
 */
int wdg_test_run_all(wdg_test_result_t *result);

/**
 * @brief 测试 wdg_register_device 各分支
 * @return 0 通过，负值失败
 */
int wdg_test_register_device(void);

/**
 * @brief 测试 wdg_find 各分支
 * @return 0 通过，负值失败
 */
int wdg_test_find(void);

/**
 * @brief 测试 wdg_start 各分支
 * @return 0 通过，负值失败
 */
int wdg_test_start(void);

/**
 * @brief 测试 wdg_stop 各分支
 * @return 0 通过，负值失败
 */
int wdg_test_stop(void);

/**
 * @brief 测试 wdg_feed 各分支
 * @return 0 通过，负值失败
 */
int wdg_test_feed(void);

/**
 * @brief 测试 wdg_set_timeout 各分支
 * @return 0 通过，负值失败
 */
int wdg_test_set_timeout(void);

/**
 * @brief 测试 wdg_set_pretimeout 各分支
 * @return 0 通过，负值失败
 */
int wdg_test_set_pretimeout(void);

/**
 * @brief 测试 wdg_get_timeout 各分支
 * @return 0 通过，负值失败
 */
int wdg_test_get_timeout(void);

/**
 * @brief 测试 wdg_get_state 各分支
 * @return 0 通过，负值失败
 */
int wdg_test_get_state(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __WDG_TEST_H__ */
