#include "stdio.h"
#include "string.h"
#include "conio.h"
#include <process.h>
#include <direct.h>
#include "ctype.h"
#include "defs.h"
#include "regmem.h"
#include "iodev.h"
#include "pdpsim.h"
#include "rl02.h"
#include "disasm.h"
#include "int_debug.h"
#include "config.h"
#include "rk05.h"

#if defined(_WIN32)
#include "windows.h"
#endif

int debug_break = 0;

tape_t tape;
FILE * logfile = NULL;

#define BREAKPOINT_MAX 16
uint16 breakpoints_arr[BREAKPOINT_MAX];
int breakpoints_num = 0;

#define TRACE_BUFFER_LEN 500

#ifdef CPU_SP_ODD_VALUE_DETECT
int detect_odd_sp = 1;
#endif

typedef struct
{
	uint16 R[8];
	uint16 PSW;
} traceBuffer_t;

traceBuffer_t traceBuffer[TRACE_BUFFER_LEN];

void TraceBuffer_Add(uint16 addr)
{
	int i;
	memmove(&traceBuffer[0], &traceBuffer[1], sizeof(traceBuffer_t)*(TRACE_BUFFER_LEN - 1));
	for (i = 0; i < 8; i++)
	{
		traceBuffer[TRACE_BUFFER_LEN - 1].R[i] = R[i];
	}
	traceBuffer[TRACE_BUFFER_LEN - 1].PSW = PSW;
}

void TraceBuffer_Reset()
{
	memset(traceBuffer, 0, sizeof(traceBuffer_t)*TRACE_BUFFER_LEN);
}

void ParseCommand_Trace(char * cmdTok_p)
{
	char mnemonic[64];
	char par1[64];
	char par2[64];
	int i;
	int num = TRACE_BUFFER_LEN;

	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p != NULL)
	{
		sscanf(cmdTok_p, "%d", &num);
		if (num >= TRACE_BUFFER_LEN)
		{
			num = TRACE_BUFFER_LEN;
		}
	}

	num = TRACE_BUFFER_LEN - num;

	printf("NOTE: registers state BEFORE instruction execution\n");
	for (i = num; i < TRACE_BUFFER_LEN; i++)
	{
		printf("\n(%d) ", i - TRACE_BUFFER_LEN + 1);
		printf("R0=%06ho ", traceBuffer[i].R[0]);
		printf("R1=%06ho ", traceBuffer[i].R[1]);
		printf("R2=%06ho ", traceBuffer[i].R[2]);
		printf("R3=%06ho ", traceBuffer[i].R[3]);
		printf("R4=%06ho ", traceBuffer[i].R[4]);
		printf("R5=%06ho\n", traceBuffer[i].R[5]);
		printf("SP=%06ho ", traceBuffer[i].R[6]);
		printf("N=%d ", !!((traceBuffer[i].PSW) & 8));
		printf("Z=%d ", !!((traceBuffer[i].PSW) & 4));
		printf("V=%d ", !!((traceBuffer[i].PSW) & 2));
		printf("C=%d ", !!((traceBuffer[i].PSW) & 1));
		printf("PC=%06ho [%06ho]", traceBuffer[i].R[7], MemoryRead16(traceBuffer[i].R[7]));
		if (Disassemble(traceBuffer[i].R[7], mnemonic, par1, par2)>0)
		{
			printf("  %s %s%c %s\n", mnemonic, par1, par2[0] ? ',' : ' ', par2);
		}
	}
}

