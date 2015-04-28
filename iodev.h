#ifndef IODEV_H
#define IODEV_H

#include "config.h"

/* PC11 high speed tape reader */
extern uint8 PC11_PRB; //Reader Buffer Register
extern uint8 PC11_PSR; //Reader Status Register
#define PC11_PSR_ADDRESS (0177550)
#define PC11_PRB_ADDRESS (0177552)
#define PC11_PSR_RDR_ENB_BIT (1<<0)
#define PC11_PSR_DONE_BITMASK (1<<7)

extern int keyboard_ctrl_d;
extern uint8 KL11_TBUF_new_data;

//For interrupts, the vector is 000060 for the Reader part, and 000064 for the Punch part.Priority level is BR4.
#define KL11_RCSR_ADDRESS (0177560)
#define KL11_RCSR_WRITE_MASK (1<<6) /* Only IE bit is writable*/
#define KL11_RCSR_RDR_ENB_BIT (1<<0)
#define KL11_RCSR_IE_BIT (1<<6) //receiver interrupt enable bit
#define KL11_RCSR_DONE_BIT (1<<7) //1, if an entire character has been received. 
#define KL11_RCSR_BUSY_BIT (1<<11) //1 between the start and the stop bit of an incoming character

#define KL11_RBUF_ADDRESS (0177562)

#define KL11_TCSR_ADDRESS (0177564)
#define KL11_TCSR_WRITE_MASK (1<<6)
#define KL11_TCSR_IE_BIT (1<<6) //Transmitter interrupt enable.
#define KL11_TCSR_RDY_BIT (1<<7) //Transmitter ready. Cleared when transmit buffer is loaded, set when transmit buffer can accept another character. Will start interrupt if transmitter interrupt is enabled.

/* A write will start transmission of a character */
#define KL11_TBUF_ADDRESS (0177566)

extern uint8 KL11_TBUF_REG; //Punch Buffer Register
extern uint8 KL11_RBUF_REG; //Punch Status Register
extern uint8 KL11_RCSR_REG; //Reader Status Register
extern uint8 KL11_RBUF_REG; //Reader Buffer Register


#define PDP_SR_ADDRESS (0177570)
extern uint16 SR_REG;

#define PSW_ADDRESS (0177776)

#ifdef ENABLE_KW11
#define KW11_CSR_ADDRESS (0177546)
#define KW11_CSR_IE_BIT (1<<6)
#define KW11_CSR_MONITOR_BIT (1<<7)
#define KW11_CSR_WRITE_MASK ((1<<7)|(1<<6))
#endif //ENABLE_KW11

void IOReset();
void KL11_Service();
int KL11_CheckInterrupts();

#ifdef ENABLE_KW11
void KW11_Kick();
int KW11_CheckInterrupt();
#endif //ENABLE_KW11

uint8 IORead8(uint16 addr16);
uint16 IORead16(uint16 addr16);
void IOWrite8(uint16 addr16, uint8 val8);
void IOWrite16(uint16 addr16, uint16 val16);

#endif
