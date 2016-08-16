/*
 * Copyright (c) 2009 PASCO scientific
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwBT Bluetooth stack.
 * 
 * Author: Laine Walker-Avina <lwalkera@ieee.org>
 * Based on code from: Conny Ohult <conny@sm.luth.se>
 *
 */


/**
 * @file bt_spp.c
 *
 * This is a control application that initialises a host controller and
 * connects to a network as a DT through a DUN or LAP enabled device.
 * When a network connection has been established, it initialises its own LAP
 * server.
 */

#define TEST    0
#if TEST
static int app_initialized = 0;
#endif

#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/stats.h"
#include "lwip/inet.h"

#include "lwbt/phybusif.h"
#include "lwbt/lwbt_memp.h"
#include "lwbt/hci.h"
#include "lwbt/l2cap.h"
#include "lwbt/sdp.h"
#include "lwbt/rfcomm.h"

#include "bt.h"
#include "event.h"


//#include "stdlib.h"

#define BT_SPP_DEBUG LWIP_DBG_ON /* Controls debug messages */

err_t command_complete(void *arg, struct hci_pcb *pcb, u8_t ogf, u8_t ocf, u8_t result);
err_t pin_req(void *arg, struct bd_addr *bdaddr);
err_t link_key_not(void *arg, struct bd_addr *bdaddr, u8_t *key);
err_t link_key_req(void *arg, struct bd_addr *bdaddr);
err_t l2cap_connected(void *arg, struct l2cap_pcb *l2cappcb, u16_t result, u16_t status);
err_t inquiry_complete(void *arg, struct hci_pcb *pcb, struct hci_inq_res *ires, u16_t result);
err_t spp_recv(void *arg, struct rfcomm_pcb *pcb, struct pbuf *p, err_t err);
err_t bt_spp_init();

struct bt_state {
	struct bd_addr bdaddr;
	struct pbuf *p;
	u8_t btctrl;
	u8_t cn;
} bt_spp_state;

static u8_t bt_cod[3];


/*
ServiceDiscoveryServerServiceClassID_UUID: TGUID = '{00001000-0000-1000-8000-00805F9B34FB}';
  BrowseGroupDescriptorServiceClassID_UUID: TGUID = '{00001001-0000-1000-8000-00805F9B34FB}';
  PublicBrowseGroupServiceClass_UUID: TGUID = '{00001002-0000-1000-8000-00805F9B34FB}';
  SerialPortServiceClass_UUID: TGUID = '{00001101-0000-1000-8000-00805F9B34FB}';
  LANAccessUsingPPPServiceClass_UUID: TGUID = '{00001102-0000-1000-8000-00805F9B34FB}';
  DialupNetworkingServiceClass_UUID: TGUID = '{00001103-0000-1000-8000-00805F9B34FB}';
  IrMCSyncServiceClass_UUID: TGUID = '{00001104-0000-1000-8000-00805F9B34FB}';
  OBEXObjectPushServiceClass_UUID: TGUID = '{00001105-0000-1000-8000-00805F9B34FB}';
  OBEXFileTransferServiceClass_UUID: TGUID = '{00001106-0000-1000-8000-00805F9B34FB}';
  IrMCSyncCommandServiceClass_UUID: TGUID = '{00001107-0000-1000-8000-00805F9B34FB}';
  HeadsetServiceClass_UUID: TGUID = '{00001108-0000-1000-8000-00805F9B34FB}';
  CordlessTelephonyServiceClass_UUID: TGUID = '{00001109-0000-1000-8000-00805F9B34FB}';
  AudioSourceServiceClass_UUID: TGUID = '{0000110A-0000-1000-8000-00805F9B34FB}';
  AudioSinkServiceClass_UUID: TGUID = '{0000110B-0000-1000-8000-00805F9B34FB}';
  AVRemoteControlTargetServiceClass_UUID: TGUID = '{0000110C-0000-1000-8000-00805F9B34FB}';
  AdvancedAudioDistributionServiceClass_UUID: TGUID = '{0000110D-0000-1000-8000-00805F9B34FB}';
  AVRemoteControlServiceClass_UUID: TGUID = '{0000110E-0000-1000-8000-00805F9B34FB}';
  VideoConferencingServiceClass_UUID: TGUID = '{0000110F-0000-1000-8000-00805F9B34FB}';
  IntercomServiceClass_UUID: TGUID = '{00001110-0000-1000-8000-00805F9B34FB}';
  FaxServiceClass_UUID: TGUID = '{00001111-0000-1000-8000-00805F9B34FB}';
  HeadsetAudioGatewayServiceClass_UUID: TGUID = '{00001112-0000-1000-8000-00805F9B34FB}';
  WAPServiceClass_UUID: TGUID = '{00001113-0000-1000-8000-00805F9B34FB}';
  WAPClientServiceClass_UUID: TGUID = '{00001114-0000-1000-8000-00805F9B34FB}';
  PANUServiceClass_UUID: TGUID = '{00001115-0000-1000-8000-00805F9B34FB}';
  NAPServiceClass_UUID: TGUID = '{00001116-0000-1000-8000-00805F9B34FB}';
  GNServiceClass_UUID: TGUID = '{00001117-0000-1000-8000-00805F9B34FB}';
  DirectPrintingServiceClass_UUID: TGUID = '{00001118-0000-1000-8000-00805F9B34FB}';
  ReferencePrintingServiceClass_UUID: TGUID = '{00001119-0000-1000-8000-00805F9B34FB}';
  ImagingServiceClass_UUID: TGUID = '{0000111A-0000-1000-8000-00805F9B34FB}';
  ImagingResponderServiceClass_UUID: TGUID = '{0000111B-0000-1000-8000-00805F9B34FB}';
  ImagingAutomaticArchiveServiceClass_UUID: TGUID = '{0000111C-0000-1000-8000-00805F9B34FB}';
  ImagingReferenceObjectsServiceClass_UUID: TGUID = '{0000111D-0000-1000-8000-00805F9B34FB}';
  HandsfreeServiceClass_UUID: TGUID = '{0000111E-0000-1000-8000-00805F9B34FB}';
  HandsfreeAudioGatewayServiceClass_UUID: TGUID = '{0000111F-0000-1000-8000-00805F9B34FB}';
  DirectPrintingReferenceObjectsServiceClass_UUID: TGUID = '{00001120-0000-1000-8000-00805F9B34FB}';
  ReflectedUIServiceClass_UUID: TGUID = '{00001121-0000-1000-8000-00805F9B34FB}';
  BasicPringingServiceClass_UUID: TGUID = '{00001122-0000-1000-8000-00805F9B34FB}';
  PrintingStatusServiceClass_UUID: TGUID = '{00001123-0000-1000-8000-00805F9B34FB}';
  HumanInterfaceDeviceServiceClass_UUID: TGUID = '{00001124-0000-1000-8000-00805F9B34FB}';
  HardcopyCableReplacementServiceClass_UUID: TGUID = '{00001125-0000-1000-8000-00805F9B34FB}';
  HCRPrintServiceClass_UUID: TGUID = '{00001126-0000-1000-8000-00805F9B34FB}';
  HCRScanServiceClass_UUID: TGUID = '{00001127-0000-1000-8000-00805F9B34FB}';
  CommonISDNAccessServiceClass_UUID: TGUID = '{00001128-0000-1000-8000-00805F9B34FB}';
  VideoConferencingGWServiceClass_UUID: TGUID = '{00001129-0000-1000-8000-00805F9B34FB}';
  UDIMTServiceClass_UUID: TGUID = '{0000112A-0000-1000-8000-00805F9B34FB}';
  UDITAServiceClass_UUID: TGUID = '{0000112B-0000-1000-8000-00805F9B34FB}';
  AudioVideoServiceClass_UUID: TGUID = '{0000112C-0000-1000-8000-00805F9B34FB}';
  SIMAccessServiceClass_UUID: TGUID = '{0000112D-0000-1000-8000-00805F9B34FB}';
  PnPInformationServiceClass_UUID: TGUID = '{00001200-0000-1000-8000-00805F9B34FB}';
  GenericNetworkingServiceClass_UUID: TGUID = '{00001201-0000-1000-8000-00805F9B34FB}';
  GenericFileTransferServiceClass_UUID: TGUID = '{00001202-0000-1000-8000-00805F9B34FB}';
  GenericAudioServiceClass_UUID: TGUID = '{00001203-0000-1000-8000-00805F9B34FB}';
  GenericTelephonyServiceClass_UUID: TGUID = '{00001204-0000-1000-8000-00805F9B34FB}';
*/

