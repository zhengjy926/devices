/**
  ******************************************************************************
  * @file        : mtd_core.c
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2024-09-26
  * @brief       : xxx
  * @attention   : xxx
  ******************************************************************************
  * @history     :
  *         V1.0 : 
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "mtd_core.h"
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
static int mtd_check_oob_ops(struct mtd_info *mtd, uint64_t offs, struct mtd_oob_ops *ops)
{
	/*
	 * Some users are setting ->datbuf or ->oobbuf to NULL, but are leaving
	 * ->len or ->ooblen uninitialized. Force ->len and ->ooblen to 0 in
	 *  this case.
	 */
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

static int mtd_read_oob_std(struct mtd_info *mtd, uint64_t from, struct mtd_oob_ops *ops)
{
	int ret;

	if (mtd->_read_oob)
		ret = mtd->_read_oob(mtd, from, ops);
	else
		ret = mtd->_read(mtd, from, ops->len, &ops->retlen, ops->datbuf);

	return ret;
}

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

int mtd_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	if (!mtd || !instr) {
		LOG_E("mtd_erase: invalid argument (mtd or instr is NULL).");
		return -EINVAL;
	}
    
	struct mtd_info *master = mtd_get_master(mtd);
	uint64_t mst_ofs = mtd_get_master_ofs(mtd, 0);
	struct erase_info adjinstr;
	int ret;

	instr->fail_addr = MTD_FAIL_ADDR_UNKNOWN;
	adjinstr = *instr;

	if (!mtd->erasesize || !master->_erase)
		return -ENOTSUPP;

	if (instr->addr >= mtd->size || instr->len > mtd->size - instr->addr)
		return -EINVAL;
	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;

	if (!instr->len)
		return 0;

	adjinstr.addr += mst_ofs;

	ret = master->_erase(master, &adjinstr);

	if (adjinstr.fail_addr != MTD_FAIL_ADDR_UNKNOWN) {
		instr->fail_addr = adjinstr.fail_addr - mst_ofs;
	}

	return ret;
}

int mtd_read_oob(struct mtd_info *mtd, uint64_t from, struct mtd_oob_ops *ops)
{
	struct mtd_info *master = mtd_get_master(mtd);
	struct mtd_ecc_stats old_stats = master->ecc_stats;
	int ret_code;

	ops->retlen = ops->oobretlen = 0;

	ret_code = mtd_check_oob_ops(mtd, from, ops);
	if (ret_code)
		return ret_code;

	/* Check the validity of a potential fallback on mtd->_read */
	if (!master->_read_oob && (!master->_read || ops->oobbuf))
		return -ENOTSUPP;

	if (ops->stats)
		memset(ops->stats, 0, sizeof(*ops->stats));

    ret_code = mtd_read_oob_std(mtd, from, ops);

	mtd_update_ecc_stats(mtd, master, &old_stats);

	/*
	 * In cases where ops->datbuf != NULL, mtd->_read_oob() has semantics
	 * similar to mtd->_read(), returning a non-negative integer
	 * representing max bitflips. In other cases, mtd->_read_oob() may
	 * return -EUCLEAN. In all cases, perform similar logic to mtd_read().
	 */
	if (ret_code < 0)
		return ret_code;
	if (mtd->ecc_strength == 0)
		return 0;	/* device lacks ecc */
	if (ops->stats)
		ops->stats->max_bitflips = ret_code;
	return ret_code >= mtd->bitflip_threshold ? -EIO : 0;
}

int mtd_read(struct mtd_info *mtd, uint64_t from, size_t len, size_t *retlen, uint8_t *buf)
{
	struct mtd_oob_ops ops = {
		.len = len,
		.datbuf = buf,
	};
	int ret;

	ret = mtd_read_oob(mtd, from, &ops);
	*retlen = ops.retlen;
    
    if (ret == 0 && *retlen != len) {
        LOG_W("mtd_read: short read, expected %zu bytes, got %zu bytes.", len, *retlen);
    }
    
    return ret;
}

static int mtd_write_oob_std(struct mtd_info *mtd, uint64_t to, struct mtd_oob_ops *ops)
{
	struct mtd_info *master = mtd_get_master(mtd);
	int ret;

	to = mtd_get_master_ofs(mtd, to);
	if (master->_write_oob)
		ret = master->_write_oob(master, to, ops);
	else
		ret = master->_write(master, to, ops->len, &ops->retlen,
				     ops->datbuf);

	return ret;
}

int mtd_write_oob(struct mtd_info *mtd, uint64_t to, struct mtd_oob_ops *ops)
{
	struct mtd_info *master = mtd_get_master(mtd);
	int ret;

	ops->retlen = ops->oobretlen = 0;

	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;

	ret = mtd_check_oob_ops(mtd, to, ops);
	if (ret)
		return ret;

	/* Check the validity of a potential fallback on mtd->_write */
	if (!master->_write_oob && (!master->_write || ops->oobbuf))
		return -ENOTSUPP;

	return mtd_write_oob_std(mtd, to, ops);
}

int mtd_write(struct mtd_info *mtd, uint64_t to, size_t len, size_t *retlen, const uint8_t *buf)
{
    int ret = 0;
    
    if (!retlen) {
        LOG_E("mtd_write: retlen pointer is NULL");
        return -EINVAL;
    }
    *retlen = 0;
    
	struct mtd_oob_ops ops = {
		.len = len,
		.datbuf = (uint8_t *)buf,
	};

	ret = mtd_write_oob(mtd, to, &ops);
    if (ret == 0)
        *retlen = ops.retlen;
    
    return ret;
}

/* Private functions ---------------------------------------------------------*/

