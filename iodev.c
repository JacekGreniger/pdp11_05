#include "stdio.h"
#include "conio.h"
#include "memory.h"
#include "string.h"
#include "defs.h"
#include "regmem.h"
#include "iodev.h"
#include "rl02.h"
#include "int_debug.h"
#include "config.h"
#include "rk05_registers.h"
#include "rk05.h"

#define KL11_IRQ_CYCLES_COUNT 50

uint8 PSW;
uint16 SR_REG;

#ifdef ENABLE_KW11
uint16 KW11_CSR_REG;
int kw11_raise_interrupt = 0;
#endif //ENABLE_KW11

uint8 PC11_PRB; //PC11 High Speed Reader Buffer Register
uint8 PC11_PSR = 0; //PC11 High Speed Reader Status Register

int keyboard_ctrl_d = 0;
uint8 KL11_RCSR_REG = 0; //Reader Status Register
uint8 KL11_RBUF_REG = 0; //Reader Buffer Register
uint8 KL11_TCSR_REG = 0; //Punch Status Register
uint8 KL11_TBUF_REG = 0; //Punch Buffer Register
uint8 KL11_TBUF_new_data = 0;

int kl11_raise_receiver_irq = 0;
int kl11_raise_transmitter_irq = 0;

int KL11_CheckInterrupts()
{
	if (kl11_raise_receiver_irq>1)
	{
		--kl11_raise_receiver_irq;
	}
	if (kl11_raise_receiver_irq == 1)
	{
		kl11_raise_receiver_irq = 0;
		R[6] -= 2;
		MemoryWrite16(R[6], PSW);
		R[6] -= 2;
		MemoryWrite16(R[6], *pPC);
		*pPC = MemoryRead16(060);
		PSW = (uint8)MemoryRead16(062);
		return 1;
	}

	if (kl11_raise_transmitter_irq>1)
	{
		--kl11_raise_transmitter_irq;
	}
	if (kl11_raise_transmitter_irq == 1)
	{
		kl11_raise_transmitter_irq = 0;
		R[6] -= 2;
		MemoryWrite16(R[6], PSW);
		R[6] -= 2;
		MemoryWrite16(R[6], *pPC);
		*pPC = MemoryRead16(064);
		PSW = (uint8)MemoryRead16(066);
		return 1;
	}
	return 0;
}

#ifdef ENABLE_KW11
void KW11_Kick()
{
	static uint16 tim = 0;

	if (tim > 10000)
	{
		KW11_CSR_REG |= KW11_CSR_MONITOR_BIT;
		if (KW11_CSR_REG & KW11_CSR_IE_BIT)
		{
			kw11_raise_interrupt = 1;
		}
		tim = 0;
	}
	else
	{
		tim++;
	}
}

int KW11_CheckInterrupt()
{
	if (kw11_raise_interrupt)
	{
		R[6] -= 2;
		MemoryWrite16(R[6], PSW);
		R[6] -= 2;
		MemoryWrite16(R[6], *pPC);
		*pPC = MemoryRead16(0100);
		PSW = (uint8)MemoryRead16(0102);

		kw11_raise_interrupt = 0;
		return 1;
	}
	else
	{
		return 0;
	}
}
#endif //ENABLE_KW11

void IOReset()
{
	SR_REG = 0;
	PC11_PRB = 0;
	PC11_PSR = 0;
	keyboard_ctrl_d = 0;
	KL11_RCSR_REG = 0; //Reader Status Register
	KL11_RBUF_REG = 0; //Reader Buffer Register
	KL11_TCSR_REG = 0; //Punch Status Register
	KL11_TBUF_REG = 0; //Punch Buffer Register
	KL11_TBUF_new_data = 0;

	kl11_raise_receiver_irq = 0;
	kl11_raise_transmitter_irq = 0;

#ifdef ENABLE_KW11
	KW11_CSR_REG = KW11_CSR_MONITOR_BIT;
	kw11_raise_interrupt = 0;
#endif //ENABLE_KW11
}

