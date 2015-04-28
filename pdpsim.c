#include "stdio.h"
#include "memory.h"
#include "string.h"
#include "defs.h"
#include "regmem.h"
#include "pdpsim.h"


void Trap10()
{
	R[6] -= 2;
	MemoryWrite16(R[6], PSW);
	R[6] -= 2;
	MemoryWrite16(R[6], *pPC);
	*pPC = MemoryRead16(010);
	PSW = (uint8)MemoryRead16(012);
}

/* Return 1 if trap occures */
int CheckBusErrorAndTrap4()
{
	if (busError)
	{
		//TRAP 4
		R[6] -= 2;
		MemoryWrite16(R[6], PSW);
		R[6] -= 2;
		MemoryWrite16(R[6], *pPC);
		*pPC = MemoryRead16(04);
		PSW = (uint8)MemoryRead16(06);
		busError = 0;
		return 1;
	}

	return 0;
}

/* If wrback_addr is !=0 then Decode_PutValue cannot be used */
uint16 Decode_GetValue16(uint16 mode, uint16 reg, char * par, uint16 *wrback_addr)
{
	uint16 retVal;
	uint16 x;
	uint16 addr;
	*wrback_addr = 0;

	switch (mode)
	{
	case 0:
		// Register	OPR Rn	The operand is in Rn
		retVal = R[reg];
		sprintf(par, "R%d", reg);
		break;

	case 1:
		// Register deferred	OPR (Rn)	Rn contains the address of the operand
		retVal = MemoryRead16(R[reg]);
		*wrback_addr = R[reg];
		sprintf(par, "(R%d)", reg);
		break;

	case 2:
		// Autoincrement	OPR (Rn)+	Rn contains the address of the operand, then increment Rn
		retVal = MemoryRead16(R[reg]);
		*wrback_addr = R[reg];
		R[reg] += 2;
		if (reg == 7) //immediate
		{
			sprintf(par, "#%ho", retVal);
		}
		else
		{
			sprintf(par, "(R%d)+", reg);
		}
		break;

	case 3:
		// Autoincrement deferred	OPR @(Rn)+	Rn contains the address of the address, then increment Rn by 2
		addr = MemoryRead16(R[reg]);
		retVal = MemoryRead16(addr);
		*wrback_addr = addr;
		R[reg] += 2;
		if (reg == 7) //absolute direct
		{
			sprintf(par, "@#%ho", addr);
		}
		else
		{
			sprintf(par, "@(R%d)+", reg);
		}
		break;

	case 4:
		// Autodecrement	OPR -(Rn)	Decrement Rn, then use it as the address
		R[reg] -= 2;
		retVal = MemoryRead16(R[reg]);
		*wrback_addr = R[reg];
		sprintf(par, "-(R%d)", reg);
		break;

	case 5:
		// Autodecrement deferred	OPR @-(Rn)	Decrement Rn by 2, then use it as the address of the address
		R[reg] -= 2;
		addr = MemoryRead16(R[reg]);
		retVal = MemoryRead16(addr);
		*wrback_addr = addr;
		sprintf(par, "@-(R%d)", reg);
		break;

	case 6:
		// Index	OPR X(Rn)	Rn+X is the address of the operand
		x = MemoryRead16(*pPC);
		*pPC += 2;
		retVal = MemoryRead16(x + R[reg]);
		*wrback_addr = x + R[reg];
		if (reg == 7) //relative direct
		{
			sprintf(par, "%ho", x + R[reg]);
		}
		else
		{
			sprintf(par, "%ho(R%d)", x, reg);
		}
		break;

	case 7:
		// Index deferred	OPR @X(Rn)	Rn+X is the address of the address
		x = MemoryRead16(*pPC);
		*pPC += 2;
		addr = MemoryRead16(x + R[reg]);
		retVal = MemoryRead16(addr);
		*wrback_addr = addr;
		if (reg == 7) //relative indirect
		{
			sprintf(par, "@%ho", x + R[reg]);
		}
		else
		{
			sprintf(par, "@%ho(R%d)", reg);
		}
		break;
	}

	return retVal;
}

uint8 Decode_GetValue8(uint16 mode, uint16 reg, char * par, uint16 *wrback_addr)
{
	uint8 retVal;
	uint16 x;
	uint16 addr;

	*wrback_addr = 0;
	switch (mode)
	{
	case 0:
		// Register	OPR Rn	The operand is in Rn
		retVal = (uint8)R[reg];
		sprintf(par, "R%d", reg);
		break;

	case 1:
		// Register deferred	OPR (Rn)	Rn contains the address of the operand
		retVal = MemoryRead8(R[reg]);
		*wrback_addr = R[reg];
		sprintf(par, "(R%d)", reg);
		break;

	case 2:
		// Autoincrement	OPR (Rn)+	Rn contains the address of the operand, then increment Rn
		retVal = MemoryRead8(R[reg]);
		*wrback_addr = R[reg];
		if (reg == 7) //immediate
		{
			sprintf(par, "#%ho", retVal);
			R[reg] += 2;
		}
		else
		{
			if (reg == 6)
			{
				R[reg] += 2;
			}
			else
			{
				R[reg] += 1;
			}
			sprintf(par, "(R%d)+", reg);
		}
		break;

	case 3:
		// Autoincrement deferred	OPR @(Rn)+	Rn contains the address of the address, then increment Rn by 2
		addr = MemoryRead16(R[reg]);
		retVal = MemoryRead8(addr);
		*wrback_addr = addr;
		R[reg] += 2; //always by 2
		if (reg == 7) //absolute direct
		{
			sprintf(par, "@#%ho", addr);
		}
		else
		{
			sprintf(par, "@(R%d)+", reg);
		}
		break;

	case 4:
		// Autodecrement	OPR -(Rn)	Decrement Rn, then use it as the address
		if ((reg == 6) || (reg == 7))
		{
			R[reg] -= 2;
		}
		else
		{
			R[reg] -= 1;
		}
		retVal = MemoryRead8(R[reg]);
		*wrback_addr = R[reg];
		sprintf(par, "-(R%d)", reg);
		break;

	case 5:
		// Autodecrement deferred	OPR @-(Rn)	Decrement Rn by 2, then use it as the address of the address
		R[reg] -= 2; //always by 2
		addr = MemoryRead16(R[reg]);
		retVal = MemoryRead8(addr);
		*wrback_addr = addr;
		sprintf(par, "@-(R%d)", reg);
		break;

	case 6:
		// Index	OPR X(Rn)	Rn+X is the address of the operand
		x = MemoryRead16(*pPC);
		*pPC += 2;
		retVal = MemoryRead8(x + R[reg]);
		*wrback_addr = x + R[reg];
		if (reg == 7) //relative direct
		{
			sprintf(par, "%ho", x + R[reg]);
		}
		else
		{
			sprintf(par, "%ho(R%d)", x, reg);
		}
		break;

	case 7:
		// Index deferred	OPR @X(Rn)	Rn+X is the address of the address
		x = MemoryRead16(*pPC);
		*pPC += 2;
		addr = MemoryRead16(x + R[reg]);
		retVal = MemoryRead8(addr);
		*wrback_addr = addr;
		if (reg == 7) //relative indirect
		{
			sprintf(par, "@%ho", x + R[reg]);
		}
		else
		{
			sprintf(par, "@%ho(R%d)", reg);
		}
		break;
	}

	return retVal;
}