static const u8_t spp_service_record[] =
{
		SDP_DES_SIZE8, 0x8, 
			SDP_UINT16, 0x0, 0x0, /* Service record handle attribute */
				SDP_UINT32, 0x00, 0x00, 0x00, 0x00, /*dummy vals, filled in on xmit*/ 
/*
		SDP_DES_SIZE8, 0x16, 
			SDP_UINT16, 0x0, 0x1, // Service class ID list attribute
			SDP_DES_SIZE8, 17,
				SDP_UUID128, 0x00, 0x00, 0x00, 0x00,
					0xde, 0xca,
					0xfa, 0xde,
					0xde, 0xca,
					0xde, 0xaf, 0xde, 0xca, 0xca, 0xff,
*/
// SPP 00001101-0000-1000-8000-00805F9B34FB
// HSP 00001108-0000-1000-8000-00805F9B34FB
// HFS 0000111E-0000-1000-8000-00805F9B34FB
		SDP_DES_SIZE8, 8, 
			SDP_UINT16, 0x0, 0x1, // Service class ID list attribute
			SDP_DES_SIZE8, 3, 
				SDP_UUID16, 0x11, 0x1E, /*Handsfree*/
				//SDP_UUID16, 0x11, 0x08, /*Headset*/
/*
			SDP_DES_SIZE8, 17, 
                SDP_UUID128, 0x00, 0x00, 0x11, 0x01,
                    0x00, 0x00,
                    0x10, 0x00,
                    0x80, 0x00,
                    0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB,
*/
/*
			SDP_DES_SIZE8, 17, 
                SDP_UUID128, 0x00, 0x00, 0x11, 0x1E,
                    0x00, 0x00,
                    0x10, 0x00,
                    0x80, 0x00,
                    0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB,
*/
/*
			SDP_DES_SIZE8, 17, 
                SDP_UUID128, 0x00, 0x00, 0x11, 0x08,
                    0x00, 0x00,
                    0x10, 0x00,
                    0x80, 0x00,
                    0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB,
*/
		SDP_DES_SIZE8, 0x11,
			SDP_UINT16, 0x0, 0x4, /* Protocol descriptor list attribute */
			SDP_DES_SIZE8, 0xc, 
				SDP_DES_SIZE8, 0x3,
					SDP_UUID16, 0x1, 0x0, /*L2CAP*/
				SDP_DES_SIZE8, 0x5,
					SDP_UUID16, 0x0, 0x3, /*RFCOMM*/
					SDP_UINT8, 0x1, /*RFCOMM channel*/
		SDP_DES_SIZE8, 0x8,
			SDP_UINT16, 0x0, 0x5, /*Browse group list */
			SDP_DES_SIZE8, 0x3,
				SDP_UUID16, 0x10, 0x02, /*PublicBrowseGroup*/
/*
		SDP_DES_SIZE8, 11,
			SDP_UINT16, 0x0, 0x9, // BluetoothProfileDescriptorList
			SDP_DES_SIZE8, 6,
				SDP_UUID16, 0x11, 0x1E, // Handsfree
                SDP_UINT16, 0x01, 0x05,
				//SDP_UUID16, 0x11, 0x1F, // Handsfree AG
*/
};

/* 
 * bt_spp_start():
 *
 * Called by the main application to initialize and connect to a network
 *
 */
void bt_spp_start(void)
{
	hci_reset_all();
	l2cap_reset_all();
	sdp_reset_all();
	rfcomm_reset_all();

	LWIP_DEBUGF(BT_SPP_DEBUG, ("bt_spp_start\n"));

	hci_cmd_complete(command_complete);
	hci_pin_req(pin_req);
	bt_spp_state.btctrl = 0;
	bt_spp_state.p = NULL;
	hci_reset();

	if(bt_spp_init() != ERR_OK) /* Initialize the SPP role */
	{
		LWIP_DEBUGF(BT_SPP_DEBUG, ("bt_spp_start: couldn't init role\n"));
		return;
	}
}

/* 
 * bt_spp_tmr():
 *
 * Called by the main application to initialize and connect to a network
 *
 */
void bt_spp_tmr(void)
{
#if 0
	u8_t update_cmd[12];

	update_cmd[0] = 1;
	update_cmd[1] = 0;
	update_cmd[2] = bt_spp_state.bdaddr.addr[5];
	update_cmd[3] = bt_spp_state.bdaddr.addr[4]; 
	update_cmd[4] = bt_spp_state.bdaddr.addr[3];
	update_cmd[5] = bt_spp_state.bdaddr.addr[2]; 
	update_cmd[6] = bt_spp_state.bdaddr.addr[1];
	update_cmd[7] = bt_spp_state.bdaddr.addr[0];
	update_cmd[8] = 0x00;
	update_cmd[9] = 0x00;
	update_cmd[10] = 0x00;
	update_cmd[11] = 0x00;

	LWIP_DEBUGF(BT_SPP_DEBUG, ("bt_spp_tmr: Update cmd bd address: 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n", update_cmd[2], update_cmd[3], update_cmd[4], update_cmd[5], update_cmd[6], update_cmd[7]));

	if(bt_spp_state.tcppcb != NULL) {
		tcp_write(bt_spp_state.tcppcb, &update_cmd, 12, 1);
	}
#endif
}

/* 
 * rfcomm_disconnected():
 *
 * Called by RFCOMM when the remote RFCOMM protocol or upper layer was disconnected.
 * Disconnects the PPP protocol.
 *
 */
err_t rfcomm_disconnected(void *arg, struct rfcomm_pcb *pcb, err_t err) 
{
	err_t ret = ERR_OK;

	LWIP_DEBUGF(BT_SPP_DEBUG, ("rfcomm_disconnected: CN = %d\n", rfcomm_cn(pcb)));
	if(rfcomm_cn(pcb) != 0) {
		; //ppp_lp_disconnected(pcb);

        bt_socket *sock = (bt_socket*)arg;
        sock->pcb = NULL;

        event ev;
        ev.type = EVENT_BT;
        ev.data.bt.type = EVENT_BT_RFCOMM_DISCONNECTED;
        ev.data.bt.sock = sock;
        event_post(&ev);
	}
	rfcomm_close(pcb);

	return ret;
}

/* 
 * l2cap_disconnected_ind():
 *
 * Called by L2CAP to indicate that remote L2CAP protocol disconnected.
 * Disconnects the RFCOMM protocol and the ACL link before it initializes a search for 
 * other devices.
 *
 */
err_t l2cap_disconnected_ind(void *arg, struct l2cap_pcb *pcb, err_t err)
{
	err_t ret = ERR_OK;

	LWIP_DEBUGF(BT_SPP_DEBUG, ("l2cap_disconnected_ind: L2CAP disconnected\n"));

	if(pcb->psm == SDP_PSM) { 
	    LWIP_DEBUGF(BT_SPP_DEBUG, ("SDP_PSM %x\n", arg));
		sdp_lp_disconnected(pcb);
		l2cap_close(pcb);

        bt_socket *sock = (bt_socket*)arg;
        sock->pcb = NULL;

        event ev;
        ev.type = EVENT_BT;
        ev.data.bt.type = EVENT_BT_RFCOMM_DISCONNECTED;
        ev.data.bt.sock = sock;
        event_post(&ev);
	} else if(pcb->psm == RFCOMM_PSM) {
	    LWIP_DEBUGF(BT_SPP_DEBUG, ("RFCOMM_PSM %x\n", arg));
		ret = rfcomm_lp_disconnected(pcb);
		/* We can do this since we know that we are the only channel on the ACL link.
		 * If ACL link already is down we get an ERR_CONN returned */
		hci_disconnect(&(pcb->remote_bdaddr), HCI_OTHER_END_TERMINATED_CONN_USER_ENDED);
		l2cap_close(pcb);
		//MV bt_spp_start();
        //MV hci_inquiry(0x009E8B33, 0x04, 0x01, inquiry_complete);

/*
        // TODO: EVENT_BT_RFCOMM_DISCONNECTED done in rfcomm_lp_disconnected() too
        bt_socket *sock = (bt_socket*)arg;
        sock->pcb = NULL;

        event ev;
        ev.type = EVENT_BT;
        ev.data.bt.type = EVENT_BT_RFCOMM_DISCONNECTED;
        ev.data.bt.sock = sock;
        event_post(&ev);
*/
	} else {
        //MV 
		l2cap_close(pcb);
    }

	return ret;
}

/* 
 * bluetoothif_init():
 *
 * Called by lwIP to initialize the lwBT network interface.
 *
 */
#if 0
err_t bluetoothif_init(struct netif *netif)
{
	netif->name[0] = 'b';
	netif->name[1] = '0' + bt_spp_netifn++;
	netif->output = ppp_netif_output;

	netif->state = NULL;
	return ERR_OK;
}
#endif

/* 
 * tcp_connected():
 *
 * Called by TCP when a connection has been established.
 * Connects to a remote gateway and give the TCP connection to the HTTP server 
 * application
 *
 */
#if 0
err_t tcp_connected(void *arg, struct tcp_pcb *pcb, err_t err)
{
	u8_t update_cmd[12];

	LWIP_DEBUGF(BT_SPP_DEBUG, ("tcp_connected\n"));

	update_cmd[0] = 1;
	update_cmd[1] = 0;

	update_cmd[2] = bt_spp_state.bdaddr.addr[5];
	update_cmd[3] = bt_spp_state.bdaddr.addr[4]; 
	update_cmd[4] = bt_spp_state.bdaddr.addr[3];
	update_cmd[5] = bt_spp_state.bdaddr.addr[2]; 
	update_cmd[6] = bt_spp_state.bdaddr.addr[1];
	update_cmd[7] = bt_spp_state.bdaddr.addr[0];

	LWIP_DEBUGF(BT_SPP_DEBUG, ("tcp_connected: bd address: 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n", bt_spp_state.bdaddr.addr[0], bt_spp_state.bdaddr.addr[1], bt_spp_state.bdaddr.addr[2], bt_spp_state.bdaddr.addr[3], bt_spp_state.bdaddr.addr[4], bt_spp_state.bdaddr.addr[5]));
	LWIP_DEBUGF(BT_SPP_DEBUG, ("tcp_connected: Update cmd bd address: 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n", update_cmd[2], update_cmd[3], update_cmd[4], update_cmd[5], update_cmd[6], update_cmd[7]));

	update_cmd[8] = 0x00;
	update_cmd[9] = 0x00;
	update_cmd[10] = 0x00;
	update_cmd[11] = 0x00;

	tcp_write(pcb, &update_cmd, 12, 1);

	LWIP_DEBUGF(BT_SPP_DEBUG, ("tcp_connected: Update command sent\n"));

	bt_spp_state.tcppcb = pcb;

	return http_accept((void *)&(bt_spp_state.bdaddr), pcb, ERR_OK);
}
#endif

