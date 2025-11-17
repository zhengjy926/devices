/**
  ******************************************************************************
  * @copyright   : Copyright To Hangzhou Dinova EP Technology Co.,Ltd
  * @file        : mtd_core.c
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2024-11-03
  * @brief       : MTD抽象层核心实现
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 整合精简版和完整版功能，支持宏配置
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "mtd.h"
#include "errno-base.h"
#include <string.h>

#define  LOG_TAG             "mtd_core"
#define  LOG_LVL             4
#include "log.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Exported variables  -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

#if MTD_SUPPORT_OOB
/**
  * @brief  检查OOB操作的参数合法性
  * @param  mtd MTD设备信息
  * @param  offs 起始偏移地址
  * @param  ops OOB操作参数
  * @retval 0=成功, 负数=错误码
  */
static int mtd_check_oob_ops(struct mtd_info *mtd, mtd_addr_t offs, struct mtd_oob_ops *ops)
{
    if (!ops->datbuf)
        ops->len = 0;

    if (!ops->oobbuf)
        ops->ooblen = 0;

    if (offs >= mtd->size || ops->len > mtd->size - offs) {
        LOG_E("mtd_check_oob_ops: data read/write out of bounds.");
        return -EINVAL;
    }

    if (ops->ooblen) {
        size_t maxooblen;

        if (ops->ooboffs >= mtd_oobavail(mtd, ops))
            return -EINVAL;

        maxooblen = ((size_t)(mtd_div_by_ws(mtd->size, mtd) - mtd_div_by_ws(offs, mtd))
                              * mtd_oobavail(mtd, ops)) - ops->ooboffs;

        if (ops->ooblen > maxooblen)
            return -EINVAL;
    }

    return 0;
}

/**
  * @brief  标准OOB读操作
  * @param  mtd MTD设备信息
  * @param  from 起始地址
  * @param  ops OOB操作参数
  * @retval 0=成功, 负数=错误码
  */
static int mtd_read_oob_std(struct mtd_info *mtd, mtd_addr_t from, struct mtd_oob_ops *ops)
{
    int ret;

#if MTD_SUPPORT_PARTITION
    struct mtd_info *master = mtd_get_master(mtd);
    from = mtd_get_master_ofs(mtd, from);
    
    if (master->_read_oob)
        ret = master->_read_oob(master, from, ops);
    else
        ret = master->_read(master, from, ops->len, &ops->retlen, ops->datbuf);
#else
    if (mtd->_read_oob)
        ret = mtd->_read_oob(mtd, from, ops);
    else
        ret = mtd->_read(mtd, from, ops->len, &ops->retlen, ops->datbuf);
#endif

    return ret;
}

/**
  * @brief  标准OOB写操作
  * @param  mtd MTD设备信息
  * @param  to 目标地址
  * @param  ops OOB操作参数
  * @retval 0=成功, 负数=错误码
  */
static int mtd_write_oob_std(struct mtd_info *mtd, mtd_addr_t to, struct mtd_oob_ops *ops)
{
    int ret;

#if MTD_SUPPORT_PARTITION
    struct mtd_info *master = mtd_get_master(mtd);
    to = mtd_get_master_ofs(mtd, to);
    
    if (master->_write_oob)
        ret = master->_write_oob(master, to, ops);
    else
        ret = master->_write(master, to, ops->len, &ops->retlen, ops->datbuf);
#else
    if (mtd->_write_oob)
        ret = mtd->_write_oob(mtd, to, ops);
    else
        ret = mtd->_write(mtd, to, ops->len, &ops->retlen, ops->datbuf);
#endif

    return ret;
}
#endif /* MTD_SUPPORT_OOB */

#if MTD_SUPPORT_ECC_STATS && MTD_SUPPORT_PARTITION
/**
  * @brief  更新ECC统计信息
  * @param  mtd MTD设备信息
  * @param  master 主设备信息
  * @param  old_stats 旧的ECC统计信息
  * @retval None
  */