void Decode_PutValue16(uint16 mode, uint16 reg, uint16 dstValue, char * par)
{
	uint16 x;
	uint16 addr;

	switch (mode)
	{
	case 0:
		// Register	OPR Rn	The operand is in Rn
		R[reg] = dstValue;
		sprintf(par, "R%d", reg);
		break;

	case 1:
		// Register deferred	OPR (Rn)	Rn contains the address of the operand
		MemoryWrite16(R[reg], dstValue);
		sprintf(par, "(R%d)", reg);
		break;

	case 2:
		// Autoincrement	OPR (Rn)+	Rn contains the address of the operand, then increment Rn
		MemoryWrite16(R[reg], dstValue);
		if (reg == 7) //immediate
		{
			sprintf(par, "#%ho", R[reg]); //???
		}
		else
		{
			sprintf(par, "(R%d)+", reg);
		}
		R[reg] += 2;
		break;

	case 3:
		// Autoincrement deferred	OPR @(Rn)+	Rn contains the address of the address, then increment Rn by 2
		addr = MemoryRead16(R[reg]);
		MemoryWrite16(addr, dstValue);
		R[reg] += 2;
		if (reg == 7) //absolute direct
		{
			sprintf(par, "@#%ho", addr);
		}
		else
		{
			sprintf(par, "@(R%d)+", reg);
		}
		break;

	case 4:
		// Autodecrement	OPR -(Rn)	Decrement Rn, then use it as the address
		R[reg] -= 2;
		MemoryWrite16(R[reg], dstValue);
		sprintf(par, "-(R%d)", reg);
		break;

	case 5:
		// Autodecrement deferred	OPR @-(Rn)	Decrement Rn by 2, then use it as the address of the address
		R[reg] -= 2;
		addr = MemoryRead16(R[reg]);
		MemoryWrite16(addr, dstValue);
		sprintf(par, "@-(R%d)", reg);
		break;

	case 6:
		// Index	OPR X(Rn)	Rn+X is the address of the operand
		x = MemoryRead16(*pPC);
		*pPC += 2;
		MemoryWrite16(x + R[reg], dstValue);
		if (reg == 7) //relative direct
		{
			sprintf(par, "%ho", x + R[reg]);
		}
		else
		{
			sprintf(par, "%ho(R%d)", x, reg);
		}
		break;

	case 7:
		// Index deferred	OPR @X(Rn)	Rn+X is the address of the address
		x = MemoryRead16(*pPC);
		*pPC += 2;
		addr = MemoryRead16(x + R[reg]);
		MemoryWrite16(addr, dstValue);
		if (reg == 7) //relative indirect
		{
			sprintf(par, "@%ho", x + R[reg]);
		}
		else
		{
			sprintf(par, "@%ho(R%d)", x, reg);
		}
		break;
	}

}

void Decode_PutValue8(uint16 mode, uint16 reg, uint8 dstValue, char * par)
{
	uint16 x;
	uint16 addr;

	switch (mode)
	{
	case 0:
		// Register	OPR Rn	The operand is in Rn
		R[reg] &= 0xff00;
		R[reg] |= dstValue;
		sprintf(par, "R%d", reg);
		break;

	case 1:
		// Register deferred	OPR (Rn)	Rn contains the address of the operand
		MemoryWrite8(R[reg], dstValue);
		sprintf(par, "(R%d)", reg);
		break;

	case 2:
		// Autoincrement	OPR (Rn)+	Rn contains the address of the operand, then increment Rn
		MemoryWrite8(R[reg], dstValue);
		if (reg == 7) //immediate
		{
			sprintf(par, "#%ho", R[reg]); //???
			R[reg] += 2;
		}
		else
		{
			if (reg == 6)
			{
				R[reg] += 2;
			}
			else
			{
				R[reg] += 1;
			}
			sprintf(par, "(R%d)+", reg);
		}
		break;

	case 3:
		// Autoincrement deferred	OPR @(Rn)+	Rn contains the address of the address, then increment Rn by 2
		addr = MemoryRead16(R[reg]);
		MemoryWrite8(addr, dstValue);
		R[reg] += 2; //always by 2
		if (reg == 7) //absolute direct
		{
			sprintf(par, "@#%ho", addr);
		}
		else
		{
			sprintf(par, "@(R%d)+", reg);
		}
		break;

	case 4:
		// Autodecrement	OPR -(Rn)	Decrement Rn, then use it as the address
		if (reg == 6)
		{
			R[reg] -= 2;
		}
		else
		{
			R[reg] -= 1;
		}
		MemoryWrite8(R[reg], dstValue);
		sprintf(par, "-(R%d)", reg);
		break;

	case 5:
		// Autodecrement deferred	OPR @-(Rn)	Decrement Rn by 2, then use it as the address of the address
		R[reg] -= 2; //always by 2
		addr = MemoryRead16(R[reg]);
		MemoryWrite8(addr, dstValue);
		sprintf(par, "@-(R%d)", reg);
		break;

	case 6:
		// Index	OPR X(Rn)	Rn+X is the address of the operand
		x = MemoryRead16(*pPC);
		*pPC += 2;
		MemoryWrite8(x + R[reg], dstValue);
		if (reg == 7) //relative direct
		{
			sprintf(par, "%ho", x + R[reg]);
		}
		else
		{
			sprintf(par, "%ho(R%d)", x, reg);
		}
		break;

	case 7:
		// Index deferred	OPR @X(Rn)	Rn+X is the address of the address
		x = MemoryRead16(*pPC);
		*pPC += 2;
		addr = MemoryRead16(x + R[reg]);
		MemoryWrite8(addr, dstValue);
		if (reg == 7) //relative indirect
		{
			sprintf(par, "@%ho", x + R[reg]);
		}
		else
		{
			sprintf(par, "@%ho(R%d)", x, reg);
		}
		break;
	}
}