#if 0
err_t ppp_accept(void *arg, struct ppp_pcb *pcb, err_t err) 
{
	LWIP_DEBUGF(BT_SPP_DEBUG, ("ppp_accept\n"));
	if(err != ERR_OK) {
		netif_remove(pcb->bluetoothif);
		pcb->bluetoothif = NULL;
	}
	return ERR_OK;
}
#endif

#if 0
err_t modem_emu(void *arg, struct rfcomm_pcb *pcb, struct pbuf *p, err_t err)
{
	u8_t *data = p->payload;
	struct pbuf *q = NULL;
	u16_t proto;
	u8_t code;
	u8_t i;

	LWIP_DEBUGF(BT_SPP_DEBUG, ("modem_emu: p->len == %d p->tot_len == %d\n", p->len, p->tot_len));

	for (i = 0; i < p->len; ++i) {
		if(data[i] == PPP_END) {
			if ((p->len - i) >= 7) {
				if (data[i + 2] == PPP_ESC) { 
					proto = ntohs(*((u16_t *)(((u8_t *)p->payload) + i + 4)));
				}
				else {
					proto = ntohs(*((u16_t *)(((u8_t *)p->payload) + i + 3)));
				}
				if(proto == PPP_LCP) {
					code = data[i + 7] ^ 0x20;
					if(code == LCP_CFG_REQ) {
						rfcomm_recv(pcb, ppp_input);
						ppp_input(arg, pcb, p, ERR_OK);
					}
				}
			}
			return ERR_OK;
		}
	}

	if(arg != NULL) {
		q = pbuf_alloc(PBUF_RAW, p->len + ((struct pbuf *)arg)->len, PBUF_RAM);
		memcpy((u8_t *)q->payload, (u8_t *)((struct pbuf *)arg)->payload, ((struct pbuf *)arg)->len);
		memcpy(((u8_t *)q->payload) + ((struct pbuf *)arg)->len, (u8_t *)p->payload, p->len);
		pbuf_free((struct pbuf *)arg);
		(struct pbuf *)arg = NULL;
		pbuf_free(p);
		data = q->payload;
		p = q;
	}

	for (i = 0; i < p->len; ++i) {
		LWIP_DEBUGF(BT_SPP_DEBUG, ("modem_emu: %c %d\n", data[i], data[i]));
	}

	if(p->len == 0) {
		LWIP_DEBUGF(BT_SPP_DEBUG, ("modem_emu: p->len == 0\n"));
		q = pbuf_alloc(PBUF_RAW, sizeof("OK\r\n"), PBUF_RAM);
		((u8_t *)q->payload) = "OK\r\n";
		pbuf_free(p);
	} else if(!strncmp(data, "ATD", 3)) {
		LWIP_DEBUGF(BT_SPP_DEBUG, ("modem_emu: !strncasecmp(data, \"ATD\", 3)\n"));
		q = pbuf_alloc(PBUF_RAW, sizeof("CONNECT\r\n"), PBUF_RAM);
		((u8_t *)q->payload) = "CONNECT\r\n";
		pbuf_free(p);
	} else if(!strncmp(data, "CLIENT", 6)) {
		LWIP_DEBUGF(BT_SPP_DEBUG, ("modem_emu: !strncasecmp(data, \"CLIENT\", 6)\n"));
		q = pbuf_alloc(PBUF_RAW, sizeof("CLIENTSERVER\r\n"), PBUF_RAM);
		((u8_t *)q->payload) = "CLIENTSERVER\r\n";
		pbuf_free(p);
	} else if(data[p->len - 1] != 0x0D) {
		LWIP_DEBUGF(BT_SPP_DEBUG, ("modem_emu: data[p->len - 1] != 0x0D\n"));
		rfcomm_arg(pcb, p);
		return ERR_OK;
	} else {
		LWIP_DEBUGF(BT_SPP_DEBUG, ("modem_emu: Unknown. Sending OK\n"));
		q = pbuf_alloc(PBUF_RAW, sizeof("OK\r\n"), PBUF_RAM);
		((u8_t *)q->payload) = "OK\r\n";
		pbuf_free(p);
	}

	if(rfcomm_cl(pcb)) {
		rfcomm_uih_credits(pcb, PBUF_POOL_SIZE - rfcomm_remote_credits(pcb), q);
	} else {
		rfcomm_uih(pcb, rfcomm_cn(pcb), q);
	}
	pbuf_free(q);

	return ERR_OK;
}
#endif

err_t spp_recv(void *arg, struct rfcomm_pcb *pcb, struct pbuf *p, err_t err)
{
	u8_t *data = p->payload;
	struct pbuf *q = NULL;
	
	LWIP_DEBUGF(BT_SPP_DEBUG, ("spp_recv: p->len == %d p->tot_len == %d\n", p->len, p->tot_len));
/* MV
	if(arg != NULL) {
		q = pbuf_alloc(PBUF_RAW, p->len + ((struct pbuf *)arg)->len, PBUF_RAM);
		LWIP_ERROR("couldn't alloc pbuf", q == NULL, return ERR_MEM);

		memcpy((u8_t *)q->payload, (u8_t *)((struct pbuf *)arg)->payload, ((struct pbuf *)arg)->len);
		memcpy(((u8_t *)q->payload) + ((struct pbuf *)arg)->len, (u8_t *)p->payload, p->len);
		pbuf_free((struct pbuf *)arg);
		//(struct pbuf *)arg = NULL;
		pbuf_free(p);
		data = q->payload;
		p = q;
	}
*/
    if (p->len) {
        int i;
        for (i = 0; i < p->len; ++i) {
            LWIP_DEBUGF(BT_SPP_DEBUG, ("data: %c %d\n", data[i], data[i]));
        }
        
    // MV
        TRACE_INFO("spp_recv %d\r\n", p->tot_len);
        event ev;
        ev.type = EVENT_BT;
        ev.data.bt.type = EVENT_BT_DATA;
        ev.data.bt.sock = arg;
        ev.data.bt.param.ptr = p;
        event_post(&ev);
    } else {
        pbuf_free(p);
    }

	return ERR_OK;


    if(p->len == 0) {
        LWIP_DEBUGF(BT_SPP_DEBUG, ("dynawa: p->len == 0\n"));
        q = pbuf_alloc(PBUF_RAW, sizeof("OK\r\n"), PBUF_RAM);
        //((u8_t *)q->payload) = "OK\r\n";
        strcpy(((u8_t *)q->payload), "OK\r\n");
        pbuf_free(p);
    } else if(!strncmp(data, "ATD", 3)) {
        LWIP_DEBUGF(BT_SPP_DEBUG, ("dynawa: !strncasecmp(data, \"ATD\", 3)\n"));
        q = pbuf_alloc(PBUF_RAW, sizeof("CONNECT\r\n"), PBUF_RAM);
        //((u8_t *)q->payload) = "CONNECT\r\n";
        strcpy(((u8_t *)q->payload), "CONNECT\r\n");
        pbuf_free(p);
    } else if(!strncmp(data, "CLIENT", 6)) {
        LWIP_DEBUGF(BT_SPP_DEBUG, ("dynawa: !strncasecmp(data, \"CLIENT\", 6)\n"));
        q = pbuf_alloc(PBUF_RAW, sizeof("CLIENTSERVER\r\n"), PBUF_RAM);
        //((u8_t *)q->payload) = "CLIENTSERVER\r\n";
        strcpy((u8_t *)q->payload, "CLIENTSERVER\r\n");
        pbuf_free(p);
/*
    } else if(data[p->len - 1] != 0x0D) {
        LWIP_DEBUGF(BT_SPP_DEBUG, ("dynawa: data[p->len - 1] != 0x0D\n"));
        rfcomm_arg(pcb, p);
        return ERR_OK;
*/
    } else {
        LWIP_DEBUGF(BT_SPP_DEBUG, ("dynawa: Unknown. Echoing\n"));
        q = pbuf_alloc(PBUF_RAW, p->len, PBUF_RAM);
        strncpy((u8_t *)q->payload, data, p->len);
        pbuf_free(p);
    }


	if(rfcomm_cl(pcb)) {
		rfcomm_uih_credits(pcb, PBUF_POOL_SIZE - rfcomm_remote_credits(pcb), q);
	} else {
		rfcomm_uih(pcb, rfcomm_cn(pcb), q);
	}
	pbuf_free(q);

	return ERR_OK;
}

