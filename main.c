/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Board specific includes. */
#include "TM4C123GH6PM.h"
#include "bsp.h"

/* Definition of the functions implementing the tasks. */
static void vUART0Task(void* pvParameters);
static void vUART3Task(void* pvParameters);

SemaphoreHandle_t uart0_binary_sema = NULL;
SemaphoreHandle_t uart3_binary_sema = NULL;

SemaphoreHandle_t transceiver_mutex = NULL;

/*-----------------------------------------------------------*/

static void vUART0Task(void* pvParameters)
{
    for (;;)
    {
        xSemaphoreTake(uart0_binary_sema, portMAX_DELAY);
        BSP_transferData(UART0, UART3);
    }
}

static void vUART3Task(void* pvParameters)
{
    for(;;) {
        xSemaphoreTake(uart3_binary_sema, portMAX_DELAY);
        BSP_transferData(UART3, UART0);
    }
}

int main()
{
    /* Create binary semaphores for UART0 and UART3 tasks. */
    uart0_binary_sema = xSemaphoreCreateBinary();
    uart3_binary_sema = xSemaphoreCreateBinary();

    /* Create Mutex for the transfer data function.
     * With a mutex, the low priority tasks are promoting to the
     * priority of the highest task using this mutex. */
    transceiver_mutex = xSemaphoreCreateMutex();

    BSP_init();

    if (uart0_binary_sema != NULL && uart3_binary_sema != NULL &&  transceiver_mutex != NULL)
    {
        xTaskCreate(vUART0Task, "UART0", configMINIMAL_STACK_SIZE, NULL, 2, NULL);

        xTaskCreate(vUART3Task, "UART3", configMINIMAL_STACK_SIZE, NULL, 2, NULL);

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
