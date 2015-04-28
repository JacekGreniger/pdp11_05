#include "config.h"
#ifdef ENABLE_RL02

#include "stdio.h"
#include "conio.h"
#include "memory.h"
#include "string.h"
#include "defs.h"
#include "regmem.h"
#include "rl02.h"
#include "int_debug.h"

#define RL_NUMSC        (40)                            /* sectors/surface */

#define RLDA_V_SECT     (0)                             /* sector */
#define RLDA_M_SECT     (077)
#define RLDA_V_TRACK    (6)                             /* track */
#define RLDA_M_TRACK    (01777)
#define RLDA_HD0        (0 << RLDA_V_TRACK)
#define RLDA_HD1        (1u << RLDA_V_TRACK)
#define RLDA_V_CYL      (7)                             /* cylinder */
#define RLDA_M_CYL      (0777)
#define RLDA_TRACK      (RLDA_M_TRACK << RLDA_V_TRACK)
#define RLDA_CYL        (RLDA_M_CYL << RLDA_V_CYL)
#define GET_SECT(x)     (((x) >> RLDA_V_SECT) & RLDA_M_SECT)
#define GET_CYL(x)      (((x) >> RLDA_V_CYL) & RLDA_M_CYL)
#define GET_TRACK(x)    (((x) >> RLDA_V_TRACK) & RLDA_M_TRACK)
#define GET_DA(x)       ((GET_TRACK (x) * RL_NUMSC) + GET_SECT (x))

/*
Track number contains cylinder number and head number on LSB

Track numbering:
 0 cyl0 head0
 1 cyl0 head1
 2 cyl1 head0
 etc...
*/

rl02_t rl02;
uint16 rl02_cs_reg;
uint16 rl02_ba_reg;
uint16 rl02_da_reg;
uint16 rl02_mp_reg;

int rldebug = 0;

#define RL02_IRQ_TIMEOUT 200 //200

void RL02_Init()
{
	rl02.filename[0] = 0;
	rl02.f = NULL;
	rl02.bytes_count = 0;
	rl02.status = 0;
	rl02.command = 0;
	rl02.execute_command = 0;
	rl02.ie = 0;
	rl02.raise_irq = 0;
	rl02.head = 0;
	rl02.cylinder = 0;
	rl02.sector = 0;
	rl02.mp_buffer_cnt = 0;
	rl02.csr_drdy = 1;
}


uint16 RL02_IORead16(addr16)
{
	uint16 retVal;

	if (addr16 == RL02_CS_REG_ADDR)
	{
		retVal = rl02_cs_reg;
		if (rl02.csr_drdy == 1)
		{
			retVal |= 1; //set bit 0 - DRDY
		}

		if (rldebug)
			printf("RL02 CS read= %06ho(oct) 0x%04x\n", rl02_cs_reg, rl02_cs_reg);
	}
	else if (addr16 == RL02_BA_REG_ADDR)
	{
		retVal = rl02_ba_reg;
		if (rldebug)
			printf("RL02 BA read= %06ho(oct) 0x%04x\n", rl02_ba_reg, rl02_ba_reg);

	}
	else if (addr16 == RL02_DA_REG_ADDR)
	{
		retVal = rl02_da_reg;
		if (rldebug)
			printf("RL02 DA read= %06ho(oct) 0x%04x\n", rl02_da_reg, rl02_da_reg);
	}
	else if (addr16 == RL02_MP_REG_ADDR)
	{
		retVal = rl02_mp_reg;
		if (rldebug)
			printf("RL02 MP read= %06ho(oct) 0x%04x\n", rl02_mp_reg, rl02_mp_reg);

		if (rl02.mp_buffer_cnt)
		{
			rl02_mp_reg = rl02.mp_buffer[0];
			rl02.mp_buffer[0] = rl02.mp_buffer[1];
			--rl02.mp_buffer_cnt;
		}
	}

	return retVal;
}

#define RL02_SECTOR_SIZE 256 //in bytes
#define RL02_SECTORS 40
#define RL02_CYLINDERS 512
#define RL02_HEADS 2

uint16 mp_buffer[3];
int mp_buffer_cnt = 0;

