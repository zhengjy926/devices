/**
  ******************************************************************************
  * @copyright   : Copyright To Hangzhou Dinova EP Technology Co.,Ltd
  * @file        : xxxx.c
  * @author      : ZJY
  * @version     : V1.0
  * @data        : 20xx-xx-xx
  * @brief       : 
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.xxx
  *
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "mtd.h"
#include <stddef.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Exported variables  -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
static int mtd_read_oob_std(struct mtd_info *mtd, uint32_t from,
			    struct mtd_oob_ops *ops)
{
	int ret;

	if (mtd->_read_oob)
		ret = mtd->_read_oob(mtd, from, ops);
	else
		ret = mtd->_read(mtd, from, ops->len, &ops->retlen,
				    ops->datbuf);

	return ret;
}

static int mtd_write_oob_std(struct mtd_info *mtd, uint32_t to,
                            struct mtd_oob_ops *ops)
{
	int ret;

	if (mtd->_write_oob)
		ret = mtd->_write_oob(mtd, to, ops);
	else
		ret = mtd->_write(mtd, to, ops->len, &ops->retlen, ops->datbuf);

	return ret;
}


static int mtd_check_oob_ops(struct mtd_info *mtd, uint32_t offs,
			     struct mtd_oob_ops *ops)
{
	if (!ops->datbuf)
		ops->len = 0;

	if (!ops->oobbuf)
		ops->ooblen = 0;

	if (offs + ops->len > mtd->size)
		return -1; // EINVAL,地址越界
    
    // 校验 OOB 操作的合法性
	if (ops->ooblen) {
		size_t maxooblen;
        
        //  检查 OOB 偏移是否超出每块可用 OOB 空间
		if (ops->ooboffs >= mtd_oobavail(mtd, ops))
			return -1; // EINVAL
        
		maxooblen = ((size_t)(mtd_div_by_ws(mtd->size, mtd) -
				      mtd_div_by_ws(offs, mtd)) * mtd_oobavail(mtd, ops)) 
                      - ops->ooboffs;
        
		if (ops->ooblen > maxooblen)
			return -1; // EINVAL
	}

	return 0;
}

int mtd_erase(struct mtd_info *mtd, struct erase_info *instr)
{
//	struct mtd_info *master = mtd_get_master(mtd);
//	u64 mst_ofs = mtd_get_master_ofs(mtd, 0);
	struct erase_info adjinstr;
	int ret;
    
	adjinstr = *instr;
    
	if (!mtd->erasesize || !mtd->_erase)
		return -1;  // ENOTSUPP，不支持擦除操作
    
	if (instr->addr >= mtd->size || instr->len > mtd->size - instr->addr)
		return -1;  // EINVAL，地址越界
    
	if (!(mtd->flags & MTD_WRITEABLE))
		return -1;  // EROFS，设备不可写

	if (!instr->len)
		return 0;   // 空操作

//	adjinstr.addr += mtd->offset;   ///< 将逻辑地址转换为物理地址

	ret = mtd->_erase(mtd, &adjinstr);

	return ret;
}

int mtd_read(struct mtd_info *mtd, uint32_t from, size_t len, size_t *retlen,
	         uint8_t *buf)
{
	struct mtd_oob_ops ops = {
		.len = len,
		.datbuf = buf,
	};
	int ret;

	ret = mtd_read_oob(mtd, from, &ops);
	*retlen = ops.retlen;

//	WARN_ON_ONCE(*retlen != len && mtd_is_bitflip_or_eccerr(ret));

	return ret;
}

int mtd_write(struct mtd_info *mtd, uint32_t to, size_t len, size_t *retlen,
                const uint8_t *buf)
{
	struct mtd_oob_ops ops = {
		.len = len,
		.datbuf = (uint8_t*)buf,
	};
	int ret;

	ret = mtd_write_oob(mtd, to, &ops);
	*retlen = ops.retlen;

	return ret;
}

/**
  * @brief  从 MTD 设备读取主数据（main data）和 OOB（Out-of-Band） 数据。
  * @param  from 起始地址（设备逻辑地址）
  * @param  ops 操作参数
  * @retval 
  */
int mtd_read_oob(struct mtd_info *mtd, uint32_t from, struct mtd_oob_ops *ops)
{
//	struct mtd_ecc_stats old_stats = mtd->ecc_stats; // 保存旧ECC错误统计信息
	int ret_code;

	ops->retlen = ops->oobretlen = 0;

	ret_code = mtd_check_oob_ops(mtd, from, ops);  // 参数校验
	if (ret_code)
		return ret_code;

	/* 若驱动未实现 _read_oob 方法则必须实现 _read 方法，且 ops->oobbuf 为空（仅读取主数据） */
	if (!mtd->_read_oob && (!mtd->_read || ops->oobbuf))
		return -1; // EOPNOTSUPP
    
    /* 若调用者提供了统计结构体 stats，将其清零以记录本次操作的统计信息 */
	if (ops->stats)
		memset(ops->stats, 0, sizeof(*ops->stats));

    ret_code = mtd_read_oob_std(mtd, from, ops);

//	mtd_update_ecc_stats(mtd, master, &old_stats);

	if (ret_code < 0)
		return ret_code;
    
	if (mtd->ecc_strength == 0)
		return 0;	/* device lacks ecc */
    
	if (ops->stats)
		ops->stats->max_bitflips = ret_code;
    
	return ret_code >= mtd->bitflip_threshold ? -3 : 0;
}

int mtd_write_oob(struct mtd_info *mtd, uint32_t to, struct mtd_oob_ops *ops)
{
	int ret;

	ops->retlen = ops->oobretlen = 0;

	if (!(mtd->flags & MTD_WRITEABLE))
		return -1; // EROFS

	ret = mtd_check_oob_ops(mtd, to, ops);
	if (ret)
		return ret;

	/* Check the validity of a potential fallback on mtd->_write */
	if (!mtd->_write_oob && (!mtd->_write || ops->oobbuf))
		return -1; // EOPNOTSUPP

	return mtd_write_oob_std(mtd, to, ops);
}
/* Private functions ---------------------------------------------------------*/
