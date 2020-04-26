#ifndef __BSP_H__
#define __BSP_H__

#include <stdint.h>  /* Standard integers. WG14/N843 C99 Standard */
#include "TM4C123GH6PM.h" /* the TM4C MCU Peripheral Access Layer (TI) */

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Data structure representing a data transfer request
 * from one UART to another. */
typedef struct {
    UART0_Type* from;
    UART0_Type* to;
} TransferRequest;

/* On-board led. */
#define LED_GREEN (1U << 3)

#define CHAR_NULL 0

/* Initialize the needed hardware and interrupts. */
void BSP_init( void );

/* Initialize the interrupts related to our UART.
 * Called by BSP_init. */
void BSP_initUARTInterrupts(void);

/* Init the USB UART. */
void BSP_initUART0(void);

/* Init the UART: PC6 (pin 14) = Receiver and PC7 (pin 13) = Transmitter. */
void BSP_initUART3(void);

/* Configure an UART (clock, word length, FIFO etc.). */
void BSP_configUART(UART0_Type* uart);

/* Init the used leds. */
void BSP_initLeds(void);

/* Send a char through an UART connection. */
void BSP_sendChar(UART0_Type* uart, unsigned char one_byte);

/* Read a char coming from UART connection. */
unsigned char BSP_readChar(UART0_Type* uart);

/* Send a string through an UART connection. */
void BSP_sendStr(UART0_Type* uart, unsigned char *buffer);

/* Read a string coming from UART connection. */
void BSP_readStr(UART0_Type* uart, unsigned char *buffer);

/* Transfer a char from one UART connection to another. */
void BSP_transferData(UART0_Type* from, UART0_Type* to);

void BSP_ledGreenToggle(void);

#endif // __BSP_H__
