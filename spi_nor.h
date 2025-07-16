/**
  ******************************************************************************
  * @file        : xxx.h
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 20xx-xx-xx
  * @brief       : 
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.xxx
  ******************************************************************************
  */
#ifndef __SPI_NOR_H__
#define __SPI_NOR_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "sys_def.h"
#include "spi.h"
#include "bitops.h"
/* Exported define -----------------------------------------------------------*/
/* SPI Nor Flash CMD codes. */
#define SPINOR_CMD_WREN		            0x06	/* Write enable */
#define SPINOR_CMD_WRDI                 0x04    /* Write disable */
#define SPINOR_CMD_RDSR                 0x05	/* Read status register 1 */
#define SPINOR_CMD_WRSR                 0x01	/* Write status register 1 */
#define SPINOR_CMD_RDSR2                0x35	/* Read status register 2 */
#define SPINOR_CMD_WRSR2                0x31	/* Write status register 2 */
#define SPINOR_CMD_RDSR3		        0x15	/* Read status register 3 */
#define SPINOR_CMD_WRSR3		        0x11	/* Write status register 3 */
#define SPINOR_CMD_READ		            0x03	/* Read data bytes (low frequency) */
#define SPINOR_CMD_READ_FAST	        0x0b	/* Read data bytes (high frequency) */
#define SPINOR_CMD_READ_1_1_2	        0x3b	/* Read data bytes (Dual Output SPI) */
#define SPINOR_CMD_READ_1_2_2	        0xbb	/* Read data bytes (Dual I/O SPI) */
#define SPINOR_CMD_READ_1_1_4	        0x6b	/* Read data bytes (Quad Output SPI) */
#define SPINOR_CMD_READ_1_4_4	        0xeb	/* Read data bytes (Quad I/O SPI) */
#define SPINOR_CMD_READ_1_1_8	        0x8b	/* Read data bytes (Octal Output SPI) */
#define SPINOR_CMD_READ_1_8_8	        0xcb	/* Read data bytes (Octal I/O SPI) */
#define SPINOR_CMD_PP		            0x02	/* Page program (up to 256 bytes) */
#define SPINOR_CMD_PP_1_1_4	            0x32	/* Quad page program */
#define SPINOR_CMD_PP_1_4_4	            0x38	/* Quad page program */
#define SPINOR_CMD_PP_1_1_8	            0x82	/* Octal page program */
#define SPINOR_CMD_PP_1_8_8	            0xc2	/* Octal page program */
#define SPINOR_CMD_BE_4K		        0x20	/* Erase 4KiB block */
#define SPINOR_CMD_BE_4K_PMC	        0xd7	/* Erase 4KiB block on PMC chips */
#define SPINOR_CMD_BE_32K	            0x52	/* Erase 32KiB block */
#define SPINOR_CMD_BE_64K	            0xd8	/* Erase 32KiB block */
#define SPINOR_CMD_CHIP_ERASE	        0xc7	/* Erase whole flash chip */
#define SPINOR_CMD_RDID		            0x9f	/* Read JEDEC ID */
#define SPINOR_CMD_RDSFDP	            0x5a	/* Read SFDP */
#define SPINOR_CMD_RDCR		            0x35	/* Read configuration register */
#define SPINOR_CMD_SRSTEN	            0x66	/* Software Reset Enable */
#define SPINOR_CMD_SRST		            0x99	/* Software Reset */
#define SPINOR_CMD_GBULK		        0x98    /* Global Block Unlock */
#define SPINOR_CMD_POWER_DOWN           0xb9    /* Power down */
#define SPINOR_CMD_RELEASE_POWER_DOWN   0xab    /* Release power down */