static void mtd_update_ecc_stats(struct mtd_info *mtd, struct mtd_info *master,
                                 const struct mtd_ecc_stats *old_stats)
{
    struct mtd_ecc_stats diff;

    if (master == mtd)
        return;

    diff = master->ecc_stats;
    diff.failed -= old_stats->failed;
    diff.corrected -= old_stats->corrected;

    while (mtd->parent) {
        mtd->ecc_stats.failed += diff.failed;
        mtd->ecc_stats.corrected += diff.corrected;
        mtd = mtd->parent;
    }
}
#endif

/**
  * @brief  擦除MTD设备
  * @param  mtd MTD设备信息
  * @param  instr 擦除信息
  * @retval 0=成功, 负数=错误码
  */
int mtd_erase(struct mtd_info *mtd, struct erase_info *instr)
{
    if (!mtd || !instr) {
        LOG_E("mtd_erase: invalid argument (mtd or instr is NULL).");
        return -EINVAL;
    }

    int ret;
    struct erase_info adjinstr;

    instr->fail_addr = MTD_FAIL_ADDR_UNKNOWN;
    adjinstr = *instr;

#if MTD_SUPPORT_PARTITION
    struct mtd_info *master = mtd_get_master(mtd);
    mtd_addr_t mst_ofs = mtd_get_master_ofs(mtd, 0);
    
    if (!mtd->erasesize || !master->_erase)
        return -ENOTSUPP;
#else
    if (!mtd->erasesize || !mtd->_erase)
        return -ENOTSUPP;
#endif

    if (instr->addr >= mtd->size || instr->len > mtd->size - instr->addr)
        return -EINVAL;

    if (!(mtd->flags & MTD_WRITEABLE))
        return -EROFS;

    if (!instr->len)
        return 0;

#if MTD_SUPPORT_PARTITION
    adjinstr.addr += mst_ofs;
    ret = master->_erase(master, &adjinstr);
    
    if (adjinstr.fail_addr != MTD_FAIL_ADDR_UNKNOWN) {
        instr->fail_addr = adjinstr.fail_addr - mst_ofs;
    }
#else
    ret = mtd->_erase(mtd, &adjinstr);
#endif

    return ret;
}

/**
  * @brief  从MTD设备读取数据
  * @param  mtd MTD设备信息
  * @param  from 起始地址
  * @param  len 读取长度
  * @param  retlen 实际读取长度
  * @param  buf 数据缓冲区
  * @retval 0=成功, 负数=错误码
  */
int mtd_read(struct mtd_info *mtd, mtd_addr_t from, size_t len, size_t *retlen, uint8_t *buf)
{
    if (!mtd || !retlen || !buf) {
        LOG_E("mtd_read: invalid argument.");
        return -EINVAL;
    }

    int ret;
    *retlen = 0;

#if MTD_SUPPORT_OOB
    struct mtd_oob_ops ops = {
        .len = len,
        .datbuf = buf,
    };

    ret = mtd_read_oob(mtd, from, &ops);
    *retlen = ops.retlen;

    if (ret == 0 && *retlen != len) {
        LOG_W("mtd_read: short read, expected %zu bytes, got %zu bytes.", len, *retlen);
    }
#else
#if MTD_SUPPORT_PARTITION
    struct mtd_info *master = mtd_get_master(mtd);
    from = mtd_get_master_ofs(mtd, from);
    ret = master->_read(master, from, len, retlen, buf);
#else
    ret = mtd->_read(mtd, from, len, retlen, buf);
#endif
#endif

    return ret;
}

/**
  * @brief  向MTD设备写入数据
  * @param  mtd MTD设备信息
  * @param  to 目标地址
  * @param  len 写入长度
  * @param  retlen 实际写入长度
  * @param  buf 数据缓冲区
  * @retval 0=成功, 负数=错误码
  */