void ParseCommand_Tape(char * cmdTok_p)
{
	char * filename;
	FILE * f;
	filename = strtok(NULL, " =,");

	if (filename == NULL)
	{
		if (tape.status)
		{
			printf("Tape: %s, bytes read=%d %s\n", tape.filename, tape.bytes_read, tape.eof?"(end of file)":"");
		}
		else
		{
			printf("Tape not opened yet\n");
		}
	}
	else if (strcmp(filename, "CLOSE")==0)
	{
		if (tape.status)
		{
			fclose(tape.f);
			tape.status = 0;
			tape.filename[0];
		}
		else
		{
			printf("Cannot close tape\n");
		}
	}
	else
	{
		if (tape.status)
		{
			printf("Tape %s closed\n", tape.filename);
			fclose(tape.f);
		}
		f = fopen(filename, "rb");
		if (f != NULL)
		{
			strcpy(tape.filename, filename);
			tape.status = 1;
			tape.f = f;
			tape.bytes_read = 0;
			tape.eof = 0;
			printf("Tape %s opened\n", tape.filename);
		}
		else
		{
			printf("Error opening file: %s\n", filename);
		}
	}
}

#ifdef ENABLE_RL02
void ParseCommand_LoadRL02Image(char * cmdTok_p)
{
	char * filename;
	FILE * f;
	filename = strtok(NULL, " =,");

	if (filename == NULL)
	{
		if (rl02.status)
		{
			printf("RL02: %s, bytes read=%d\n", rl02.filename, rl02.bytes_count);
		}
		else
		{
			printf("RL02 image not opened yet\n");
		}
	}
	else if (strcmp(filename, "CLOSE") == 0)
	{
		if (rl02.status)
		{
			fclose(rl02.f);
			rl02.status = 0;
			rl02.filename[0];
		}
		else
		{
			printf("Cannot close RL02 image\n");
		}
	}
	else
	{
		if (rl02.status)
		{
			printf("RL02 image %s closed\n", rl02.filename);
			fclose(rl02.f);
		}
		f = fopen(filename, "rb");
		if (f != NULL)
		{
			strcpy(rl02.filename, filename);
			rl02.status = 1;
			rl02.f = f;
			rl02.bytes_count = 0;
			printf("RL02 image %s opened\n", rl02.filename);
		}
		else
		{
			printf("Error opening file: %s\n", filename);
		}
	}
}
#endif // ENABLE_RL02

#ifdef ENABLE_RK05
void ParseCommand_LoadRK05Image(char * cmdTok_p)
{
	char * filename;
	FILE * f;
	filename = strtok(NULL, " =,");

	if (filename == NULL)
	{
		if (rk05.f)
		{
			printf("RK05: %s\n", rk05.filename);
		}
		else
		{
			printf("RK05 image not opened yet\n");
		}
	}
	else if (strcmp(filename, "CLOSE") == 0)
	{
		if (rk05.f)
		{
			fclose(rk05.f);
			rk05.filename[0];
			rk05.f = 0;
		}
		else
		{
			printf("Cannot close RK05 image\n");
		}
	}
	else
	{
		if (rk05.f)
		{
			printf("RK05 image %s closed\n", rk05.filename);
			fclose(rk05.f);
			rk05.f = 0;
		}
		f = fopen(filename, "r+b");
		if (f != NULL)
		{
			strcpy(rk05.filename, filename);
			rk05.f = f;
			printf("RK05 image %s opened\n", rk05.filename);
		}
		else
		{
			printf("Error opening file: %s\n", filename);
		}
	}
}
#endif // ENABLE_RK05

void ParseCommand_Examine(char * cmdTok_p)
{
	uint16 addr16;
	uint16 size = 1;
	int i;
	int res;
	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p == NULL)
	{
		printf("mr=addr16 [N] where N is number of words to read\n");
		return;
	}
	res = sscanf(cmdTok_p, "%ho", &addr16);
	if (res != 1)
	{
		printf("error in address, must be in octal format\n");
		return;
	}
	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p != NULL)
	{
		if (strncmp(cmdTok_p, "0", 1) == 0)
		{
			cmdTok_p++;
			res = sscanf(cmdTok_p, "%ho", &size);

		}
		else
		{
			res = sscanf(cmdTok_p, "%hd", &size);
		}
		if (res != 1)
		{
			printf("error in size , must be in octal or decimal format\n");
			return;
		}
	}
	for (i = 0; i < size; i++)
	{
		printf("%06ho: %06ho\n", addr16, MemoryRead16(addr16));
		addr16 += 2;
	}
}