/* 4-byte address opcodes */
#define SPINOR_CMD_ENTER_4B             0xb7    /* Enter 4-Byte Address Mode */
#define SPINOR_CMD_EXIT_4B              0xb7    /* Exit 4-Byte Address Mode */
#define SPINOR_CMD_READ_4B	            0x13	/* Read data bytes (low frequency) */
#define SPINOR_CMD_READ_FAST_4B	        0x0c	/* Read data bytes (high frequency) */
#define SPINOR_CMD_READ_1_1_2_4B	    0x3c	/* Read data bytes (Dual Output SPI) */
#define SPINOR_CMD_READ_1_2_2_4B	    0xbc	/* Read data bytes (Dual I/O SPI) */
#define SPINOR_CMD_READ_1_1_4_4B	    0x6c	/* Read data bytes (Quad Output SPI) */
#define SPINOR_CMD_READ_1_4_4_4B	    0xec	/* Read data bytes (Quad I/O SPI) */
#define SPINOR_CMD_READ_1_1_8_4B	    0x7c	/* Read data bytes (Octal Output SPI) */
#define SPINOR_CMD_READ_1_8_8_4B	    0xcc	/* Read data bytes (Octal I/O SPI) */
#define SPINOR_CMD_PP_4B		        0x12	/* Page program (up to 256 bytes) */
#define SPINOR_CMD_PP_1_1_4_4B	        0x34	/* Quad page program */
#define SPINOR_CMD_PP_1_4_4_4B	        0x3e	/* Quad page program */
#define SPINOR_CMD_PP_1_1_8_4B	        0x84	/* Octal page program */
#define SPINOR_CMD_PP_1_8_8_4B	        0x8e	/* Octal page program */
#define SPINOR_CMD_BE_4K_4B	            0x21	/* Erase 4KiB block */
#define SPINOR_CMD_BE_32K_4B	        0x5c	/* Erase 32KiB block */
#define SPINOR_CMD_BE_64K_4B	        0xdc	/* Erase 32KiB block */

/* Double Transfer Rate opcodes - defined in JEDEC JESD216B. */
#define SPINOR_CMD_READ_1_1_1_DTR	    0x0d
#define SPINOR_CMD_READ_1_2_2_DTR	    0xbd
#define SPINOR_CMD_READ_1_4_4_DTR	    0xed

#define SPINOR_CMD_READ_1_1_1_DTR_4B	0x0e
#define SPINOR_CMD_READ_1_2_2_DTR_4B	0xbe
#define SPINOR_CMD_READ_1_4_4_DTR_4B	0xee

/* Used for SST flashes only. */
#define SPINOR_CMD_BP		            0x02	/* Byte program */
#define SPINOR_CMD_AAI_WP	            0xad	/* Auto address increment word program */

/* Used for Macronix and Winbond flashes. */
#define SPINOR_CMD_EN4B		            0xb7	/* Enter 4-byte mode */
#define SPINOR_CMD_EX4B		            0xe9	/* Exit 4-byte mode */

/* Used for Spansion flashes only. */
#define SPINOR_CMD_BRWR		            0x17	/* Bank register write */

/* Used for Micron flashes only. */
#define SPINOR_CMD_RD_EVCR              0x65    /* Read EVCR register */
#define SPINOR_CMD_WD_EVCR              0x61    /* Write EVCR register */

/* Used for GigaDevices and Winbond flashes. */
#define SPINOR_CMD_ESECR		        0x44	/* Erase Security registers */
#define SPINOR_CMD_PSECR		        0x42	/* Program Security registers */
#define SPINOR_CMD_RSECR		        0x48	/* Read Security registers */

