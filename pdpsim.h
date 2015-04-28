#ifndef PDPSIM_H
#define PDPSIM_H

typedef enum
{
	SIM_OK,
	SIM_HALT,
	SIM_ILLEGAL_INSTRUCTION
} SIM_ERROR;

SIM_ERROR PDP_Simulate(char * mnemonic, char *par1, char *par2);


int CheckBusErrorAndTrap4();
void Trap10();
void IRQ(uint16 vector);

#endif