err_t rfcomm_accept(void *arg, struct rfcomm_pcb *pcb, err_t err) 
{
	LWIP_DEBUGF(BT_SPP_DEBUG, ("rfcomm_accept: CN = %d\n", rfcomm_cn(pcb)));

	rfcomm_disc(pcb, rfcomm_disconnected);
	if(pcb->cn != 0) {
        bt_socket *sock = (bt_socket*)arg;

        bt_socket *client_sock = bt_socket_new();
        if (client_sock == NULL) {
            return ERR_MEM;
        }
        rfcomm_arg(pcb, client_sock);

        client_sock->state = BT_SOCKET_STATE_RFCOMM_CONNECTED;
        client_sock->pcb = pcb;
    
        event ev;
        ev.type = EVENT_BT;
        ev.data.bt.type = EVENT_BT_RFCOMM_ACCEPTED;
        ev.data.bt.sock = sock;
        ev.data.bt.param.ptr = client_sock;
        event_post(&ev);
		//set recv callback
		rfcomm_recv(pcb, spp_recv);
	}
	return ERR_OK;
}

static err_t bt_disconnect_ind(void *arg, struct l2cap_pcb *pcb, err_t err)
{
	err_t ret;

	LWIP_DEBUGF(BT_SPP_DEBUG, ("bt_disconnect_ind\n"));

	if(pcb->psm == SDP_PSM) { 
		sdp_lp_disconnected(pcb);
	} else if(pcb->psm == RFCOMM_PSM) {
		ret = rfcomm_lp_disconnected(pcb);
	}

	l2cap_close(pcb);
	return ERR_OK;
}

err_t bt_connect_ind(void *arg, struct l2cap_pcb *pcb, err_t err)
{
	LWIP_DEBUGF(BT_SPP_DEBUG, ("bt_connect_ind\n"));

	/* Tell L2CAP that we wish to be informed of a disconnection request */
	l2cap_disconnect_ind(pcb, bt_disconnect_ind);

	/* Tell L2CAP that we wish to be informed of incoming data */
	if(pcb->psm == SDP_PSM) {
		l2cap_recv(pcb, sdp_recv);
	} else if (pcb->psm == RFCOMM_PSM) {
		l2cap_recv(pcb, rfcomm_input);
	}
	return ERR_OK;  
}

err_t bt_spp_init(void)
{
	struct l2cap_pcb *l2cappcb;
	struct rfcomm_pcb *rfcommpcb;
	struct sdp_record *record;

//return ERR_OK;

	if((l2cappcb = l2cap_new()) == NULL) {
		LWIP_DEBUGF(BT_SPP_DEBUG, ("bt_spp_init: Could not alloc L2CAP PCB for SDP_PSM\n"));
		return ERR_MEM;
	}
	l2cap_connect_ind(l2cappcb, SDP_PSM, bt_connect_ind);

	if((l2cappcb = l2cap_new()) == NULL) {
		LWIP_DEBUGF(BT_SPP_DEBUG, ("bt_spp_init: Could not alloc L2CAP PCB for RFCOMM_PSM\n"));
		return ERR_MEM;
	}
	l2cap_connect_ind(l2cappcb, RFCOMM_PSM, bt_connect_ind);

	LWIP_DEBUGF(RFCOMM_DEBUG, ("bt_spp_init: Allocate RFCOMM PCB for CN 0\n"));
	if((rfcommpcb = rfcomm_new(NULL)) == NULL) {
		LWIP_DEBUGF(BT_SPP_DEBUG, ("bt_spp_init: Could not alloc RFCOMM PCB for channel 0\n"));
		return ERR_MEM;
	}
	rfcomm_listen(rfcommpcb, 0, rfcomm_accept);

/*
	LWIP_DEBUGF(RFCOMM_DEBUG, ("bt_spp_init: Allocate RFCOMM PCB for CN 1\n"));
	if((rfcommpcb = rfcomm_new(NULL)) == NULL) {
		LWIP_DEBUGF(BT_SPP_DEBUG, ("lap_init: Could not alloc RFCOMM PCB for channel 1\n"));
		return ERR_MEM;
	}
	rfcomm_listen(rfcommpcb, 1, rfcomm_accept);

	if((record = sdp_record_new((u8_t *)spp_service_record, sizeof(spp_service_record))) == NULL) {
		LWIP_DEBUGF(BT_SPP_DEBUG, ("bt_spp_init: Could not alloc SDP record\n"));
		return ERR_MEM;
	} else {
		sdp_register_service(record);
	}
*/

    // MV
    hci_link_key_not(link_key_not); /* Set function to be called if a new link key is created */
    hci_link_key_req(link_key_req); /* Set function to be called a link key is requested */

	LWIP_DEBUGF(BT_SPP_DEBUG, ("SPP initialized\n"));
	return ERR_OK;
}

/*
 * ppp_connected():
 *
 * Called by PPP when a LCP and IPCP connection has been established.
 * Connects to a given TCP host.
 *
 */
#if 0
err_t ppp_connected(void *arg, struct ppp_pcb *pcb, err_t err)
{
	struct tcp_pcb *tcppcb;
	struct ip_addr ipaddr;
	err_t ret;
	u8_t flag = 0x03;

	LWIP_DEBUGF(BT_SPP_DEBUG, ("ppp_connected: err = %d\n", err));

	/* return ppp_echo(pcb, NULL); */

	if(err != ERR_OK) {
		netif_remove(pcb->bluetoothif);
		pcb->bluetoothif = NULL;
		//TODO: RESTART??
		return ERR_OK;
	}

	//pcb->bluetoothif->netmask.addr = htonl(~(ntohl(pcb->bluetoothif->gw.addr) ^ ntohl(pcb->bluetoothif->ip_addr.addr)));

	nat_init(pcb->bluetoothif, ip_input); /* Set function to be called by NAT to pass a packet up to the TCP/IP 
											 stack */

	ret = lap_init(); /* Initialize the LAP role */

	//if(bt_spp_state.profile == DUN_PROFILE) {
	/* Make LAP discoverable */
	hci_cmd_complete(command_complete);
	hci_set_event_filter(0x02, 0x00, &flag); /* Auto accept all connections with role switch enabled */
	//} 

	//tcppcb = tcp_new();
	//IP4_ADDR(&ipaddr, 130,240,45,234);

	//tcp_connect(tcppcb, &ipaddr, 8989, tcp_connected);

	return ret;
}
#endif

/*
 * at_input():
 *
 * Called by RFCOMM during the DUN profile GPRS connection attempt.
 * When a GPRS connection is established, PPP is connected.
 *
 */
u8_t at_state;
err_t at_input(void *arg, struct rfcomm_pcb *pcb, struct pbuf *p, err_t err) 
{
#if 0
	struct ppp_pcb *ppppcb;
	struct ip_addr ipaddr, netmask, gw;
	struct netif *netif;;

	//  u16_t i;
	struct pbuf *q;

	//for(q = p; q != NULL; q = q->next) {
	//  for(i = 0; i < q->len; ++i) {
	//    LWIP_DEBUGF(BT_SPP_DEBUG, ("at_input: 0x%x\n",((u8_t *)p->payload)[i]));
	//  }
	//  LWIP_DEBUGF(BT_SPP_DEBUG, ("*\n"));
	//}

	LWIP_DEBUGF(BT_SPP_DEBUG, ("at_input: %s\n", ((u8_t *)p->payload)));
	LWIP_DEBUGF(BT_SPP_DEBUG, ("state == %d\n", at_state));
	if(at_state == 0 && ((u8_t *)p->payload)[2] == 'O') {
		//q = pbuf_alloc(PBUF_RAW, sizeof("AT&F\r")-1, PBUF_RAM);
		//((u8_t *)q->payload) = "AT&F\r";
		q = pbuf_alloc(PBUF_RAW, sizeof("ATE1\r"), PBUF_RAM);
		((u8_t *)q->payload) = "ATE1\r";
		if(rfcomm_cl(pcb)) {
			rfcomm_uih_credits(pcb, 2, q);
		} else {
			rfcomm_uih(pcb, rfcomm_cn(pcb), q);
		}
		pbuf_free(q);

		at_state = 1;
	} else if(at_state == 1 && ((u8_t *)p->payload)[2] == 'O') {
		q = pbuf_alloc(PBUF_RAW, sizeof("AT+cgdcont=1,\"IP\",\"online.telia.se\"\r"), PBUF_RAM);
		((u8_t *)q->payload) = "AT+cgdcont=1,\"IP\",\"online.telia.se\"\r";
		if(rfcomm_cl(pcb)) {
			rfcomm_uih_credits(pcb, 2, q);
		} else {
			rfcomm_uih(pcb, rfcomm_cn(pcb), q);
		}
		pbuf_free(q);

		at_state = 4;
	} else if(at_state == 4 && ((u8_t *)p->payload)[2] == 'O') {
		q = pbuf_alloc(PBUF_RAW, sizeof("ATD*99***1#\r"), PBUF_RAM);
		((u8_t *)q->payload) = "ATD*99***1#\r";
		if(rfcomm_cl(pcb)) {
			rfcomm_uih_credits(pcb, 2, q);
		} else {
			rfcomm_uih(pcb, rfcomm_cn(pcb), q);
		}
		pbuf_free(q);

		at_state = 5;
	} else if(at_state == 5 && ((u8_t *)p->payload)[2] == 'C') {
		at_state = 6;
		/* Establish a PPP connection */
		if((ppppcb = ppp_new(pcb)) == NULL) {
			LWIP_DEBUGF(BT_SPP_DEBUG, ("rfcomm_msc_rsp: Could not alloc PPP pcb\n"));
			return ERR_MEM;
		}
		/* Add PPP network interface to lwIP and initialize NAT */
		gw.addr = 0;
		ipaddr.addr = 0;
		IP4_ADDR(&netmask, 255,255,255,0);

		netif = netif_add(&ipaddr, &netmask, &gw, NULL, bluetoothif_init, nat_input);

		netif_set_default(netif);

		ppp_netif(ppppcb, netif);

		rfcomm_recv(pcb, ppp_input);
		ppp_disconnected(ppppcb, ppp_is_disconnected);
		return ppp_connect(ppppcb, ppp_connected);
	}
	pbuf_free(p);
#endif
	return ERR_OK;
}