void SingleOp_PutBack16(uint16 mode, uint16 reg, uint16 dstValue, uint16 wrback_addr)
{
	if (mode == 0)
	{
		R[reg] = dstValue;
	}
	else
	{
		MemoryWrite16(wrback_addr, dstValue);
	}
}

void SingleOp_PutBack8(uint16 mode, uint16 reg, uint8 dstValue, uint16 wrback_addr)
{
	if (mode == 0)
	{
		R[reg] &= 0xff00;
		R[reg] |= dstValue;
	}
	else
	{
		MemoryWrite8(wrback_addr, dstValue);
	}
}

void FlagC(int value)
{
	if (value)
	{
		PSW |= PSW_C_SETMASK;
	}
	else
	{
		PSW &= PSW_C_CLEARMASK;
	}
}

void FlagN(int value)
{
	if (value)
	{
		PSW |= PSW_N_SETMASK;
	}
	else
	{
		PSW &= PSW_N_CLEARMASK;
	}
}


void FlagZ(int value)
{
	if (value)
	{
		PSW |= PSW_Z_SETMASK;
	}
	else
	{
		PSW &= PSW_Z_CLEARMASK;
	}
}


void FlagV(int value)
{
	if (value)
	{
		PSW |= PSW_V_SETMASK;
	}
	else
	{
		PSW &= PSW_V_CLEARMASK;
	}
}


