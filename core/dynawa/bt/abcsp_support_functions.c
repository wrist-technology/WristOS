#include <stdio.h>
#include <stdlib.h>
#include "uSched.h"
#include "SerialCom.h"
#include "abcsp_support_functions.h"
#include "hci.h"
#include "lwbt/hci.h"
#include "bccmd.h"
#include "debug/trace.h"
#include "lwip/pbuf.h"
#include "bt.h"

abcsp AbcspInstanceData;

#define BUG   0
#define BCSP_TEST   0

extern uint32_t bc_hci_event_count;

extern int NumberOfHciCommands;
extern uint16 bc_state;

/* Used as defines in config_rxmsg.h */

static unsigned char cmdCompleteCount = 0;

void buf2pbuf(u8_t *b, struct pbuf *p, int hdr_len, int param_len) {
    TRACE_BT("buf2pbuf %x %x\r\n", p->payload, b);
    memcpy(p->payload, b, hdr_len);
    b += hdr_len;
    pbuf_header(p, -hdr_len);

    int rem_len = param_len;
    while (rem_len) {
        if (p == NULL) {
            TRACE_ERROR("PBUF=NULL\r\n");
            panic("buf2pbuf");
            return;
        }
        int chunk_len = p->len;
        TRACE_BT("pbuf len %d\r\n", chunk_len); 
        int n = rem_len > chunk_len ? chunk_len : rem_len;
        memcpy(p->payload, b, n);
        b += n;
        rem_len -= n;
        p = p->next;
    }
}

