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
//#include <conio.h>
#include "uSched.h"
#include "SerialCom.h"
#include "abcsp.h"
#include "abcsp_support_functions.h"
#include "hci.h"
#include "bccmd.h"
#include "rtos.h"

#include "lwip/pbuf.h"
#include "lwip/mem.h"
#include "lwip/sys.h"
#include "lwbt/hci.h"
#include "lwbt/rfcomm.h"
#include "bt.h"
#include "event.h"
#include "io.h"
#include <ff.h>

#include "debug/trace.h"

/*
// PSKEY_BDADDR
&0001 = 0000 a5a5 005b 0002

human readable
00:02:5b:00:a5:a5

00:02:5b - Cambridge Silicon
*/

#define SET_BDADDR                  1
#define SET_HOST_UART_HW_FLOW       0
#define SET_HOST_WAKE_UART_BREAK    0
#define SET_HOST_WAKE_PIO           1

#define SET_TX_POWER        0
#define TX_POWER            2

#define BT_COMMAND_QUEUE_LEN 10

xSemaphoreHandle bt_wakeup_semaphore;
xQueueHandle bt_wakeup_queue;

uint32_t bc_hci_event_count;
static Task bt_task_handle;
static xQueueHandle command_queue;
static xQueueHandle command_result_queue;
static Io bt_io;

static unsigned int bt_open_count = 0;

static unsigned long baudRate = 460800;
//static unsigned long baudRate = 115200;
//static unsigned long baudRate = 38400;

//#define USART_BAUDRATE_38400
//#define USART_BAUDRATE_115200
//#define USART_BAUDRATE_230400
//#define USART_BAUDRATE_460800
#define USART_BAUDRATE_921600

#if defined(USART_BAUDRATE_38400)
#define USART_BAUDRATE      38400
#define USART_BAUDRATE_CD   0x009d
#elif defined(USART_BAUDRATE_115200)
#define USART_BAUDRATE      115200
#define USART_BAUDRATE_CD   0x01d8
#elif defined(USART_BAUDRATE_230400)
#define USART_BAUDRATE      230400
#define USART_BAUDRATE_CD   0x03b0
#elif defined(USART_BAUDRATE_460800)
#define USART_BAUDRATE      460800
#define USART_BAUDRATE_CD    0x075f
#elif defined(USART_BAUDRATE_921600)
#define USART_BAUDRATE      921600
#define USART_BAUDRATE_CD    0x0ebf
#endif

static struct bd_addr this_device_bdaddr = {0xa5, 0xa5, 0x00, 0x5b, 0x02, 0x00};

uint16_t bt_ps_set_bdaddr(uint16_t index) {
    uint16_t w;
    switch(index) {
        case 2:
        case 3:
            w = this_device_bdaddr.addr[index];
            break;
        default:
            w = this_device_bdaddr.addr[index] | (this_device_bdaddr.addr[index + 1] << 8);
            break;
    }
    return w;
}

