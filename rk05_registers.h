#ifndef RK05_REGISTERS_H
#define RK05_REGISTERS_H

#define RK05_REG_FIRST_ADDR 0177400
#define RK05_REG_LAST_ADDR   0177412

/* Drive Status Register */
#define RKDS_REG_ADDR 0177400

#define RKDS_SC_MASK (7<<0) /* Sector counter */
#define RKDS_SC_SHIFT (0)
#define RKDS_SC_EQ_SA_BIT (1<<4) /* Sector counter equals sector address */
#define RKDS_WPS_BIT (1<<5) /* Write protect */
#define RKDS_ARDY_BIT (1<<6) /* Access ready */
#define RKDS_DRY_BIT (1<<7) /* Drive ready */
#define RKDS_SOK_BIT (1<<8) /* Sector counter OK */
#define RKDS_SIN_BIT (1<<9) /* Seek incomplete */
#define RKDS_DRU_BIT (1<<10) /* Drive unsafe */
#define RKDS_HDEN_BIT (1<<11) /* High density disk, 1 if RK05 */
#define RKDS_DPL_BIT (1<<12) /* Seek incomplete */
#define RKDS_SIN_BIT (1<<9) /* Drive power low */
#define RKDS_ID_BITS (1<<0) /* Sector counter */
#define RKDS_ID_MASK (7<<13)
#define RKDS_ID_SHIFT (13)

/* Error Register */
#define RKER_REG_ADDR (0177402)
#define RKER_WLO_BIT (1<<13) /* Write Lockout Violated */
#define RKER_OVR_BIT (1<<14) /* Overrun */

#define RKCS_REG_ADDR (0177404)
#define RKCS_WRITE_MASK (017577)
#define RKCS_GO_BIT (1<<0)
#define RKCS_FN_MASK (7<<1)
#define RKCS_FN_SHIFT (1)
#define RKCS_FN_CONTROL_RESET (0)
#define RKCS_FN_WRITE (1)
#define RKCS_FN_READ (2)
#define RKCS_FN_WRITE_CHECK (3)
#define RKCS_FN_SEEK (4)
#define RKCS_FN_READ_CHECK (5)
#define RKCS_FN_DRIVE_RESET (6)
#define RKCS_FN_WRITE_LOCK (7)
#define RKCS_IDE_BIT (1<<6) /* Interrupt on Done Enable */
#define RKCS_RDY_BIT (1<<7) /* Control Ready */
#define RKCS_SSE_BIT (1<<8) /* Stop on soft error */
#define RKCS_RWA_BIT (1<<9) /* Read/Write All */
#define RKCS_FMT_BIT (1<<10) /* Format */
#define RKCS_IBA_BIT (1<<11) /* Inhibit incrementing the RKBA */
#define RKCS_MAINT_BIT (1<<12) /* Maintenance mode */
#define RKCS_SCP_BIT (1<<13) /* Search complete */
#define RKCS_HE_BIT (1<<14) /* Hard error */
#define RKCS_ERR_BIT (1<<15) /* Error */

/* Word Count Register */
#define RKWC_REG_ADDR (0177406)

/* Current Bus Address Register */
#define RKBA_REG_ADDR (0177410)

/* Disk Address Register */
#define RKDA_REG_ADDR (0177412)
#define RKDA_SC_MASK (017<<0)
#define RKDA_SC_SHIFT (0)
#define RKDA_SUR_BIT (1<<4)
#define RKDA_CYL_MASK (0377<<5)
#define RKDA_CYL_SHIFT (5)
#define RKDA_ID_MASK (7<<13)
#define RKDA_ID_SHIFT (13)

#define RK05_SECTORS (12)
#define RK05_CYLINDERS (203)
#define RK05_SECTOR_SIZE_IN_WORDS (256)

#endif
