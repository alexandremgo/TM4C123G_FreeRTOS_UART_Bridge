/*
 * Transfer data from UART0 to UART3 and UART3 to UART0 on a Tiva TM4C123G.
 */

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Board specific includes. */
#include "TM4C123GH6PM.h"
#include "bsp.h"

/* Definition of the functions implementing the tasks. */
static void vUARTGatekeeperTask(void* pvParameters);

/* Handle of the queue that stores the data transfer requests
 * that should be done by the UART gatekeeper task. */
QueueHandle_t uart_transfer_queue;

/*-----------------------------------------------------------*/

/* A gatekeeper task is a cleaner method of implementing mutual exclusion.
 * Only the gatekeeper task is allowed to directly access the resource.
 * Other tasks and interrupts can access the resource through the
 * gatekeeper's queue.
 */
static void vUARTGatekeeperTask(void *pvParameters)
{
    TransferRequest request;

    for (;;)
    {
        /* Wait for a message to arrive. */
        xQueueReceive(uart_transfer_queue, &request, portMAX_DELAY);
        BSP_transferData(request.from, request.to);
    }
}

int main()
{
    /* The queue is created to hold a maximum of 3 TransferRequest.
     * Normally, with a baud rate of 115200 and a system clock of 50Mhz,
     * only a size 2 should be needed. */
    uart_transfer_queue = xQueueCreate(3, sizeof(TransferRequest));

    BSP_init();

    if (uart_transfer_queue != NULL)
    {
        xTaskCreate(vUARTGatekeeperTask, "UART_Gatekeeper", configMINIMAL_STACK_SIZE, NULL, 2, NULL);

        BSP_sendStr(UART0, (unsigned char*) "\n*****UART TRANSCEIVER*****\n");
        BSP_sendStr(UART0, (unsigned char*) "* pin (Tiva) PC6 (RX) should be connected to pin (RPi) 8 (TX)\n");
        BSP_sendStr(UART0, (unsigned char*) "* pin (Tiva) PC7 (TX) should be connected to pin (RPi) 10 (RX)\n");

        /* Start the scheduler so the created tasks start executing. */
        vTaskStartScheduler();
    }

    return 0;
}

/*
 * Run time stack overflow checking is performed if
 * configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.
 * This hook function is called if a stack overflow is detected.
 */
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    for (;;)
    {
    }
}
