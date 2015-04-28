#include "stdio.h"
#include "memory.h"
#include "string.h"
#include "defs.h"
#include "regmem.h"

uint16 disPC;

/* If wrback_addr is !=0 then Decode_PutValue cannot be used */
void Dism_DecodeOperand(uint16 mode, uint16 reg, char * par)
{
	uint16 retVal;
	uint16 x;
	uint16 addr;

	switch (mode)
	{
	case 0:
		// Register	OPR Rn	The operand is in Rn
		sprintf(par, "R%d", reg);
		break;

	case 1:
		// Register deferred	OPR (Rn)	Rn contains the address of the operand
		sprintf(par, "(R%d)", reg);
		break;

	case 2:
		// Autoincrement	OPR (Rn)+	Rn contains the address of the operand, then increment Rn
		if (reg == 7) //immediate
		{
			retVal = MemoryRead16(disPC);
			sprintf(par, "#%ho", retVal);
			disPC += 2;
		}
		else
		{
			sprintf(par, "(R%d)+", reg);
		}
		break;

	case 3:
		// Autoincrement deferred	OPR @(Rn)+	Rn contains the address of the address, then increment Rn by 2
		if (reg == 7) //absolute direct
		{
			addr = MemoryRead16(disPC);
			disPC += 2;
			sprintf(par, "@#%ho", addr);
		}
		else
		{
			sprintf(par, "@(R%d)+", reg);
		}
		break;

	case 4:
		// Autodecrement	OPR -(Rn)	Decrement Rn, then use it as the address
		sprintf(par, "-(R%d)", reg);
		break;

	case 5:
		// Autodecrement deferred	OPR @-(Rn)	Decrement Rn by 2, then use it as the address of the address
		sprintf(par, "@-(R%d)", reg);
		break;

	case 6:
		// Index	OPR X(Rn)	Rn+X is the address of the operand
		x = MemoryRead16(disPC);
		disPC += 2;
		if (reg == 7) //relative direct
		{
			sprintf(par, "%ho", x + disPC);
		}
		else
		{
			sprintf(par, "%ho(R%d)", x, reg);
		}
		break;

	case 7:
		// Index deferred	OPR @X(Rn)	Rn+X is the address of the address
		x = MemoryRead16(disPC);
		disPC += 2;
		if (reg == 7) //relative indirect
		{
			sprintf(par, "@%ho", x + disPC);
		}
		else
		{
			sprintf(par, "@%ho(R%d)", reg);
		}
		break;
	}
}


int Dism_DecodeDoubleOperandInst(uint16 currWord, char * mnemonic, char *par1, char *par2)
{
	uint16 opcode;
	uint16 opcode_srcMode;
	uint16 opcode_srcReg;
	uint16 opcode_dstMode;
	uint16 opcode_dstReg;

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
		Dism_DecodeOperand(opcode_srcMode, opcode_srcReg, par1);
		Dism_DecodeOperand(opcode_dstMode, opcode_dstReg, par2);
		break;
	case 011: //	MOVB
		strcpy(mnemonic, "MOVB");
		Dism_DecodeOperand(opcode_srcMode, opcode_srcReg, par1);
		Dism_DecodeOperand(opcode_dstMode, opcode_dstReg, par2);
		break;
	case 002: //	CMP	Compare: compute src - dest, set flags only
		strcpy(mnemonic, "CMP");
		Dism_DecodeOperand(opcode_srcMode, opcode_srcReg, par1);
		Dism_DecodeOperand(opcode_dstMode, opcode_dstReg, par2);
		break;
	case 012: //	CMPB
		strcpy(mnemonic, "CMPB");
		Dism_DecodeOperand(opcode_srcMode, opcode_srcReg, par1);
		Dism_DecodeOperand(opcode_dstMode, opcode_dstReg, par2);
		break;
	case 003: //	BIT	Bit test: compute dest & src, set flags only
		strcpy(mnemonic, "BIT");
		Dism_DecodeOperand(opcode_srcMode, opcode_srcReg, par1);
		Dism_DecodeOperand(opcode_dstMode, opcode_dstReg, par2);
		break;
	case 013: //	BITB
		strcpy(mnemonic, "BITB");
		Dism_DecodeOperand(opcode_srcMode, opcode_srcReg, par1);
		Dism_DecodeOperand(opcode_dstMode, opcode_dstReg, par2);
		break;
	case 004: //	BIC	Bit clear: dest &= ~src
		strcpy(mnemonic, "BIC");
		Dism_DecodeOperand(opcode_srcMode, opcode_srcReg, par1);
		Dism_DecodeOperand(opcode_dstMode, opcode_dstReg, par2);
		break;
	case 014: //	BICB
		strcpy(mnemonic, "BICB");
		Dism_DecodeOperand(opcode_srcMode, opcode_srcReg, par1);
		Dism_DecodeOperand(opcode_dstMode, opcode_dstReg, par2);
		break;
	case 005: //	BIS	Bit set, a.k.a. logical OR: dest |= src
		strcpy(mnemonic, "BIS");
		Dism_DecodeOperand(opcode_srcMode, opcode_srcReg, par1);
		Dism_DecodeOperand(opcode_dstMode, opcode_dstReg, par2);
		break;
	case 015: //	BISB
		strcpy(mnemonic, "BISB");
		Dism_DecodeOperand(opcode_srcMode, opcode_srcReg, par1);
		Dism_DecodeOperand(opcode_dstMode, opcode_dstReg, par2);
		break;
	case 006: //	ADD	Add, dest += src
		strcpy(mnemonic, "ADD");
		Dism_DecodeOperand(opcode_srcMode, opcode_srcReg, par1);
		Dism_DecodeOperand(opcode_dstMode, opcode_dstReg, par2);
		break;
	case 016: //	SUB	Subtract, dest -= src
		strcpy(mnemonic, "SUB");
		Dism_DecodeOperand(opcode_srcMode, opcode_srcReg, par1);
		Dism_DecodeOperand(opcode_dstMode, opcode_dstReg, par2);
		break;
	default:
		return 0;
	}
	return 1;
}