int DecodeDoubleOperandInst(uint16 currWord, char * mnemonic, char *par1, char *par2)
{
	uint16 opcode;
	uint16 opcode_srcMode;
	uint16 opcode_srcReg;
	uint16 opcode_dstMode;
	uint16 opcode_dstReg;
	uint16 src16;
	uint16 dst16;
	uint8 src8;
	uint8 dst8;
	uint16 res16;
	uint32 res32;
	uint8 res8;
	uint16 wrback_addr;

	/*
	Double-operand instructions

	15 14  12 11 9 8    6 5	 3 2         0
	B  Opcode Mode Source Mode Destination
	*/
	opcode = currWord >> 12;
	opcode_srcMode = currWord >> 9 & 0x07;
	opcode_srcReg = currWord >> 6 & 0x07;
	opcode_dstMode = currWord >> 3 & 0x07;
	opcode_dstReg = currWord >> 0 & 0x07;

	switch (opcode)
	{
	case 001: //	MOV	Move: dest = src
		strcpy(mnemonic, "MOV");
		src16 = Decode_GetValue16(opcode_srcMode, opcode_srcReg, par1, &wrback_addr);
		dst16 = src16;
		Decode_PutValue16(opcode_dstMode, opcode_dstReg, dst16, par2);
		FlagN(src16 & 0x8000);
		FlagZ(src16 == 0);
		FlagV(0);
		break;
	case 011: //	MOVB
		strcpy(mnemonic, "MOVB");
		src8 = Decode_GetValue8(opcode_srcMode, opcode_srcReg, par1, &wrback_addr);
		if (0 == opcode_dstMode) //MOVB to register so extend sign
		{
			dst16 = src8;
			dst16 |= (src8 & 0x80) ? 0xff00 : 0;
			Decode_PutValue16(opcode_dstMode, opcode_dstReg, dst16, par2);
		}
		else
		{
			dst8 = src8;
			Decode_PutValue8(opcode_dstMode, opcode_dstReg, dst8, par2);
		}
		FlagN(src8 & 0x80);
		FlagZ(src8 == 0);
		FlagV(0);
		break;
	case 002: //	CMP	Compare: compute src - dest, set flags only
		strcpy(mnemonic, "CMP");
		src16 = Decode_GetValue16(opcode_srcMode, opcode_srcReg, par1, &wrback_addr);
		dst16 = Decode_GetValue16(opcode_dstMode, opcode_dstReg, par2, &wrback_addr);
		res16 = src16 - dst16;
		FlagN(res16 & 0x8000);
		FlagZ(res16 == 0);
		if ((!(src16 & 0x8000) && (dst16 & 0x8000) && (res16 & 0x8000)) || //+--
			((src16 & 0x8000) && !(dst16 & 0x8000) && !(res16 & 0x8000)))    //-++
		{
			FlagV(1);
		}
		else
		{
			FlagV(0);
		}

		dst16 = ~dst16;
		res32 = src16 + dst16 + 1;
		FlagC(res32 < 65536);
		break;
	case 012: //	CMPB
		strcpy(mnemonic, "CMPB");
		src8 = Decode_GetValue8(opcode_srcMode, opcode_srcReg, par1, &wrback_addr);
		dst8 = Decode_GetValue8(opcode_dstMode, opcode_dstReg, par2, &wrback_addr);
		res8 = src8 - dst8;
		FlagN(res8 & 0x80);
		FlagZ(res8 == 0);
		if ((!(src8 & 0x80) && (dst8 & 0x80) && (res8 & 0x80)) || //+--
			((src8 & 0x80) && !(dst8 & 0x80) && !(res8 & 0x80)))    //-++
		{
			FlagV(1);
		}
		else
		{
			FlagV(0);
		}

		dst8 = ~dst8;
		res16 = src8 + dst8 + 1;
		FlagC(res16 < 256);
		break;
	case 003: //	BIT	Bit test: compute dest & src, set flags only
		strcpy(mnemonic, "BIT");
		src16 = Decode_GetValue16(opcode_srcMode, opcode_srcReg, par1, &wrback_addr);
		dst16 = Decode_GetValue16(opcode_dstMode, opcode_dstReg, par2, &wrback_addr);
		res16 = src16 & dst16;
		FlagN(res16 & 0x8000);
		FlagZ(res16 == 0);
		FlagV(0);
		break;
	case 013: //	BITB
		strcpy(mnemonic, "BITB");
		src8 = Decode_GetValue8(opcode_srcMode, opcode_srcReg, par1, &wrback_addr);
		dst8 = Decode_GetValue8(opcode_dstMode, opcode_dstReg, par2, &wrback_addr);
		res8 = src8 & dst8;
		FlagN(res8 & 0x80);
		FlagZ(res8 == 0);
		FlagV(0);
		break;
	case 004: //	BIC	Bit clear: dest &= ~src
		strcpy(mnemonic, "BIC");
		src16 = Decode_GetValue16(opcode_srcMode, opcode_srcReg, par1, &wrback_addr);
		dst16 = Decode_GetValue16(opcode_dstMode, opcode_dstReg, par2, &wrback_addr);
		res16 = ~src16 & dst16;
		SingleOp_PutBack16(opcode_dstMode, opcode_dstReg, res16, wrback_addr);
		FlagN(res16 & 0x8000);
		FlagZ(res16 == 0);
		FlagV(0);
		break;
	case 014: //	BICB
		strcpy(mnemonic, "BICB");
		src8 = Decode_GetValue8(opcode_srcMode, opcode_srcReg, par1, &wrback_addr);
		dst8 = Decode_GetValue8(opcode_dstMode, opcode_dstReg, par2, &wrback_addr);
		res8 = ~src8 & dst8;
		SingleOp_PutBack8(opcode_dstMode, opcode_dstReg, res8, wrback_addr);
		FlagN(res8 & 0x80);
		FlagZ(res8 == 0);
		FlagV(0);
		break;
	case 005: //	BIS	Bit set, a.k.a. logical OR: dest |= src
		strcpy(mnemonic, "BIS");
		src16 = Decode_GetValue16(opcode_srcMode, opcode_srcReg, par1, &wrback_addr);
		dst16 = Decode_GetValue16(opcode_dstMode, opcode_dstReg, par2, &wrback_addr);
		res16 = src16 | dst16;
		SingleOp_PutBack16(opcode_dstMode, opcode_dstReg, res16, wrback_addr);
		FlagN(res16 & 0x8000);
		FlagZ(res16 == 0);
		FlagV(0);
		break;
	case 015: //	BISB
		strcpy(mnemonic, "BISB");
		src8 = Decode_GetValue8(opcode_srcMode, opcode_srcReg, par1, &wrback_addr);
		dst8 = Decode_GetValue8(opcode_dstMode, opcode_dstReg, par2, &wrback_addr);
		res8 = src8 | dst8;
		SingleOp_PutBack8(opcode_dstMode, opcode_dstReg, res8, wrback_addr);
		FlagN(res8 & 0x80);
		FlagZ(res8 == 0);
		FlagV(0);
		break;
	case 006: //	ADD	Add, dest += src
		strcpy(mnemonic, "ADD");
		src16 = Decode_GetValue16(opcode_srcMode, opcode_srcReg, par1, &wrback_addr);
		dst16 = Decode_GetValue16(opcode_dstMode, opcode_dstReg, par2, &wrback_addr);
		res16 = src16 + dst16;
		SingleOp_PutBack16(opcode_dstMode, opcode_dstReg, res16, wrback_addr);
		FlagN(res16 & 0x8000);
		FlagZ(res16 == 0);
		if (((src16 & 0x8000) && (dst16 & 0x8000) && !(res16 & 0x8000)) || 
			(!(src16 & 0x8000) && !(dst16 & 0x8000) && (res16 & 0x8000)))
		{
			//set if both operand are the same sign and the result is of the opposite sign
			FlagV(1);
		}
		else
		{
			FlagV(0);
		}
		res32 = src16 + dst16;
		FlagC(res32 > 65535);
		break;
	case 016: //	SUB	Subtract, dest -= src
		strcpy(mnemonic, "SUB");
		src16 = Decode_GetValue16(opcode_srcMode, opcode_srcReg, par1, &wrback_addr);
		dst16 = Decode_GetValue16(opcode_dstMode, opcode_dstReg, par2, &wrback_addr);
		res16 = dst16 - src16;
		SingleOp_PutBack16(opcode_dstMode, opcode_dstReg, res16, wrback_addr);
		FlagN(res16 & 0x8000);
		FlagZ(res16 == 0);
		if ((!(src16 & 0x8000) && (dst16 & 0x8000) && !(res16 & 0x8000)) || //+-+
			((src16 & 0x8000) && !(dst16 & 0x8000) && (res16 & 0x8000)))    //-+-
		{
			FlagV(1);
		}
		else
		{
			FlagV(0);
		}

		src16 = ~src16;
		res32 = dst16 + src16 + 1;
		FlagC(res32 < 65536);
		break;
	default:
		return 0;
	}
	return 1;
}