void KL11_Service()
{
	static int cnt = 0;

	/* Send console data */
	if (KL11_TBUF_new_data)
	{
		KL11_TBUF_new_data = 0;
		if (KL11_TBUF_REG > 0)
		{
			if ((KL11_TBUF_REG & 0x7f) == 7)
			{
				printf("<BEL>");
			}
			else
			{
				printf("%c", KL11_TBUF_REG & 0x7f);
			}
		}
		if (KL11_TCSR_REG & KL11_TCSR_IE_BIT)
		{
			//raise transmitter irq if IE bit is enabled
			kl11_raise_transmitter_irq = KL11_IRQ_CYCLES_COUNT;
		}
	}
	/* Transmitter ready. Cleared when transmit buffer is loaded, set when transmit buffer can accept another character. Will start interrupt if transmitter interrupt is enabled.*/
	KL11_TCSR_REG |= KL11_TCSR_RDY_BIT;

	if (50000 == cnt)
	{
		cnt = 0;

		if (_kbhit())
		{
			int key = _getch();
			if (key == 4) //Ctrl-D
			{
				keyboard_ctrl_d = 1;
			}
			else
			{
				if (key == 1) /* Translate CTRL-A to CTRL-C*/
				{
					key = 3;
				}
				if (key == 8) /* Translate backspace to RUBOUT */
				{
					key = 127;
				}
				//put keycode in console buffer and set flag
				KL11_RBUF_REG = key;
				KL11_RCSR_REG |= KL11_RCSR_DONE_BIT;
				if (KL11_RCSR_REG & KL11_RCSR_IE_BIT)
				{
					//raise receiver irq if IE bit is enabled
					kl11_raise_receiver_irq = KL11_IRQ_CYCLES_COUNT;
				}
			}
		}
	}
	else
	{
		++cnt;
	}

}


uint16 IORead16(uint16 addr16)
{
	uint16 retVal;

	busError = 0;

	/*
	* PC11 - High Speed Tape Reader
	*/
	if (addr16 == PC11_PRB_ADDRESS)
	{
		PC11_PSR &= ~(PC11_PSR_DONE_BITMASK);
		retVal = PC11_PRB;
	}
	else if (addr16 == PC11_PSR_ADDRESS)
	{
		retVal = PC11_PSR;
	}

	/*
	 * KL11 - console
	 */
	else if (addr16 == KL11_RCSR_ADDRESS) //reader status register
	{
		retVal = KL11_RCSR_REG;
	}
	else if (addr16 == KL11_RBUF_ADDRESS) //reader buffer register
	{
		KL11_RCSR_REG &= ~KL11_RCSR_DONE_BIT; //clear flag - char receiver
		retVal = KL11_RBUF_REG;
	}
	else if (addr16 == KL11_TCSR_ADDRESS) //punch status register
	{
		retVal = KL11_TCSR_REG;
	}

#ifdef ENABLE_RL02
	/*
	 * RL02
	 */
	else if ((addr16 >= RL02_REG_FIRST_ADDR) && (addr16 <= RL02_REG_LAST_ADDR)) //RL02
	{
		retVal = RL02_IORead16(addr16);
	}
#endif //ENABLE_RL02

#ifdef ENABLE_RK05
	/*
	* RK05
	*/
	else if ((addr16 >= RK05_REG_FIRST_ADDR) && (addr16 <= RK05_REG_LAST_ADDR))
	{
		retVal = RK05_IORead16(addr16);
	}
#endif //ENABLE_RK05

	/*
	 * SR
	 */
	else if (addr16 == PDP_SR_ADDRESS) //PDP switch register
	{
		retVal = SR_REG;
	}

#ifdef ENABLE_KW11
	/*
	* KW11 - line clock
	*/
	else if (addr16 == KW11_CSR_ADDRESS) //console punch buffer register
	{
		retVal = KW11_CSR_REG & KW11_CSR_WRITE_MASK;
	}
#endif //ENABLE_KW11

	/*
	 * PSW
	 */
	else if (addr16 == PSW_ADDRESS) //PSW
	{
		retVal = PSW;
	}
	else
	{
		busError = addr16;
		retVal = 0;
	}

	return retVal;
}


