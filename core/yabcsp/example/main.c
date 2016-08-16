/*******************************************************************************

				(C) COPYRIGHT Cambridge Silicon Radio

FILE
				main.c 

DESCRIPTION:
				Main program for (y)abcsp windows test program

REVISION:		$Revision: 1.1.1.1 $ by $Author: ca01 $
*******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include "uSched.h"
#include "SerialCom.h"
#include "abcsp.h"
#include "abcsp_support_functions.h"
#include "hci.h"


/* -------------------- Command line args processing -------------------- */

static unsigned long baudRate = 115200;
static char comPortString[128] = "COM1";


static void usage(char * programName)
{
	fprintf(stderr, "%s [-C <COMx>] [-B <baud rate>]\n", programName);
	fprintf(stderr, "    <COMx> COM port the Casira is attached to. Default is %s\n", comPortString);
	fprintf(stderr, "    <baud rate> the baud rate used on the Casira COM port. Default is %u\n", baudRate);
	exit(0);
}


static void passCommandLineArgs(int argc, char * argv[])
{
	int argumentIndex;
	for (argumentIndex = 1; argumentIndex < argc; argumentIndex++)		
	{
		if (argv[argumentIndex][0] == '-')
		{
			if (toupper(argv[argumentIndex][1]) == 'B')
			{
				if (argv[argumentIndex][2] == '\0')
				{
					if (++argumentIndex < argc)
					{
						if (sscanf(argv[argumentIndex], "%lu", &baudRate) != 1)
						{
							fprintf(stderr, "INVALID \"B\" ARGUMENT!\n");
							usage(argv[0]);
						}
					}
					else
					{
						fprintf(stderr, "INVALID \"B\" ARGUMENT!\n");
						usage(argv[0]);
					}
				}
				else
				{
					if (sscanf(&argv[argumentIndex][2], "%lu", &baudRate) != 1)
					{
						fprintf(stderr, "INVALID \"B\" ARGUMENT!\n");
						usage(argv[0]);
					}
				}
			}
			else if (toupper(argv[argumentIndex][1]) == 'C')
			{
				if (argv[argumentIndex][2] == '\0')
				{
					if (++argumentIndex < argc)
					{
						strcpy(comPortString, argv[argumentIndex]);
					}
					else
					{
						fprintf(stderr, "INVALID \"C\" ARGUMENT!\n");
						usage(argv[0]);
					}
				}
				else
				{
					strcpy(comPortString, &argv[argumentIndex][2]);
				}
			}
			else if ((toupper(argv[argumentIndex][1]) == 'H') || (argv[argumentIndex][1] == '?'))
			{
				usage(argv[0]);
			}
			else
			{
				fprintf(stderr, "INVALID SWITCH: \"%c\"!\n", argv[argumentIndex][1]);
				usage(argv[0]);
			}
		}
		else
		{
			fprintf(stderr, "INVALID SWITCH START, must start with \"-\"!\n");
			usage(argv[0]);
		}
	}
}


/* -------------------- The task -------------------- */


typedef struct TransmitQueueEntryStructTag
{
	unsigned channel;
	unsigned reliableFlag;
	MessageBuffer * messageBuffer;
	struct TransmitQueueEntryStructTag * nextQueueEntry;
} TransmitQueueEntry;

static TransmitQueueEntry * transmitQueue;
int NumberOfHciCommands;

void queueMessage(unsigned char channel, unsigned reliableFlag, unsigned length, unsigned char * payload)
{
	MessageBuffer * messageBuffer;

	messageBuffer = (MessageBuffer *) malloc(sizeof(MessageBuffer));
	messageBuffer->length = length;
	messageBuffer->buffer = payload;
	messageBuffer->index = 0;

	if (reliableFlag)
	{
		if (transmitQueue)
		{
			TransmitQueueEntry * searchPtr;

			for (searchPtr = transmitQueue; searchPtr->nextQueueEntry; searchPtr = searchPtr->nextQueueEntry)
			{
				;
			}
			searchPtr->nextQueueEntry = (TransmitQueueEntry *) malloc(sizeof(TransmitQueueEntry));
			searchPtr = searchPtr->nextQueueEntry;
			searchPtr->nextQueueEntry = NULL;
			searchPtr->channel = channel;
			searchPtr->reliableFlag = reliableFlag;
			searchPtr->messageBuffer = messageBuffer;
		}
		else
		{
			if (!abcsp_sendmsg(&AbcspInstanceData, messageBuffer, channel, reliableFlag))
			{
				transmitQueue = (TransmitQueueEntry *) malloc(sizeof(TransmitQueueEntry));
				transmitQueue->nextQueueEntry = NULL;
				transmitQueue->channel = channel;
				transmitQueue->reliableFlag = reliableFlag;
				transmitQueue->messageBuffer = messageBuffer;
			}
		}
	}
	else /* unreliable - just send */
	{
		abcsp_sendmsg(&AbcspInstanceData, messageBuffer, channel, reliableFlag);
	}
}

static void pumpInternalMessage(void)
{
	while (transmitQueue)
	{
		if (abcsp_sendmsg(&AbcspInstanceData, transmitQueue->messageBuffer, transmitQueue->channel, transmitQueue->reliableFlag))
		{
			TransmitQueueEntry * tmpPtr;

			tmpPtr = transmitQueue;
			transmitQueue = tmpPtr->nextQueueEntry;

			free(tmpPtr);
		}
		else
		{
			break;
		}
	}
}


void BgIntPump(void)
{
	abcsp_pumptxmsgs(&AbcspInstanceData);

	pumpInternalMessage();
}

static unsigned char cmdIssueCount;

static void initTestTask(void)
{
	NumberOfHciCommands = 0;
	cmdIssueCount = 0;
}

static void testTask(void)
{
	unsigned char * readBdAddr;

	cmdIssueCount++;
	if (cmdIssueCount >= 100)
	{
		printf(".");
		cmdIssueCount = 0;
	}

	if (NumberOfHciCommands > 0)
	{
		NumberOfHciCommands--;

		readBdAddr = malloc(3);
		readBdAddr[0] = (unsigned char) ((HCI_COMMAND_READ_BD_ADDR) & 0x00FF);
		readBdAddr[1] = (unsigned char) (((HCI_COMMAND_READ_BD_ADDR) >> 8) & 0x00FF);
		readBdAddr[2] = 0;
		queueMessage(HCI_COMMAND_CHANNEL, 1, 3, readBdAddr);
	}
}


/* -------------------- The keyboard handler -------------------- */

#define KEYBOARD_SCAN_INTERVAL	250000
#define ESC_KEY					0x1B

static void keyboardHandler(void)
{
	if (_kbhit())
	{
		switch (getch())
		{
			case ESC_KEY:
				printf("\nUser exit...\n");
				TerminateMicroSched();
				break;

			default:
				break;
		}
	}
	StartTimer(KEYBOARD_SCAN_INTERVAL, keyboardHandler);
}


/* -------------------- MAIN -------------------- */

int main(int argc, char * argv[])
{
	InitMicroSched(initTestTask, testTask);

	passCommandLineArgs(argc, argv);

	UartDrv_RegisterHandlers();

	abcsp_init(&AbcspInstanceData);

	UartDrv_Configure(baudRate, comPortString);

	if (!UartDrv_Start())
	{
		printf("UartDrv_Start() failed! Exiting...\n");

		/* clean-up */
		TerminateMicroSched();
		MicroSched();
	}
	else
	{
		StartTimer(KEYBOARD_SCAN_INTERVAL, keyboardHandler);
		MicroSched();
		UartDrv_Stop();
	}

	return 0;
}
