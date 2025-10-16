/**
  ******************************************************************************
  * @file        : mtd_core.h
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
#ifndef __MTD_CORE_H__
#define __MTD_CORE_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "sys_def.h"
#include "my_list.h"

/* Exported define -----------------------------------------------------------*/
#define MTD_FAIL_ADDR_UNKNOWN               -1ULL

#define MTD_WRITEABLE		                0x400	/* Device is writeable */
#define MTD_BIT_WRITEABLE	                0x800	/* Single bits can be flipped */
#define MTD_NO_ERASE		                0x1000	/* No erase necessary */

/* Exported typedef ----------------------------------------------------------*/
struct mtd_info;

/**
 * MTD operation modes
 *
 * @MTD_OPS_PLACE_OOB:	OOB data are placed at the given offset (default)
 * @MTD_OPS_AUTO_OOB:	OOB data are automatically placed at the free areas
 *			which are defined by the internal ecclayout
 * @MTD_OPS_RAW:	data are transferred as-is, with no error correction;
 *			this mode implies %MTD_OPS_PLACE_OOB
 *
 * These modes can be passed to ioctl(MEMWRITE) and ioctl(MEMREAD); they are
 * also used internally. See notes on "MTD file modes" for discussion on
 * %MTD_OPS_RAW vs. %MTD_FILE_MODE_RAW.
 */
enum {
	MTD_OPS_PLACE_OOB = 0,
	MTD_OPS_AUTO_OOB = 1,
	MTD_OPS_RAW = 2,
};

/**
 * struct mtd_ecc_stats - error correction stats
 *
 * @corrected:	number of corrected bits
 * @failed:	number of uncorrectable errors
 * @badblocks:	number of bad blocks in this partition
 * @bbtblocks:	number of blocks reserved for bad block tables
 */
struct mtd_ecc_stats {
	uint32_t corrected;
	uint32_t failed;
	uint32_t badblocks;
	uint32_t bbtblocks;
};

struct mtd_req_stats {
	unsigned int uncorrectable_errors;
	unsigned int corrected_bitflips;
	unsigned int max_bitflips;
};

struct mtd_oob_ops {
	unsigned int mode;              /**< operation mode */
	size_t		len;                /**< number of data bytes to write/read */
	size_t		retlen;             /**< number of data bytes written/read */
	size_t		ooblen;             /**< number of oob bytes to write/read */
	size_t		oobretlen;          /**< number of oob bytes written/read */
	uint32_t	ooboffs;            /**< offset of oob data in the oob area (only relevant when
                                         mode = MTD_OPS_PLACE_OOB or MTD_OPS_RAW) */
	uint8_t		*datbuf;            /**< data buffer - if NULL only oob data are read/written */
	uint8_t		*oobbuf;            /**< oob data buffer */
    struct mtd_req_stats *stats;
};

/*
 * If the erase fails, fail_addr might indicate exactly which block failed. If
 * fail_addr = MTD_FAIL_ADDR_UNKNOWN, the failure was not at the device level
 * or was not specific to any particular block.
 */
struct erase_info {
	uint64_t addr;
	uint64_t len;
	uint64_t fail_addr;
};

struct mtd_erase_region_info {
	uint64_t offset;		    /* At which this region starts, from the beginning of the MTD */
	uint32_t erasesize;		    /* For this region */
	uint32_t numblocks;		    /* Number of blocks of erasesize in this region */
	unsigned long *lockmap;		/* If keeping bitmap of locks */
};

/**
 * struct mtd_part - MTD partition specific fields
 *
 * @node: list node used to add an MTD partition to the parent partition list
 * @offset: offset of the partition relatively to the parent offset
 * @size: partition size. Should be equal to mtd->size unless
 *	  MTD_SLC_ON_MLC_EMULATION is set
 * @flags: original flags (before the mtdpart logic decided to tweak them based
 *	   on flash constraints, like eraseblock/pagesize alignment)
 *
 * This struct is embedded in mtd_info and contains partition-specific
 * properties/fields.
 */