/*
 * pin_req():
 *
 * Called by HCI when a request for a PIN code has been received. A PIN code
 * is required to create a new link key.
 * Replys to the request with the given PIN code
 */
err_t pin_req(void *arg, struct bd_addr *bdaddr)
{
	//u8_t pin[] = "1234";
	u8_t pin[] = "0000";
	LWIP_DEBUGF(BT_SPP_DEBUG, ("pin_req\n"));
	TRACE_INFO("pin_req\r\n");
	return hci_pin_code_request_reply(bdaddr, 4, pin);
}

/*
 * link_key_not():
 *
 * Called by HCI when a new link key has been created for the connection
 * Writes the key to the Bluetooth host controller, where it can be stored for future
 * connection attempts.
 *
 */
err_t link_key_not(void *arg, struct bd_addr *bdaddr, u8_t *key)
{
	LWIP_DEBUGF(BT_SPP_DEBUG, ("link_key_not\n"));
	TRACE_INFO("link_key_not\r\n");

    struct bt_bdaddr_link_key *ev_bdaddr_link_key = malloc(sizeof(struct bt_bdaddr_link_key));
    if (ev_bdaddr_link_key == NULL) {
        panic("link_key_not"); 
    }
    byte_memcpy(&ev_bdaddr_link_key->bdaddr, bdaddr, sizeof(struct bd_addr));
    memcpy(&ev_bdaddr_link_key->link_key, key, sizeof(struct bt_link_key));

    trace_bytes("bdaddr", bdaddr, BT_BDADDR_LEN);
    trace_bytes("linkkey", key, BT_LINK_KEY_LEN);

    event ev;
    ev.type = EVENT_BT;
    ev.data.bt.type = EVENT_BT_LINK_KEY_NOT;
    ev.data.bt.sock = arg;
    ev.data.bt.param.ptr = ev_bdaddr_link_key;
    event_post(&ev);

	return hci_write_stored_link_key(bdaddr, key); /* Write link key to be stored in the
													  Bluetooth host controller */
}

/*
 * link_key_req():
 *
 * Called by HCI when a link key is requested to create a new connection
 *
 */
err_t link_key_req(void *arg, struct bd_addr *bdaddr)
{
	LWIP_DEBUGF(BT_SPP_DEBUG, ("link_key_req\n"));
	TRACE_INFO("link_key_req\r\n");

    struct bd_addr *ev_bdaddr = malloc(sizeof(struct bd_addr));
    
    if (ev_bdaddr == NULL) {
        panic("link_key_req"); 
    }

    byte_memcpy(ev_bdaddr, bdaddr, sizeof(struct bd_addr));

    event ev;
    ev.type = EVENT_BT;
    ev.data.bt.type = EVENT_BT_LINK_KEY_REQ;
    ev.data.bt.sock = arg;
    ev.data.bt.param.ptr = ev_bdaddr;
    event_post(&ev);
}

/*
 * l2cap_disconnected_cfm():
 *
 * Called by L2CAP to confirm that the L2CAP disconnection request was successful
 *
 */
err_t l2cap_disconnected_cfm(void *arg, struct l2cap_pcb *pcb) 
{
	LWIP_DEBUGF(BT_SPP_DEBUG, ("l2cap_disconnected_cfm\n"));
	l2cap_close(pcb);
	return ERR_OK;
}

/*
 * get_rfcomm_cn():
 *
 * Parse the RFCOMM channel number from an SDP attribute list
 *
 */
u8_t get_rfcomm_cn(u16_t attribl_bc, struct pbuf *attribute_list)
{
	u8_t i;
TRACE_BT(">>>get_rfcomm_cn %d\r\n", attribl_bc);
	for(i = 0; i < attribl_bc; i++) {
		if(((u8_t *)attribute_list->payload)[i] == (SDP_DE_TYPE_UUID | SDP_DE_SIZE_16)) {
			//if(ntohs(*((u16_t *)(((u8_t *)attribute_list->payload)+i+1))) == 0x0003) {
			if(ntohs(U16LE2CPU((u8_t *)attribute_list->payload + i + 1)) == 0x0003) {
				return *(((u8_t *)attribute_list->payload)+i+4);
			}
		}
	}
	return 0;
}

/*
 * rfcomm_connected():
 *
 * Called by RFCOMM when a connection attempt response was received.
 * Creates a RFCOMM connection for the channel retreived from SDP.
 * Initializes a search for other devices if the connection attempt failed.
 */
err_t rfcomm_connected(void *arg, struct rfcomm_pcb *pcb, err_t err) 
{
	//struct pbuf *p;
	//struct ip_addr ipaddr, netmask, gw;
	//struct netif *netif;
	//struct ppp_pcb *ppppcb;

	if(err == ERR_OK) {
		LWIP_DEBUGF(BT_SPP_DEBUG, ("rfcomm_connected. CN = %d\n", rfcomm_cn(pcb)));
		rfcomm_disc(pcb, rfcomm_disconnected);
#if 0
		if(bt_spp_state.profile == DUN_PROFILE) {
			/* Establish a GPRS connection */
			LWIP_DEBUGF(BT_SPP_DEBUG, ("rfcomm_msc_rsp: Establish a GPRS connection\n"));
			rfcomm_recv(pcb, at_input);
			//p = pbuf_alloc(PBUF_RAW, sizeof("ATZ\r")-1, PBUF_RAM);
			//((u8_t *)p->payload) = "ATZ\r";
			p = pbuf_alloc(PBUF_RAW, sizeof("AT\r"), PBUF_RAM);
			((u8_t *)p->payload) = "AT\r";
			at_state = 0;
			if(rfcomm_cl(pcb)) {
				rfcomm_uih_credits(pcb, 6,  p);
			} else {
				rfcomm_uih(pcb, rfcomm_cn(pcb), p);
			}
			pbuf_free(p);
		} else {
			/* Establish a PPP connection */
			//if((ppppcb = ppp_new(pcb)) == NULL) {
			//	LWIP_DEBUGF(BT_SPP_DEBUG, ("rfcomm_msc_rsp: Could not alloc PPP pcb\n"));
			//	return ERR_MEM;
			//}

			/* Add PPP network interface to lwIP and initialize NAT */
			//gw.addr = 0;
			//ipaddr.addr = 0;
			//IP4_ADDR(&netmask, 255,255,255,0);

			//netif = netif_add(&ipaddr, &netmask, &gw, NULL, bluetoothif_init, nat_input);

			//netif_set_default(netif);

			//ppp_netif(ppppcb, netif);

			rfcomm_recv(pcb, ppp_input);
			//ppp_disconnected(ppppcb, ppp_is_disconnected);
			return ERR_OK; //ppp_connect(ppppcb, ppp_connected);
		}
#endif
        // MV
	    if(rfcomm_cn(pcb) != 0) {
            rfcomm_recv(pcb, spp_recv);

            bt_socket *sock = (bt_socket*)arg;
            sock->state = BT_SOCKET_STATE_RFCOMM_CONNECTED;
            sock->pcb = pcb;

            event ev;
            ev.type = EVENT_BT;
            ev.data.bt.type = EVENT_BT_RFCOMM_CONNECTED;
            ev.data.bt.sock = sock;
            event_post(&ev);

            //bt_rfcomm_send(pcb, "AT*SEAM=\"MBW-150\",13\r");
        }
	} else {
		LWIP_DEBUGF(BT_SPP_DEBUG, ("rfcomm_connected. Connection attempt failed CN = %d\n", rfcomm_cn(pcb)));
		l2cap_close(pcb->l2cappcb);
		rfcomm_close(pcb);
		//MV bt_spp_start();
	}
	return ERR_OK;
}

/*
 * sdp_attributes_recv():
 *
 * Can be used as a callback by SDP when a response to a service attribute request or 
 * a service search attribute request was received.
 * Disconnects the L2CAP SDP channel and connects to the RFCOMM one.
 * If no RFCOMM channel was found it initializes a search for other devices.
 */