void abcsp_delivermsg(abcsp * thisInstance, ABCSP_RXMSG * message, unsigned channel, unsigned reliableFlag)
{
    (void)thisInstance;
    (void)reliableFlag;
	MessageBuffer * messageBuffer;

    TRACE_BT("abcsp_delivermsg %d\r\n", channel);

#if _BT_LED
    ledrgb_set(0x4, 0, 0, BT_LED_HIGH);
#endif

	messageBuffer = (MessageBuffer *) message;
	if (channel == BCCMD_CHANNEL)
    {
        uint16 *cmd = (uint16 *)(messageBuffer->buffer);
        TRACE_BT("BCCMD_CHANNEL\r\n");
        int i;
        for (i = 0; i < 9; i++) {
            TRACE_BT("%x\r\n", cmd[i]);
        }
        if (cmd[0] == BCCMDPDU_GETRESP) {
            if (bt_is_ps_set()) {      
                bc_state = BC_STATE_PS_SET;
            }
        }
        NumberOfHciCommands = 1;
        ScheduleTaskToRun();
    } else if (channel == 3) {
        TRACE_BT("HQ_CHANNEL\r\n");
    } else if (channel == 4) {
        TRACE_BT("DEVICE_MGT_CHANNEL\r\n");
    }
	else if (channel == HCI_EVENT_CHANNEL)
	{
        TRACE_BT("HCI_EVENT_CHANNEL %x\r\n", (messageBuffer->buffer[0]));
        if (!BCSP_TEST && bc_state == BC_STATE_READY) {
            TRACE_BT(">>>>> bc_hci_event_count %d\r\n", bc_hci_event_count);
            //if (bc_hci_event_count == 9)
            if (BUG && bc_hci_event_count == 5) {
                panic("abcsp_delivermsg");
            }
            bc_hci_event_count++;
/*
            if (messageBuffer->buffer[0] == HCI_COMMAND_STATUS_EVENT) {
                int cmd = ((((uint16) messageBuffer->buffer[HCI_CSE_COMMAND_OPCODE_HIGH_BYTE]) << 8)
                     | ((uint16) messageBuffer->buffer[HCI_CSE_COMMAND_OPCODE_LOW_BYTE]));
                int NumberOfHciCommands = messageBuffer->buffer[HCI_CSE_NUM_HCI_COMMAND_PACKETS_OFFSET];
                //TRACE_BT("HCI_COMMAND_STATUS_EVENT %d %d %d %x\r\n", messageBuffer->buffer[1], messageBuffer->buffer[2], NumberOfHciCommands, cmd);
                TRACE_BT("HCI_COMMAND_STATUS_EVENT %x %x %x %x %x\r\n", messageBuffer->buffer[1], messageBuffer->buffer[2], messageBuffer->buffer[3], messageBuffer->buffer[4], messageBuffer->buffer[5]);
            } else if (messageBuffer->buffer[0] == HCI_COMMAND_COMPLETE_EVENT) {
                int len = messageBuffer->buffer[1] + HCI_EVENT_HDR_LEN;
                int i;
                TRACE_BT("HCI_COMMAND_COMPLETE_EVENT\r\n");
                for(i = 0 ; i < len; i++) {
                    TRACE_BT("%d=%x\r\n", i, messageBuffer->buffer[i]);
                }
            }
*/
            u8_t param_len = messageBuffer->buffer[1];
            int len = HCI_EVENT_HDR_LEN + param_len;
            TRACE_BT("total_len %d\r\n", len);
            struct pbuf *p;
            //if((p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL)) == NULL) {
            if((p = pbuf_alloc(PBUF_RAW, len, PBUF_RAM)) == NULL) {
                TRACE_ERROR("NOMEM\r\n");
                panic("abcsp_delivermsg 1");
            } else {
                buf2pbuf(messageBuffer->buffer, p, HCI_EVENT_HDR_LEN, param_len);
                hci_event_input(p);
                pbuf_free(p);

                TRACE_BT("<<<<< bc_hci_event_count %d\r\n", bc_hci_event_count);
                if (BUG && bc_hci_event_count == 10) {
                    panic("abcsp_delivermsg");
                }
            }
        } else {
            if (messageBuffer->buffer[0] == HCI_COMMAND_COMPLETE_EVENT)
            {
                int cmd = ((((uint16) messageBuffer->buffer[HCI_CCE_COMMAND_OPCODE_HIGH_BYTE]) << 8)
                     | ((uint16) messageBuffer->buffer[HCI_CCE_COMMAND_OPCODE_LOW_BYTE]));
                TRACE_BT("HCI_COMMAND_COMPLETE_EVENT %x\r\n", cmd);
                NumberOfHciCommands = messageBuffer->buffer[HCI_CCE_NUM_HCI_COMMAND_PACKETS_OFFSET];
                if (((((uint16) messageBuffer->buffer[HCI_CCE_COMMAND_OPCODE_HIGH_BYTE]) << 8)
                     | ((uint16) messageBuffer->buffer[HCI_CCE_COMMAND_OPCODE_LOW_BYTE]))
                    == HCI_COMMAND_READ_BD_ADDR)
                {
                    cmdCompleteCount++;
                    if (cmdCompleteCount >= 100)
                    {
                        //printf("*");
                        TRACE_INFO("*");
                        cmdCompleteCount = 0;
                    }
                }
            }
            else if (messageBuffer->buffer[0] == HCI_COMMAND_STATUS_EVENT)
            {
                int cmd = ((((uint16) messageBuffer->buffer[HCI_CSE_COMMAND_OPCODE_HIGH_BYTE]) << 8)
                     | ((uint16) messageBuffer->buffer[HCI_CSE_COMMAND_OPCODE_LOW_BYTE]));
                NumberOfHciCommands = messageBuffer->buffer[HCI_CSE_NUM_HCI_COMMAND_PACKETS_OFFSET];
                TRACE_BT("HCI_COMMAND_STATUS_EVENT %d %d %d %x\r\n", messageBuffer->buffer[1], messageBuffer->buffer[2], NumberOfHciCommands, cmd);
            }
            else if (messageBuffer->buffer[0] == HCI_HARDWARE_ERROR_EVENT)
            {
                TRACE_BT("HCI_HARDWARE_ERROR_EVENT %d\r\n", messageBuffer->buffer[2]);
            }

            if (NumberOfHciCommands)
            {
                ScheduleTaskToRun();
            }
        }
	}
	else if (channel == HCI_ACL_CHANNEL)
	{
        TRACE_BT("HCI_ACL_CHANNEL\r\n");
        if (bc_state == BC_STATE_READY) {
            u16_t param_len = U16LE2CPU((u8_t*)(messageBuffer->buffer)+2);
            int len = HCI_ACL_HDR_LEN + param_len;
            TRACE_BT("total_len %d\r\n", len);
            int i;
/*
            for(i = 0 ; i < len; i++) {
                TRACE_BT("%d=%x\r\n", i, messageBuffer->buffer[i]);
            }
*/
            struct pbuf *p;
            //if((p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL)) == NULL) {
            if((p = pbuf_alloc(PBUF_RAW, len, PBUF_RAM)) == NULL) {
                TRACE_ERROR("NOMEM\r\n");
                panic("abcsp_delivermsg 2");
            } else {
                buf2pbuf(messageBuffer->buffer, p, HCI_ACL_HDR_LEN, param_len);
                hci_acl_input(p);
                //pbuf_free(p);    // done by hci_acl_input()/l2cap_input()
            }
        }
    } else {
        TRACE_ERROR("Unknown channel %x\r\n", channel);
    }
	free(messageBuffer->buffer);
	free(messageBuffer);

#if _BT_LED
    ledrgb_set(0x4, 0, 0, BT_LED_LOW);
#endif
}