int mtd_write(struct mtd_info *mtd, mtd_addr_t to, size_t len, size_t *retlen, const uint8_t *buf)
{
    if (!mtd || !retlen || !buf) {
        LOG_E("mtd_write: invalid argument.");
        return -EINVAL;
    }

    int ret;
    *retlen = 0;

    if (!(mtd->flags & MTD_WRITEABLE))
        return -EROFS;

#if MTD_SUPPORT_OOB
    struct mtd_oob_ops ops = {
        .len = len,
        .datbuf = (uint8_t *)buf,
    };

    ret = mtd_write_oob(mtd, to, &ops);
    if (ret == 0)
        *retlen = ops.retlen;
#else
#if MTD_SUPPORT_PARTITION
    struct mtd_info *master = mtd_get_master(mtd);
    to = mtd_get_master_ofs(mtd, to);
    ret = master->_write(master, to, len, retlen, buf);
#else
    ret = mtd->_write(mtd, to, len, retlen, buf);
#endif
#endif

    return ret;
}

#if MTD_SUPPORT_OOB
/**
  * @brief  从MTD设备读取数据和OOB
  * @param  mtd MTD设备信息
  * @param  from 起始地址
  * @param  ops OOB操作参数
  * @retval 0=成功, 负数=错误码, 正数=最大bit翻转数
  */
int mtd_read_oob(struct mtd_info *mtd, mtd_addr_t from, struct mtd_oob_ops *ops)
{
    if (!mtd || !ops) {
        LOG_E("mtd_read_oob: invalid argument.");
        return -EINVAL;
    }

#if MTD_SUPPORT_ECC_STATS && MTD_SUPPORT_PARTITION
    struct mtd_info *master = mtd_get_master(mtd);
    struct mtd_ecc_stats old_stats = master->ecc_stats;
#endif

    int ret_code;

    ops->retlen = ops->oobretlen = 0;

    ret_code = mtd_check_oob_ops(mtd, from, ops);
    if (ret_code)
        return ret_code;

#if MTD_SUPPORT_PARTITION
    struct mtd_info *master_dev = mtd_get_master(mtd);
    if (!master_dev->_read_oob && (!master_dev->_read || ops->oobbuf))
        return -ENOTSUPP;
#else
    if (!mtd->_read_oob && (!mtd->_read || ops->oobbuf))
        return -ENOTSUPP;
#endif

#if MTD_SUPPORT_ECC_STATS
    if (ops->stats)
        memset(ops->stats, 0, sizeof(*ops->stats));
#endif

    ret_code = mtd_read_oob_std(mtd, from, ops);

#if MTD_SUPPORT_ECC_STATS && MTD_SUPPORT_PARTITION
    mtd_update_ecc_stats(mtd, master, &old_stats);
#endif

    if (ret_code < 0)
        return ret_code;

#if MTD_SUPPORT_ECC_STATS
    if (mtd->ecc_strength == 0)
        return 0;

    if (ops->stats)
        ops->stats->max_bitflips = ret_code;

    return ((uint32_t)ret_code >= mtd->bitflip_threshold) ? -EUCLEAN : 0;
#else
    return 0;
#endif
}

/**
  * @brief  向MTD设备写入数据和OOB
  * @param  mtd MTD设备信息
  * @param  to 目标地址
  * @param  ops OOB操作参数
  * @retval 0=成功, 负数=错误码
  */
int mtd_write_oob(struct mtd_info *mtd, mtd_addr_t to, struct mtd_oob_ops *ops)
{
    if (!mtd || !ops) {
        LOG_E("mtd_write_oob: invalid argument.");
        return -EINVAL;
    }

    int ret;

    ops->retlen = ops->oobretlen = 0;

    if (!(mtd->flags & MTD_WRITEABLE))
        return -EROFS;

    ret = mtd_check_oob_ops(mtd, to, ops);
    if (ret)
        return ret;

#if MTD_SUPPORT_PARTITION
    struct mtd_info *master = mtd_get_master(mtd);
    if (!master->_write_oob && (!master->_write || ops->oobbuf))
        return -ENOTSUPP;
#else
    if (!mtd->_write_oob && (!mtd->_write || ops->oobbuf))
        return -ENOTSUPP;
#endif

    return mtd_write_oob_std(mtd, to, ops);
}
#endif /* MTD_SUPPORT_OOB */

/* Private functions ---------------------------------------------------------*/
