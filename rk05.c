#include "config.h"
#include "rk05_registers.h"

#ifdef ENABLE_RK05

#include "stdio.h"
#include "conio.h"
#include "memory.h"
#include "string.h"
#include "defs.h"
#include "regmem.h"
#include "int_debug.h"
#include "pdpsim.h"
#include "rk05.h"

int rkdebug = 1;

rk05_t rk05;

#define RK05_RDY_DELAY 1000

void RK05_Reset()
{
	memset(&rk05, 0, sizeof(rk05));
}

int RK05_Service()
{
	int irq = 0;

	if (rk05.rdy_cnt)
	{
		--rk05.rdy_cnt;
		if (0 == rk05.rdy_cnt)
		{
			rk05.rkcs |= RKCS_RDY_BIT;
			irq = 1;
		}
	}

	if (irq && (rk05.rkcs & RKCS_IDE_BIT))
	{
		IRQ(0220);
	}

	return 0;
}

void UpdateRKDA(int sec)
{
	int sector = (rk05.rkda & RKDA_SC_MASK) >> RKDA_SC_SHIFT;
	int cylinder = (rk05.rkda & RKDA_CYL_MASK) >> RKDA_CYL_SHIFT;
	int surface = !!(rk05.rkda & RKDA_SUR_BIT);
	uint16 new_rkda = 0;

	sector += sec;
	if (sector >= RK05_SECTORS)
	{
		sector -= RK05_SECTORS;
		cylinder++;
		if (cylinder >= RK05_CYLINDERS)
		{
			cylinder = 0;
			surface = !surface;
		}
	}

	new_rkda = sector << RKDA_SC_SHIFT;
	new_rkda |= cylinder << RKDA_CYL_SHIFT;
	new_rkda |= surface?RKDA_SUR_BIT:0;

	rk05.rkda = new_rkda;
}

void RK05_ExecuteCommand()
{
	int command = (rk05.rkcs & RKCS_FN_MASK) >> RKCS_FN_SHIFT;
	int offset = 0;
	int sector = (rk05.rkda & RKDA_SC_MASK) >> RKDA_SC_SHIFT;
	int cylinder = (rk05.rkda & RKDA_CYL_MASK) >> RKDA_CYL_SHIFT;
	int surface = !!(rk05.rkda & RKDA_SUR_BIT);
	int read_size = 0;
	int write_size = 0;
	uint16 addr16;
	int res;

	switch (command)
	{
	case RKCS_FN_READ:
		offset = (2*cylinder+surface) *  RK05_SECTORS;
		offset += sector;
		offset *= RK05_SECTOR_SIZE_IN_WORDS * 2;

		res = fseek(rk05.f, offset, SEEK_SET);
		read_size = 2 * (65536 - rk05.rkwc);

		addr16 = rk05.rkba;

		if (rkdebug)
			printf("RK05: READ sec=%d, cyl=%d, surface=%d, offset=%d, read_size=%d, BA=%06ho\n", sector, cylinder, surface, offset, read_size, addr16);

		if (addr16 + read_size > PDP_MEMORY_SIZE)
		{
			printf("RK05 FATAL ERROR: read size bigger than memory size, dest=%d, read_size=%d\n", addr16, read_size);
		}

		if (read_size != (res = fread(&memory[addr16], 1, read_size, rk05.f)))
		{
			printf("error reading RK05 image\n");
		}
		else
		{
			rk05.rkba += read_size; //byte address
			rk05.rkwc += (read_size >> 1); //word count
		}


		/* Shift sector/cyl/surface by number of read sectors */
		UpdateRKDA((read_size+511)/512);

		//rk05.rkcs |= RKCS_RDY_BIT;
		rk05.rdy_cnt = RK05_RDY_DELAY;
		rk05.rkcs &= ~RKCS_GO_BIT;
		break;

	case RKCS_FN_SEEK:
		if (rkdebug)
			printf("RK05: SEEK sec=%d, cyl=%d, surface=%d\n", sector, cylinder, surface);

		/* SCP bit signifies that the interrupt was result of SEEK or DRIVE RESET function */
		rk05.rkcs |= RKCS_SCP_BIT;
		rk05.rdy_cnt = RK05_RDY_DELAY;
		rk05.rkcs &= ~RKCS_GO_BIT;
		break;

	case RKCS_FN_WRITE:
		if (rk05.write_protect)
		{
			rk05.rker |= RKER_WLO_BIT;

			rk05.rdy_cnt = RK05_RDY_DELAY;
			rk05.rkcs &= ~RKCS_GO_BIT;
			rk05.rkcs |= (RKCS_HE_BIT | RKCS_ERR_BIT);
			break;
		}
		offset = (2 * cylinder + surface) *  RK05_SECTORS;
		offset += sector;
		offset *= RK05_SECTOR_SIZE_IN_WORDS * 2;

		res = fseek(rk05.f, offset, SEEK_SET);
		write_size = 2 * (65536 - rk05.rkwc);

		addr16 = rk05.rkba;

		if (rkdebug)
			printf("RK05: WRITE sec=%d, cyl=%d, surface=%d, offset=%d, write_size=%d, BA=%06ho\n", sector, cylinder, surface, offset, write_size, addr16);

		if (write_size != (res = fwrite(&memory[addr16], 1, write_size, rk05.f)))
		{
			printf("error writing to RK05 image\n");
		}
		else
		{
			rk05.rkba += write_size; //byte address
			rk05.rkwc += (write_size >> 1); //word count
		}
		/* Shift sector/cyl/surface by number of read sectors */
		UpdateRKDA((write_size + 511) / 512);

		//rk05.rkcs |= RKCS_RDY_BIT;
		rk05.rdy_cnt = RK05_RDY_DELAY;
		rk05.rkcs &= ~RKCS_GO_BIT;
		break;

	case RKCS_FN_WRITE_LOCK:
		if (rkdebug)
			printf("RK05: WRITE LOCK\n");
		rk05.write_protect = 1;
		rk05.rdy_cnt = RK05_RDY_DELAY;
		rk05.rkcs &= ~RKCS_GO_BIT;
		break;

	case RKCS_FN_CONTROL_RESET:
		if (rkdebug)
			printf("RK05: CONTROL RESET\n");
		rk05.write_protect = 0;
		rk05.rker = 0; /* reset error register */
		rk05.rdy_cnt = RK05_RDY_DELAY;
		rk05.rkcs &= ~RKCS_GO_BIT;
		rk05.rkcs &= ~(RKCS_HE_BIT | RKCS_ERR_BIT);
		break;

	case RKCS_FN_DRIVE_RESET:
		rk05.rkda = 0; //move head to sec 0, cyl 0, surface 0;
		/* SCP bit signifies that the interrupt was result of SEEK or DRIVE RESET function */
		rk05.rkcs |= RKCS_SCP_BIT;
		rk05.rdy_cnt = RK05_RDY_DELAY;
		rk05.rkcs &= ~RKCS_GO_BIT;
		break;

	default:
		if (rkdebug)
			printf("RK05: unsupported command\n");

	}
}