void ParseCommand_ExamineWithDisassembly(char * cmdTok_p)
{
	char mnemonic[64];
	char par1[64];
	char par2[64];
	uint16 addr16;
	uint16 size = 1;
	int i;
	int res;
	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p == NULL)
	{
		printf("mr=addr16 [N] where N is number of words to read\n");
		return;
	}
	res = sscanf(cmdTok_p, "%ho", &addr16);
	if (res != 1)
	{
		printf("error in address, must be in octal format\n");
		return;
	}
	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p != NULL)
	{
		if (strncmp(cmdTok_p, "0", 1) == 0)
		{
			cmdTok_p++;
			res = sscanf(cmdTok_p, "%ho", &size);

		}
		else
		{
			res = sscanf(cmdTok_p, "%hd", &size);
		}
		if (res != 1)
		{
			printf("error in size , must be in octal or decimal format\n");
			return;
		}
	}
	for (i = 0; i < size; i++)
	{
		int retVal = Disassemble(addr16, mnemonic, par1, par2);
		printf("%06ho: %06ho", addr16, MemoryRead16(addr16));
		if (retVal>0)
		{
			addr16 += retVal;
			printf("  %s %s%c %s\n", mnemonic, par1, par2[0]?',':' ', par2);
		}
		else
		{
			addr16 += 2;
			printf("\n");
		}
	}
}


void ParseCommand_Deposit(char * cmdTok_p)
{
	uint16 addr16;
	uint16 value16;
	int res;

	/* Get address from commands string */
	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p == NULL)
	{
		printf("Syntax error\n");
		return;
	}
	res = sscanf(cmdTok_p, "%ho", &addr16);
	if (res != 1)
	{
		printf("error in address, must be in octal format\n");
		return;
	}

	if (addr16 & 1)
	{
		printf("error in address, must be word aligned\n");
		return;
	}

	/* Get value from commands string */
	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p == NULL)
	{
		printf("Syntax error\n");
		return;
	}
	res = sscanf(cmdTok_p, "%ho", &value16);
	if (res != 1)
	{
		printf("Error in value, must be in octal format\n");
		return;
	}

	MemoryWrite16(addr16, value16);
}

void ParseCommand_RegWrite(char * cmdTok_p)
{
	uint16 reg = 0;
	uint16 val16;
	int res;

	if (strcmp(cmdTok_p, "PC") == 0)
	{
		reg = 7;
	}
	else if (strcmp(cmdTok_p, "SP") == 0)
	{
		reg = 6;
	}
	else if (*cmdTok_p == 'R')
	{
		cmdTok_p++;
		if (*cmdTok_p >= '0' && *cmdTok_p <= '7')
		{
			reg = *cmdTok_p - '0';
		}
		else
		{
			printf(" R(0-7)/PC/SP[=NEW_OCTAL_VALUE] - read/write CPU register\n");
			return;
		}
	}

	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p == NULL)
	{
		printf("R%d=%06ho\n", reg, R[reg]);
		return;
	}
	res = sscanf(cmdTok_p, "%ho", &val16);
	if (res != 1)
	{
		printf("error in value, must be in octal format\n");
		return;
	}
	R[reg] = val16;
}

void ParseCommand_SRReg(char * cmdTok_p)
{
	uint16 reg = 0;
	uint16 val16;
	int res;

	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p == NULL)
	{
		printf("SR=%06ho\n", SR_REG);
		return;
	}
	res = sscanf(cmdTok_p, "%ho", &val16);
	if (res != 1)
	{
		printf("error in value, must be in octal format\n");
		return;
	}
	SR_REG = val16;
}

int ParseCommand_OpenScript(char * cmdTok_p, FILE ** inifile)
{
	/* Open script file */
	if (*inifile != NULL)
	{
		printf("Script file already opened");
		return 0;
	}
	cmdTok_p = strtok(NULL, " =,");
	*inifile = fopen(cmdTok_p, "r");
	if (*inifile == NULL)
	{
		printf("Cannot open script file: %s\n", cmdTok_p);
		return 0;
	}

	return 1;
}

