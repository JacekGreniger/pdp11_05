#ifndef RL02_H
#define RL02_H

#include "config.h"

#ifdef ENABLE_RL02

#define RL02_REG_FIRST_ADDR 0174400
#define RL02_REG_LAST_ADDR   0174406

#define RL02_CS_REG_ADDR 0174400
#define RL02_BA_REG_ADDR 0174402
#define RL02_DA_REG_ADDR 0174404
#define RL02_MP_REG_ADDR 0174406

extern uint16 rl02_cs_reg;
extern uint16 rl02_ba_reg;
extern uint16 rl02_da_reg;
extern uint16 rl02_mp_reg;

typedef struct
{
	char filename[255];
	FILE * f;
	int bytes_count;
	int status;
	int command;
	int execute_command;
	int ie;
	int raise_irq;
	int head;
	int cylinder;
	int sector;
	uint16 mp_buffer[3];
	int mp_buffer_cnt;
	int csr_drdy;
} rl02_t;

extern rl02_t rl02;
extern int rldebug;

void RL02_Init();
uint16 RL02_IORead16(addr16);
void RL02_IOWrite16(uint16 addr16, uint16 val16);

#endif //ENABLE_RL02

#endif