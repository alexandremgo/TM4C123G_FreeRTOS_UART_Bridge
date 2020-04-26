/* Board Support Package (BSP) for the EK-TM4C123GXL board */
#include "bsp.h"

extern QueueHandle_t uart_transfer_queue;

#define TRANSFER_R_FROM_UART0 0
#define TRANSFER_R_FROM_UART3 1
static const TransferRequest transfer_requests[2] = { { UART0, UART3 }, /* Used by UART0_IRQHandler. */
                                                      { UART3, UART0 }  /* Used by UART3_IRQHandler. */
};

void BSP_init(void)
{
    SystemCoreClockUpdate();

    BSP_initLeds();
    BSP_initUART0();
    BSP_initUART3();
    BSP_initUARTInterrupts();
}

void BSP_initLeds(void)
{
    /* Enable clock for GPIOF. */
    SYSCTL->RCGC2 |= (1U << 5);
    /* Enable Advanced High-Performance Bus for GPIOF. */
    SYSCTL->GPIOHBCTL |= (1U << 5);

    /* Set GPIO Pin Direction as output. */
    GPIOF_AHB->DIR |= LED_GREEN;
    /* Set Digital Enable. */
    GPIOF_AHB->DEN |= LED_GREEN;
}

void BSP_initUARTInterrupts(void)
{
    /* Set the interrupt priorities:
     * A higher-urgency interrupt (lower priority number) can preempt
     * a lower-urgency interrupt (higher priority number).
     * PendSV has the highest value. */
    NVIC_SetPriority(UART0_IRQn, configLIBRARY_LOWEST_INTERRUPT_PRIORITY);
    NVIC_SetPriority(UART3_IRQn, configLIBRARY_LOWEST_INTERRUPT_PRIORITY);

    /* Enable IRQs in NVIC. */
    NVIC_EnableIRQ(UART0_IRQn);
    NVIC_EnableIRQ(UART3_IRQn);

    /* Enable interrupts only on receiving. */
    UART0->IM = (1u << 4);
    UART3->IM = (1u << 4);

    /* Clear interrupt receiver source. */
    UART0->ICR = 0xFFFU;
    UART3->ICR = 0xFFFU;
}

void BSP_initUART0(void) {
    /* Enable the UART module 0 by providing a clock. */
    SYSCTL->RCGCUART |= 1u << 0;

    /* Enable and provide a clock to GPIO Port A. */
    SYSCTL->RCGCGPIO |= 1u << 0;

    /* The associated pin of PA0 and PA1 functions as a peripheral signal
     * and is controlled by the alternate hardware function. */
    GPIOA->AFSEL |= (1u << 1) | (1u << 0);

    /* GPIO Port Control. Configure the PMC0 and PMC1 in the GPIOPCTL
     * register to assign the UART signals to the appropriate pins. */
    GPIOA->PCTL |= (1 << 4) | (1 << 0);

    /* Enable the digital functions for the pins PD6 and PD7. GPIODEN. */
    GPIOA->DEN |= (1u << 1) | (1u << 0);

    BSP_configUART(UART0);
}

void BSP_initUART3(void) {
    /* On pin 14 (Receiver) = PC6 and pin 13 (Transmitter) = PC7. */

    /* Enable the UART module 3 by providing a clock. */
    SYSCTL->RCGCUART |= 1u << 3;

    /* Enable and provide a clock to GPIO Port C. */
    SYSCTL->RCGCGPIO |= 1u << 2;

    /* The associated pin of PC6 and PC7 functions as a peripheral signal
     * and is controlled by the alternate hardware function. */
    GPIOC->AFSEL |= (1u << 7) | (1u << 6);

    /* GPIO Port Control. Configure the PMC6 and PMC7 in the GPIOPCTL
     * register to assign the UART signals to the appropriate pins. */
    GPIOC->PCTL |= (1 << 28) | (1 << 24);

    /* Enable the digital functions for the pins PC6 and PC7. GPIODEN. */
    GPIOC->DEN |= (1u << 7) | (1u << 6);

    BSP_configUART(UART3);
}

#define UART_BAUD_RATE 115200