void ParseCommand_LoadBinary(char * cmdTok_p)
{
	FILE * f;
	char filename[255];
	uint16 load_address;
	int c;
	int bytes_read = 0;
	uint16 load_address_save;
	uint16 new_pc;

	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p == NULL)
	{
		printf("load filename load_address [newPC]\n");
		return;
	}
	strcpy(filename, cmdTok_p);

	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p == NULL)
	{
		printf("load filename load_address [newPC]\n");
		return;
	}
	if (1 != sscanf(cmdTok_p, "%ho", &load_address))
	{
		printf("load filename load_address [newPC]\n");
		return;
	}
	load_address_save = load_address;

	f = fopen(filename, "rb");
	if (f == NULL)
	{
		printf("Cannot open %s\n", filename);
		return;
	}

	while ((c = fgetc(f)) >= 0)
	{
		bytes_read++;
		memory[load_address++] = c;
	}
	fclose(f);

	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p)
	{
		if (1 == sscanf(cmdTok_p, "%ho", &new_pc))
		{
			*pPC = new_pc;
		}
		else
		{
			printf("Syntax error: PC not set\n");
		}
	}

	printf("File %s: loaded %d bytes into 0%ho\n", filename, bytes_read, load_address_save);
}

void ParseCommand_Breakpoint(char * cmdTok_p)
{
	int i;
	uint16 address;
	int breakpoint_found = 0;

	cmdTok_p = strtok(NULL, " =,");

	if (cmdTok_p == NULL)
	{
		for (i = 0; i < breakpoints_num; i++)
		{
			printf("breakpoint %d: %06ho\n", i, breakpoints_arr[i]);
		}
		return;
	}

	if (1 != sscanf(cmdTok_p, "%ho", &address))
	{
		printf("wrong breakpoint address\n");
		return;
	}

	for (i = 0; i < breakpoints_num; i++)
	{
		if (breakpoints_arr[i] == address)
		{
			//remove breakpoint
			--breakpoints_num;
			for (; i < breakpoints_num; i++)
			{
				breakpoints_arr[i] = breakpoints_arr[i + 1];
			}
			printf("breakpoint removed\n");
			return;
		}
	}
	if (breakpoints_num + 1 < BREAKPOINT_MAX)
	{
		breakpoints_arr[breakpoints_num++] = address;
		printf("breakpoint set\n");
	}
}


void strtoupper(char * st)
{
	while (*st)
	{
		*st = toupper(*st);
		st++;
	}
}