/* Status Register bits. */
#define SPINOR_SR1_WIP			    BIT(0)	/* Write in progress */
#define SPINOR_SR1_WEL			    BIT(1)	/* Write enable latch */
/* meaning of other SR_* bits may differ between vendors */
#define SPINOR_SR1_BP0			    BIT(2)	/* Block protect 0 */
#define SPINOR_SR1_BP1			    BIT(3)	/* Block protect 1 */
#define SPINOR_SR1_BP2			    BIT(4)	/* Block protect 2 */
#define SPINOR_SR1_BP3			    BIT(5)	/* Block protect 3 */
#define SPINOR_SR1_TB_BIT5		    BIT(5)	/* Top/Bottom protect */
#define SPINOR_SR1_BP3_BIT6		    BIT(6)	/* Block protect 3 */
#define SPINOR_SR1_TB_BIT6		    BIT(6)	/* Top/Bottom protect */
#define SPINOR_SR1_SRWD			    BIT(7)	/* SR write protect */
/* Spansion/Cypress specific status bits */
#define SPINOR_SR1_E_ERR		    BIT(5)
#define SPINOR_SR1_P_ERR		    BIT(6)

#define SPINOR_SR1_QUAD_EN_BIT6	    BIT(6)

#define SPINOR_SR_BP_SHIFT		    2

/* Enhanced Volatile Configuration Register bits */
#define EVCR_QUAD_EN_MICRON	        BIT(7)	/* Micron Quad I/O */

/* Status Register 2 bits. */
#define SPINOR_SR2_QUAD_EN_BIT1	    BIT(1)
#define SPINOR_SR2_LB1			    BIT(3)	/* Security Register Lock Bit 1 */
#define SPINOR_SR2_LB2			    BIT(4)	/* Security Register Lock Bit 2 */
#define SPINOR_SR2_LB3			    BIT(5)	/* Security Register Lock Bit 3 */
#define SPINOR_SR2_QUAD_EN_BIT7	    BIT(7)

/* Supported SPI protocols */
#define SNOR_PROTO_INST_MASK	    0x00FF0000
#define SNOR_PROTO_INST_SHIFT	    16
#define SNOR_PROTO_INST(_nbits)	    \
	((((uint32_t)(_nbits)) << SNOR_PROTO_INST_SHIFT) & SNOR_PROTO_INST_MASK)

#define SNOR_PROTO_ADDR_MASK	    0x0000FF00
#define SNOR_PROTO_ADDR_SHIFT	    8
#define SNOR_PROTO_ADDR(_nbits)	    \
	((((uint32_t)(_nbits)) << SNOR_PROTO_ADDR_SHIFT) & SNOR_PROTO_ADDR_MASK)

#define SNOR_PROTO_DATA_MASK	    0x000000FF
#define SNOR_PROTO_DATA_SHIFT	    0
#define SNOR_PROTO_DATA(_nbits)	    \
	((((uint32_t)(_nbits)) << SNOR_PROTO_DATA_SHIFT) & SNOR_PROTO_DATA_MASK)

#define SNOR_PROTO_IS_DTR	        BIT(24)	/* Double Transfer Rate */

#define SNOR_PROTO_STR(_inst_nbits, _addr_nbits, _data_nbits)	\
	(SNOR_PROTO_INST(_inst_nbits) |				                \
	 SNOR_PROTO_ADDR(_addr_nbits) |				                \
	 SNOR_PROTO_DATA(_data_nbits))

#define SNOR_PROTO_DTR(_inst_nbits, _addr_nbits, _data_nbits)	\
	(SNOR_PROTO_IS_DTR |					                    \
	 SNOR_PROTO_STR(_inst_nbits, _addr_nbits, _data_nbits))

enum spi_nor_protocol {
	SNOR_PROTO_1_1_1 = SNOR_PROTO_STR(1, 1, 1),
	SNOR_PROTO_1_1_2 = SNOR_PROTO_STR(1, 1, 2),
	SNOR_PROTO_1_1_4 = SNOR_PROTO_STR(1, 1, 4),
	SNOR_PROTO_1_1_8 = SNOR_PROTO_STR(1, 1, 8),
	SNOR_PROTO_1_2_2 = SNOR_PROTO_STR(1, 2, 2),
	SNOR_PROTO_1_4_4 = SNOR_PROTO_STR(1, 4, 4),
	SNOR_PROTO_1_8_8 = SNOR_PROTO_STR(1, 8, 8),
	SNOR_PROTO_2_2_2 = SNOR_PROTO_STR(2, 2, 2),
	SNOR_PROTO_4_4_4 = SNOR_PROTO_STR(4, 4, 4),
	SNOR_PROTO_8_8_8 = SNOR_PROTO_STR(8, 8, 8),

