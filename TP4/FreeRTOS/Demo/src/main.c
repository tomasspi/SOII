/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */


/*
 * This project contains an application demonstrating the use of the
 * FreeRTOS.org mini real time scheduler on the Luminary Micro LM3S811 Eval
 * board.  See http://www.FreeRTOS.org for more information.
 *
 * main() simply sets up the hardware, creates all the demo application tasks,
 * then starts the scheduler.  http://www.freertos.org/a00102.html provides
 * more information on the standard demo tasks.
 *
 * In addition to a subset of the standard demo application tasks, main.c also
 * defines the following tasks:
 *
 * + A 'Producer' task.  The producer task only sends its task name to the queue
 * so the Consurmer task can read it.
 *
 * + A 'Consumer' task.  The consumer task reads the message queue and print its
 * contents via UART. NOTES:  The dumb terminal must be closed in
 * order to reflash the microcontroller.  A very basic interrupt driven UART
 * driver is used that does not use the FIFO.  19200 baud is used.
 *
 * + A 'top' task.  The top task only executes every five seconds but has a
 * high priority so is guaranteed to get processor time.  Its function is to
 * print the system status.
 */

/* Environment includes. */
#include "DriverLib.h"
#include "printf-stdarg.h"
#include <string.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Demo app includes. */
#include "integer.h"
#include "PollQ.h"
#include "semtest.h"
#include "BlockQ.h"

/* Delay between cycles of the 'check' task. */
#define mainCHECK_DELAY						( ( TickType_t ) 5000 / portTICK_PERIOD_MS )

/* UART configuration - note this does not use the FIFO so is not very
efficient. */
#define mainBAUD_RATE				( 19200 )
#define mainFIFO_SET				( 0x10 )

/* Misc. */
#define mainQUEUE_SIZE				( 5 )
#define mainDEBOUNCE_DELAY		( ( TickType_t ) 150 / portTICK_PERIOD_MS )
#define mainNO_DELAY					( ( TickType_t ) 0 )

/*
 * Configure the processor and peripherals for this demo.
 */
static void prvSetupHardware( void );

/*
 * Configure TIMERS for Runtime stats.
 */
void vSetupHighFrequencyTimer( void );

/*
 * Interrupt handler in which the jitter is measured.
 */
void Timer0IntHandler( void );

/*
 * The 'top' task, as described at the top of this file.
 */
static void vTopTask( void *pvParameters );

/*
 * Productor: envía su nombre a la cola.
 */
static void vProducerTask( void *pvParameters );

/*
 * Consumidor: imprime los contenidos de la cola.
 */
static void vConsumerTask( void *pvParameter );

/*
 * @brief Sends message to UART.
 * @param msg Message.
 */
static void vUARTPrint(char *msg);

/* String that is transmitted on the UART. */
static volatile char *pcNextChar;

/* The queue used to send strings to the print task for display on the LCD. */
QueueHandle_t xPrintQueue;

/* Semphore for the vUARTPrint function */
SemaphoreHandle_t xSemaphore = NULL;

/*-----------------------------------------------------------*/

int main( void )
{
	/* Configure the clocks, UART and GPIO. */
	prvSetupHardware();
	vSetupHighFrequencyTimer();

	/* Create the queue used to pass message to vConsumer. */
	xPrintQueue = xQueueCreate( mainQUEUE_SIZE, sizeof( char * ) );

	/* Create the mutex for the vUARTPrint function. */
	xSemaphore = xSemaphoreCreateMutex();

	/* Start the tasks defined within the file. */

	/* Consumer has max priority (MAX-1 = 4) */
	xTaskCreate( vConsumerTask, "C1", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL );

	xTaskCreate( vProducerTask, "P1", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 3, NULL );
	xTaskCreate( vProducerTask, "P2", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 3, NULL );
	xTaskCreate( vProducerTask, "P3", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 3, NULL );
	xTaskCreate( vProducerTask, "P4", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 3, NULL );

	xTaskCreate( vTopTask, "top", configMINIMAL_STACK_SIZE*2, NULL, configMAX_PRIORITIES - 1, NULL );

	/* Start the scheduler. */
	vTaskStartScheduler();

	/* Will only get here if there was insufficient heap to start the
	scheduler. */

	return 0;
}

