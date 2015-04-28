#ifndef RK05_H
#define RK05_H

typedef struct
{
	uint16 rkds;
	uint16 rker;
	uint16 rkcs;
	uint16 rkwc;
	uint16 rkba;
	uint16 rkda;

	int write_protect;

	int rdy_cnt;
	char filename[128];
	FILE *f;
} rk05_t;

extern rk05_t rk05;
extern int rkdebug;

void RK05_Reset();
int RK05_Service();
void RK05_ExecuteCommand();
uint16 RK05_IORead16(addr16);
void RK05_IOWrite16(uint16 addr16, uint16 val16);

#endif