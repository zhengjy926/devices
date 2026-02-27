/**
 ******************************************************************************
 * @file        : wdg_test.c
 * @author      : ZJY
 * @version     : V1.0
 * @date        : 2025-01-XX
 * @brief       : 看门狗驱动逻辑测试（框架层 mock，不依赖硬件）
 * @attention   : None
 ******************************************************************************
 * @history     :
 *         V1.0 : 1.添加看门狗各接口逻辑分支测试
 ******************************************************************************
 */
/* Includes ------------------------------------------------------------------*/
#include "wdg_test.h"
#include "wdg.h"
#include "errno-base.h"

/* Private define ------------------------------------------------------------*/
#define WDG_TEST_ASSERT(cond)  do { if (!(cond)) { return -1; } } while (0)
#define WDG_TEST_MOCK_NAME     "wdg_test_mock"
#define WDG_TEST_MIN_TIMEOUT   (1U)
#define WDG_TEST_MAX_TIMEOUT   (10U)

/* Private variables ---------------------------------------------------------*/
/** Mock 返回值控制（用于测试 start/stop 失败分支） */
static int mock_start_ret = 0;
static int mock_stop_ret = 0;
static uint32_t mock_status_val = WDOG_HW_RUNNING;

/* Mock ops 实现 */
static int mock_start(struct wdg_device *wdg)
{
    (void)wdg;
    return mock_start_ret;
}

static int mock_stop(struct wdg_device *wdg)
{
    (void)wdg;
    return mock_stop_ret;
}

static int mock_feed(struct wdg_device *wdg)
{
    (void)wdg;
    return 0;
}

static uint32_t mock_status(struct wdg_device *wdg)
{
    (void)wdg;
    return mock_status_val;
}

static int mock_set_timeout(struct wdg_device *wdg, uint32_t timeout)
{
    wdg->timeout = timeout;
    return 0;
}

static int mock_set_pretimeout(struct wdg_device *wdg, uint32_t timeout_ms)
{
    (void)wdg;
    (void)timeout_ms;
    return 0;
}

/* 完整 ops（用于正常路径） */
static const struct wdg_ops mock_ops_full = {
    .start       = mock_start,
    .stop        = mock_stop,
    .feed        = mock_feed,
    .status      = mock_status,
    .set_timeout = mock_set_timeout,
    .set_pretimeout = mock_set_pretimeout,
};

/* 仅用于 register 测试：ops->start == NULL */
static const struct wdg_ops mock_ops_no_start = {
    .start       = NULL,
    .stop        = mock_stop,
    .feed        = mock_feed,
    .status      = mock_status,
    .set_timeout = mock_set_timeout,
    .set_pretimeout = mock_set_pretimeout,
};

/* 用于 stop 测试：ops->stop == NULL */
static const struct wdg_ops mock_ops_no_stop = {
    .start       = mock_start,
    .stop        = NULL,
    .feed        = mock_feed,
    .status      = mock_status,
    .set_timeout = mock_set_timeout,
    .set_pretimeout = mock_set_pretimeout,
};

/* 用于 feed 测试：ops->feed == NULL */
static const struct wdg_ops mock_ops_no_feed = {
    .start       = mock_start,
    .stop        = mock_stop,
    .feed        = NULL,
    .status      = mock_status,
    .set_timeout = mock_set_timeout,
    .set_pretimeout = mock_set_pretimeout,
};

/* 用于 get_state 测试：ops->status == NULL */
static const struct wdg_ops mock_ops_no_status = {
    .start       = mock_start,
    .stop        = mock_stop,
    .feed        = mock_feed,
    .status      = NULL,
    .set_timeout = mock_set_timeout,
    .set_pretimeout = mock_set_pretimeout,
};

/* 用于 set_timeout 测试：ops->set_timeout == NULL */
static const struct wdg_ops mock_ops_no_set_timeout = {
    .start       = mock_start,
    .stop        = mock_stop,
    .feed        = mock_feed,
    .status      = mock_status,
    .set_timeout = NULL,
    .set_pretimeout = mock_set_pretimeout,
};

/* 用于 set_pretimeout 测试：ops->set_pretimeout == NULL */
static const struct wdg_ops mock_ops_no_set_pretimeout = {
    .start       = mock_start,
    .stop        = mock_stop,
    .feed        = mock_feed,
    .status      = mock_status,
    .set_timeout = mock_set_timeout,
    .set_pretimeout = NULL,
};

/** 主 mock 设备（会注册到链表，供 find/start/stop/feed 等使用） */
static struct wdg_device mock_wdg = {
    .name        = WDG_TEST_MOCK_NAME,
    .ops         = &mock_ops_full,
    .timeout     = 1U,
    .min_timeout = WDG_TEST_MIN_TIMEOUT,
    .max_timeout = WDG_TEST_MAX_TIMEOUT,
    .status      = 0U,
};
/* node 未显式初始化，list_add_tail 会正确链接 */