int DecodeSingleOperandInst(uint16 currWord, char * mnemonic, char *par1, char *par2)
{
	uint16 opcode;
	uint16 opcode_mode;
	uint16 opcode_reg;
	uint16 val16;
	uint8 val8;
	uint16 wrback_addr;
	uint8 flag_c;

	opcode = currWord >> 6;
	opcode_mode = currWord >> 3 & 7;
	opcode_reg = currWord >> 0 & 7;
	/*
	Single-operand instructions

	15      11 10   6 5  3 2      0
	B  0 0 0 1 Opcode Mode Register
	*/
	par1[0] = 0;
	par2[0] = 0;

	switch (opcode)
	{
	case 00001: //	JMP
		strcpy(mnemonic, "JMP");
		if (opcode_mode == 0)
		{
			return 0; //illegal instruction, TODO: trap to 4 not 10
		}
		//don't want to get value but only its address
		(void)Decode_GetValue16(opcode_mode, opcode_reg, par1, &wrback_addr);
		*pPC = wrback_addr;
		break;
	case 00003: //	SWAB	Swap bytes: rotate 8 bits
		strcpy(mnemonic, "SWAB");
		val16 = Decode_GetValue16(opcode_mode, opcode_reg, par1, &wrback_addr);
		val16 = (val16 >> 8) | (val16 << 8);
		SingleOp_PutBack16(opcode_mode, opcode_reg, val16, wrback_addr);
		FlagN(val16 & 0x0080);
		FlagZ((val16&0xff) == 0);
		FlagV(0);
		FlagC(0);
		break;
	case 00050: //	CLR	Clear: dest = 0
		strcpy(mnemonic, "CLR");
		val16 = 0;
		Decode_PutValue16(opcode_mode, opcode_reg, val16, par1);
		FlagN(0);
		FlagZ(1);
		FlagV(0);
		FlagC(0);
		break;
	case 01050: //	CLRB
		strcpy(mnemonic, "CLRB");
		val8 = 0;
		Decode_PutValue8(opcode_mode, opcode_reg, val8, par1);
		FlagN(0);
		FlagZ(1);
		FlagV(0);
		FlagC(0);
		break;
	case 00051: //	COM	Complement: dest = ~dest
		strcpy(mnemonic, "COM");
		val16 = Decode_GetValue16(opcode_mode, opcode_reg, par1, &wrback_addr);
		val16 = ~val16;
		SingleOp_PutBack16(opcode_mode, opcode_reg, val16, wrback_addr);
		FlagN(val16 & 0x8000);
		FlagZ(val16==0);
		FlagV(0);
		FlagC(1);
		break;
	case 01051: //	COMB
		strcpy(mnemonic, "COMB");
		val8 = Decode_GetValue8(opcode_mode, opcode_reg, par1, &wrback_addr);
		val8 = ~val8;
		SingleOp_PutBack8(opcode_mode, opcode_reg, val8, wrback_addr);
		FlagN(val8 & 0x80);
		FlagZ(val8 == 0);
		FlagV(0);
		FlagC(1);
		break;
	case 00052: //	INC	Increment: dest += 1
		strcpy(mnemonic, "INC");
		val16 = Decode_GetValue16(opcode_mode, opcode_reg, par1, &wrback_addr);
		FlagV(val16 == 077777); //0x7fff
		val16++;
		SingleOp_PutBack16(opcode_mode, opcode_reg, val16, wrback_addr);
		FlagN(val16 & 0x8000);
		FlagZ(val16 == 0);
		break;
	case 01052: //	INCB
		strcpy(mnemonic, "INCB");
		val8 = Decode_GetValue8(opcode_mode, opcode_reg, par1, &wrback_addr);
		FlagV(val8 == 0177); //0x7f
		val8++;
		SingleOp_PutBack8(opcode_mode, opcode_reg, val8, wrback_addr);
		FlagN(val8 & 0x80);
		FlagZ(val8 == 0);
		break;
	case 00053: //	DEC	Decrement: dest -= 1
		strcpy(mnemonic, "DEC");
		val16 = Decode_GetValue16(opcode_mode, opcode_reg, par1, &wrback_addr);
		FlagV(val16 == 0100000); //0x8000
		--val16;
		SingleOp_PutBack16(opcode_mode, opcode_reg, val16, wrback_addr);
		FlagN(val16 & 0x8000);
		FlagZ(val16 == 0);
		break;
	case 01053: //	DECB
		strcpy(mnemonic, "DECB");
		val8 = Decode_GetValue8(opcode_mode, opcode_reg, par1, &wrback_addr);
		FlagV(val8 == 0200); //0x80
		--val8;
		SingleOp_PutBack8(opcode_mode, opcode_reg, val8, wrback_addr);
		FlagN(val8 & 0x80);
		FlagZ(val8 == 0);
		break;
	case 00054: //	NEG	Negate: dest = -dest
		strcpy(mnemonic, "NEG");
		val16 = Decode_GetValue16(opcode_mode, opcode_reg, par1, &wrback_addr);
		val16 = ~val16;
		val16 += 1;
		SingleOp_PutBack16(opcode_mode, opcode_reg, val16, wrback_addr);
		FlagN(val16 & 0x8000);
		FlagZ(val16 == 0);
		FlagV(val16 == 0100000); //0x8000
		FlagC(val16 != 0);
		break;
	case 01054: //	NEGB
		strcpy(mnemonic, "NEGB");
		val8 = Decode_GetValue8(opcode_mode, opcode_reg, par1, &wrback_addr);
		val8 = ~val8;
		val8 += 1;
		SingleOp_PutBack8(opcode_mode, opcode_reg, val8, wrback_addr);
		FlagN(val8 & 0x80);
		FlagZ(val8 == 0);
		FlagV(val8 == 0200); //0x80
		FlagC(val8 != 0);
		break;
	case 00055: //	ADC	Add carry: dest += C
		strcpy(mnemonic, "ADC");
		val16 = Decode_GetValue16(opcode_mode, opcode_reg, par1, &wrback_addr);
		flag_c = PSW_C();
		FlagV((val16 == 077777) && flag_c);
		FlagC((val16 == 0177777) && flag_c);
		val16 += (flag_c ? 1 : 0);
		SingleOp_PutBack16(opcode_mode, opcode_reg, val16, wrback_addr);
		FlagN(val16 & 0x8000);
		FlagZ(val16 == 0);
		break;
	case 01055: //	ADCB
		strcpy(mnemonic, "ADCB");
		val8 = Decode_GetValue8(opcode_mode, opcode_reg, par1, &wrback_addr);
		flag_c = PSW_C();
		FlagV((val8 == 0177) && flag_c);
		FlagC((val8 == 0377) && flag_c);
		val8 += (flag_c ? 1 : 0);
		SingleOp_PutBack8(opcode_mode, opcode_reg, val8, wrback_addr);
		FlagN(val8 & 0x80);
		FlagZ(val8 == 0);
		break;
	case 00056: //	SBC	Subtract carry: dest -= C
		strcpy(mnemonic, "SBC");
		val16 = Decode_GetValue16(opcode_mode, opcode_reg, par1, &wrback_addr);
		FlagV(val16 == 0100000);
		val16 -= (PSW_C() ? 1 : 0);
		SingleOp_PutBack16(opcode_mode, opcode_reg, val16, wrback_addr);
		/* Manuals contains errors regarding C flag: proper is to set flag when dst _was_ 0 and C=1, cleared otherwise */
		FlagC((val16 == 0177777) && PSW_C());
		FlagN(val16 & 0x8000);
		FlagZ(val16 == 0);		
		break;
	case 01056: //	SBCB
		strcpy(mnemonic, "SBCB");
		val8 = Decode_GetValue8(opcode_mode, opcode_reg, par1, &wrback_addr);
		FlagV(val8 == 0200);
		val8 -= (PSW_C() ? 1 : 0);
		SingleOp_PutBack8(opcode_mode, opcode_reg, val8, wrback_addr);
		FlagC((val8 == 0200) && PSW_C());
		FlagN(val8 & 0x80);
		FlagZ(val8 == 0);
		break;
	case 00057: //	TST	Test: Load src, set flags only
		strcpy(mnemonic, "TST");
		val16 = Decode_GetValue16(opcode_mode, opcode_reg, par1, &wrback_addr);
		FlagN(val16 & 0x8000);
		FlagZ(val16 == 0);
		FlagV(0);
		FlagC(0);
		break;
	case 01057: //	TSTB
		strcpy(mnemonic, "TSTB");
		val8 = Decode_GetValue8(opcode_mode, opcode_reg, par1, &wrback_addr);
		FlagN(val8 & 0x80);
		FlagZ(val8 == 0);
		FlagV(0);
		FlagC(0);
		break;
	case 00060: //	ROR	Rotate right 1 bit
		strcpy(mnemonic, "ROR");
		val16 = Decode_GetValue16(opcode_mode, opcode_reg, par1, &wrback_addr);
		flag_c = PSW_C();
		FlagC(val16 & 1);
		val16 = (val16 >> 1) | ((flag_c ? 1 : 0)<<15);
		SingleOp_PutBack16(opcode_mode, opcode_reg, val16, wrback_addr);
		FlagN(val16 & 0x8000);
		FlagZ(val16 == 0);
		FlagV(!PSW_C() != !PSW_N()); //logical xor !a != !b
		break;
	case 01060: //	RORB
		strcpy(mnemonic, "RORB");
		val8 = Decode_GetValue8(opcode_mode, opcode_reg, par1, &wrback_addr);
		flag_c = PSW_C();
		FlagC(val8 & 1);
		val8 = (val8 >> 1) | ((flag_c ? 1 : 0) << 7);
		SingleOp_PutBack8(opcode_mode, opcode_reg, val8, wrback_addr);
		FlagN(val8 & 0x80);
		FlagZ(val8 == 0);
		FlagV(!PSW_C() != !PSW_N()); //logical xor !a != !b
		break;
	case 00061: //	ROL	Rotate left 1 bit
		strcpy(mnemonic, "ROL");
		val16 = Decode_GetValue16(opcode_mode, opcode_reg, par1, &wrback_addr);
		flag_c = PSW_C();
		FlagC(val16 & 0x8000);
		val16 = (val16 << 1) | (flag_c ? 1 : 0);
		SingleOp_PutBack16(opcode_mode, opcode_reg, val16, wrback_addr);
		FlagN(val16 & 0x8000);
		FlagZ(val16 == 0);
		FlagV(!PSW_C() != !PSW_N()); //logical xor !a != !b
		break;
	case 01061: //	ROLB
		strcpy(mnemonic, "ROLB");
		val8 = Decode_GetValue8(opcode_mode, opcode_reg, par1, &wrback_addr);
		flag_c = PSW_C();
		FlagC(val8 & 0x80);
		val8 = (val8 << 1) | (flag_c ? 1 : 0);
		SingleOp_PutBack8(opcode_mode, opcode_reg, val8, wrback_addr);
		FlagN(val8 & 0x80);
		FlagZ(val8 == 0);
		FlagV(!PSW_C() != !PSW_N()); //logical xor !a != !b
		break;
	case 00062: //	ASR	Shift right: dest >>= 1
		strcpy(mnemonic, "ASR");
		val16 = Decode_GetValue16(opcode_mode, opcode_reg, par1, &wrback_addr);
		FlagC(val16 & 1);
		val16 = (val16 >> 1) | (val16 & 0x8000);
		SingleOp_PutBack16(opcode_mode, opcode_reg, val16, wrback_addr);
		FlagN(val16 & 0x8000);
		FlagZ(val16 == 0);
		FlagV(!PSW_C() != !PSW_N()); //logical xor !a != !b
		break;
	case 01062: //	ASRB
		strcpy(mnemonic, "ASRB");
		val8 = Decode_GetValue8(opcode_mode, opcode_reg, par1, &wrback_addr);
		FlagC(val8 & 1);
		val8 = (val8 >> 1) | (val8 & 0x80);
		SingleOp_PutBack8(opcode_mode, opcode_reg, val8, wrback_addr);
		FlagN(val8 & 0x80);
		FlagZ(val8 == 0);
		FlagV(!PSW_C() != !PSW_N()); //logical xor !a != !b
		break;
	case 00063: //	ASL	Shift left: dest <<= 1
		strcpy(mnemonic, "ASL");
		val16 = Decode_GetValue16(opcode_mode, opcode_reg, par1, &wrback_addr);
		FlagC(val16 & 0x8000);
		val16 = (val16 << 1);
		SingleOp_PutBack16(opcode_mode, opcode_reg, val16, wrback_addr);
		FlagN(val16 & 0x8000);
		FlagZ(val16 == 0);
		FlagV(!PSW_C() != !PSW_N()); //logical xor !a != !b
		break;
	case 01063: //	ASLB
		strcpy(mnemonic, "ASLB");
		val8 = Decode_GetValue8(opcode_mode, opcode_reg, par1, &wrback_addr);
		FlagC(val8 & 0x80);
		val8 = (val8 << 1);
		SingleOp_PutBack8(opcode_mode, opcode_reg, val8, wrback_addr);
		FlagN(val8 & 0x80);
		FlagZ(val8 == 0);
		FlagV(!PSW_C() != !PSW_N()); //logical xor !a != !b
		break;

	default:
		return 0;
	}

	par2[0] = 0;
	return 1;
}