	SNOR_PROTO_1_1_1_DTR = SNOR_PROTO_DTR(1, 1, 1),
	SNOR_PROTO_1_2_2_DTR = SNOR_PROTO_DTR(1, 2, 2),
	SNOR_PROTO_1_4_4_DTR = SNOR_PROTO_DTR(1, 4, 4),
	SNOR_PROTO_1_8_8_DTR = SNOR_PROTO_DTR(1, 8, 8),
	SNOR_PROTO_8_8_8_DTR = SNOR_PROTO_DTR(8, 8, 8),
};

// 错误码定义
#define SPI_NOR_OK                  0
#define SPI_NOR_ERR_TIMEOUT         -1
#define SPI_NOR_ERR_WRITE           -2
#define SPI_NOR_ERR_ERASE           -3
#define SPI_NOR_ERR_NOT_ALIGNED     -4
#define SPI_NOR_ERR_INVALID_ADDR    -5
#define SPI_NOR_ERR_NOT_SUPPORTED   -6
#define SPI_NOR_ERR_BUSY            -7
/* Exported typedef ----------------------------------------------------------*/
struct spi_nor
{
    struct spi_device *spi;      // SPI设备指针
    uint32_t capacity;           // 存储器容量 (字节)
    uint16_t sector_size;        // 扇区大小 (字节)
    uint16_t page_size;          // 页大小 (字节)
    uint32_t unique_id;          // 唯一ID
    uint8_t manufacturer_id;     // 制造商ID
    uint16_t device_id;          // 设备ID
    char name[8];                // 设备名称
};
/* Exported macro ------------------------------------------------------------*/

/* Exported variable prototypes ----------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/
/* 基本电源管理与控制命令 */
int spi_nor_power_down(struct spi_nor *nor);
int spi_nor_release_power_down(struct spi_nor *nor);
int spi_nor_software_reset(struct spi_nor *nor);
int spi_nor_suspend(struct spi_nor *nor);
int spi_nor_resume(struct spi_nor *nor);

/* 状态寄存器操作 */
int spi_nor_read_sr1(struct spi_nor *nor);
int spi_nor_read_sr2(struct spi_nor *nor);
int spi_nor_read_sr3(struct spi_nor *nor);
int spi_nor_write_sr1(struct spi_nor *nor, uint8_t status);
int spi_nor_write_sr2(struct spi_nor *nor, uint8_t status);
int spi_nor_write_sr3(struct spi_nor *nor, uint8_t status);
int spi_nor_write_volatile_sr1(struct spi_nor *nor, uint8_t status);
int spi_nor_write_volatile_sr2(struct spi_nor *nor, uint8_t status);
int spi_nor_write_volatile_sr3(struct spi_nor *nor, uint8_t status);
int spi_nor_write_enable_for_volatile_sr(struct spi_nor *nor);

/* 安全寄存器操作 */
int spi_nor_read_security_register(struct spi_nor *nor, uint32_t param1, uint8_t *param2);
int spi_nor_program_security_register(struct spi_nor *nor, uint32_t param1, uint8_t *param2);
int spi_nor_erase_security_register(struct spi_nor *nor, uint32_t param);

/* 设备ID与参数读取 */
int spi_nor_read_unique_id(struct spi_nor *nor, uint8_t *unique_id);
int spi_nor_read_jedec_id(struct spi_nor *nor);
int spi_nor_read_mftr_id(struct spi_nor *nor);
int spi_nor_read_mftr_id_dual_io(struct spi_nor *nor);
int spi_nor_read_mftr_id_quad_io(struct spi_nor *nor);
int spi_nor_read_sfdp_register(struct spi_nor *nor, uint32_t read_address, uint8_t *read_buf);