void sdp_attributes_recv(void *arg, struct sdp_pcb *sdppcb, u16_t attribl_bc, struct pbuf *p)
{
	struct l2cap_pcb *l2cappcb;

	l2ca_disconnect_req(sdppcb->l2cappcb, l2cap_disconnected_cfm);
	/* Get the RFCOMM channel identifier from the protocol descriptor list */
	if((bt_spp_state.cn = get_rfcomm_cn(attribl_bc, p)) != 0) {
		if((l2cappcb = l2cap_new()) == NULL) {
			LWIP_DEBUGF(BT_SPP_DEBUG, ("sdp_attributes_recv: Could not alloc L2CAP pcb\n"));
			return;
		}
		LWIP_DEBUGF(BT_SPP_DEBUG, ("sdp_attributes_recv: RFCOMM channel: %d\n", bt_spp_state.cn));

		//if(bt_spp_state.profile == DUN_PROFILE) {
		//	l2ca_connect_req(l2cappcb, &(sdppcb->l2cappcb->remote_bdaddr), RFCOMM_PSM, 0, l2cap_connected);
		//} else {
			l2ca_connect_req(l2cappcb, &(sdppcb->l2cappcb->remote_bdaddr), RFCOMM_PSM, HCI_ALLOW_ROLE_SWITCH, l2cap_connected);
		//}

	} else {
		bt_spp_start();
	}
	sdp_free(sdppcb);
}

/*
 * l2cap_connected():
 *
 * Called by L2CAP when a connection response was received.
 * Sends a L2CAP configuration request.
 * Initializes a search for other devices if the connection attempt failed.
 */
err_t l2cap_connected(void *arg, struct l2cap_pcb *l2cappcb, u16_t result, u16_t status)
{
	struct sdp_pcb *sdppcb;
	struct rfcomm_pcb *rfcommpcb;

	u8_t ssp[] = {0x35, 0x03, 0x19, 0x11, 0x01}; /* Service search pattern with LAP UUID is default */ 
    // 0x8e771401
	err_t ret;

	u8_t attrids[] = {0x35, 0x03, 0x09, 0x00, 0x04}; /* Attribute IDs to search for in data element 
														sequence form */

	if(result == L2CAP_CONN_SUCCESS) {
		LWIP_DEBUGF(BT_SPP_DEBUG, ("l2cap_connected: L2CAP connected pcb->state = %d\n", l2cappcb->state));
		/* Tell L2CAP that we wish to be informed of a disconnection request */
		l2cap_disconnect_ind(l2cappcb, l2cap_disconnected_ind);
		switch(l2cap_psm(l2cappcb)) {
			case SDP_PSM:
				LWIP_DEBUGF(BT_SPP_DEBUG, ("l2cap_connected: SDP L2CAP configured. Result = %d\n", result));
				if((sdppcb = sdp_new(l2cappcb)) == NULL) {
					LWIP_DEBUGF(BT_SPP_DEBUG, ("l2cap_connected: Failed to create a SDP PCB\n"));
					return ERR_MEM;
				}

				l2cap_recv(l2cappcb, sdp_recv);

				ret = sdp_service_search_attrib_req(sdppcb, 0xFFFF, ssp, sizeof(ssp),
						attrids, sizeof(attrids), sdp_attributes_recv);
				return ret;

			case RFCOMM_PSM:
				LWIP_DEBUGF(BT_SPP_DEBUG, ("l2cap_connected: RFCOMM L2CAP configured. Result = %d CN = %d\n", result, bt_spp_state.cn));
				l2cap_recv(l2cappcb, rfcomm_input);

				if((rfcommpcb = rfcomm_new(l2cappcb)) == NULL) {
					LWIP_DEBUGF(BT_SPP_DEBUG, ("l2cap_connected: Failed to create a RFCOMM PCB\n"));
					return ERR_MEM;
				}

				//MV hci_link_key_not(link_key_not); /* Set function to be called if a new link key is created */

                // MV
	            rfcomm_disc(rfcommpcb, rfcomm_disconnected);

				return rfcomm_connect(rfcommpcb, bt_spp_state.cn, rfcomm_connected); /* Connect with DLCI 0 */
			default:
				return ERR_VAL;
		}
	} else {
		LWIP_DEBUGF(BT_SPP_DEBUG, ("l2cap_connected: L2CAP not connected. Redo inquiry\n"));
		l2cap_close(l2cappcb);
		//MV bt_spp_start();
        // RESPONSE
	}

	return ERR_OK;
}

/*
 * inquiry_complete():
 *
 * Called by HCI when a inquiry complete event was received.
 * Connects to the first device in the list.
 * Initializes a search for other devices if the inquiry failed.
 */
err_t inquiry_complete(void *arg, struct hci_pcb *pcb, struct hci_inq_res *ires, u16_t result)
{
	struct l2cap_pcb *l2cappcb;

	if(result == HCI_SUCCESS) {
		LWIP_DEBUGF(BT_SPP_DEBUG, ("successful Inquiry\n"));
		if(ires != NULL) {
			LWIP_DEBUGF(BT_SPP_DEBUG, ("Initiate L2CAP connection\n"));
			LWIP_DEBUGF(BT_SPP_DEBUG, ("ires->psrm %d\n ires->psm %d\n ires->co %d\n", ires->psrm, ires->psm, ires->co));
			LWIP_DEBUGF(BT_SPP_DEBUG, ("ires->bdaddr %02x:%02x:%02x:%02x:%02x:%02x\n",
						ires->bdaddr.addr[5], ires->bdaddr.addr[4], ires->bdaddr.addr[3],
						ires->bdaddr.addr[2], ires->bdaddr.addr[1], ires->bdaddr.addr[0]));

			/*if((ires->cod[1] & 0x1F) == 0x03) {
				bt_spp_state.profile = LAP_PROFILE;
			} else {
				bt_spp_state.profile = DUN_PROFILE;
			}*/

			if((l2cappcb = l2cap_new()) == NULL) {
				LWIP_DEBUGF(BT_SPP_DEBUG, ("inquiry_complete: Could not alloc L2CAP pcb\n"));
				return ERR_MEM;
			} 

			//if(bt_spp_state.profile == DUN_PROFILE) {
			//	l2ca_connect_req(l2cappcb, &(ires->bdaddr), SDP_PSM, 0, l2cap_connected);
			//} else {
				l2ca_connect_req(l2cappcb, &(ires->bdaddr), SDP_PSM, HCI_ALLOW_ROLE_SWITCH, l2cap_connected);
			//}
		} else {
			hci_inquiry(0x009E8B33, 0x04, 0x01, inquiry_complete);
		}
	} else {
		LWIP_DEBUGF(BT_SPP_DEBUG, ("Unsuccessful Inquiry.\n"));
		hci_inquiry(0x009E8B33, 0x04, 0x01, inquiry_complete);
	}
	return ERR_OK;
}

/*
 * acl_wpl_complete():
 *
 * Called by HCI when a successful write link policy settings complete event was
 * received.
 */
err_t acl_wpl_complete(void *arg, struct bd_addr *bdaddr)
{
	hci_sniff_mode(bdaddr, 200, 100, 10, 10);
	return ERR_OK;
}

/*
 * acl_conn_complete():
 *
 * Called by HCI when a connection complete event was received.
 */
err_t acl_conn_complete(void *arg, struct bd_addr *bdaddr)
{
	hci_wlp_complete(acl_wpl_complete);
	hci_write_link_policy_settings(bdaddr, HCI_LP_ENABLE_ROLE_SWITCH | HCI_LP_ENABLE_HOLD_MODE | HCI_LP_ENABLE_SNIFF_MODE | HCI_LP_ENABLE_PARK_STATE);
	return ERR_OK;
}

/*
 * read_bdaddr_complete():
 *
 * Called by HCI when a read local bluetooth device address complete event was received.
 */
err_t read_bdaddr_complete(void *arg, struct bd_addr *bdaddr)
{
TRACE_BT("bdaddr %x %x\r\n", &(bt_spp_state.bdaddr), bdaddr);
	//memcpy((void*)&(bt_spp_state.bdaddr), (void*)bdaddr, 6);
    int i;
    for(i = 0; i < BD_ADDR_LEN; i++ ) {
        TRACE_BT("bdaddr[%d]=%x\r\n", i, bdaddr->addr[i]);
        bt_spp_state.bdaddr.addr[i] = bdaddr->addr[i];
    }
	LWIP_DEBUGF(BT_SPP_DEBUG, ("read_bdaddr_complete: %02x:%02x:%02x:%02x:%02x:%02x\n",
				bdaddr->addr[5], bdaddr->addr[4], bdaddr->addr[3],
				bdaddr->addr[2], bdaddr->addr[1], bdaddr->addr[0]));
	return ERR_OK;
}

/*
 * command_complete():
 *
 * Called by HCI when an issued command has completed during the initialization of the
 * host controller. Waits for a connection from remote device once connected.
 *
 * Event Sequence:
 * HCI Reset -> Read Buf Size -> Read BDAddr -> Set Ev Filter -+
 * +-----------------------------------------------------------+
 * |_/-> Write CoD -> Cng Local Name -> Write Pg Timeout -> Inq -> Complete
 *   \-> Scan Enable -> Complete
 */