uint8 IORead8(uint16 addr16)
{
	uint8 retVal;
	busError = 0;

	/*
	 * PC11 - High Speed Tape Reader
	 */
	if (addr16 == PC11_PRB_ADDRESS)
	{
		PC11_PSR &= ~(PC11_PSR_DONE_BITMASK);
		retVal = PC11_PRB;
	}
	else if (addr16 == PC11_PSR_ADDRESS)
	{
		retVal = PC11_PSR;
	}

	/*
	 * KL11 console
	 */
	else if (addr16 == KL11_RCSR_ADDRESS) //reader status register
	{
		retVal = KL11_RCSR_REG;
	}
	else if (addr16 == KL11_RBUF_ADDRESS) //reader buffer register
	{
		KL11_RCSR_REG &= ~KL11_RCSR_DONE_BIT; //clear flag - char receiver
		retVal = KL11_RBUF_REG;
	}
	else if (addr16 == KL11_TCSR_ADDRESS) //punch status register
	{
		retVal = KL11_TCSR_REG;
	}

#ifdef ENABLE_RL02
	/*
	 * RL02
	 */
	else if ((addr16 >= RL02_REG_FIRST_ADDR) && (addr16 <= RL02_REG_LAST_ADDR)) //RL02
	{
		retVal = (uint8)RL02_IORead16(addr16);
	}
#endif // ENABLE_RL02

#ifdef ENABLE_RK05
	/*
	* RK05
	*/
	else if ((addr16 >= RK05_REG_FIRST_ADDR) && (addr16 <= RK05_REG_LAST_ADDR))
	{
		retVal = (uint8)RK05_IORead16(addr16);
	}
#endif // ENABLE_RK05

	/*
	 * PDP switch register
	 */
	else if (addr16 == PDP_SR_ADDRESS)
	{
		retVal = (uint8)SR_REG;
	}

#ifdef ENABLE_KW11
	/*
	* KW11 - line clock
	*/
	else if (addr16 == KW11_CSR_ADDRESS) //console punch buffer register
	{
		retVal = (uint8)(KW11_CSR_REG & KW11_CSR_WRITE_MASK);
	}
#endif //ENABLE_KW11

	/*
	 * PSW
	 */
	else if (addr16 == PSW_ADDRESS)
	{
		retVal = PSW;
	}
	else
	{
		retVal = 0;
		busError = addr16;
	}

	return retVal;
}

void IOWrite8(uint16 addr16, uint8 val8)
{
	busError = 0;

	/*
	 * PC11 - High Speed Tape Reader
	 */
	if (addr16 == PC11_PSR_ADDRESS) //high speed reader
	{
		if (val8 & PC11_PSR_RDR_ENB_BIT)
		{
			int c;
			//read byte from file
			c = fgetc(tape.f);
			if (c != EOF)
			{
				tape.bytes_read++;
				PC11_PRB = c;
				PC11_PSR = PC11_PSR_DONE_BITMASK; //if read success
			}
			else
			{
				tape.eof = 1;
			}
		}
	}

	/*
	 * KL11 - console 
	 */
	else if (addr16 == KL11_RCSR_ADDRESS) //177560
	{
		//only bit 6 -IE is writable
		KL11_RCSR_REG &= ~KL11_RCSR_WRITE_MASK;
		KL11_RCSR_REG |= (val8 & KL11_RCSR_WRITE_MASK);
		IntDebug_Set(KL11_READER_IE, val8 & KL11_RCSR_IE_BIT);
	}
	else if (addr16 == KL11_TCSR_ADDRESS)
	{
		//only bit 6 - IE is writable
		KL11_TCSR_REG &= ~KL11_TCSR_WRITE_MASK;
		KL11_TCSR_REG |= (val8 & KL11_TCSR_WRITE_MASK);
		IntDebug_Set(KL11_WRITER_IE, val8 & KL11_TCSR_IE_BIT);
		if (val8 & KL11_TCSR_IE_BIT)
		{
			kl11_raise_transmitter_irq = KL11_IRQ_CYCLES_COUNT;
		}

	}
	else if (addr16 == KL11_TBUF_ADDRESS) //console punch buffer register
	{
		KL11_TBUF_new_data = 1;
		KL11_TBUF_REG = val8;
		/* Transmitter ready. Cleared when transmit buffer is loaded, set when transmit buffer can accept another character. Will start interrupt if transmitter interrupt is enabled.*/
		KL11_TCSR_REG &= ~KL11_TCSR_RDY_BIT;
	}

#ifdef ENABLE_KW11
	/*
	* KW11 - line clock
	*/
	else if (addr16 == KW11_CSR_ADDRESS) //console punch buffer register
	{
		KW11_CSR_REG = val8 & KW11_CSR_WRITE_MASK;
		IntDebug_Set(KW11_IE, val8 & KW11_CSR_IE_BIT);
#ifdef ENABLE_KW11_DEBUG
		printf("KW11: IE = %d\n", !!(val8&KW11_CSR_IE_BIT));
#endif //ENABLE_KW11_DEBUG
	}
#endif //ENABLE_KW11

	/*
	 * PSW
	 */
	else if (addr16 == PSW_ADDRESS) //PSW
	{
		PSW = val8;
	}

	/*
	 * PDP Status Register
	 */
	else if (addr16 == PDP_SR_ADDRESS) 
	{
		SR_REG = val8;
	}

#ifdef ENABLE_RL02
	/*
	 * RL02
	 */
	else if ((addr16 >= RL02_REG_FIRST_ADDR) && (addr16 <= RL02_REG_LAST_ADDR))
	{
		RL02_IOWrite16(addr16, val8);
	}
#endif // ENABLE_RL02

#ifdef ENABLE_RK05
	/*
	* RK05
	*/
	else if ((addr16 >= RK05_REG_FIRST_ADDR) && (addr16 <= RK05_REG_LAST_ADDR))
	{
		RK05_IOWrite16(addr16, val8);
	}
#endif // ENABLE_RK05

	else
	{
		busError = addr16;
	}
}