/* checked 13.03.2015 */
int DecodeProgramControlInst(uint16 currWord, char * mnemonic, char *par1, char *par2)
{
	uint16 opcode;
	int16 offset;
	uint16 new_pc_addr;

	opcode = currWord >> 8;
	offset = currWord & 0xff;
	if (offset & 0x80)
	{
		//sign extend
		offset |= 0xff00;
	}

/* Example
 6512: 000770: BR 6474
 offset = 0370;
 sign extended 177770
 2* offset = 177740;
 006514+177740=006454
*/
	new_pc_addr = R[7] + (2 * offset);

	switch (opcode)
	{
	case 00400>>8: //BR
		strcpy(mnemonic, "BR");
		R[7] = new_pc_addr;
		break;
	case 01000>>8: //BNE
		strcpy(mnemonic, "BNE");
		if (!PSW_Z())
		{
			R[7] = new_pc_addr;
		}
		break;
	case 01400>>8: //BEQ
		strcpy(mnemonic, "BEQ");
		if (PSW_Z())
		{
			R[7] = new_pc_addr;
		}
		break;
	case 02000>>8: //BGE
		strcpy(mnemonic, "BGE");
		if (!PSW_N() == !PSW_V()) //N xor V = 0
		{
			R[7] = new_pc_addr;
		}
		break;
	case 02400>>8: //BLT
		strcpy(mnemonic, "BLT");
		if (!PSW_N() != !PSW_V()) //N xor V = 1
		{
			R[7] = new_pc_addr;
		}
		break;
	case 03000 >> 8: //BGT
		strcpy(mnemonic, "BGT");
		if ((PSW_Z() || (!PSW_N() != !PSW_V()))==0) //Z or (N xor V) = 0
		{
			R[7] = new_pc_addr;
		}
		break;
	case 03400 >> 8: //BLE
		strcpy(mnemonic, "BLE");
		if (PSW_Z() || (!PSW_N() != !PSW_V())) //Z or (N xor V) = 1
		{
			R[7] = new_pc_addr;
		}
		break;
	case 0100000 >> 8: //BPL
		strcpy(mnemonic, "BPL");
		if (!PSW_N())
		{
			R[7] = new_pc_addr;
		}
		break;
	case 0100400>>8: //BMI
		strcpy(mnemonic, "BMI");
		if (PSW_N())
		{
			R[7] = new_pc_addr;
		}
		break;
	case 0101000 >> 8: //BHI
		strcpy(mnemonic, "BHI");
		if ((0==PSW_C()) && (0==PSW_Z()))
		{
			R[7] = new_pc_addr;
		}
		break;
	case 0101400 >> 8: //BLOS, branch if carry or zero
		strcpy(mnemonic, "BLOS");
		if (PSW_C() || PSW_Z())
		{
			R[7] = new_pc_addr;
		}
		break;
	case 0102000 >> 8: //BVC
		strcpy(mnemonic, "BVC");
		if (!PSW_V())
		{
			R[7] = new_pc_addr;
		}
		break;
	case 0102400 >> 8: //BVS
		strcpy(mnemonic, "BVS");
		if (PSW_V())
		{
			R[7] = new_pc_addr;
		}
		break;
	case 0103000 >> 8: //BCC
		strcpy(mnemonic, "BCC");
		if (!PSW_C())
		{
			R[7] = new_pc_addr;
		}
		break;
	case 0103400 >> 8: //BCS
		strcpy(mnemonic, "BCS");
		if (PSW_C())
		{
			R[7] = new_pc_addr;
		}
		break;

	default:
		return 0;
	}

	sprintf(par1, "%ho", new_pc_addr);

	return 1;
}