int Dism_DecodeSingleOperandInst(uint16 currWord, char * mnemonic, char *par1, char *par2)
{
	uint16 opcode;
	uint16 opcode_mode;
	uint16 opcode_reg;

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
			return 0; //illegal instruction
		}
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 00003: //	SWAB	Swap bytes: rotate 8 bits
		strcpy(mnemonic, "SWAB");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 00050: //	CLR	Clear: dest = 0
		strcpy(mnemonic, "CLR");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 01050: //	CLRB
		strcpy(mnemonic, "CLRB");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 00051: //	COM	Complement: dest = ~dest
		strcpy(mnemonic, "COM");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 01051: //	COMB
		strcpy(mnemonic, "COMB");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 00052: //	INC	Increment: dest += 1
		strcpy(mnemonic, "INC");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 01052: //	INCB
		strcpy(mnemonic, "INCB");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 00053: //	DEC	Decrement: dest -= 1
		strcpy(mnemonic, "DEC");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 01053: //	DECB
		strcpy(mnemonic, "DECB");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 00054: //	NEG	Negate: dest = -dest
		strcpy(mnemonic, "NEG");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 01054: //	NEGB
		strcpy(mnemonic, "NEGB");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 00055: //	ADC	Add carry: dest += C
		strcpy(mnemonic, "ADC");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 01055: //	ADCB
		strcpy(mnemonic, "ADCB");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 00056: //	SBC	Subtract carry: dest -= C
		strcpy(mnemonic, "SBC");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 01056: //	SBCB
		strcpy(mnemonic, "SBCB");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 00057: //	TST	Test: Load src, set flags only
		strcpy(mnemonic, "TST");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 01057: //	TSTB
		strcpy(mnemonic, "TSTB");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 00060: //	ROR	Rotate right 1 bit
		strcpy(mnemonic, "ROR");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 01060: //	RORB
		strcpy(mnemonic, "RORB");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 00061: //	ROL	Rotate left 1 bit
		strcpy(mnemonic, "ROL");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 01061: //	ROLB
		strcpy(mnemonic, "ROLB");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 00062: //	ASR	Shift right: dest >>= 1
		strcpy(mnemonic, "ASR");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 01062: //	ASRB
		strcpy(mnemonic, "ASRB");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 00063: //	ASL	Shift left: dest <<= 1
		strcpy(mnemonic, "ASL");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	case 01063: //	ASLB
		strcpy(mnemonic, "ASLB");
		Dism_DecodeOperand(opcode_mode, opcode_reg, par1);
		break;
	default:
		return 0;
	}

	return 1;
}