ABCSP_RXMSG * abcsp_rxmsg_create(abcsp * thisInstance, unsigned length)
{
    (void)thisInstance;
	MessageBuffer * messageBuffer;

    TRACE_BT("abcsp_rxmsg_create\r\n");
	messageBuffer = malloc(sizeof(MessageBuffer));
	messageBuffer->length = length;
	messageBuffer->index = 0;
	messageBuffer->buffer = (unsigned char *) malloc(length);

	return (ABCSP_RXMSG *) messageBuffer;
}

char * abcsp_rxmsg_getbuf(ABCSP_RXMSG * message, unsigned * bufferSize)
{
    // MV (void)bufferSize;
	MessageBuffer * messageBuffer;

    TRACE_BT("abcsp_rxmsg_getbuf\r\n");
	messageBuffer = (MessageBuffer *) message;

    // MV
    *bufferSize = messageBuffer->length - messageBuffer->index;
	return (char *) &messageBuffer->buffer[messageBuffer->index];
}

void abcsp_rxmsg_write(ABCSP_RXMSG * message, char * buffer, unsigned number)
{
    (void)buffer;
	MessageBuffer * messageBuffer;

    TRACE_BT("abcsp_rxmsg_write\r\n");
	messageBuffer = (MessageBuffer *) message;
	messageBuffer->index += number;
}

void abcsp_rxmsg_complete(abcsp * thisInstance, ABCSP_RXMSG * message)
{
    (void)thisInstance;
    (void)message;
    TRACE_BT("abcsp_rxmsg_complete\r\n");
}

void abcsp_rxmsg_destroy(abcsp * thisInstance, ABCSP_RXMSG * message)
{
    (void)thisInstance;
	MessageBuffer * messageBuffer;

    TRACE_BT("abcsp_rxmsg_destroy\r\n");
	messageBuffer = (MessageBuffer *) message;

	free(messageBuffer->buffer);
	free(messageBuffer);
}


/* Used as defines in config_event.h */

static char eventStrings[][48] =
{
	"ABCSP_EVT_NULL?!?!",
	"ABCSP_EVT_START",
	"ABCSP_EVT_INITED",
	"ABCSP_EVT_LE_SYNC",
	"ABCSP_EVT_LE_CONF",
	"ABCSP_EVT_LE_SYNC_LOST",
	"ABCSP_EVT_UNINITED",
	"ABCSP_EVT_SLIP_SYNC",
	"ABCSP_EVT_SLIP_SYNC_LOST",
	"ABCSP_EVT_RX_CHOKE_DISCARD",
	"ABCSP_EVT_TX_CHOKE_DISCARD",
	"ABCSP_EVT_TX_WINDOW_FULL_DISCARD",
	"ABCSP_EVT_OVERSIZE_DISCARD",
	"ABCSP_EVT_MISSEQ_DISCARD",
	"ABCSP_EVT_CHECKSUM",
	"ABCSP_EVT_SHORT_PAYLOAD",
	"ABCSP_EVT_OVERRUN",
	"ABCSP_EVT_CRC_FAIL",
	"ABCSP_EVT_MALLOC"
};

void abcsp_event(abcsp * thisInstance, unsigned event)
{
    (void)thisInstance;
    TRACE_INFO("abcsp_event %d %s\r\n", event, eventStrings[event]);
    //TRACE_BT("abcsp_event %d\r\n", event);
	if (event == ABCSP_EVT_LE_SYNC_LOST)
	{
        TRACE_ERROR("SYNC_LOST: %s\r\n", eventStrings[event]);
/*
		printf("\nabcsp_event: %s\n\n", eventStrings[event]);
		printf("The BCSP Link Establishment engine has detected that the peer BCSP-LE\n");
		printf("engine has restarted.  This presumably means that the peer BCSP stack\n");
		printf("(or system) has been restarted.  A common local response would be to\n");
		printf("restart (abcsp_init()) the local BCSP stack.\n");
		printf("\nThis example exits the program! Please, wait...\n");
*/

		//TerminateMicroSched();
        abcsp_init(thisInstance);
	}
}

void abcsp_req_pumptxmsgs(abcsp * thisInstance)
{
    (void)thisInstance;
    TRACE_BT("abcsp_req_pumptxmsgs\r\n");
	bg_int2();
}


/* Used as defines in config_timer.h */

static void * bcspTimerHandle = NULL;
static void * tshyTimerHandle = NULL;
static void * confTimerHandle = NULL;

static void bcspTimeout(void)
{
    TRACE_INFO("bcspTimeout\r\n");
	bcspTimerHandle = NULL;
	abcsp_bcsp_timed_event(&AbcspInstanceData);
}

static void tshyTimeout(void)
{
    TRACE_INFO("tshyTimeout\r\n");
	tshyTimerHandle = NULL;
	abcsp_tshy_timed_event(&AbcspInstanceData);
}