int DecodeConditionCodeInstr(uint16 currWord, char * mnemonic, char *par1, char *par2)
{
	par1[0] = 0;
	par2[0] = 0;

	if ((currWord & 0177740) == 000240)
	{
		if (currWord & 001)
		{
			FlagC((currWord & 020)?1:0);
		}
		if (currWord & 002)
		{
			FlagV((currWord & 020) ? 1 : 0);
		}
		if (currWord & 004)
		{
			FlagZ((currWord & 020) ? 1 : 0);
		}
		if (currWord & 010)
		{
			FlagN((currWord & 020) ? 1 : 0);
		}

		switch (currWord)
		{
		case 000241: //CLC
			strcpy(mnemonic, "CLC");
			break;
		case 000261: //SEC
			strcpy(mnemonic, "SEC");
			break;
		case 000242: //CLV
			strcpy(mnemonic, "CLV");
			break;
		case 000262: //SEV
			strcpy(mnemonic, "SEV");
			break;
		case 000244: //CLZ
			strcpy(mnemonic, "CLZ");
			break;
		case 000264: //SEZ
			strcpy(mnemonic, "SEZ");
			break;
		case 000250: //CLN
			strcpy(mnemonic, "CLN");
			break;
		case 000270: //SEN
			strcpy(mnemonic, "SEN");
			break;
		case 000257: //CCC
			strcpy(mnemonic, "CCC");
			break;
		case 000277: //SCC
			strcpy(mnemonic, "SCC");
			break;
		case 000263:
			strcpy(mnemonic, "SCVN");
			break;
		}
	}
	else
	{
		switch (currWord)
		{
		case 000005: //RESET
			strcpy(mnemonic, "RESET");
			break;
		case 000240: //NOP
			strcpy(mnemonic, "NOP");
			break;
		default:
			return 0;
		}
	}

	return 1;
}