int Dism_DecodeProgramControlInst(uint16 currWord, char * mnemonic, char *par1, char *par2)
{
	uint16 opcode;
	int16 offset;
	uint16 addr;

	opcode = currWord >> 8;
	offset = currWord & 0xff;
	if (offset & 0x80)
	{
		//sign extend
		offset |= 0xff00;
	}

	addr = disPC + (2 * offset);

	switch (opcode)
	{
	case 00400>>8: //BR
		strcpy(mnemonic, "BR");
		break;
	case 01000>>8: //BNE
		strcpy(mnemonic, "BNE");
		break;
	case 01400>>8: //BEQ
		strcpy(mnemonic, "BEQ");
		break;
	case 02000>>8: //BGE
		strcpy(mnemonic, "BGE");
		break;
	case 02400>>8: //BLT
		strcpy(mnemonic, "BLT");
		break;
	case 03000 >> 8: //BGT
		strcpy(mnemonic, "BGT");
		break;
	case 03400 >> 8: //BLE
		strcpy(mnemonic, "BLE");
		break;
	case 0100000 >> 8: //BPL
		strcpy(mnemonic, "BPL");
		break;
	case 0100400>>8: //BMI
		strcpy(mnemonic, "BMI");
		break;
	case 0101000 >> 8: //BHI
		strcpy(mnemonic, "BHI");
		break;
	case 0101400 >> 8: //BLOS, branch if carry or zero
		strcpy(mnemonic, "BLOS");
		break;
	case 0102000 >> 8: //BVC
		strcpy(mnemonic, "BVC");
		break;
	case 0102400 >> 8: //BVS
		strcpy(mnemonic, "BVS");
		break;
	case 0103000 >> 8: //BCC
		strcpy(mnemonic, "BCC");
		break;
	case 0103400 >> 8: //BCS
		strcpy(mnemonic, "BCS");
		break;

	default:
		return 0;
	}

	sprintf(par1, "%ho", addr);

	return 1;
}

int Dism_DecodeConditionCodeInstr(uint16 currWord, char * mnemonic, char *par1, char *par2)
{
	par1[0] = 0;
	par2[0] = 0;

	switch (currWord)
	{
	case 000005: //RESET
		strcpy(mnemonic, "RESET");
		break;
	case 000240: //NOP
		strcpy(mnemonic, "NOP");
		break;
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
//	case 000254: //CNZ
//		strcpy(mnemonic, "CNZ");
		break;
	case 000257: //CCC
		strcpy(mnemonic, "CCC");
		break;
	case 000277: //SCC
		strcpy(mnemonic, "SCC");
		break;

	default:
		return 0;
	}

	return 1;
}


/* Returns number of disassembled bytes */
int Disassemble(uint16 instr_addr, char * mnemonic, char *par1, char *par2)
{
	uint16 currWord;
	disPC = instr_addr;

	mnemonic[0] = 0;
	par1[0] = 0;
	par2[0] = 0;

	/* Fetch instruction */
	currWord = MemoryRead16(disPC);
	disPC += 2;

	/*
	Double-operand instructions
	
	15 14  12 11 9 8    6 5	 3 2         0
	B  Opcode Mode Source Mode Destination
	*/
	if (Dism_DecodeDoubleOperandInst(currWord, mnemonic, par1, par2))
	{
	}
	else if (Dism_DecodeSingleOperandInst(currWord, mnemonic, par1, par2))
	{
	}
	else if (Dism_DecodeProgramControlInst(currWord, mnemonic, par1, par2))
	{
	}
	else if (Dism_DecodeConditionCodeInstr(currWord, mnemonic, par1, par2))
	{
	}
	else if (currWord == 000000) //HALT
	{
		strcpy(mnemonic, "HALT");
	}
	else if (currWord == 000001) //WAIT
	{
		strcpy(mnemonic, "WAIT");
	}
	else if ((currWord&0177000) == 004000) //JSR
	{
		uint8 reg = (currWord>>6)&7;
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

		Dism_DecodeOperand(opcode_mode, opcode_reg, par2);
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
	}
	else if ((currWord & 0177400) == 0104400) //TRAP
	{
		uint16 code = currWord & 0377;
		strcpy(mnemonic, "TRAP");
		sprintf(par1, "%ho", code);
	}
	else if ((currWord & 0177400) == 0104000) //EMT
	{
		strcpy(mnemonic, "EMT");
	}
	else if (currWord  == 0000004) //IOT
	{
		strcpy(mnemonic, "IOT");
	}
	else if (currWord == 0000002) //RTI
	{
		strcpy(mnemonic, "RTI");
	}
	else
	{
		return 0; //SIM_ILLEGAL_INSTRUCTION;
	}
	return (disPC - instr_addr);
}