/** 重复注册用：同名设备 */
static struct wdg_device mock_wdg_dup = {
    .name        = WDG_TEST_MOCK_NAME,
    .ops         = &mock_ops_full,
    .timeout     = 1U,
    .min_timeout = WDG_TEST_MIN_TIMEOUT,
    .max_timeout = WDG_TEST_MAX_TIMEOUT,
    .status      = 0U,
};

/** 仅用于参数校验的假设备（不注册） */
static struct wdg_device wdg_ops_null = {
    .name        = "null_ops",
    .ops         = NULL,
    .min_timeout = 1U,
    .max_timeout = 10U,
};

static struct wdg_device wdg_no_start = {
    .name        = "no_start",
    .ops         = &mock_ops_no_start,
    .min_timeout = 1U,
    .max_timeout = 10U,
};

static struct wdg_device wdg_invalid_range = {
    .name        = "inv_range",
    .ops         = &mock_ops_full,
    .min_timeout = 10U,
    .max_timeout = 5U,
};

static struct wdg_device wdg_zero_max = {
    .name        = "zero_max",
    .ops         = &mock_ops_full,
    .min_timeout = 0U,
    .max_timeout = 0U,
};

static struct wdg_device wdg_no_stop = {
    .name        = "no_stop",
    .ops         = &mock_ops_no_stop,
    .min_timeout = 1U,
    .max_timeout = 10U,
};

static struct wdg_device wdg_no_feed = {
    .name        = "no_feed",
    .ops         = &mock_ops_no_feed,
    .min_timeout = 1U,
    .max_timeout = 10U,
};

static struct wdg_device wdg_no_status = {
    .name        = "no_status",
    .ops         = &mock_ops_no_status,
    .min_timeout = 1U,
    .max_timeout = 10U,
};

static struct wdg_device wdg_no_set_timeout = {
    .name        = "no_set_timeout",
    .ops         = &mock_ops_no_set_timeout,
    .min_timeout = 1U,
    .max_timeout = 10U,
};

static struct wdg_device wdg_no_set_pretimeout = {
    .name        = "no_pretimeout",
    .ops         = &mock_ops_no_set_pretimeout,
    .min_timeout = 1U,
    .max_timeout = 10U,
};

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
int wdg_test_register_device(void)
{
    int ret;

    /* NULL -> -EINVAL */
    ret = wdg_register_device(NULL);
    WDG_TEST_ASSERT(ret == -EINVAL);

    /* ops == NULL -> -EINVAL */
    ret = wdg_register_device(&wdg_ops_null);
    WDG_TEST_ASSERT(ret == -EINVAL);

    /* ops->start == NULL -> -EINVAL */
    ret = wdg_register_device(&wdg_no_start);
    WDG_TEST_ASSERT(ret == -EINVAL);

    /* min_timeout > max_timeout -> -EINVAL */
    ret = wdg_register_device(&wdg_invalid_range);
    WDG_TEST_ASSERT(ret == -EINVAL);

    /* max_timeout == 0 -> -EINVAL */
    ret = wdg_register_device(&wdg_zero_max);
    WDG_TEST_ASSERT(ret == -EINVAL);

    /* 正常注册主 mock */
    ret = wdg_register_device(&mock_wdg);
    WDG_TEST_ASSERT(ret == 0);

    /* 重复同名 -> -EEXIST */
    ret = wdg_register_device(&mock_wdg_dup);
    WDG_TEST_ASSERT(ret == -EEXIST);

    return 0;
}

int wdg_test_find(void)
{
    struct wdg_device *p;

    /* name == NULL -> NULL */
    p = wdg_find(NULL);
    WDG_TEST_ASSERT(p == NULL);

    /* 不存在的名字 -> NULL */
    p = wdg_find("nonexistent_wdg_name_xyz");
    WDG_TEST_ASSERT(p == NULL);

    /* 存在的名字 -> 返回对应设备 */
    p = wdg_find(WDG_TEST_MOCK_NAME);
    WDG_TEST_ASSERT(p != NULL);
    WDG_TEST_ASSERT(p == &mock_wdg);

    return 0;
}