SIM_ERROR PDP_Simulate(char * mnemonic, char *par1, char *par2)
{
	uint16 currWord;

	mnemonic[0] = 0;
	par1[0] = 0;
	par2[0] = 0;

	/* Fetch instruction */
	currWord = MemoryRead16(*pPC);
	*pPC += 2;

	/*
	Double-operand instructions
	
	15 14  12 11 9 8    6 5	 3 2         0
	B  Opcode Mode Source Mode Destination
	*/
	if (DecodeDoubleOperandInst(currWord, mnemonic, par1, par2))
	{
		return SIM_OK;
	}
	else if (DecodeSingleOperandInst(currWord, mnemonic, par1, par2))
	{
		return SIM_OK;
	}
	else if (DecodeProgramControlInst(currWord, mnemonic, par1, par2))
	{
		return SIM_OK;
	}
	else if (DecodeConditionCodeInstr(currWord, mnemonic, par1, par2))
	{
		return SIM_OK;
	}
	else if (currWord == 000000) //HALT
	{
		strcpy(mnemonic, "HALT");
		return SIM_HALT;
	}
	else if (currWord == 000001) //WAIT
	{
		strcpy(mnemonic, "WAIT");
		return SIM_HALT;
	}
	else if ((currWord&0177000) == 004000) //JSR
	{
		uint8 reg = (currWord>>6)&7;
		uint16 wrback_addr;
		uint8 opcode_mode = (currWord >> 3) & 7;
		uint8 opcode_reg = currWord & 7;

		//http://pages.cpsc.ucalgary.ca/~dsb/PDP11/Control.html
		strcpy(mnemonic, "JSR");
		if (reg == 7)
		{
			sprintf(par1, "PC");
		}
		else
		{
			sprintf(par1, "R%d", reg);
		}

		(void)Decode_GetValue16(opcode_mode, opcode_reg, par2, &wrback_addr);

		//The contents of the register Rn are pushed onto the processor stack.
		R[6] -= 2;
		MemoryWrite16(R[6], R[reg]);

		//The contents of the PC register are copied to Rn.
		R[reg] = *pPC;
		//The effective address pointed to by the destination operand is loaded into the PC.
		*pPC = wrback_addr;

		return SIM_OK;
	}
	else if ((currWord & 0177770) == 000200) //RTS
	{
		uint8 reg = currWord & 7;
		strcpy(mnemonic, "RTS");
		if (reg == 7)
		{
			sprintf(par1, "PC");
		}
		else
		{
			sprintf(par1, "R%d", reg);
		}
		//The contents of Rn are copied to the PC.
		*pPC = R[reg];
		//The top of the processor stack is popped into the register Rn.
		R[reg] = MemoryRead16(R[6]);
		R[6] += 2;
		return SIM_OK;
	}
	else if ((currWord & 0177400) == 0104400) //TRAP
	{
		strcpy(mnemonic, "TRAP");
		R[6] -= 2;
		MemoryWrite16(R[6], PSW);
		R[6] -= 2;
		MemoryWrite16(R[6], *pPC);
		*pPC = MemoryRead16(034);
		PSW = (uint8)MemoryRead16(036);
		return SIM_OK;
	}
	else if ((currWord & 0177400) == 0104000) //EMT
	{
		strcpy(mnemonic, "EMT");
		R[6] -= 2;
		MemoryWrite16(R[6], PSW);
		R[6] -= 2;
		MemoryWrite16(R[6], *pPC);
		*pPC = MemoryRead16(030);
		PSW = (uint8)MemoryRead16(032);
		return SIM_OK;
	}
	else if (currWord  == 0000004) //IOT
	{
		strcpy(mnemonic, "IOT");
		R[6] -= 2;
		MemoryWrite16(R[6], PSW);
		R[6] -= 2;
		MemoryWrite16(R[6], *pPC);
		*pPC = MemoryRead16(020);
		PSW = (uint8)MemoryRead16(022);
		return SIM_OK;
	}
	else if (currWord == 0000002) //RTI
	{
		strcpy(mnemonic, "RTI");
		*pPC = MemoryRead16(R[6]);
		R[6] += 2;
		PSW = (uint8)MemoryRead16(R[6]);
		R[6] += 2;
		return SIM_OK;
	}
	return SIM_ILLEGAL_INSTRUCTION;
}

void IRQ(uint16 vector)
{
	R[6] -= 2;
	MemoryWrite16(R[6], PSW);
	R[6] -= 2;
	MemoryWrite16(R[6], *pPC);
	*pPC = MemoryRead16(vector);
	PSW = (uint8)MemoryRead16(vector+2);
}