err_t command_complete(void *arg, struct hci_pcb *pcb, u8_t ogf, u8_t ocf, u8_t result)
{
	//u8_t cod_spp[] = {0x08,0x04,0x24};
	u8_t cod_wearable[] = {0x00,0x07,0x04};
	//u8_t devname[] = "iAirlink----";
	u8_t devname[] = "DynawaTCH---";
	u8_t n1, n2, n3;
	u8_t flag = HCI_SET_EV_FILTER_AUTOACC_ROLESW;

	switch(ogf) {
		case HCI_INFO_PARAM:
			switch(ocf) {
				case HCI_READ_BUFFER_SIZE:
					if(result == HCI_SUCCESS) {
						LWIP_DEBUGF(BT_SPP_DEBUG, ("successful HCI_READ_BUFFER_SIZE.\n"));
                      
						hci_read_bd_addr(read_bdaddr_complete);
					} else {
						LWIP_DEBUGF(BT_SPP_DEBUG, ("Unsuccessful HCI_READ_BUFFER_SIZE.\n"));
						return ERR_CONN;
					}
					break;
				case HCI_READ_BD_ADDR:
					if(result == HCI_SUCCESS) {
						LWIP_DEBUGF(BT_SPP_DEBUG, ("successful HCI_READ_BD_ADDR.\n"));
#if TEST
                        if (app_initialized) 
						    hci_read_bd_addr(read_bdaddr_complete);
                        else
#endif
						/* Make discoverable */
						hci_set_event_filter(HCI_SET_EV_FILTER_CONNECTION,
								HCI_SET_EV_FILTER_ALLDEV, &flag);

					} else {
						LWIP_DEBUGF(BT_SPP_DEBUG, ("Unsuccessful HCI_READ_BD_ADDR.\n"));
						return ERR_CONN;
					}
					break;
				default:
					LWIP_DEBUGF(BT_SPP_DEBUG, ("Unknown HCI_INFO_PARAM command complete event\n"));
					break;
			}
			break;
		case HCI_HC_BB_OGF:
			switch(ocf) {
				case HCI_RESET:
					if(result == HCI_SUCCESS) {
						LWIP_DEBUGF(BT_SPP_DEBUG, ("successful HCI_RESET.\n")); 
						hci_read_buffer_size();
					} else {
						LWIP_DEBUGF(BT_SPP_DEBUG, ("Unsuccessful HCI_RESET.\n"));
						return ERR_CONN;
					}
					break;
				case HCI_WRITE_SCAN_ENABLE:
					if(result == HCI_SUCCESS) {
						LWIP_DEBUGF(BT_SPP_DEBUG, ("successful HCI_WRITE_SCAN_ENABLE.\n")); 
						//hci_cmd_complete(NULL); /* Initialization done, don't come back */
                        //MV hci_write_cod(cod_spp);
					} else {
						LWIP_DEBUGF(BT_SPP_DEBUG, ("Unsuccessful HCI_WRITE_SCAN_ENABLE.\n"));
						return ERR_CONN;
					}
					break;
				case HCI_SET_EVENT_FILTER:
					if(result == HCI_SUCCESS) {
						LWIP_DEBUGF(BT_SPP_DEBUG, ("successful HCI_SET_EVENT_FILTER.\n"));
                        //hci_write_cod(cod_spp);
                        //hci_write_cod(cod_wearable);
                        hci_write_cod(bt_cod);
                        // discoverable
                        hci_write_scan_enable(HCI_SCAN_EN_INQUIRY | HCI_SCAN_EN_PAGE);
                        // nondiscoverable
                        //hci_write_scan_enable(HCI_SCAN_EN_PAGE);
					} else {
						LWIP_DEBUGF(BT_SPP_DEBUG, ("Unsuccessful HCI_SET_EVENT_FILTER.\n"));
						return ERR_CONN;
					}
					break;
				case HCI_CHANGE_LOCAL_NAME:
					if(result == HCI_SUCCESS) {
						LWIP_DEBUGF(BT_SPP_DEBUG, ("Successful HCI_CHANGE_LOCAL_NAME.\n"));
						hci_write_page_timeout(0x4000); /* 10.24s */
					} else {
						LWIP_DEBUGF(BT_SPP_DEBUG, ("Unsuccessful HCI_CHANGE_LOCAL_NAME.\n"));
						return ERR_CONN;
					}
					break;
				case HCI_WRITE_COD:
					if(result == HCI_SUCCESS) {
						LWIP_DEBUGF(BT_SPP_DEBUG, ("Successful HCI_WRITE_COD.\n"));
						n1 = (u8_t)(bt_spp_state.bdaddr.addr[0] / 100);
						n2 = (u8_t)(bt_spp_state.bdaddr.addr[0] / 10) - n1 * 10;
						n3 = bt_spp_state.bdaddr.addr[0] - n1 * 100 - n2 * 10;
						devname[9] = '0' + n1;
						devname[10] = '0' + n2;
						devname[11] = '0' + n3;
						hci_change_local_name(devname, sizeof(devname));
					} else {
						LWIP_DEBUGF(BT_SPP_DEBUG, ("Unsuccessful HCI_WRITE_COD.\n"));
						return ERR_CONN;
					}
					break;
				case HCI_WRITE_PAGE_TIMEOUT:
					if(result == HCI_SUCCESS) {
						LWIP_DEBUGF(BT_SPP_DEBUG, ("successful HCI_WRITE_PAGE_TIMEOUT.\n"));
						//hci_cmd_complete(NULL); /* Initialization done, don't come back */
#if TEST
                        app_initialized = 1;
                        hci_read_bd_addr(read_bdaddr_complete);
#endif
						hci_connection_complete(acl_conn_complete);
						LWIP_DEBUGF(BT_SPP_DEBUG, ("Initialization done.\n"));
                        
                        event ev;
                        ev.type = EVENT_BT;
                        ev.data.bt.type = EVENT_BT_STARTED;
                        event_post(&ev);

                        //MV
                        //struct bd_addr bdaddr = {0x00, 0x23, 0x76, 0x72, 0x2b, 0xa2}; // HD2
                        //struct bd_addr bdaddr = {0x00, 0x1f, 0xe1, 0xe9, 0x60, 0x04}; // W500
                        //struct bd_addr bdaddr = {0x00, 0x1f, 0xcc, 0x87, 0xb0, 0x37}; // SGH-I780
                        //bt_rfcomm_connect(&bdaddr, 1);
/*
						LWIP_DEBUGF(BT_SPP_DEBUG, ("Discover other Bluetooth devices.\n"));
						hci_inquiry(0x009E8B33, 0x04, 0x01, inquiry_complete); //FAILED????
*/
					} else {
						LWIP_DEBUGF(BT_SPP_DEBUG, ("Unsuccessful HCI_WRITE_PAGE_TIMEOUT.\n"));
						return ERR_CONN;
					}
					break;
				default:
					LWIP_DEBUGF(BT_SPP_DEBUG, ("Unknown HCI_HC_BB_OGF command complete event\n"));
					break;
			}
			break;
		default:
			LWIP_DEBUGF(BT_SPP_DEBUG, ("Unknown command complete event. OGF = 0x%x OCF = 0x%x\n", ogf, ocf));
			break;
	}
	return ERR_OK;
}

// MV


/*
 * sdp_attributes_recv():
 *
 * Can be used as a callback by SDP when a response to a service attribute request or 
 * a service search attribute request was received.
 * Disconnects the L2CAP SDP channel and connects to the RFCOMM one.
 * If no RFCOMM channel was found it initializes a search for other devices.
 */
void sdp_attributes_recv2(void *arg, struct sdp_pcb *sdppcb, u16_t attribl_bc, struct pbuf *p)
{
	struct l2cap_pcb *l2cappcb;
    uint8_t cn;

	l2ca_disconnect_req(sdppcb->l2cappcb, l2cap_disconnected_cfm);
	/* Get the RFCOMM channel identifier from the protocol descriptor list */

	if((cn = get_rfcomm_cn(attribl_bc, p)) != 0) {
		LWIP_DEBUGF(BT_SPP_DEBUG, ("sdp_attributes_recv: RFCOMM channel: %d\n", cn));
    }

    event ev;
    ev.type = EVENT_BT;
    ev.data.bt.type = EVENT_BT_FIND_SERVICE_RESULT;
    ev.data.bt.sock = (bt_socket*)arg;
    ev.data.bt.param.service.cn = cn;
    event_post(&ev);

	sdp_free(sdppcb);
}


/*
 * l2cap_connected():
 *
 * Called by L2CAP when a connection response was received.
 * Sends a L2CAP configuration request.
 * Initializes a search for other devices if the connection attempt failed.
 */

