/**
  ******************************************************************************
  * @file        : mtd.h
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
#ifndef __MTD_H__
#define __MTD_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include "mtd_config.h"

#if MTD_SUPPORT_PARTITION
#include "../../utilities/my_list.h"
#endif

/* Exported define -----------------------------------------------------------*/
#define MTD_FAIL_ADDR_UNKNOWN               0xFFFFFFFFUL

#define MTD_WRITEABLE		                0x400	/* Device is writeable */
#define MTD_BIT_WRITEABLE	                0x800	/* Single bits can be flipped */
#define MTD_NO_ERASE		                0x1000	/* No erase necessary */

/* Exported typedef ----------------------------------------------------------*/
struct mtd_info;

#if MTD_SUPPORT_OOB
enum {
    MTD_OPS_PLACE_OOB = 0,
    MTD_OPS_AUTO_OOB = 1,
    MTD_OPS_RAW = 2,
};
#endif

#if MTD_SUPPORT_ECC_STATS
struct mtd_ecc_stats {
    uint32_t corrected;
    uint32_t failed;
    uint32_t badblocks;
    uint32_t bbtblocks;
};

struct mtd_req_stats {
    uint32_t uncorrectable_errors;
    uint32_t corrected_bitflips;
    uint32_t max_bitflips;
};
#endif

#if MTD_SUPPORT_OOB
struct mtd_oob_ops {
    uint32_t mode;
    size_t len;
    size_t retlen;
    size_t ooblen;
    size_t oobretlen;
    uint32_t ooboffs;
    uint8_t *datbuf;
    uint8_t *oobbuf;
#if MTD_SUPPORT_ECC_STATS
    struct mtd_req_stats *stats;
#endif
};
#endif

struct erase_info {
    mtd_addr_t addr;
    mtd_addr_t len;
    mtd_addr_t fail_addr;
};

#if MTD_SUPPORT_PARTITION
struct mtd_part {
    list_t node;
    mtd_addr_t offset;
    mtd_addr_t size;
    uint32_t flags;
};
#endif

typedef struct mtd_info {
    const char *name;
    uint8_t type;
    uint32_t flags;
    
    mtd_addr_t size;
    uint32_t erasesize;
    uint32_t writesize;
    uint8_t writesize_shift;
    
#if MTD_SUPPORT_OOB
    uint32_t oobsize;
    uint32_t oobavail;
#endif

#if MTD_SUPPORT_ECC_STATS
    uint32_t ecc_strength;
    uint32_t bitflip_threshold;
    struct mtd_ecc_stats ecc_stats;
#endif

    int (*_read)(struct mtd_info *mtd, mtd_addr_t from, size_t len, size_t *retlen, uint8_t *buf);
    int (*_write)(struct mtd_info *mtd, mtd_addr_t to, size_t len, size_t *retlen, const uint8_t *buf);
    int (*_erase)(struct mtd_info *mtd, struct erase_info *instr);

#if MTD_SUPPORT_OOB
    int (*_read_oob)(struct mtd_info *mtd, mtd_addr_t from, struct mtd_oob_ops *ops);
    int (*_write_oob)(struct mtd_info *mtd, mtd_addr_t to, struct mtd_oob_ops *ops);
#endif

#if MTD_SUPPORT_NAND
    int (*block_isbad)(struct mtd_info *mtd, mtd_addr_t offs);
    int (*block_markbad)(struct mtd_info *mtd, mtd_addr_t offs);
#endif

#if MTD_SUPPORT_PARTITION
    struct mtd_info *parent;
    struct mtd_part part;
#endif

    void *priv;
} mtd_info_t;

/* Exported macro ------------------------------------------------------------*/


/* Exported variable prototypes ----------------------------------------------*/


/* Exported function prototypes ----------------------------------------------*/

#if MTD_SUPPORT_PARTITION
static inline struct mtd_info *mtd_get_master(struct mtd_info *mtd)
{
    while (mtd->parent) {
        mtd = mtd->parent;
    }
    return mtd;
}

static inline mtd_addr_t mtd_get_master_ofs(struct mtd_info *mtd, mtd_addr_t ofs)
{
    while (mtd->parent) {
        ofs += mtd->part.offset;
        mtd = mtd->parent;
    }
    return ofs;
}
#endif

#if MTD_SUPPORT_OOB
static inline uint32_t mtd_oobavail(struct mtd_info *mtd, struct mtd_oob_ops *ops)
{
    return ops->mode == MTD_OPS_AUTO_OOB ? mtd->oobavail : mtd->oobsize;
}

static inline uint32_t mtd_div_by_ws(mtd_addr_t sz, struct mtd_info *mtd)
{
    if (mtd->writesize_shift)
        return sz >> mtd->writesize_shift;
    return sz / mtd->writesize;
}
#endif

int mtd_erase(struct mtd_info *mtd, struct erase_info *instr);
int mtd_read(struct mtd_info *mtd, mtd_addr_t from, size_t len, size_t *retlen, uint8_t *buf);
int mtd_write(struct mtd_info *mtd, mtd_addr_t to, size_t len, size_t *retlen, const uint8_t *buf);

#if MTD_SUPPORT_OOB
int mtd_read_oob(struct mtd_info *mtd, mtd_addr_t from, struct mtd_oob_ops *ops);
int mtd_write_oob(struct mtd_info *mtd, mtd_addr_t to, struct mtd_oob_ops *ops);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MTD_H__ */

