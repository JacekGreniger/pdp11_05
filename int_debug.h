#ifndef INT_DEBUG_H
#define INT_DEBUG_H

#include "config.h"
typedef enum
{
	KL11_READER_IE = 0,
	KL11_WRITER_IE,
#ifdef ENABLE_RL02
	RL02_IE,
#endif
	KW11_IE,
#ifdef ENABLE_RK05
	RK05_IE,
#endif
	LIST_END_IE
} pdp_interrupts_enum_t;

void IntDebug_Initialize();
void IntDebug_Set(pdp_interrupts_enum_t name, int state);
void IntDebug_Print();

#endif