err_t l2cap_connected2(void *arg, struct l2cap_pcb *l2cappcb, u16_t result, u16_t status)
{
	struct sdp_pcb *sdppcb;
	struct rfcomm_pcb *rfcommpcb;

	//u8_t ssp[] = {0x35, 0x03, 0x19, 0x11, 0x01}; /* Service search pattern with LAP UUID is default */ 
	u8_t ssp[] = {0x35, 0x05, 0x1a, 0x8e, 0x77, 0x14, 0x01}; /* Service search pattern with LAP UUID is default */ 
	err_t ret;

	u8_t attrids[] = {0x35, 0x03, 0x09, 0x00, 0x04}; /* Attribute IDs to search for in data element 
														sequence form */

    bt_socket *sock = (bt_socket*)arg;

	if(result == L2CAP_CONN_SUCCESS) {
		LWIP_DEBUGF(BT_SPP_DEBUG, ("l2cap_connected: L2CAP connected pcb->state = %d\n", l2cappcb->state));
		/* Tell L2CAP that we wish to be informed of a disconnection request */
		l2cap_disconnect_ind(l2cappcb, l2cap_disconnected_ind);
		switch(l2cap_psm(l2cappcb)) {
			case SDP_PSM:
				LWIP_DEBUGF(BT_SPP_DEBUG, ("l2cap_connected: SDP L2CAP configured. Result = %d\n", result));
				if((sdppcb = sdp_new(l2cappcb)) == NULL) {
					LWIP_DEBUGF(BT_SPP_DEBUG, ("l2cap_connected: Failed to create a SDP PCB\n"));
					return ERR_MEM;
				}

                sock->state = BT_SOCKET_STATE_SDP_CONNECTED;
                sdp_arg(sdppcb, sock);
				l2cap_recv(l2cappcb, sdp_recv);

				ret = sdp_service_search_attrib_req(sdppcb, 0xFFFF, ssp, sizeof(ssp),
						attrids, sizeof(attrids), sdp_attributes_recv2);
				return ret;

			case RFCOMM_PSM:
				LWIP_DEBUGF(BT_SPP_DEBUG, ("l2cap_connected: RFCOMM L2CAP configured. Result = %d CN = %d\n", result, bt_spp_state.cn));
				l2cap_recv(l2cappcb, rfcomm_input);

				if((rfcommpcb = rfcomm_new(l2cappcb)) == NULL) {
					LWIP_DEBUGF(BT_SPP_DEBUG, ("l2cap_connected: Failed to create a RFCOMM PCB\n"));
					return ERR_MEM;
				}

                sock->state = BT_SOCKET_STATE_RFCOMM_CONNECTING;
                rfcomm_arg(rfcommpcb, sock);
				//MV hci_link_key_not(link_key_not); /* Set function to be called if a new link key is created */

                // MV
	            rfcomm_disc(rfcommpcb, rfcomm_disconnected);

                bt_socket *sock = (bt_socket*)arg;
				return rfcomm_connect(rfcommpcb, sock->cn, rfcomm_connected); /* Connect with DLCI 0 */
			default:
				return ERR_VAL;
		}
	} else {
		LWIP_DEBUGF(BT_SPP_DEBUG, ("l2cap_connected: L2CAP not connected.\n"));
		l2cap_close(l2cappcb);
		//MV bt_spp_start();

        bt_socket *sock = (bt_socket*)arg;
        sock->pcb = NULL;

        event ev;
        ev.type = EVENT_BT;
        //ev.data.bt.type = EVENT_BT_RFCOMM_DISCONNECTED;
        ev.data.bt.type = EVENT_BT_ERROR;
        ev.data.bt.sock = sock;
        ev.data.bt.param.error = BT_SOCKET_ERROR_L2CAP_CANNOT_CONNECT;
        event_post(&ev);
	}

	return ERR_OK;
}

err_t _bt_rfcomm_listen(bt_socket *sock, u8_t cn) {
	struct rfcomm_pcb *rfcommpcb;
	struct sdp_record *record;
    err_t ret;

    if (cn == 0) {
	    LWIP_DEBUGF(RFCOMM_DEBUG, ("_bt_rfcomm_listen: Getting a free port\n"));
	    err_t ret = rfcomm_get_free_port(&cn);
        if (ret != ERR_OK) {
            return ret;
        }
    }

	LWIP_DEBUGF(RFCOMM_DEBUG, ("_bt_rfcomm_listen: Allocate RFCOMM PCB for CN %d\n", cn));
	if((rfcommpcb = rfcomm_new(NULL)) == NULL) {
		LWIP_DEBUGF(BT_SPP_DEBUG, ("_bt_rfcomm_listen: Could not alloc RFCOMM PCB for channel %d\n", cn));
		return ERR_MEM;
	}
    sock->state = BT_SOCKET_STATE_RFCOMM_LISTENING;

    sock->cn = cn;

    rfcomm_arg(rfcommpcb, sock);
	ret = rfcomm_listen(rfcommpcb, cn, rfcomm_accept);
    if (ret != ERR_OK) {
        rfcomm_free(rfcommpcb);
        return ret;
    }

/*
    int i;
    for(i = 0; i < sizeof(spp_service_record); i++) {
        TRACE_INFO(" %x", spp_service_record[i]);
    }
    TRACE_INFO("\r\n");
*/
#if 0
	if((record = sdp_record_new((u8_t *)spp_service_record, sizeof(spp_service_record))) == NULL) {
		LWIP_DEBUGF(BT_SPP_DEBUG, ("_bt_rfcomm_listen: Could not alloc SDP record\n"));
		return ERR_MEM;
	} else {
		sdp_register_service(record);
	}
#endif
    return ERR_OK;
}

err_t _bt_advertise_service(bt_socket *sock, u8_t *record_de_list, u16_t rlen) {

	struct sdp_record *record;

/*
    int i;
    for(i = 0; i < rlen; i++) {
        TRACE_INFO(" %x", record_de_list[i]);
    }
    TRACE_INFO("\r\n");
*/
	if((record = sdp_record_new(record_de_list, rlen)) == NULL) {
		LWIP_DEBUGF(BT_SPP_DEBUG, ("_bt_advertise_service: Could not alloc SDP record\n"));
		return ERR_MEM;
	} else {
		sdp_register_service(record);
	}
    return ERR_OK;
}

err_t _bt_rfcomm_connect(bt_socket *sock, struct bd_addr *bdaddr, u8_t cn) {
	struct l2cap_pcb *l2cappcb;

    if((l2cappcb = l2cap_new()) == NULL) {
        LWIP_DEBUGF(BT_SPP_DEBUG, ("bt_rfcomm_connect: Could not alloc L2CAP pcb\n"));
        return ERR_MEM;
    }
    sock->state = BT_SOCKET_STATE_L2CAP_CONNECTING;

    sock->cn = cn;
    l2cap_arg(l2cappcb, sock);
    LWIP_DEBUGF(BT_SPP_DEBUG, ("bt_rfcomm_connect: RFCOMM channel: %d\n", sock->cn));

    return l2ca_connect_req(l2cappcb, bdaddr, RFCOMM_PSM, HCI_ALLOW_ROLE_SWITCH, l2cap_connected2);
}

err_t _bt_find_service(bt_socket *sock, struct bd_addr *bdaddr) {
	struct l2cap_pcb *l2cappcb;

    if((l2cappcb = l2cap_new()) == NULL) {
        LWIP_DEBUGF(BT_SPP_DEBUG, ("_bt_sdp_search: Could not alloc L2CAP pcb\n"));
        return ERR_MEM;
    } 

    // MV test
    sock->state = BT_SOCKET_STATE_L2CAP_CONNECTING;

    l2cap_arg(l2cappcb, sock);
    l2cap_disconnect_ind(l2cappcb, l2cap_disconnected_ind);

    l2ca_connect_req(l2cappcb, bdaddr, SDP_PSM, HCI_ALLOW_ROLE_SWITCH, l2cap_connected2);
    return ERR_OK;
}

void _bt_inquiry() {
    hci_inquiry(0x009E8B33, 0x04, 0x01, inquiry_complete);
}

void _bt_rfcomm_send(bt_socket *sock, struct pbuf *p) {
    struct rfcomm_pcb *pcb = sock->pcb;

    if (pcb == NULL) {
        panic("_bt_rfcomm_send");
    }

    // TODO: check if pcb valid
    rfcomm_arg(pcb, sock);
    if(rfcomm_cl(pcb)) {
        rfcomm_uih_credits(pcb, PBUF_POOL_SIZE - rfcomm_remote_credits(pcb), p);
    } else {
        rfcomm_uih(pcb, rfcomm_cn(pcb), p);
    }
}

err_t remote_name(void *arg, struct hci_pcb *pcb, struct bd_addr *bdaddr, u8_t *remote_name, u16_t result)
{
    if (result == HCI_SUCCESS) {
        sdp_memlog(bdaddr, BT_BDADDR_LEN);

        event ev;
        ev.type = EVENT_BT;
        ev.data.bt.type = EVENT_BT_REMOTE_NAME;
        byte_memcpy(&ev.data.bt.param.remote_name.bdaddr, bdaddr, BT_BDADDR_LEN);

        int l;
        for(l = 0; l < 248; l++) {
            if (remote_name[l] == '\0') {
                break;
            }
        }
        TRACE_INFO("len %d\r\n", l);
        u8_t *name = malloc(l + 1);
    
        if (name == NULL) {
            panic("remote_name"); 
        }
        // ev.data.bt.param.remote_name.name (UTF8, max 248B. If less, null-terminated)
        if (l) 
            byte_memcpy(name, remote_name, l);
        name[l] = '\0';

        //sdp_memlog(name, l);
        TRACE_INFO("remote_name %s\r\n", name);

        ev.data.bt.param.remote_name.name = name;
        event_post(&ev);
    }
    return ERR_OK;
}

void _bt_remote_name_req(struct bd_addr *bdaddr) {
    hci_remote_name_req(bdaddr, remote_name);
}

void bt_set_cod(u8_t *cod) {
    memcpy(bt_cod, cod, 3);
}