static void confTimeout(void)
{
    TRACE_INFO("confTimeout\r\n");
	confTimerHandle = NULL;
	abcsp_tconf_timed_event(&AbcspInstanceData);
}

void abcsp_start_bcsp_timer(abcsp * thisInstance)
{
    (void)thisInstance;
    TRACE_BT("abcsp_start_bcsp_timer\r\n");
	if (bcspTimerHandle != NULL)
	{
		StopTimer(bcspTimerHandle);
	}
	bcspTimerHandle = StartTimer(250000, bcspTimeout);
}

void abcsp_start_tshy_timer(abcsp * thisInstance)
{
    (void)thisInstance;
    TRACE_BT("abcsp_start_tshy_timer\r\n");
	if (tshyTimerHandle != NULL)
	{
		StopTimer(tshyTimerHandle);
	}
	tshyTimerHandle = StartTimer(250000, tshyTimeout);
}

void abcsp_start_tconf_timer(abcsp * thisInstance)
{
    (void)thisInstance;
    TRACE_BT("abcsp_start_tconf_timer\r\n");
	if (confTimerHandle != NULL)
	{
		StopTimer(confTimerHandle);
	}
	confTimerHandle = StartTimer(250000, confTimeout);
}

void abcsp_cancel_bcsp_timer(abcsp * thisInstance)
{
    (void)thisInstance;
    TRACE_BT("abcsp_cancel_bcsp_timer\r\n");
	StopTimer(bcspTimerHandle);
	bcspTimerHandle = NULL;
}

void abcsp_cancel_tshy_timer(abcsp * thisInstance)
{
    (void)thisInstance;
    TRACE_BT("abcsp_cancel_tshy_timer\r\n");
	StopTimer(tshyTimerHandle);
	tshyTimerHandle = NULL;
}

void abcsp_cancel_tconf_timer(abcsp * thisInstance)
{
    (void)thisInstance;
    TRACE_BT("abcsp_cancel_tconf_timer\r\n");
	StopTimer(confTimerHandle);
	confTimerHandle = NULL;
}


/* Used as defines in config_panic.h */

void abcsp_panic(abcsp * thisInstance, unsigned panicCode)
{
    (void)thisInstance;
	//printf("abcsp_panic: %u\n", panicCode);
	TRACE_ERROR("abcsp_panic: %u\r\n", panicCode);
    panic("abcsp_panic");
}


/* Used as defines in config_txmsg.h */

void abcsp_uart_sendbytes(abcsp * thisInstance, unsigned number)
{
	uint16_t num_send;
	uint8_t * buffer = thisInstance->txslip.ubuf;
    TRACE_BT("abcsp_uart_sendbytes %d\r\n", number);

	if (!UartDrv_Tx(buffer, number , &num_send))
	{
		//printf("Tx buffer overrun");
		TRACE_ERROR("Tx buffer overrun\r\n");
	}
}

void abcsp_txmsg_init_read(ABCSP_TXMSG * message)
{
	MessageBuffer * messageBuffer;
    TRACE_BT("abcsp_txmsg_init_read %x\r\n", message);

	messageBuffer = (MessageBuffer *) message;
	messageBuffer->index = 0;
}

unsigned abcsp_txmsg_length(ABCSP_TXMSG * message)
{
	MessageBuffer * messageBuffer;
    TRACE_BT("abcsp_txmsg_length\r\n");

	messageBuffer = (MessageBuffer *) message;
	return messageBuffer->length;
}

char * abcsp_txmsg_getbuf(ABCSP_TXMSG * message, unsigned * bufferLength)
{
	MessageBuffer * messageBuffer;

    TRACE_BT("abcsp_txmsg_getbuf\r\n");
	messageBuffer = (MessageBuffer *) message;
	if (messageBuffer->index >= messageBuffer->length)
	{
		*bufferLength = 0;
		return NULL;
	}
	else
	{
		*bufferLength = messageBuffer->length - messageBuffer->index;
		return &messageBuffer->buffer[messageBuffer->index];
	}
}

void abcsp_txmsg_taken(ABCSP_TXMSG * message, unsigned numberTaken)
{
	MessageBuffer * messageBuffer;

    TRACE_BT("abcsp_txmsg_taken\r\n");
	messageBuffer = (MessageBuffer *) message;
	messageBuffer->index += numberTaken;
}

void abcsp_txmsg_done(abcsp * thisInstance, ABCSP_TXMSG * message)
{
    (void)thisInstance;
	MessageBuffer * messageBuffer;

    TRACE_BT("abcsp_txmsg_done\r\n");
	messageBuffer = (MessageBuffer *) message;

	free(messageBuffer->buffer);
	free(messageBuffer);
}