int wdg_test_start(void)
{
    int ret;
    struct wdg_device *wdg;

    wdg = wdg_find(WDG_TEST_MOCK_NAME);
    WDG_TEST_ASSERT(wdg != NULL);

    /* NULL -> -EINVAL */
    ret = wdg_start(NULL);
    WDG_TEST_ASSERT(ret == -EINVAL);

    /* ops == NULL -> -ENOTSUPP */
    ret = wdg_start(&wdg_ops_null);
    WDG_TEST_ASSERT(ret == -ENOTSUPP);

    /* 已运行：不再次调用 start，直接返回 0 */
    wdg->status = WDOG_HW_RUNNING;
    ret = wdg_start(wdg);
    WDG_TEST_ASSERT(ret == 0);
    wdg->status = 0U;

    /* start 返回 0 -> 成功并置位 WDOG_HW_RUNNING */
    mock_start_ret = 0;
    ret = wdg_start(wdg);
    WDG_TEST_ASSERT(ret == 0);
    WDG_TEST_ASSERT((wdg->status & WDOG_HW_RUNNING) != 0U);

    /* start 返回失败 -> 返回错误，不置位 */
    wdg->status = 0U;
    mock_start_ret = -EIO;
    ret = wdg_start(wdg);
    WDG_TEST_ASSERT(ret == -EIO);
    WDG_TEST_ASSERT((wdg->status & WDOG_HW_RUNNING) == 0U);
    mock_start_ret = 0;

    return 0;
}

int wdg_test_stop(void)
{
    int ret;
    struct wdg_device *wdg;

    wdg = wdg_find(WDG_TEST_MOCK_NAME);
    WDG_TEST_ASSERT(wdg != NULL);

    /* NULL -> -EINVAL */
    ret = wdg_stop(NULL);
    WDG_TEST_ASSERT(ret == -EINVAL);

    /* ops->stop == NULL -> -ENOTSUPP */
    ret = wdg_stop(&wdg_no_stop);
    WDG_TEST_ASSERT(ret == -ENOTSUPP);

    /* 未运行：不调用 stop，直接返回 0 */
    wdg->status = 0U;
    ret = wdg_stop(wdg);
    WDG_TEST_ASSERT(ret == 0);

    /* 已运行且 stop 返回 0 -> 成功并清除 WDOG_HW_RUNNING */
    wdg->status = WDOG_HW_RUNNING;
    mock_stop_ret = 0;
    ret = wdg_stop(wdg);
    WDG_TEST_ASSERT(ret == 0);
    WDG_TEST_ASSERT((wdg->status & WDOG_HW_RUNNING) == 0U);

    /* 已运行但 stop 返回失败 -> 返回错误，不清除位 */
    wdg->status = WDOG_HW_RUNNING;
    mock_stop_ret = -EIO;
    ret = wdg_stop(wdg);
    WDG_TEST_ASSERT(ret == -EIO);
    WDG_TEST_ASSERT((wdg->status & WDOG_HW_RUNNING) != 0U);
    mock_stop_ret = 0;
    wdg->status = 0U;

    return 0;
}

int wdg_test_feed(void)
{
    int ret;
    struct wdg_device *wdg;

    wdg = wdg_find(WDG_TEST_MOCK_NAME);
    WDG_TEST_ASSERT(wdg != NULL);

    /* NULL -> -EINVAL */
    ret = wdg_feed(NULL);
    WDG_TEST_ASSERT(ret == -EINVAL);

    /* ops->feed == NULL -> -ENOTSUPP */
    ret = wdg_feed(&wdg_no_feed);
    WDG_TEST_ASSERT(ret == -ENOTSUPP);

    /* 正常 -> 0 */
    ret = wdg_feed(wdg);
    WDG_TEST_ASSERT(ret == 0);

    return 0;
}

int wdg_test_set_timeout(void)
{
    int ret;
    struct wdg_device *wdg;

    wdg = wdg_find(WDG_TEST_MOCK_NAME);
    WDG_TEST_ASSERT(wdg != NULL);

    /* NULL -> -EINVAL */
    ret = wdg_set_timeout(NULL, 5U);
    WDG_TEST_ASSERT(ret == -EINVAL);

    /* ops->set_timeout == NULL -> -ENOTSUPP */
    ret = wdg_set_timeout(&wdg_no_set_timeout, 5U);
    WDG_TEST_ASSERT(ret == -ENOTSUPP);

    /* timeout < min_timeout -> -EINVAL */
    ret = wdg_set_timeout(wdg, 0U);
    WDG_TEST_ASSERT(ret == -EINVAL);

    /* timeout > max_timeout -> -EINVAL */
    ret = wdg_set_timeout(wdg, 100U);
    WDG_TEST_ASSERT(ret == -EINVAL);

    /* 正常：在 [min, max] 内 -> 0，且 mock 会更新 wdg->timeout */
    ret = wdg_set_timeout(wdg, 5U);
    WDG_TEST_ASSERT(ret == 0);
    WDG_TEST_ASSERT(wdg->timeout == 5U);

    return 0;
}