struct mtd_part {
	list_t node;
	uint64_t offset;
	uint64_t size;
	uint32_t flags;
};

struct mtd_info {
    const char *name;               /** Name of the mtd device */
    uint8_t type;
    uint32_t flags;
    int index;                      /** Number of the mtd device */
    uint64_t size;                  /** Total size of the MTD */
    uint32_t erasesize;             /** "Major" erase size for the device. */
    uint32_t writesize;             /** Minimal writable flash unit size. */
	uint32_t writebufsize;          /** Size of the write buffer used by the MTD. */
    uint32_t oobsize;               // Amount of OOB data per block (e.g. 16)
	uint32_t oobavail;              // Available OOB bytes per block
    
	/*
	 * If erasesize is a power of 2 then the shift is stored in
	 * erasesize_shift otherwise erasesize_shift is zero. Ditto writesize.
	 */
	unsigned int erasesize_shift;
	unsigned int writesize_shift;
    
	/*
	 * read ops return -EUCLEAN if max number of bitflips corrected on any
	 * one region comprising an ecc step equals or exceeds this value.
	 * Settable by driver, else defaults to ecc_strength.  User can override
	 * in sysfs.  N.B. The meaning of the -EUCLEAN return code has changed;
	 * see Documentation/ABI/testing/sysfs-class-mtd for more detail.
	 */
	unsigned int bitflip_threshold;
    
    /* max number of correctible bit errors per ecc step */
	unsigned int ecc_strength;
    
	/* Data for variable erase regions. If numeraseregions is zero,
	 * it means that the whole device has erasesize as given above.
	 */
    int numeraseregions;
	struct mtd_erase_region_info *eraseregions;
    
    int (*_erase) (struct mtd_info *mtd, struct erase_info *instr);
    int (*_read)(struct mtd_info *mtd, uint64_t from, size_t len, size_t *retlen, uint8_t *buf);
    int (*_write)(struct mtd_info *mtd, uint64_t to, size_t len, size_t *retlen, const uint8_t *buf);
	int (*_read_oob)(struct mtd_info *mtd, uint64_t from, struct mtd_oob_ops *ops);
    int (*_write_oob)(struct mtd_info *mtd, uint64_t to, struct mtd_oob_ops *ops);
    
    /* ECC status information */
	struct mtd_ecc_stats ecc_stats;
    
	/*
	 * Parent device from the MTD partition point of view.
	 *
	 * MTD masters do not have any parent, MTD partitions do. The parent
	 * MTD device can itself be a partition.
	 */
	struct mtd_info *parent;
    struct mtd_part part;
    void *priv;                     /** Private data of the mtd device */
};

/* Exported macro ------------------------------------------------------------*/


/* Exported variable prototypes ----------------------------------------------*/


/* Exported function prototypes ----------------------------------------------*/
static inline struct mtd_info *mtd_get_master(struct mtd_info *mtd)
{
	while (mtd->parent) {
        mtd = mtd->parent;
    }
	return mtd;
}

static inline uint64_t mtd_get_master_ofs(struct mtd_info *mtd, uint64_t ofs)
{
	while (mtd->parent) {
		ofs += mtd->part.offset;
		mtd = mtd->parent;
	}
	return ofs;
}

static inline uint32_t mtd_oobavail(struct mtd_info *mtd, struct mtd_oob_ops *ops)
{
	return ops->mode == MTD_OPS_AUTO_OOB ? mtd->oobavail : mtd->oobsize;
}

static inline uint32_t mtd_div_by_ws(uint64_t sz, struct mtd_info *mtd)
{
	if (mtd->writesize_shift)
		return sz >> mtd->writesize_shift;
    
	sz /= mtd->writesize;
	return sz;
}

int mtd_erase(struct mtd_info *mtd, struct erase_info *instr);
int mtd_read(struct mtd_info *mtd, uint64_t from, size_t len, size_t *retlen, uint8_t *buf);
int mtd_write(struct mtd_info *mtd, uint64_t to, size_t len, size_t *retlen, const uint8_t *buf);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MTD_CORE_H__ */