void RL02_ExecuteCommand()
{
	int offset;
	uint16 addr16;
	int res;
	int sectors;

	if (rl02.command == 6) //read command
	{
		uint16 read_size;

		if (!rl02.status)
		{
			printf("RL02 image not opened\n");
			return;
		}
		//cylinder and head set by SEEK command
		//rl02.cylinder = (rl02_da_reg >> 7) & 0x1ff;
		//rl02.head = (rl02_da_reg >> 6) & 1;
		rl02.sector = (rl02_da_reg >> 0) & 0x3f;

		offset = GET_DA(rl02_da_reg);
		offset *= RL02_SECTOR_SIZE;

		fseek(rl02.f, offset, SEEK_SET);
		read_size = 2 * (65536 - rl02_mp_reg);
		sectors = (read_size + (RL02_SECTOR_SIZE-1)) / RL02_SECTOR_SIZE;
		if (sectors + rl02.sector > RL02_SECTORS)
		{
			//data can be read up to end of current track
			printf("RL02 ERROR: curr_sector=%d, requested_sectors=%d read_size=%d\n", rl02.sector, sectors, read_size);
			read_size = RL02_SECTOR_SIZE*(RL02_SECTORS - rl02.sector);
			sectors = (read_size + (RL02_SECTOR_SIZE - 1)) / RL02_SECTOR_SIZE;
			rl02_cs_reg |= (1 << 15) | (1 << 12) | (1 << 10); //set error incomplete operation
		}
		addr16 = rl02_ba_reg;

		if (rldebug)
			printf("RL02: READ sec=%d, cyl=%d, head=%d, offset=%d, read_size=%d, BA=%06ho\n", rl02.sector, rl02.cylinder, rl02.head, offset, read_size, addr16);

		if (addr16 + read_size > PDP_MEMORY_SIZE)
		{
			printf("RL02 FATAL ERROR: read size bigger than memory size, dest=%d, read_size=%d\n", addr16, read_size);
		}

		if (read_size != (res = fread(&memory[addr16], 1, read_size, rl02.f)))
		{
			printf("error reading RL02 image\n");
		}
		else
		{
			rl02.bytes_count += read_size;
			rl02_ba_reg += read_size; //ba is byte address
			rl02_mp_reg += (read_size >> 1); //mp is word count
			rl02.sector += sectors;

			//update DA register with next sector number
			rl02_da_reg &= 0xFFC0;
			rl02_da_reg |= rl02.sector;
		}

		if (rl02.ie)
		{
			rl02.raise_irq = RL02_IRQ_TIMEOUT;
		}

	}
	else if (rl02.command == 2) //get status
	{
		if (rl02_da_reg & (1 << 3)) //RST bit
		{
			//clear errors
			rl02_cs_reg &= 0x03ff; 
		}
		//rl02_mp_reg = 0x0098;//BOOT-U error message
		rl02_mp_reg = 0x009D;
		rl02.mp_buffer_cnt = 0;
		if (rldebug)
			printf("RL02: GET STATUS\n");

		if (rl02.head) //current head
		{
			rl02_mp_reg |= (1 << 6);
		}

		if (rl02.ie)
		{
			rl02.raise_irq = RL02_IRQ_TIMEOUT;
		}
	}
	else if (rl02.command == 4) //read header
	{
		rl02_mp_reg = (rl02.cylinder << 7) | (rl02.head << 6) | (rl02.sector);
		rl02.mp_buffer[0] = 0;
		rl02.mp_buffer[1] = 0xbabe; //header crc information
		rl02.mp_buffer_cnt = 2;
		if (rldebug)
			printf("RL02: READ HEADER\n");

		if (rl02.ie)
		{
			rl02.raise_irq = RL02_IRQ_TIMEOUT;
		}
	}
	else if (rl02.command == 3) //seek
	{
		int dir = (rl02_da_reg >> 2) & 1;
		int head = (rl02_da_reg >> 4) & 1;
		int cyl_move = rl02_da_reg >> 7;

		if ((rl02_da_reg & 0x6B) != 1) //check bits 0,1,3,5,6
		{
			//bit 0 = 1, bits 1,3,5,6 = 0
			printf("RL02 SEEK ERROR: wrong DA register\n");
		}

		rl02.head = head;
		if (dir)
		{
			rl02.cylinder += cyl_move;
			if (rl02.cylinder >= RL02_CYLINDERS)
			{
				rl02.cylinder = RL02_CYLINDERS - 1;
			}
		}
		else
		{
			rl02.cylinder -= cyl_move;
			if (rl02.cylinder < 0)
			{
				rl02.cylinder = 0;
			}
		}

		if (rl02.ie)
		{
			rl02.raise_irq = RL02_IRQ_TIMEOUT;
		}
		if (rldebug)
			printf("RL02: SEEK dir=%d, cyl_move=%d, head=%d, sec=%d, cyl=%d, head=%d\n", dir, cyl_move, head, rl02.sector, rl02.cylinder, rl02.head);

	}
	else if (rl02.command == 0) //NOP
	{
		if (rl02.ie)
		{
			rl02.raise_irq = RL02_IRQ_TIMEOUT;
		}
	}
	else if (rl02.command == 5) //write command
	{
		uint16 read_size;

		if (!rl02.status)
		{
			printf("RL02 image not opened\n");
			return;
		}
		//cylinder and head set by SEEK command
		//rl02.cylinder = (rl02_da_reg >> 7) & 0x1ff;
		//rl02.head = (rl02_da_reg >> 6) & 1;
		rl02.sector = (rl02_da_reg >> 0) & 0x3f;

		offset = GET_DA(rl02_da_reg);
		offset *= RL02_SECTOR_SIZE;

		fseek(rl02.f, offset, SEEK_SET);
		read_size = 2 * (65536 - rl02_mp_reg);
		sectors = (read_size + (RL02_SECTOR_SIZE - 1)) / RL02_SECTOR_SIZE;
		if (sectors + rl02.sector > RL02_SECTORS)
		{
			//data can be read up to end of current track
			printf("RL02 ERROR: curr_sector=%d, requested_sectors=%d read_size=%d\n", rl02.sector, sectors, read_size);
			read_size = RL02_SECTOR_SIZE*(RL02_SECTORS - rl02.sector);
			sectors = (read_size + (RL02_SECTOR_SIZE - 1)) / RL02_SECTOR_SIZE;
			rl02_cs_reg |= (1 << 15) | (1 << 12) | (1 << 10); //set error incomplete operation
		}
		addr16 = rl02_ba_reg;

		if (rldebug)
			printf("RL02: READ sec=%d, cyl=%d, head=%d, offset=%d, read_size=%d, BA=%06ho\n", rl02.sector, rl02.cylinder, rl02.head, offset, read_size, addr16);

		if (addr16 + read_size > PDP_MEMORY_SIZE)
		{
			printf("RL02 FATAL ERROR: read size bigger than memory size, dest=%d, read_size=%d\n", addr16, read_size);
		}
#if 0
		if (read_size != (res = fread(&memory[addr16], 1, read_size, rl02.f)))
		{
			printf("error reading RL02 image\n");
		}
#endif
		else
		{
			rl02.bytes_count += read_size;
			rl02_ba_reg += read_size; //ba is byte address
			rl02_mp_reg += (read_size >> 1); //mp is word count
			rl02.sector += sectors;

			//update DA register with next sector number
			rl02_da_reg &= 0xFFC0;
			rl02_da_reg |= rl02.sector;
		}

		if (rl02.ie)
		{
			rl02.raise_irq = RL02_IRQ_TIMEOUT;
		}

	}

	else
	{
		printf("Unsupported RL02 command\n");
	}
}

