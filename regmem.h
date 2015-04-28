#ifndef REGMEM_H
#define REGMEM_H

#define PDP_MEMORY_SIZE (1024*(56))

#define PSW_T_SHIFT (4)
#define PSW_N_SHIFT (3)
#define PSW_Z_SHIFT (2)
#define PSW_V_SHIFT (1)
#define PSW_C_SHIFT (0)

#define PSW_N() (PSW&(1<<3))
#define PSW_Z() (PSW&(1<<2))
#define PSW_V() (PSW&(1<<1))
#define PSW_C() (PSW&(1<<0))

#define PSW_N_SETMASK (1<<3)
#define PSW_Z_SETMASK (1<<2)
#define PSW_V_SETMASK (1<<1)
#define PSW_C_SETMASK (1<<0)

#define PSW_N_CLEARMASK (~(1<<3))
#define PSW_Z_CLEARMASK (~(1<<2))
#define PSW_V_CLEARMASK (~(1<<1))
#define PSW_C_CLEARMASK (~(1<<0))
#define PSW_CVZN_CLEARMASK (~(0x0f)

extern uint8 memory[PDP_MEMORY_SIZE];
extern uint16 R[8];
extern uint16 * const pPC;
extern uint16 * const pSP;
extern uint8 PSW;

extern int busError;

uint16 MemoryRead16(uint16 addr16);
uint8 MemoryRead8(uint16 addr16);
void MemoryWrite16(uint16 addr16, uint16 val16);
void MemoryWrite8(uint16 addr16, uint8 val8);


void MemoryReset();
void PutTapeBootstrap();
void PrintRegisters();

typedef struct
{
	FILE * f;
	char filename[255];
	int status;
	int bytes_read;
	int eof;
	int read_request;
} tape_t;

extern tape_t tape;
#endif