static ps_setrq_count;
static struct bccmd_index_value {
    uint16_t index, (*func_value)(), value;
} ps_setrq[] = {
    {0, NULL, 9},
    {5, NULL, PSKEY_ANAFREQ},
    {6, NULL, 1},
    {8, NULL, 0x6590},

    {0, NULL, 9},
    {1, NULL, 9},
    {5, NULL, PSKEY_BAUDRATE},
    {6, NULL, 1},
    {8, NULL, USART_BAUDRATE_CD},
#if SET_BDADDR
    {0, NULL, 12},
    {5, NULL, PSKEY_BDADDR},
    {6, NULL, 4},
    {8, bt_ps_set_bdaddr, 2},    // 0xaabb},       // bdaddr PSKEY is 8bytes long!
    {9, bt_ps_set_bdaddr, 0},   // 0102
    {10, bt_ps_set_bdaddr, 3},  // 0304
    {11, bt_ps_set_bdaddr, 4},  // 0506
#endif
#if SET_HOST_UART_HW_FLOW
    {0, NULL, 9},
    {5, NULL, PSKEY_UART_CONFIG_BCSP},
    {6, NULL, 1},
    {8, NULL, 0x0802}, // hw flow on (default 0x0806)
#endif

#if SET_HOST_WAKE_UART_BREAK
    {0, NULL, 9},
    {5, NULL, PSKEY_UART_HOST_WAKE_SIGNAL},
    {6, NULL, 1},
    {8, NULL, 3}, /*  
        bit 0 - 3
        0 - repeated byte sequence (only for H4DS)
        1 - positive pulse on PIO
        2 - negative pulse on PIO
        3 - enable UART BREAK

        bit 4 - 7
            0 => PIO[0] 
            1 => PIO[1] 
            ...
            */
    {0, NULL, 12},
    {5, NULL, PSKEY_UART_HOST_WAKE},
    {6, NULL, 4},     
    {8, NULL, 0x0001},    // 1 enable, 4 - disable
    {9, NULL, 0x01f4},    /* Sleep_Delay = 500ms
        Sleep_Delay: Milliseconds after tx to host or rx from host,
after which host will be assumed to have gone into
deep sleep state. (Range 1 -> 65535)
When using BCSP or H5 host transports it is
recommended that this is greater than the
acknowledge delay (set by PSKEY_UART_ACK_TIMEOUT)
                    */

    {10, NULL, 0x0001},   /* Break_Length = 5ms (1 - 1000)
Duration of wake signal in milliseconds (Range 1 -> 1000)
                    */
    {11, NULL, 0x0020},   /* Pause_Length = 32ms (0 - 1000)
Pause_Length: Milliseconds between end of wake signal and sending data
to the host. (Range 0 -> 1000.)
                    */

#elif SET_HOST_WAKE_PIO
    {0, NULL, 9},
    {5, NULL, PSKEY_UART_HOST_WAKE_SIGNAL},
    {6, NULL, 1},
    {8, NULL, 0x1}, // enable PIO #0 POSITIVE EDGE (pppp0001)

    {0, NULL, 12},
    {5, NULL, PSKEY_UART_HOST_WAKE},
    {6, NULL, 4},
    {8, NULL, 0x0001},    // 1 enable, 4 - disable
    //{9, NULL, 0x01f4},    // sleep timeout = 500ms
    {9, NULL, BT_HOST_WAKE_SLEEP_TIMEOUT},    // sleep timeout = 500ms
    //{10, NULL, 0x0005},   // break len = 5ms
    {10, NULL, BT_HOST_WAKE_BREAK_LEN},   // break len = 5ms
    //{11, NULL, 0x0020},   // pause length = 32ms
    {11, NULL, BT_HOST_WAKE_PAUSE_LEN},   // pause length = 32ms
#endif

#if SET_TX_POWER
    {0, NULL, 9},
    {5, NULL, PSKEY_LC_MAX_TX_POWER},
    {6, NULL, 1},
    {8, NULL, TX_POWER},

    {0, NULL, 9},
    {5, NULL, PSKEY_LC_DEFAULT_TX_POWER},
    {6, NULL, 1},
    {8, NULL, TX_POWER},

    {0, NULL, 9},
    {5, NULL, PSKEY_LC_MAX_TX_POWER_NO_RSSI},
    {6, NULL, 1},
    {8, NULL, TX_POWER},
#endif
    {0, NULL, 0}
};

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
// MV messageBuffer a messageBuffer->buffer freed later by abcsp_txmsg_done()
    TRACE_BT("queueMessage\r\n");
	messageBuffer = (MessageBuffer *) malloc(sizeof(MessageBuffer));
	messageBuffer->length = length;
	messageBuffer->buffer = payload;
	messageBuffer->index = 0;

	if (reliableFlag)
	{
        TRACE_BT("reliable flag on\r\n");
		if (transmitQueue)
		{
			TransmitQueueEntry * searchPtr;

            TRACE_BT("Message queued\r\n");
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
            TRACE_BT("abcsp_sendmsg\r\n");
			if (!abcsp_sendmsg(&AbcspInstanceData, messageBuffer, channel, reliableFlag))
			{
                TRACE_BT("Message not delivered, queued\r\n");
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
    TRACE_BT("pumpInternalMessage\r\n");
	while (transmitQueue)
	{
        TRACE_BT("abcsp_sendmsg\r\n");
		if (abcsp_sendmsg(&AbcspInstanceData, transmitQueue->messageBuffer, transmitQueue->channel, transmitQueue->reliableFlag))
		{
            TRACE_BT("sent. removed from queue\r\n");
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

void phybusif_output(struct pbuf *p, u16_t len)
{
    /* Send pbuf on UART */
    //LWIP_DEBUGF(PHYBUSIF_DEBUG, ("phybusif_output: Send pbuf on UART\n"));
    
    unsigned char *t = p->payload;
    TRACE_BT("phybusif_output %d %d %d\r\n", len, t[0], t[1]);

    int channel;
    switch (t[0]) {
    case HCI_COMMAND_DATA_PACKET:
        channel = HCI_COMMAND_CHANNEL;
        break;
    case HCI_ACL_DATA_PACKET:
        channel = HCI_ACL_CHANNEL;
        break;
    default:
        TRACE_ERROR("Unknown packet type\r\n");
    }

    len--;
    u8_t *msg = malloc(len);
    if (msg == NULL) {
        TRACE_ERROR("NOMEM\r\n");
        panic("phybusif_output 1");
        return;
    }
    // TODO: pbuf2buf()
    int remain = len;
    struct pbuf *q = p;
    u8_t *b = msg;
    int count = 0;
    while (remain) {
        if (q == NULL) {
            TRACE_ERROR("PBUF=NULL\r\n");
            panic("phybusif_output 2");
            return;
        }
        int offset = count ? 0 : 1; // to ignore payload[0] = packet type

        int chunk_len = q->len - offset;
        TRACE_BT("pbuf len %d\r\n", chunk_len);
        int n = remain > chunk_len ? chunk_len : remain;
        memcpy(b, q->payload + offset, n);
        b += n;
        remain -= n;
        q = q->next;
        count++;
    }
{
/*
    int cmd = ((((uint16) msg[HCI_CSE_COMMAND_OPCODE_HIGH_BYTE]) << 8)
                     | ((uint16) msg[HCI_CSE_COMMAND_OPCODE_LOW_BYTE]));
*/
    int i;
    for(i = 0; i < len; i++) {
        TRACE_BT("%d=%x\r\n", i, msg[i]);
    }
}
    queueMessage(channel, 1, len, msg);
}

void BgIntPump(void)
{
    TRACE_BT("BgIntPump\r\n");

    int more_todo;
// loop by MV
    do {
TRACE_BT("abcsp_pumptxmsgs\r\n");
	    more_todo = abcsp_pumptxmsgs(&AbcspInstanceData);

	    pumpInternalMessage();
    } while (more_todo);
    // MV } while (0);
}

err_t bt_rbd_complete(void *arg, struct bd_addr *bdaddr) {
    TRACE_INFO("bt_rbd_complete %02x:%02x:%02x:%02x:%02x:%02x\r\n", \
            (bdaddr)->addr[0], \
            (bdaddr)->addr[1], \
            (bdaddr)->addr[2], \
            (bdaddr)->addr[3], \
            (bdaddr)->addr[4], \
            (bdaddr)->addr[5]);
    return ERR_OK;
}

static unsigned char cmdIssueCount;

static void u_init_bt_task(void)
{
    TRACE_BT("u_init_bt_task\r\n");
	NumberOfHciCommands = 0;
	cmdIssueCount = 0;
}

static void pumpHandler(void);
static void restartHandler(void);
#define PUMP_INTERVAL	        1000000
#define BT_INTERVAL	            1000000

#if !defined(TCP_TMR_INTERVAL)
#define TCP_TMR_INTERVAL        250
#endif
#define TCP_INTERVAL	        (TCP_TMR_INTERVAL * 1000)

uint16 bc_state = BC_STATE_STOPPED;

static void u_bt_task(bt_command *cmd)
{
	unsigned char * readBdAddr;

    TRACE_BT("u_bt_task begin\r\n");

    if (cmd) {
        switch(cmd->id) {
        case BT_COMMAND_STOP:
            // TODO: stop all BT connections
            TerminateMicroSched();
            break;
        case BT_COMMAND_SEND:
            {
                TRACE_INFO("BT_COMMAND_SEND\r\n");
                bt_socket *sock = cmd->sock;
                struct pbuf *p = cmd->param.ptr;

                _bt_rfcomm_send(sock, p);

                pbuf_free(p);
            }
            break;
        case BT_COMMAND_SET_LINK_KEY:
            {
                TRACE_INFO("BT_COMMAND_SET_LINK_KEY\r\n");
                struct bt_bdaddr_link_key *bdaddr_link_key = cmd->param.ptr;

                hci_write_stored_link_key(&bdaddr_link_key->bdaddr, &bdaddr_link_key->link_key);

                free(bdaddr_link_key);
            }
            break;
        case BT_COMMAND_LINK_KEY_REQ_REPLY:
            {
                TRACE_INFO("BT_COMMAND_LINK_KEY_REQ_REPLY\r\n");
                struct bt_bdaddr_link_key *bdaddr_link_key = cmd->param.ptr;

                hci_link_key_request_reply(&bdaddr_link_key->bdaddr, &bdaddr_link_key->link_key);

                free(bdaddr_link_key);
            }
            break;
        case BT_COMMAND_LINK_KEY_REQ_NEG_REPLY:
            {
                TRACE_INFO("BT_COMMAND_LINK_KEY_REQ_NEG_REPLY\r\n");
                struct bd_addr *bdaddr = cmd->param.ptr;

                hci_link_key_request_neg_reply(bdaddr);

                free(bdaddr);
            }
            break;
        case BT_COMMAND_RFCOMM_LISTEN:
            {
                TRACE_INFO("BT_COMMAND_RFCOMM_LISTEN\r\n");
                bt_socket *sock = cmd->sock;
                bt_command_result res;

                res.error = _bt_rfcomm_listen(sock, cmd->param.cn);

                bt_set_command_result(&res);
            }
            break;
        case BT_COMMAND_RFCOMM_CONNECT:
            {
                TRACE_INFO("BT_COMMAND_RFCOMM_CONNECT\r\n");
                bt_socket *sock = cmd->sock;
                struct bt_bdaddr_cn *bdaddr_cn = cmd->param.ptr;
                bt_command_result res;

                res.error = _bt_rfcomm_connect(sock, &bdaddr_cn->bdaddr, bdaddr_cn->cn);

                free(bdaddr_cn);
                bt_set_command_result(&res);
            }
            break;
        case BT_COMMAND_FIND_SERVICE:
            {
                TRACE_INFO("BT_COMMAND_FIND_SERVICE\r\n");
                bt_socket *sock = cmd->sock;
                struct bd_addr *bdaddr = cmd->param.ptr;

                sock->current_cmd = cmd->id;
                _bt_find_service(sock, bdaddr);

                free(bdaddr);
            }
            break;
        case BT_COMMAND_INQUIRY:
            {
                TRACE_INFO("BT_COMMAND_INQUIRY\r\n");
                _bt_inquiry();
            }
            break;
        case BT_COMMAND_REMOTE_NAME_REQ:
            {
                TRACE_INFO("BT_COMMAND_REMOTE_NAME_REQ\r\n");
                struct bd_addr *bdaddr = cmd->param.ptr;

                _bt_remote_name_req(bdaddr);

                free(bdaddr);
            }
            break;
        case BT_COMMAND_ADVERTISE_SERVICE:
            {
                TRACE_INFO("BT_COMMAND_ADVERTISE_SERVICE\r\n");
                bt_socket *sock = cmd->sock;
                bt_command_result res;

                res.error = _bt_advertise_service(sock, cmd->param.service.record, cmd->param.service.len);
                bt_set_command_result(&res);
            }
            break;
        }
    }

	cmdIssueCount++;
	if (cmdIssueCount >= 100)
	{
		//printf(".");
		TRACE_INFO(".");
		cmdIssueCount = 0;
	}

	if (NumberOfHciCommands > 0)
	{
        uint16_t *bccmd;

		NumberOfHciCommands--;

        switch(bc_state) {
            case BC_STATE_READY:
/*
                readBdAddr = malloc(3);
                readBdAddr[0] = (unsigned char) ((HCI_COMMAND_READ_BD_ADDR) & 0x00FF);
                readBdAddr[1] = (unsigned char) (((HCI_COMMAND_READ_BD_ADDR) >> 8) & 0x00FF);
                readBdAddr[2] = 0;
                queueMessage(HCI_COMMAND_CHANNEL, 1, 3, readBdAddr);
*/
                hci_read_bd_addr(bt_rbd_complete);
                break;
            case BC_STATE_STARTED:
                {
                    uint16 *bccmd = NULL;
                    uint16_t len, size;
                    while(1) {
                        struct bccmd_index_value *ps_setrq_value = &ps_setrq[ps_setrq_count];        
                        if (ps_setrq_value->index) {
                            uint16_t value;
                            if (ps_setrq_value->func_value) {
                                bccmd[ps_setrq_value->index] = (*ps_setrq_value->func_value)(ps_setrq_value->value);
                            } else {
                                bccmd[ps_setrq_value->index] = ps_setrq_value->value;
                            }
                            ps_setrq_count++;
                        } else if (bccmd) {
                            queueMessage(BCCMD_CHANNEL, 1, size, bccmd);
                            break;
                        } else { 
                            len = ps_setrq_value->value;
                            size = sizeof(uint16_t) * len;
                            bccmd = malloc(size);
                            if (bccmd == NULL) {
                                panic("u_bt_task 1");
                            }
                            memset(bccmd, 0, size);

                            bccmd[0] = BCCMDPDU_SETREQ;
                            bccmd[1] = len;         // number of uint16s in PDU
                            bccmd[2] = ps_setrq_count;    // value choosen by host
                            bccmd[3] = BCCMDVARID_PS;
                            bccmd[4] = BCCMDPDU_STAT_OK;

                            ps_setrq_count++;
                        }
                    }
                }
                break;
            case BC_STATE_PS_SET:
                bccmd = malloc(sizeof(uint16) * 9);
                //bccmd[0] = 0;         // BCCMDPDU_GETREQ
                bccmd[0] = BCCMDPDU_SETREQ;
                bccmd[1] = 9;         // number of uint16s in PDU
                bccmd[2] = 3;    // value choosen by host
                //bccmd[3] = BCCMDVARID_CHIPVER;
                bccmd[3] = BCCMDVARID_WARM_RESET;
                bccmd[4] = BCCMDPDU_STAT_OK;
                bccmd[5] = 0;         // empty
                // bccmd[6-8]         // ignored, zero padding
                bc_state = BC_STATE_RESTARTING;
                TRACE_INFO("RESTARTING BC\r\n");
                queueMessage(BCCMD_CHANNEL, 1, sizeof(uint16) * 9, bccmd);
                StartTimer(250000, restartHandler);
                break;
            default:
                panic("u_bt_task 2");
        }
	}
    TRACE_BT("u_bt_task end\r\n");
}

static void pumpHandler(void)
{
    TRACE_INFO("pumpHandler\r\n");
    BgIntPump();
	StartTimer(PUMP_INTERVAL, pumpHandler);
}

static int btiptmr = 0;

static void btHandler(void) {
    l2cap_tmr();
    rfcomm_tmr();
    bt_spp_tmr();

    //ppp_tmr();
    //nat_tmr();

    if(++btiptmr == 5/*sec*/) {
        //  bt_ip_tmr();
        btiptmr = 0;
    }
	StartTimer(BT_INTERVAL, btHandler);
}

static void tcpHandler(void) {
    tcp_tmr();
	StartTimer(TCP_INTERVAL, tcpHandler);
}

static void restartHandler()
{
    TRACE_INFO("BC RESTARTED\r\n");
    bc_state = BC_STATE_READY;
    bc_hci_event_count = 0;
    Serial_setBaud(0, USART_BAUDRATE); 
#if SET_HOST_UART_HW_FLOW
    Serial_setHandshaking(0, true);
#endif
    abcsp_init(&AbcspInstanceData);
#if defined(TCPIP)
    //echo_init();
    httpd_init();
#endif
    bt_spp_start();
    TRACE_INFO("Applications started.\r\n");

/*
    event ev;
    ev.type = EVENT_BT;
    ev.data.bt.type = EVENT_BT_STARTED;
    event_post(&ev);
*/

#if defined(TCPIP)
    StartTimer(TCP_INTERVAL, tcpHandler);
#endif
    StartTimer(BT_INTERVAL, btHandler);

    //hci_read_bd_addr(bt_rbd_complete);
}


/* -------------------- MAIN -------------------- */

#include <board.h>
volatile AT91PS_PIO  pPIOB = AT91C_BASE_PIOB;
volatile AT91PS_PIO  pPIOA = AT91C_BASE_PIOA;

//extern volatile portTickType xTickCount;

static bool bt_wakeup_pin_high = false;

void bt_io_isr_handler(void *context) {

    //bt_wakeup_pin_high = !bt_wakeup_pin_high;
#ifndef CFG_DEEP_SLEEP
    bt_wakeup_pin_high = Io_value(&bt_io);

    if (!bt_wakeup_pin_high) {
        return;
    }
#endif

    //TRACE_INFO("bt_io_isr_handler %d\r\n", xTickCount);
    TRACE_INFO("bt_io_isr_handler %d\r\n", Timer_tick_count_nonblock());
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    uint16_t event = 1;
    xQueueSendFromISR(bt_wakeup_queue, &event, &xHigherPriorityTaskWoken);
/*
    xSemaphoreGiveFromISR(bt_wakeup_semaphore, &xHigherPriorityTaskWoken);

    //AT91C_BASE_PIOB->PIO_IDR = BC_WAKEUP_MASK; 

*/
    if( xHigherPriorityTaskWoken ) {
        portYIELD_FROM_ISR();
    }
}

void bt_task(void *p)
//int bcsp_main()
{
    TRACE_BT("bt_task %x\r\n", xTaskGetCurrentTaskHandle());

    bt_wakeup_queue = xQueueCreate(1, sizeof(uint16_t));
    vSemaphoreCreateBinary(bt_wakeup_semaphore);
    xSemaphoreTake(bt_wakeup_semaphore, -1);

#if BT_LED
    ledrgb_open();
    ledrgb_set(0x4, 0, 0, BT_LED_START);
#endif

    // TODO sys_init();
#ifdef PERF
    perf_init("/tmp/minimal.perf");
#endif /* PERF */
#ifdef STATS
    stats_init();
#endif /* STATS */
    mem_init();
    memp_init();
    pbuf_init();
    TRACE_INFO("mem mgmt initialized\r\n");


#if defined(TCPIP)
    netif_init();
    ip_init();
    //udp_init();
    tcp_init();
    TRACE_INFO("TCP/IP initialized.\r\n");
#endif
    lwbt_memp_init();
    //phybusif_init(argv[1]);
    if(hci_init() != ERR_OK) {
        TRACE_ERROR("HCI initialization failed!\r\n");
        return -1;
    }
    l2cap_init();
    sdp_init();
    rfcomm_init();
#if defined(TCPIP)
    ppp_init();
#endif
    TRACE_INFO("Bluetooth initialized.\r\n");

	InitMicroSched(u_init_bt_task, u_bt_task);

	UartDrv_RegisterHandlers();
	UartDrv_Configure(baudRate);

/* BC4
-BCRES - PB30
BCBOOT0 - PB23 -> BC4 PIO0
BCBOOT1 - PB25 -> BC4 PIO1
BCBOOT2 - PB29 -> BC4 PIO4

PIO[0]
PIO[1]
PIO[4]
Host Transport
Auto System Clock Adaptation
Auto Baud Rate Adaptation
0
0
0
BCSP (default) (a)
Available (b)
Available (c)
0
0
1
BCSP with UART configured to use 2 stop bits and no parity
Available (b)
Available (c)
0
1
0
USB, 16 MHz crystal (d)
Not available
Not appropriate
0
1
1
USB, 26 MHz crystal (d)
Not available
Not appropriate
1
0
0
Three-wire UART
Available (b)
Available (c)
1
0
1
H4DS
Available (b)
Available (c)
1
1
0
UART (H4)
Available (b)
Available (c)
1
1
1
Undefined
-
-

Petr: takze nejprve drzet v resetu a potom nastavit piny BCBOOT0:2 na jaky protokol ma BC naject, pak -BCRES do 1

#define BCBOOT0_MASK (1 << 23)  // BC4 PIO0
#define BCBOOT1_MASK (1 << 25)  // BC4 PIO1
#define BCBOOT2_MASK (1 << 29)  // BC4 PIO4
#define BCNRES_MASK (1 << 30)

#define BC_WAKEUP_MASK (1 << 23)  // BC4 PIO0

//MV CTS/RTS
    pPIOA->PIO_PDR = (1 << 7);
    pPIOA->PIO_PDR = (1 << 8);
*/

    pPIOB->PIO_PER = BCBOOT0_MASK;
    pPIOB->PIO_OER = BCBOOT0_MASK;
    pPIOB->PIO_CODR = BCBOOT0_MASK; //set to log0

    pPIOB->PIO_PER = BCBOOT1_MASK;
    pPIOB->PIO_OER = BCBOOT1_MASK;                                         
    pPIOB->PIO_CODR = BCBOOT1_MASK; //set to log0

    pPIOB->PIO_PER = BCBOOT2_MASK;
    pPIOB->PIO_OER = BCBOOT2_MASK;
    pPIOB->PIO_CODR = BCBOOT2_MASK; //set to log0

    //TRACE_INFO("B_PIO_ODSR %x\r\n", pPIOB->PIO_ODSR);

    //pPIOB->PIO_PER = BCNRES_MASK;
    //pPIOB->PIO_OER = BCNRES_MASK;
    pPIOB->PIO_CODR = BCNRES_MASK; //set to log0

    //TRACE_INFO("B_PIO_ODSR %x\r\n", pPIOB->PIO_ODSR);
    //TRACE_INFO("B_PIO_PSR %x\r\n", pPIOB->PIO_PSR);

    Task_sleep(20);
/*
    pPIOB->PIO_SODR = BCNRES_MASK;
    Task_sleep(20);
    pPIOB->PIO_CODR = BCNRES_MASK; //set to log0
    Task_sleep(20);
*/
    pPIOB->PIO_SODR = BCNRES_MASK; // Run BC, run!
    //TRACE_INFO("B_PIO_ODSR %x\r\n", pPIOB->PIO_ODSR);

    Task_sleep(10);

    pPIOB->PIO_PDR = BCBOOT0_MASK | BCBOOT1_MASK | BCBOOT2_MASK;
    Io_init(&bt_io, IO_PB23, IO_GPIO, INPUT);
// TODO: Io_addInterruptHandler() max 8 handlers!!! To be moved to bt_init() probably
    Io_addInterruptHandler(&bt_io, bt_io_isr_handler, NULL);
    //AT91C_BASE_PIOB->PIO_IDR = BC_WAKEUP_MASK;

    ps_setrq_count = 0;

    bc_state = BC_STATE_STARTED;

    TRACE_BT("BC restarted\r\n");
 

    if (!UartDrv_Start())
    {
        TerminateMicroSched();
    } else {
        //TRACE_INFO("A_PIO_OSR %x\r\n", pPIOA->PIO_OSR);
        //StartTimer(KEYBOARD_SCAN_INTERVAL, keyboardHandler);
	    abcsp_init(&AbcspInstanceData);
        //StartTimer(PUMP_INTERVAL, pumpHandler);
#if BT_LED
    ledrgb_set(0x4, 0, 0, 0x0);
    ledrgb_close();
#endif

        MicroSched();
#if BT_LED
    ledrgb_open();
    ledrgb_set(0x4, 0, 0, BT_LED_START);
#endif
        UartDrv_Stop();
        CloseMicroSched();
    }

#if 1
    Io_removeInterruptHandler(&bt_io);
#else
    pPIOB->PIO_PDR = BC_WAKEUP_MASK;
#endif
    vQueueDelete(bt_wakeup_semaphore);
    vQueueDelete(bt_wakeup_queue);

    pPIOB->PIO_CODR = BCNRES_MASK; // BC Stop
    bc_state = BC_STATE_STOPPED;

    event ev;
    ev.type = EVENT_BT;
    ev.data.bt.type = EVENT_BT_STOPPED;
    event_post(&ev);

#if BT_LED
    ledrgb_set(0x4, 0, 0, 0x0);
    ledrgb_close();
#endif
    vTaskDelete(NULL);
}

bool bt_get_command(bt_command *cmd) {
    return xQueueReceive(command_queue, cmd, 0);
}

bool bt_set_command_result(bt_command_result *res) {
    return xQueueSend(command_result_queue, res, portMAX_DELAY);
}

void bt_stop_callback() {
    vQueueDelete(command_queue);
    vQueueDelete(command_result_queue);
}

void trace_bytes(char *text, uint8_t *bytes, int len) {
    int i;
    for (i = 0; i < len; i++) {
        TRACE_INFO("%s[%d] = %02x\r\n", text, i, bytes[i]);
    }
}

bool bt_is_ps_set(void) {
    struct bccmd_index_value *ps_setrq_value = &ps_setrq[ps_setrq_count];
    return !ps_setrq_value->index && !ps_setrq_value->value;
}

int bt_wait_for_data(void) {
    TRACE_INFO("bt_wait_for_data\r\n");

    uint16_t event;
    if (bc_state == BC_STATE_READY) {
        pm_unlock();
        xQueueReceive(bt_wakeup_queue, &event, portMAX_DELAY);
        pm_lock();
        TRACE_INFO("data ready\r\n");
    }
/*
    if (bc_state == BC_STATE_READY) {
        AT91C_BASE_PIOB->PIO_IER = BC_WAKEUP_MASK;
        if ( xSemaphoreTake(bt_wakeup_semaphore, -1) != pdTRUE) {
            TRACE_BT("xSemaphoreTake err\r\n");
            return 0;
        }
    }
*/
    return 1;
}



int bt_read_bdaddr_from_disk (const char *path, struct bd_addr *bdaddr) {
    FATFS fatfs;
    FRESULT f;
    int err = 0;

    if ((f = disk_initialize (0)) != FR_OK) {
        f_printerror (f);
        return 1;
    }
    if ((f = f_mount (0, &fatfs)) != FR_OK) {
        f_printerror (f);
        return 1;
    }

    FILE *file = fopen(path, "r");

    if (file == NULL) {
        err = 1;
    } else {
        //int res = fscanf(file, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx", &bdaddr->addr[5], &bdaddr->addr[4], &bdaddr->addr[3], &bdaddr->addr[2], &bdaddr->addr[1], &bdaddr->addr[0]);
        uint16_t tmp_bdaddr[6];
        int res = fscanf(file, "%2hx:%2hx:%2hx:%2hx:%2hx:%2hx", &tmp_bdaddr[5], &tmp_bdaddr[4], &tmp_bdaddr[3], &tmp_bdaddr[2], &tmp_bdaddr[1], &tmp_bdaddr[0]);

        if (res == EOF || res != 6)
            err = 1;

        fclose(file);

        int i;
        for (i = 0; i < 6; i++) {
            bdaddr->addr[i] = (uint8_t)tmp_bdaddr[i];
        }
    }

    if ((f = f_mount (0, NULL)) != FR_OK) {
        f_printerror (f);
    }
    return err;
}

// commands 

int bt_init() {
    struct bd_addr device_bdaddr;

    pPIOB->PIO_PER = BCNRES_MASK;
    pPIOB->PIO_OER = BCNRES_MASK;
    pPIOB->PIO_CODR = BCNRES_MASK; //set to log0

    bc_state = BC_STATE_STOPPED;
    if (!bt_read_bdaddr_from_disk("bdaddr.cfg", &device_bdaddr)) {
        byte_memcpy(&this_device_bdaddr, &device_bdaddr, sizeof(device_bdaddr));
    }
    return 0;
}

int bt_open(uint8_t *cod) {

    if(!bt_open_count++) {
        bt_set_cod(cod);
        bc_state = BC_STATE_STARTING;
        command_queue = xQueueCreate(1, sizeof(bt_command));
        command_result_queue = xQueueCreate(1, sizeof(bt_command_result));
        //bt_task_handle = Task_create( bt_task, "bt_main", TASK_BT_MAIN_STACK, TASK_BT_MAIN_PRI, NULL );
        xTaskCreate(bt_task, "bt_main", TASK_STACK_SIZE(TASK_BT_MAIN_STACK), NULL, TASK_BT_MAIN_PRI, &bt_task_handle);
        return BT_OK;
    }
    return BT_ERR_ALREADY_STARTED;
}

int bt_close() {

    if(bt_open_count && --bt_open_count == 0) {
        bt_command cmd;

        bc_state = BC_STATE_STOPPING;
        if (bt_task_handle == NULL) {
            return BT_OK;
        }
        cmd.id = BT_COMMAND_STOP;

        xQueueSend(command_queue, &cmd, portMAX_DELAY);
        scheduler_wakeup();
    }
    return BT_OK;
}

int bt_set_link_key(uint8_t *bdaddr, uint8_t *link_key) {
    bt_command cmd;

    TRACE_INFO("bt_set_link_key\r\n");

    struct bt_bdaddr_link_key *bdaddr_link_key = malloc(sizeof(struct bt_bdaddr_link_key)); 
    if (bdaddr_link_key == NULL) {
        return BT_ERR_MEM;
    }
    memcpy(&bdaddr_link_key->bdaddr, bdaddr, BT_BDADDR_LEN);
    memcpy(&bdaddr_link_key->link_key, link_key, BT_LINK_KEY_LEN);

    trace_bytes("bdaddr", bdaddr, BT_BDADDR_LEN);
    trace_bytes("linkkey", link_key, BT_LINK_KEY_LEN);

    cmd.id = BT_COMMAND_SET_LINK_KEY;
    cmd.param.ptr = bdaddr_link_key;

    xQueueSend(command_queue, &cmd, portMAX_DELAY);
    scheduler_wakeup();
    return BT_OK;
}

int bt_link_key_req_reply(uint8_t *bdaddr, uint8_t *link_key) {
    bt_command cmd;

    TRACE_INFO("bt_link_key_req_reply\r\n");

    struct bt_bdaddr_link_key *bdaddr_link_key = malloc(sizeof(struct bt_bdaddr_link_key)); 
    if (bdaddr_link_key == NULL) {
        return BT_ERR_MEM;
    }
    memcpy(&bdaddr_link_key->bdaddr, bdaddr, BT_BDADDR_LEN);
    memcpy(&bdaddr_link_key->link_key, link_key, BT_LINK_KEY_LEN);

    trace_bytes("bdaddr", bdaddr, BT_BDADDR_LEN);
    trace_bytes("linkkey", link_key, BT_LINK_KEY_LEN);

    cmd.id = BT_COMMAND_LINK_KEY_REQ_REPLY;
    cmd.param.ptr = bdaddr_link_key;

    xQueueSend(command_queue, &cmd, portMAX_DELAY);
    scheduler_wakeup();
    return BT_OK;
}

int bt_link_key_req_neg_reply(uint8_t *bdaddr) {
    bt_command cmd;

    TRACE_INFO("bt_link_key_req_neg_reply\r\n");

    struct bd_addr *cmd_bdaddr = malloc(sizeof(struct bd_addr)); 
    if (cmd_bdaddr == NULL) {
        return BT_ERR_MEM;
    }
    memcpy(cmd_bdaddr, bdaddr, BT_BDADDR_LEN);

    trace_bytes("bdaddr", bdaddr, BT_BDADDR_LEN);

    cmd.id = BT_COMMAND_LINK_KEY_REQ_NEG_REPLY;
    cmd.param.ptr = cmd_bdaddr;

    xQueueSend(command_queue, &cmd, portMAX_DELAY);
    scheduler_wakeup();
    return BT_OK;
}

int bt_inquiry() {
    bt_command cmd;

    TRACE_INFO("bt_inquiry\r\n");

    cmd.id = BT_COMMAND_INQUIRY;

    xQueueSend(command_queue, &cmd, portMAX_DELAY);
    scheduler_wakeup();
    return BT_OK;
}

int bt_remote_name_req(uint8_t *bdaddr) {
    bt_command cmd;

    TRACE_INFO("bt_remote_name_req\r\n");

    struct bd_addr *cmd_bdaddr = malloc(sizeof(struct bd_addr)); 
    if (cmd_bdaddr == NULL) {
        return BT_ERR_MEM;
    }
    memcpy(cmd_bdaddr, bdaddr, BT_BDADDR_LEN);

    trace_bytes("bdaddr", bdaddr, BT_BDADDR_LEN);

    cmd.id = BT_COMMAND_REMOTE_NAME_REQ;
    cmd.param.ptr = cmd_bdaddr;

    xQueueSend(command_queue, &cmd, portMAX_DELAY);
    scheduler_wakeup();
    return BT_OK;
}

// socket commands

int bt_rfcomm_listen(bt_socket *sock, uint8_t *channel) {
    bt_command cmd;
    bt_command_result res;

    TRACE_INFO("bt_rfcomm_listen %x %d\r\n", sock, *channel);

    cmd.id = BT_COMMAND_RFCOMM_LISTEN;
    cmd.sock = sock;
    cmd.param.cn = *channel;

    xQueueSend(command_queue, &cmd, portMAX_DELAY);
    scheduler_wakeup();

    xQueueReceive(command_result_queue, &res, portMAX_DELAY);
    return BT_OK;
}

int bt_rfcomm_connect(bt_socket *sock, uint8_t *bdaddr, uint8_t channel) {
    bt_command cmd;
    bt_command_result res;

    TRACE_INFO("bt_rfcomm_connect %x %d\r\n", sock, channel);

    struct bt_bdaddr_cn *bdaddr_cn = malloc(sizeof(struct bt_bdaddr_cn)); 
    if (bdaddr_cn == NULL) {
        return BT_ERR_MEM;
    }
    memcpy(&bdaddr_cn->bdaddr, bdaddr, BT_BDADDR_LEN);
    bdaddr_cn->cn = channel;

    trace_bytes("bdaddr", bdaddr, BT_BDADDR_LEN);

    cmd.id = BT_COMMAND_RFCOMM_CONNECT;
    cmd.sock = sock;
    cmd.param.ptr = bdaddr_cn;

    xQueueSend(command_queue, &cmd, portMAX_DELAY);
    scheduler_wakeup();

    xQueueReceive(command_result_queue, &res, portMAX_DELAY);
    return BT_OK;
}

int bt_find_service(bt_socket *sock, uint8_t *bdaddr) {
    bt_command cmd;

    TRACE_INFO("bt_find_service %x\r\n", sock);

    struct bd_addr *cmd_bdaddr = malloc(sizeof(struct bd_addr)); 
    if (cmd_bdaddr == NULL) {
        return BT_ERR_MEM;
    }
    memcpy(cmd_bdaddr, bdaddr, BT_BDADDR_LEN);

    trace_bytes("bdaddr", bdaddr, BT_BDADDR_LEN);

    cmd.id = BT_COMMAND_FIND_SERVICE;
    cmd.sock = sock;
    cmd.param.ptr = cmd_bdaddr;

    xQueueSend(command_queue, &cmd, portMAX_DELAY);
    scheduler_wakeup();
    return BT_OK;
}

int bt_rfcomm_send(bt_socket *sock, const char *data, size_t len) {
    bt_command cmd;
    struct pbuf *p;

    //uint16_t len = strlen(data) + 1;
    
    TRACE_INFO("bt_rfcomm_send %s %d\r\n", data, len);
    p = pbuf_alloc(PBUF_RAW, len, PBUF_RAM);
    if (p == NULL) {
        return BT_ERR_MEM;
    }
    //strcpy(p->payload, data);
    memcpy(p->payload, data, len);

    cmd.id = BT_COMMAND_SEND;
    cmd.sock = sock;
    cmd.param.ptr = p;

    xQueueSend(command_queue, &cmd, portMAX_DELAY);
    scheduler_wakeup();
    return BT_OK;
}


int bt_rfcomm_advertise_service(bt_socket *sock, const char *record, size_t len) {
    bt_command cmd;
    bt_command_result res;

    cmd.id = BT_COMMAND_ADVERTISE_SERVICE;
    
    cmd.param.service.record = record;
    cmd.param.service.len = len;

    xQueueSend(command_queue, &cmd, portMAX_DELAY);
    scheduler_wakeup();

    xQueueReceive(command_result_queue, &res, portMAX_DELAY);
    return BT_OK;
}