/* 擦除操作 */
int spi_nor_chip_erase(struct spi_nor *nor);
int spi_nor_block_erase(struct spi_nor *nor, uint32_t addr);
int spi_nor_block_erase_4byte_address(struct spi_nor *nor, uint32_t addr);
int spi_nor_block_erase_32KB(struct spi_nor *nor, uint32_t addr);
int spi_nor_sector_erase(struct spi_nor *nor, uint32_t addr);
int spi_nor_sector_erase_4byte_address(struct spi_nor *nor, uint32_t addr);
int spi_nor_block_erase_no_rb_check(struct spi_nor *nor, uint32_t addr);

/* 编程操作 */
int spi_nor_page_program(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data);
int spi_nor_page_program_4byte_address(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data);
int spi_nor_quad_input_page_program(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data);
int spi_nor_quad_page_program_4byte_address(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data);
int spi_nor_page_program_no_rb_check(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data);
int spi_nor_enable_one_time_program(struct spi_nor *nor);

/* 标准读取操作 */
int spi_nor_read_data(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data);
int spi_nor_read_data_4byte_address(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data);
int spi_nor_fast_read(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data);
int spi_nor_fast_read_4byte_address(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data);

/* 双线读取操作 */
int spi_nor_fast_read_dual_output(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data);
int spi_nor_fast_read_dual_output_4byte_address(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data);
int spi_nor_fast_read_dual_io(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data);
int spi_nor_fast_read_dual_io_4byte_address(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data);
int spi_nor_dtr_fast_read_dual_io(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data);
int spi_nor_dtr_fast_read_dual_io_4byte_address(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data);

/* 四线读取操作 */
int spi_nor_fast_read_quad_output(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data);
int spi_nor_fast_read_quad_output_4byte_address(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data);
int spi_nor_fast_read_quad_io(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data);
int spi_nor_fast_read_quad_io_4byte_address(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data);
int spi_nor_dtr_fast_read_quad_io(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data);
int spi_nor_dtr_fast_read_quad_io_4byte_address(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data);

/* DTR读取操作 */
int spi_nor_dtr_fast_read(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data);
int spi_nor_dtr_fast_read_4byte_address(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t *data);

/* QPI模式操作 */
int spi_nor_enter_qpi_mode(struct spi_nor *nor);
int spi_nor_exit_qpi_mode(struct spi_nor *nor);
int spi_nor_read_status_register_2_qpi(struct spi_nor *nor);
int spi_nor_burst_read_with_wrap_qpi(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t dummy_clock, uint8_t *data);
int spi_nor_dtr_burst_read_with_wrap_qpi(struct spi_nor *nor, uint32_t addr, uint32_t len, uint8_t dummy_clock, uint8_t *data);

/* 扇区锁定操作 */
int spi_nor_individual_block_sector_lock(struct spi_nor *nor, uint32_t addr);
int spi_nor_individual_block_sector_unlock(struct spi_nor *nor, uint32_t addr);
int spi_nor_read_block_sector_lock(struct spi_nor *nor, uint32_t addr, uint8_t read_lock_bit);
int spi_nor_global_block_sector_lock(struct spi_nor *nor);
int spi_nor_global_block_sector_unlock(struct spi_nor *nor);

/* 地址模式控制 */
int spi_nor_enter_4byte_address_mode(struct spi_nor *nor);
int spi_nor_exit_4byte_address_mode(struct spi_nor *nor);

/* 其他配置操作 */
int spi_nor_set_burst_with_wrap(struct spi_nor *nor, uint32_t wrap_setting);
int spi_nor_set_read_parameters(struct spi_nor *nor, uint8_t read_setting);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SPI_NOR_H__ */