void RL02_IOWrite16(uint16 addr16, uint16 val16)
{
	if (addr16 == RL02_CS_REG_ADDR)
	{
		int prev_ie = rl02.ie;

		if (rldebug)
		{
			printf("RL02 CS wr= %06ho(oct) 0x%04x\n", val16, val16);
			printf("RL02: IE = %d\n", !!(val16&(1 << 6)));
		}

		rl02_cs_reg = val16 & 0x3FE;
		rl02.command = (rl02_cs_reg >> 1) & 7;
		rl02.ie = !!(val16&(1 << 6));
		IntDebug_Set(RL02_IE, !!(val16&(1 << 6)));

		rl02.execute_command = !(val16 & (1 << 7));
		if (rl02.execute_command)
		{
			RL02_ExecuteCommand();
		}
		else
		{
			if (!prev_ie && rl02.ie)
			{
				/* No new command issued but IE becomes enabled */
				rl02.raise_irq = RL02_IRQ_TIMEOUT;
			}
		}
		//rl02_cs_reg |= 1 << 0; //Drive ready
		rl02_cs_reg |= 1 << 7; //Controller ready
	}
	else if (addr16 == RL02_BA_REG_ADDR)
	{
		rl02_ba_reg = val16 & 0xFFFE;
		if (rldebug)
			printf("RL02 BA wr= %06ho(oct) 0x%04x\n", val16, val16);
	}
	else if (addr16 == RL02_DA_REG_ADDR)
	{
		rl02_da_reg = val16;
		if (rldebug)
			printf("RL02 DA wr= %06ho(oct) 0x%04x\n", val16, val16);
	}
	else if (addr16 == RL02_MP_REG_ADDR)
	{
		rl02_mp_reg = val16;
		if (rldebug)
			printf("RL02 MP wr= %06ho(oct) 0x%04x\n", val16, val16);
	}
}

#endif //ENABLE_RL02
