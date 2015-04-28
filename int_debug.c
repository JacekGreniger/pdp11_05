#include "stdio.h"
#include "conio.h"
#include "memory.h"
#include "string.h"
#include "int_debug.h"

char * pdp_interrupt_names[] = 
{
	{ "KL11_READER_IE" },
	{ "KL11_WRITER_IE" },
#ifdef ENABLE_RL02
	{ "RL02_IE" },
#endif
	{ "KW11_IE" },
#ifdef ENABLE_RK05
	{ "RK05_IE" }
#endif
};

int pdp_ie[LIST_END_IE];

void IntDebug_Initialize()
{
	memset(pdp_ie, 0, sizeof(int)*LIST_END_IE);
}

void IntDebug_Set(pdp_interrupts_enum_t name, int state)
{
	pdp_ie[name] = !!state;
}

void IntDebug_Print()
{
	int i;

	printf("Interrupt status:\n");
	for (i = 0; i < LIST_END_IE; i++)
	{
		printf("  %s = %d\n", pdp_interrupt_names[i], pdp_ie[i]);
	}
}