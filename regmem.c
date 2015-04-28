#include "stdio.h"
#include "conio.h"
#include "memory.h"
#include "string.h"
#include "defs.h"
#include "regmem.h"
#include "iodev.h"
#include "disasm.h"

uint8 memory[PDP_MEMORY_SIZE];
uint16 R[8];
uint16 * const pPC = &R[7];
uint16 * const pSP = &R[6];

int busError = 0;

uint16 MemoryRead16(uint16 addr16)
{
	uint16 retVal;

	if (addr16 < 0xE000)
	{
		retVal = memory[addr16 + 1] << 8 | memory[addr16];
	}
	else
	{
		retVal = IORead16(addr16);
	}

	return retVal;
}

uint8 MemoryRead8(uint16 addr16)
{
	uint8 retVal;

	if (addr16 < 0xE000)
	{
		retVal = memory[addr16];
	}
	else
	{
		retVal = IORead8(addr16);
	}

	return retVal;
}

void MemoryWrite16(uint16 addr16, uint16 val16)
{
	if (addr16 < 0xE000)
	{
		memory[addr16] = val16 & 0xff;
		memory[addr16 + 1] = val16 >> 8;
	}
	else
	{
		IOWrite16(addr16, val16);
	}
}


void MemoryWrite8(uint16 addr16, uint8 val8)
{
	if (addr16 < 0xE000)
	{
		memory[addr16] = val8;
	}
	else
	{
		IOWrite8(addr16, val8);
	}
}


void PutTapeBootstrap()
{
	/*
	printf("Paper Basic bootstrap\n");
	printf(" 037744: 016701  MOV 37776, R1\n");
	printf(" 037746: 000026\n");
	printf(" 037750: 012702  MOV #352, R2\n");
	printf(" 037752: 000352\n");
	printf(" 037754: 005211  INC(R1)\n");
	printf(" 037756: 105711  TSTB(R1)\n");
	printf(" 037760: 100376  BPL 37756\n");
	printf(" 037762: 116162  MOVB 2(R1), 37400(R2)\n");
	printf(" 037764: 000002\n");
	printf(" 037766: 037400\n");
	printf(" 037770: 005267  INC 37752\n");
	printf(" 037772: 177756\n");
	printf(" 037774: 000765  BR 37750\n");
	printf(" 037776: 177550\n");
	*/
	MemoryWrite16(037744, 0016701);
	MemoryWrite16(037746, 0000026);
	MemoryWrite16(037750, 0012702);
	MemoryWrite16(037752, 0000352);
	MemoryWrite16(037754, 0005211);
	MemoryWrite16(037756, 0105711);
	MemoryWrite16(037760, 0100376);
	MemoryWrite16(037762, 0116162);
	MemoryWrite16(037764, 0000002);
	MemoryWrite16(037766, 0037400);
	MemoryWrite16(037770, 0005267);
	MemoryWrite16(037772, 0177756);
	MemoryWrite16(037774, 0000765);
	MemoryWrite16(037776, 0177550);

	*pPC = 037744;
}

/* Reset memory content */
void MemoryReset()
{
	memset(memory, 0, PDP_MEMORY_SIZE);
	PSW = 0;
	memset(&R[0], 0, sizeof(R));
}

void PrintRegisters()
{
	char mnemonic[64];
	char par1[64];
	char par2[64];

	printf("R0=%06ho ", R[0]);
	printf("R1=%06ho ", R[1]);
	printf("R2=%06ho ", R[2]);
	printf("R3=%06ho ", R[3]);
	printf("R4=%06ho ", R[4]);
	printf("R5=%06ho\n", R[5]);
	printf("SP=%06ho ", R[6]);
	printf("N=%d ", !!((PSW) & 8));
	printf("Z=%d ", !!((PSW) & 4));
	printf("V=%d ", !!((PSW) & 2));
	printf("C=%d ", !!((PSW) & 1));
	printf("PC=%06ho [%06ho]", R[7], MemoryRead16(R[7]));
	if (Disassemble(R[7], mnemonic, par1, par2)>0)
	{
		printf("  %s %s%c %s\n", mnemonic, par1, par2[0] ? ',' : ' ', par2);
	}
}