/*-----------------------------------------------------------*/

static void vUARTPrint(char *msg)
{
	if( xSemaphore != NULL )
	{
		while( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) != pdTRUE );

		int i;
		int len = strlen(msg);

		for(i = 0; i < len; i++)
		{
			UARTCharPut(UART0_BASE, msg[i]);
		}

		xSemaphoreGive( xSemaphore );
	}
}

/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
	/* Setup the PLL. */
	SysCtlClockSet( SYSCTL_SYSDIV_10 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_6MHZ );
}

/*-----------------------------------------------------------*/

void vUART_ISR(void)
{
	unsigned long ulStatus;

	/* What caused the interrupt. */
	ulStatus = UARTIntStatus( UART0_BASE, pdTRUE );

	/* Clear the interrupt. */
	UARTIntClear( UART0_BASE, ulStatus );

	/* Was a Tx interrupt pending? */
	if( ulStatus & UART_INT_TX )
	{
		/* Send the next character in the string.  We are not using the FIFO. */
		if( *pcNextChar != 0 )
		{
			if( !( HWREG( UART0_BASE + UART_O_FR ) & UART_FR_TXFF ) )
			{
				HWREG( UART0_BASE + UART_O_DR ) = *pcNextChar;
			}
			pcNextChar++;
		}
	}
}

/*-----------------------------------------------------------*/

static void vConsumerTask( void *pvParameters )
{
	UBaseType_t xStatus;
	char *pcMessage;
	char *pcTaskName = pcTaskGetName( NULL );

	for( ;; )
	{
		// vTaskDelay(1000);
		/* Wait for a message to arrive. */
		if(xQueueReceive( xPrintQueue, &pcMessage, portMAX_DELAY ) == pdFALSE)
		{
			;//vUARTPrint("Empty queue.\n\r"); portMAX_DELAY
		}
		else
		{
			/* Print Consumer name -> Producer name */
			vUARTPrint(pcTaskName);
			vUARTPrint(" -> ");
			vUARTPrint(pcMessage);
			vUARTPrint("\r\n");
		}
	}
}

/*-----------------------------------------------------------*/

static void vProducerTask( void *pvParameters )
{
	UBaseType_t uxHighWaterMark;
	uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
	char *pcMessage = pcTaskGetName( NULL );

	for( ;; )
	{
		/* Send message to the queue. */
		if(xQueueSend( xPrintQueue, &pcMessage, 0 ) != pdTRUE)
		{
			;
			// vUARTPrint(pcMessage);
			// vUARTPrint(" cannot send message.\n\r");
		}

		uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
	}
}

/*-----------------------------------------------------------*/

static void vTopTask( void *pvParameters )
{
	TickType_t xLastExecutionTime;
	char *buffer = pvPortMalloc(sizeof(char));

	/* Initialise xLastExecutionTime so the first call to vTaskDelayUntil()
	works correctly. */
	xLastExecutionTime = xTaskGetTickCount();

	for( ;; )
	{
		/* Perform this check every mainCHECK_DELAY milliseconds. */
		vTaskDelayUntil( &xLastExecutionTime, mainCHECK_DELAY );

		/* Imprimir 'top' cada 5 segundos */
		vUARTPrint("\n----------------Runtime Stats-----------------\n\r");
		vUARTPrint("Task\t\tABS\t\t%CPU\n\r");
		vUARTPrint("----------------------------------------------\n\r");

		vTaskGetRunTimeStats(buffer);
		vUARTPrint(buffer);
		vUARTPrint("----------------------------------------------\n\r");

		vUARTPrint("\n------------------Task State------------------\n\r");
		vUARTPrint("Task\t\tState\tPrio\tStack\tNum\n\r");
		vUARTPrint("----------------------------------------------\n\r");
		vTaskList(buffer);
		vUARTPrint(buffer);
		vUARTPrint("----------------------------------------------\n\r");
	}
}