uint16 RK05_IORead16(addr16)
{
	uint16 retVal;

	if (addr16 == RKDS_REG_ADDR)
	{
		int sector = (rk05.rkda & RKDA_SC_MASK) >> RKDA_SC_SHIFT;
		rk05.rkds = sector << RKDS_SC_SHIFT;
		rk05.rkds |= RKDS_WPS_BIT ? rk05.write_protect : 0;
		rk05.rkds |= RKDS_SC_EQ_SA_BIT;
		rk05.rkds |= RKDS_ARDY_BIT;
		rk05.rkds |= RKDS_DRY_BIT;
		rk05.rkds |= RKDS_SOK_BIT;
		rk05.rkds |= RKDS_HDEN_BIT;

		if (rkdebug)
			printf("RK05 RKDS rd= %06ho\n", rk05.rkds);

		retVal = rk05.rkds;
	}
	else if (addr16 == RKER_REG_ADDR)
	{
		if (rkdebug)
			printf("RK05 RKER rd= %06ho\n", rk05.rker);

		retVal = rk05.rker;
	}
	else if (addr16 == RKCS_REG_ADDR)
	{
		if (rkdebug)
			printf("RK05 RKCS rd= %06ho\n", rk05.rkcs);

		retVal = rk05.rkcs;
	}
	else if (addr16 == RKWC_REG_ADDR)
	{
		if (rkdebug)
			printf("RK05 RKWC rd= %06ho\n", rk05.rkwc);

		retVal = rk05.rkwc;
	}
	else if (addr16 == RKBA_REG_ADDR)
	{
		if (rkdebug)
			printf("RK05 RKBA rd= %06ho\n", rk05.rkba);

		retVal = rk05.rkba;
	}
	else if (addr16 == RKDA_REG_ADDR)
	{
		if (rkdebug)
			printf("RK05 RKDA rd= %06ho\n", rk05.rkda);

		retVal = rk05.rkda;
	}

	return retVal;
}

void RK05_IOWrite16(uint16 addr16, uint16 val16)
{
	if (addr16 == RKDS_REG_ADDR)
	{
		/* Read only register */
	}
	else if (addr16 == RKER_REG_ADDR)
	{
		/* Read only register */
	}
	else if (addr16 == RKCS_REG_ADDR)
	{
		if (rkdebug)
		{
			printf("RK05 RKCS wr = %06ho\n", val16);
		}
		rk05.rkcs = val16 & RKCS_WRITE_MASK;

		IntDebug_Set(RK05_IE, !!(val16 & RKCS_IDE_BIT));

		if ((val16 & RKCS_IDE_BIT) && rkdebug)
		{
			printf("RK05: Interrupt enabled\n");
		}

		if (val16 & RKCS_GO_BIT)
		{
			/* RDY and SCP bits is cleared when GO is issued */
			rk05.rkcs &= ~RKCS_RDY_BIT;
			rk05.rkcs &= ~RKCS_SCP_BIT;

			RK05_ExecuteCommand();
		}
	}
	else if (addr16 == RKWC_REG_ADDR)
	{
		if (rkdebug)
		{
			printf("RK05 RKWC wr = %06ho\n", val16);
		}
		rk05.rkwc = val16;
	}
	else if (addr16 == RKBA_REG_ADDR)
	{
		if (rkdebug)
		{
			printf("RK05 RKBA wr = %06ho\n", val16);
		}
		rk05.rkba = val16;
	}
	else if (addr16 == RKDA_REG_ADDR)
	{
		if (rkdebug)
		{
			printf("RK05 RKDA wr = %06ho\n", val16);
		}
		rk05.rkda = val16;
	}

}
#endif //ENABLE_RK05