void IOWrite16(uint16 addr16, uint16 val16)
{
	busError = 0;

	/*
	 * PC11 - High Speed Tape Reader
	 */
	if (addr16 == PC11_PSR_ADDRESS) //high speed reader
	{
		if (val16 & PC11_PSR_RDR_ENB_BIT)
		{
			if (tape.f != NULL)
			{

				int c;
				//read byte from file
				c = fgetc(tape.f);
				if (c != EOF)
				{
					tape.bytes_read++;
					PC11_PRB = c;
					PC11_PSR = PC11_PSR_DONE_BITMASK; //if read success
				}
				else
				{
					printf("End of tape\n");
					tape.read_request = 1;
					tape.eof = 1;
					//set busy bits
				}
			}
			else
			{
				printf("Tape not ready\n");
				tape.read_request = 1;
				//set busy bits
			}
		}
	}

	/*
	 * KL11 - console
	 */
	else if (addr16 == KL11_RCSR_ADDRESS) //177560
	{
		//only bit 6 -IE is writable
		KL11_RCSR_REG &= ~KL11_RCSR_WRITE_MASK;
		KL11_RCSR_REG |= (val16 & KL11_RCSR_WRITE_MASK);
		IntDebug_Set(KL11_READER_IE, val16 & KL11_RCSR_IE_BIT);
	}
	else if (addr16 == KL11_TCSR_ADDRESS)
	{
		//only bit 6 - IE is writable
		KL11_TCSR_REG &= ~KL11_TCSR_WRITE_MASK;
		KL11_TCSR_REG |= (val16 & KL11_TCSR_WRITE_MASK);
		IntDebug_Set(KL11_READER_IE, val16 & KL11_TCSR_IE_BIT);
		/* Writer is ready so raise transmitter interrupt if corresponding IE is set */
		if (val16 & KL11_TCSR_IE_BIT)
		{
			kl11_raise_transmitter_irq = KL11_IRQ_CYCLES_COUNT;
		}
	}
	else if (addr16 == KL11_TBUF_ADDRESS) //console punch buffer register
	{
		KL11_TBUF_new_data = 1;
		KL11_TBUF_REG = val16 & 0xff;
		/* Transmitter ready. Cleared when transmit buffer is loaded, set when transmit buffer can accept another character. Will start interrupt if transmitter interrupt is enabled.*/
		KL11_TCSR_REG &= ~KL11_TCSR_RDY_BIT;
	}

#ifdef ENABLE_KW11
	/*
	 * KW11 - line clock
	 */
	else if (addr16 == KW11_CSR_ADDRESS) //console punch buffer register
	{
		KW11_CSR_REG = val16 & KW11_CSR_WRITE_MASK;
		IntDebug_Set(KW11_IE, val16 & KW11_CSR_IE_BIT);
#ifdef ENABLE_KW11_DEBUG
		printf("KW11: IE = %d\n", !!(val16&KW11_CSR_IE_BIT));
#endif //ENABLE_KW11_DEBUG
	}
#endif //ENABLE_KW11

	/*
	 * PSW
	 */
	else if (addr16 == PSW_ADDRESS)
	{
		PSW = (uint8)val16;
	}

#ifdef ENABLE_RL02
	/*
	 * RL02
	 */
	else if ((addr16 >= RL02_REG_FIRST_ADDR) && (addr16 <= RL02_REG_LAST_ADDR))
	{
		RL02_IOWrite16(addr16, val16);
	}
#endif // ENABLE_RL02

#ifdef ENABLE_RK05
	/*
	* RK05
	*/
	else if ((addr16 >= RK05_REG_FIRST_ADDR) && (addr16 <= RK05_REG_LAST_ADDR))
	{
		RK05_IOWrite16(addr16, val16);
	}
#endif // ENABLE_RK05

	/*
	 * PDP Status Register
	 */
	else if (addr16 == PDP_SR_ADDRESS)
	{
		SR_REG = val16;
	}
	else
	{
		busError = addr16;
	}
}