int wdg_test_set_pretimeout(void)
{
    int ret;
    struct wdg_device *wdg;

    wdg = wdg_find(WDG_TEST_MOCK_NAME);
    WDG_TEST_ASSERT(wdg != NULL);

    /* NULL -> -EINVAL */
    ret = wdg_set_pretimeout(NULL, 100U);
    WDG_TEST_ASSERT(ret == -EINVAL);

    /* ops->set_pretimeout == NULL -> -ENOTSUPP */
    ret = wdg_set_pretimeout(&wdg_no_set_pretimeout, 100U);
    WDG_TEST_ASSERT(ret == -ENOTSUPP);

    /* 正常 -> 0 */
    ret = wdg_set_pretimeout(wdg, 100U);
    WDG_TEST_ASSERT(ret == 0);

    return 0;
}

int wdg_test_get_timeout(void)
{
    int ret;
    uint32_t val;
    struct wdg_device *wdg;

    wdg = wdg_find(WDG_TEST_MOCK_NAME);
    WDG_TEST_ASSERT(wdg != NULL);
    wdg->timeout = 5U;

    /* NULL wdg -> -EINVAL */
    ret = wdg_get_timeout(NULL, &val);
    WDG_TEST_ASSERT(ret == -EINVAL);

    /* NULL timeout 指针 -> -EINVAL */
    ret = wdg_get_timeout(wdg, NULL);
    WDG_TEST_ASSERT(ret == -EINVAL);

    /* 正常 -> 0 且 *timeout == wdg->timeout */
    ret = wdg_get_timeout(wdg, &val);
    WDG_TEST_ASSERT(ret == 0);
    WDG_TEST_ASSERT(val == 5U);

    return 0;
}

int wdg_test_get_state(void)
{
    int ret;
    uint32_t state;
    struct wdg_device *wdg;

    wdg = wdg_find(WDG_TEST_MOCK_NAME);
    WDG_TEST_ASSERT(wdg != NULL);

    /* NULL wdg -> -EINVAL */
    ret = wdg_get_state(NULL, &state);
    WDG_TEST_ASSERT(ret == -EINVAL);

    /* NULL state 指针 -> -EINVAL */
    ret = wdg_get_state(wdg, NULL);
    WDG_TEST_ASSERT(ret == -EINVAL);

    /* ops->status == NULL -> -ENOTSUPP */
    ret = wdg_get_state(&wdg_no_status, &state);
    WDG_TEST_ASSERT(ret == -ENOTSUPP);

    /* 正常 -> 0 且 *state == ops->status(wdg) */
    mock_status_val = WDOG_HW_RUNNING;
    ret = wdg_get_state(wdg, &state);
    WDG_TEST_ASSERT(ret == 0);
    WDG_TEST_ASSERT(state == WDOG_HW_RUNNING);

    mock_status_val = 0U;
    ret = wdg_get_state(wdg, &state);
    WDG_TEST_ASSERT(ret == 0);
    WDG_TEST_ASSERT(state == 0U);
    mock_status_val = WDOG_HW_RUNNING;

    return 0;
}

int wdg_test_run_all(wdg_test_result_t *result)
{
    int ret;
    uint32_t passed = 0U;
    uint32_t failed = 0U;
    const uint32_t total = 9U;

    if (result != NULL) {
        result->total_tests = total;
        result->passed_tests = 0U;
        result->failed_tests = 0U;
    }

    /* 1. register_device（内部会注册 mock_wdg，后续用例依赖） */
    ret = wdg_test_register_device();
    if (ret == 0) {
        passed++;
    } else {
        failed++;
    }

    /* 2. find */
    ret = wdg_test_find();
    if (ret == 0) {
        passed++;
    } else {
        failed++;
    }

    /* 3. start */
    ret = wdg_test_start();
    if (ret == 0) {
        passed++;
    } else {
        failed++;
    }

    /* 4. stop */
    ret = wdg_test_stop();
    if (ret == 0) {
        passed++;
    } else {
        failed++;
    }

    /* 5. feed */
    ret = wdg_test_feed();
    if (ret == 0) {
        passed++;
    } else {
        failed++;
    }

    /* 6. set_timeout */
    ret = wdg_test_set_timeout();
    if (ret == 0) {
        passed++;
    } else {
        failed++;
    }

    /* 7. set_pretimeout */
    ret = wdg_test_set_pretimeout();
    if (ret == 0) {
        passed++;
    } else {
        failed++;
    }

    /* 8. get_timeout */
    ret = wdg_test_get_timeout();
    if (ret == 0) {
        passed++;
    } else {
        failed++;
    }

    /* 9. get_state */
    ret = wdg_test_get_state();
    if (ret == 0) {
        passed++;
    } else {
        failed++;
    }

    if (result != NULL) {
        result->passed_tests = passed;
        result->failed_tests = failed;
    }

    return (failed > 0U) ? -1 : 0;
}