void BSP_configUART(UART0_Type* uart) {

    /* Disable UART during setting by clearing
     * the UARTEN bit in the UARTCTL register. */
    uart->CTL &= ~(1u << 0);

    /* UART Clock Configuration.
     * Use the PIOSC clock in the project, so the clock is independent
     * from the value of the system clock (PLL).
     * 0x0: System clock (based on clock source and divisor factor).
     * 0x5: PIOSC = 16 MHz. */
    uart->CC = 0x5;

    /* Find the Baud-Rate Divider BRD.
     * UARTSysClk = 16MHz, ClkDiv = 16 and Baud Rate = 115200
     * BRD = IBRD + FBRD = UARTSysClk / (ClkDiv * Baud Rate)
     *
     * IBRD = Integer part of BRD = UARTSysclk/clkdiv*115200
     *      = 8
     *
     * FNRD = Integer part of: Fractional portion of the BRD * 64 + 0.5
     *      = int(0.6805 * 64 + 0.5) = 44
     */
    uart->IBRD = 8;
    uart->FBRD = 44;

    /* UART Line Control. Enable FIFO and set UART Word length = 8 bits
     * UART0->LCRH |= ((1 << 6) | (1 << 5) | (1 << 4));
     * 8-bit, no parity, 1-stop bit. */
    uart->LCRH = (0x3 << 5);

    /* Enable UART, transmission and reception
     * by setting the UARTEN bit in the UARTCTL. */
    uart->CTL |= (1u << 0) | (1u << 8) | (1u << 9);
}


/***** IRQ Handlers *****/

void UART0_IRQHandler(void) {

    /* Debug: to monitor the needed size for the uart_transfer_queue. */
    static unsigned nb_UART0errQueueFull = 0;

    /* If context switch is required, if will get set to pdTRUE
     * inside xQueueSendToBackFromISR. */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Raw Interrupt Status: check interrupt caused by receiving status. */
    if ((UART0->RIS & (1u << 4)) != 0u)
    {
        BaseType_t status;

        status = xQueueSendToBackFromISR(uart_transfer_queue,
                                &transfer_requests[TRANSFER_R_FROM_UART0],
                                &xHigherPriorityTaskWoken);

        if(status == errQUEUE_FULL) {
            nb_UART0errQueueFull++;
        }
    }

    /* Clear interrupt sources. */
    UART0->ICR = 0xFFFu;

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void UART3_IRQHandler(void) {

    /* Debug: to monitor the needed size for the uart_transfer_queue. */
    static unsigned nb_UART3errQueueFull = 0;

    /* If context switch is required, if will get set to pdTRUE
     * inside xQueueSendToBackFromISR. */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Raw Interrupt Status: check interrupt caused by receiving status. */
    if ((UART3->RIS & (1u << 4)) != 0u)
    {
        BaseType_t status;

        status = xQueueSendToBackFromISR(uart_transfer_queue,
                                &transfer_requests[TRANSFER_R_FROM_UART3],
                                &xHigherPriorityTaskWoken);

        if(status == errQUEUE_FULL) {
            nb_UART3errQueueFull++;
        }
    }

    /* Clear interrupt sources. */
    UART3->ICR = 0xFFFu;

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**** UART data handlers *****/

void BSP_sendChar(UART0_Type* uart, unsigned char c)
{
    /* Wait that any previous transmission has completed. */
    while ((uart->FR & (1u << 5)) != 0);
    uart->DR = c;
}

unsigned char BSP_readChar(UART0_Type* uart)
{
    unsigned char c;

    /* Check that the data register is not empty. */
    if((uart->FR & (1u << 4)) != 0) {
        return CHAR_NULL;
    }

    c = uart->DR;
    return c;
}

void BSP_readStr(UART0_Type* uart, unsigned char *buffer) {

    uint8_t volatile i;
    uint8_t len = sizeof(buffer)/sizeof(unsigned char);

    /* Wait that the data register is not empty. */
    while ((uart->FR & (1u << 4)) != 0);

    for(i = 0; (i < len) && ((uart->FR & (1u << 4)) == 0); i++) {
        buffer[i] = uart->DR;
    }
}

void BSP_sendStr(UART0_Type* uart, unsigned char *buffer)
{
    while (*buffer != 0)
    {
        BSP_sendChar(uart, *buffer);
        buffer++;
    }
}

void BSP_transferData(UART0_Type* from, UART0_Type* to)
{
    BSP_ledGreenToggle();

//    while ((from->FR & (1u << 4)) == 0)
//    {
        char c = BSP_readChar(from);
        if(c != CHAR_NULL) {
            BSP_sendChar(to, c);
        }
//    }
}


/***** LED handlers *****/

void BSP_ledGreenToggle(void)
{
    GPIOF_AHB->DATA ^= LED_GREEN;
}
