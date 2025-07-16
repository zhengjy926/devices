/**
  ******************************************************************************
  * @file        : sfdp.c
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2025-06-03
  * @brief       : 
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.xxx
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "sfdp.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define SFDP_PARAM_HEADER_ID(p)	(((p)->id_msb << 8) | (p)->id_lsb)
#define SFDP_PARAM_HEADER_PTP(p) \
	(((p)->parameter_table_pointer[2] << 16) | \
	 ((p)->parameter_table_pointer[1] <<  8) | \
	 ((p)->parameter_table_pointer[0] <<  0))
#define SFDP_PARAM_HEADER_PARAM_LEN(p) ((p)->length * 4)

#define SFDP_BFPT_ID		0xff00	/* Basic Flash Parameter Table */
#define SFDP_SECTOR_MAP_ID	0xff81	/* Sector Map Table */
#define SFDP_4BAIT_ID		0xff84  /* 4-byte Address Instruction Table */
#define SFDP_PROFILE1_ID	0xff05	/* xSPI Profile 1.0 table. */
#define SFDP_SCCR_MAP_ID	0xff87	/*
                                     * Status, Control and Configuration
                                     * Register Map.
                                     */
#define SFDP_SCCR_MAP_MC_ID	0xff88	/*
                                     * Status, Control and Configuration
                                     * Register Map Offsets for Multi-Chip
                                     * SPI Memory Devices.
                                     */

#define SFDP_SIGNATURE		0x50444653U
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Exported variables  -------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