void main(int argc, char ** argv)
{
	char cmdStr[255];
	char *cmdTok_p;
	char mnemonic[64];
	char par1[64];
	char par2[64];
	int i = 0;
	char * c;
	FILE * inifile = NULL;

	printf("PDP11/05 sim %s %s\n\n", __DATE__, __TIME__);

#if defined(_WIN32)
	SetConsoleCtrlHandler(NULL, TRUE);
#endif

#ifdef ENABLE_RL02
	printf("RL02 enabled\n");
#endif //ENABLE_RL02

#ifdef ENABLE_RK05
	printf("RK05 enabled\n");
#endif //ENABLE_RK05

#ifdef ENABLE_KW11
	printf("KW11 enabled\n");
#endif //ENABLE_KW11

	if (argc > 1)
	{
		inifile = fopen(argv[1], "r");
		if (inifile == NULL)
		{
			printf("Cannot open %s\n", argv[1]);
		}
	}
	else
	{
		inifile = fopen("pdp11_05.ini", "r");
	}

	tape.status = 0;
	tape.filename[0];
	tape.eof = 0;
	tape.read_request = 0;
	MemoryReset();
	IOReset();
#ifdef ENABLE_RL02
	RL02_Init();
#endif
#ifdef ENABLE_RK05
	RK05_Reset();
#endif
	TraceBuffer_Reset();
	IntDebug_Initialize();

	while (1)
	{
		printf("pdp> ");
		
		if (inifile == NULL)
		{
			gets(cmdStr);
		}
		else
		{
			//get command from file
			if (NULL == fgets(cmdStr, 255, inifile))
			{
				// end of ini file
				fclose(inifile);
				inifile = NULL;
				continue;
			}
			//fgets copies newline to the the buffer
			if (strrchr(cmdStr, 13))
			{
				*strrchr(cmdStr, 13) = 0;
			}
			if (strrchr(cmdStr, 10))
			{
				*strrchr(cmdStr, 10) = 0;
			}
			printf("%s\n", cmdStr);
		}

		strtoupper(cmdStr);
		if (c = strchr(cmdStr, ';'))
		{
			*c = 0;
		}
			
		cmdTok_p = strtok(cmdStr, " =,");
		if (cmdTok_p == NULL)
		{
			continue;
		}

		/* Quit */
		if (strncmp(cmdTok_p, "Q", 1) == 0)
		{
			if (tape.status)
			{
				fclose(tape.f);
			}
			if (logfile)
			{
				fclose(logfile);
			}
			return;
		}
		/* Breakpoint set/print/remove */
		else if (strncmp(cmdTok_p, "BR", 2) == 0)
		{
			ParseCommand_Breakpoint(cmdTok_p);
		}
		else if (strcmp(cmdTok_p, "DIR") == 0)
		{
			system("DIR");
		}
		else if (strcmp(cmdTok_p, "CD") == 0)
		{
			cmdTok_p = strtok(NULL, "\n");
			if (cmdTok_p == NULL)
			{
				continue;
			}
			_chdir(cmdTok_p);
		}
		else if (strcmp(cmdTok_p, "INT") == 0)
		{
			IntDebug_Print();
		}
		/* Reset everything */
		else if (strcmp(cmdTok_p, "RESET") == 0)
		{
			MemoryReset();
			IOReset();
#ifdef ENABLE_RL02
			RL02_Init();
#endif // ENABLE_RL02
#ifdef ENABLE_RK05
			RK05_Reset();
#endif
			TraceBuffer_Reset();
			IntDebug_Initialize();
			if (tape.status)
			{
				fclose(tape.f);
				tape.status = 0;
				tape.filename[0];
			}
			breakpoints_num = 0;
			rldebug = 0;
			rkdebug = 0;
		}
		/* Put basic bootstap in memory and set PC */
		else if (strcmp(cmdTok_p, "M") == 0)
		{
			PutTapeBootstrap();
		}
		/* Examine */
		else if (strncmp(cmdTok_p, "E", 1) == 0)
		{
			ParseCommand_Examine(cmdTok_p);
		}
		/* Examine */
		else if (strncmp(cmdTok_p, "DIS", 3) == 0)
		{
			ParseCommand_ExamineWithDisassembly(cmdTok_p);
		}
		/* Execute script file */
		else if ((strcmp(cmdTok_p, "DO") == 0) && (inifile == NULL))
		{
			int res;
			res = ParseCommand_OpenScript(cmdTok_p, &inifile);
		}
		/* Deposit */
		else if (strncmp(cmdTok_p, "D", 1) == 0)
		{
			ParseCommand_Deposit(cmdTok_p);
		}
		/* Print trace buffer */
		else if (strcmp(cmdTok_p, "TRACE") == 0)
		{
			ParseCommand_Trace(cmdTok_p);
		}
		/* Tape open/info/close */
		else if (strncmp(cmdTok_p, "T", 1) == 0)
		{
			ParseCommand_Tape(cmdTok_p);
		}
		/* Load binary file to memory */
		else if (strncmp(cmdTok_p, "L", 1) == 0)
		{
			ParseCommand_LoadBinary(cmdTok_p);
		}
#ifdef ENABLE_RL02
		/* Open RL02 disk image */
		else if (strcmp(cmdTok_p, "RL") == 0)
		{
			ParseCommand_LoadRL02Image(cmdTok_p);
		}
		else if (strcmp(cmdTok_p, "RLDEBUG") == 0)
		{
			rldebug = !rldebug;
		}
#endif //ENABLE_RL02
#ifdef ENABLE_RK05
		/* Open RK05 disk image */
		else if (strcmp(cmdTok_p, "RK") == 0)
		{
			ParseCommand_LoadRK05Image(cmdTok_p);
		}
		else if (strcmp(cmdTok_p, "RKDEBUG") == 0)
		{
			rkdebug = !rkdebug;
		}
#endif //ENABLE_RL02
		else if (strcmp(cmdTok_p, "V") == 0)
		{
			logfile = fopen("pdp.log", "w");
			if (logfile == NULL)
			{
				printf("Cannot create pdp.log\n");
			}
		}
		else if (strcmp(cmdTok_p, "VV") == 0)
		{
			if (logfile)
			{
				fclose(logfile);
				logfile = NULL;
			}
		}
		/* Print all register */
		else if (strcmp(cmdTok_p, "R") == 0)
		{
			PrintRegisters();
		}
		/* Print/set Status Register */
		else if (strcmp(cmdTok_p, "SR") == 0)
		{
			ParseCommand_SRReg(cmdTok_p);
		}
		/* Print/set R0-R7 registers */
		else if (strcmp(cmdTok_p, "PC") == 0 ||
			strcmp(cmdTok_p, "SP") == 0 ||
			strncmp(cmdTok_p, "R", 1) == 0)
		{
			ParseCommand_RegWrite(cmdTok_p);
		}
		/* Print help */
		else if (strncmp(cmdTok_p, "H", 1) == 0)
		{
			printf("PDP11/05 sim %s %s\n\n", __DATE__, __TIME__);
			printf(" M - put tape bootstrap in memory\n");
			printf(" BReakpoint [ADDR] - breakpoint list/set\n");
			printf(" RESET - clear memory/registers, close tape files\n");
			printf(" TRACE [num]- program execution trace\n");
			printf(" Examine ADDR [N] - examine memory [N-words]\n");
			printf(" DISassembly ADDR [N] - examine and disassembly memory [N-instructions]\n");
			printf(" Deposit ADDR VALUE - deposit value in memory\n");
			printf(" R(0-7)/PC/SP[=NEW_OCTAL_VALUE] - read/write CPU register\n");
			printf(" SR [=NEW_OCTAL_VALUE] - read/write SR register\n");
			printf(" Tape [filename/CLOSE] - status/open/close tape image file\n");
#ifdef ENABLE_RL02
			printf(" RL [filename/CLOSE] - status/open/close RL02 image file\n");
#endif // ENABLE_RL02
#ifdef ENABLE_RK05
			printf(" RK [filename/CLOSE] - status/open/close RK05 image file\n");
#endif // ENABLE_RL02
			printf(" Step [number of steps] - step CPU simulator\n");
			printf(" Go [new PC] - run CPU simulator\n");
			printf(" Load filename load_address [new PC] - load binary file into address\n");
			//printf(" V/VV - create/close pdp.log file for CPU logging\n");
			printf(" DO filename - execute script\n");
			printf(" DIR - list content of current directory\n");
			printf(" CD directory - change current directory\n");
			printf(" Quit\n");
			printf("\n");
		}
		/* CPU step */
		else if (strncmp(cmdTok_p, "S", 1) == 0)
		{
			SIM_ERROR simError;
			int steps = 1;
			uint16 addr16;

			cmdTok_p = strtok(NULL, " =,");
			if ((cmdTok_p == NULL) || (sscanf(cmdTok_p, "%d", &steps) != 1))
			{
				steps = 1; //error converting string to int
			}

			while (steps--)
			{
				addr16 = *pPC;

				for (i = 0; i < breakpoints_num; i++)
				{
					if (breakpoints_arr[i] == *pPC)
					{
						printf("breakpoint hit at %06ho\n", *pPC);
					}
				}

				PrintRegisters();
				TraceBuffer_Add(*pPC);
				simError = PDP_Simulate(mnemonic, par1, par2);

				if (simError == SIM_HALT)
				{
					printf("HALT instruction\n");
					break;
				}
				else if (simError == SIM_ILLEGAL_INSTRUCTION)
				{
#ifdef CPU_ILLEGAL_INSTRUCTION_INFO
					printf("ERROR: illegal instruction at %06ho: %06ho\n", addr16, MemoryRead16(addr16));
#endif //CPU_ILLEGAL_INSTRUCTION_INFO
					Trap10();
					break;
				}
#ifdef ENABLE_KW11
				if (KL11_CheckInterrupts())
				{
					break;
				}
#endif
#ifdef ENABLE_RK05
				if (RK05_Service())
				{
					continue;
				}
#endif //ENABLE_RK05
#ifdef ENABLE_RL02
				if (rl02.raise_irq)
				{
					rl02.raise_irq = 0;
					IRQ(0160);
				}
#endif // ENABLE_RL02

			}
		}
		/* CPU run */
		else if (strncmp(cmdTok_p, "G", 1) == 0)
		{
			SIM_ERROR simError;
			uint16 addr16;
			int breakpoint_hit = 0;

			cmdTok_p = strtok(NULL, " =,");
			if (cmdTok_p)
			{
				if (1 == sscanf(cmdTok_p, "%ho", &addr16))
				{
					*pPC = addr16;
				}
				else
				{
					printf("Syntax error: go [new PC]\n");
				}
			}

			printf("Running CPU, PC=%ho, Ctrl-A to simulate Ctrl-C, Ctrl-D to break\n", *pPC);
			while (!breakpoint_hit)
			{			
				if (debug_break)
				{
					printf("DEBUG_BREAK\n");
					debug_break = 0;
					break;
				}
#ifdef CPU_SP_ODD_VALUE_DETECT
				if (R[6] & 1)
				{
					if (detect_odd_sp)
					{
						printf("ERROR: SP is odd\n");
						detect_odd_sp = 0;
						break;
					}
				}
				else
				{
					detect_odd_sp = 1;
				}
#endif
				KL11_Service();
#ifdef ENABLE_KW11
				KW11_Kick();
#endif //ENABLE_KW11

				addr16 = *pPC;

				if (keyboard_ctrl_d)
				{
					printf("\nProgram execution stopped by user\n");
					keyboard_ctrl_d = 0;
					break;
				}

				TraceBuffer_Add(*pPC);
				simError = PDP_Simulate(mnemonic, par1, par2);

				for (i = 0; i < breakpoints_num; i++)
				{
					if (breakpoints_arr[i] == *pPC)
					{
						printf("breakpoint hit at %06ho\n", *pPC);
						breakpoint_hit = 1;
					}
				}

				if (simError == SIM_HALT)
				{
					printf("HALT instruction\n");
					break;
				}
				else if (simError == SIM_ILLEGAL_INSTRUCTION)
				{
#ifdef CPU_ILLEGAL_INSTRUCTION_INFO
					printf("ERROR: illegal instruction at %06ho: %06ho\n", addr16, MemoryRead16(addr16));
#endif //CPU_ILLEGAL_INSTRUCTION_INFO
					Trap10();
					continue;
				}

				if (CheckBusErrorAndTrap4())
				{
					continue;
				}
				if (KL11_CheckInterrupts())
				{
					continue;
				}
#ifdef ENABLE_KW11
				if (KW11_CheckInterrupt())
				{
					continue;
				}
#endif //ENABLE_KW11
#ifdef ENABLE_RK05
				if (RK05_Service())
				{
					continue;
				}
#endif //ENABLE_RK05

#ifdef ENABLE_RL02
				if (rl02.raise_irq)
				{
					if (rl02.raise_irq == 1)
					{
						IRQ(0160);
						rl02.raise_irq = 0;
					}
					else
					{
						--rl02.raise_irq;
					}
				}
#endif //ENABLE_RL02
			}
			PrintRegisters();
		}
		else
		{
			printf("???\n");
		}

	}
}