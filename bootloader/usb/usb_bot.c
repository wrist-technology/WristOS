/**
 * \file usb_bot.c
 * Bulk only transfer 
 * 
 * Bulk only transfer handling functions
 *
 * AT91SAM7S-128 USB Mass Storage Device with SD Card by Michael Wolf\n
 * Copyright (C) 2008 Michael Wolf\n\n
 * 
 * This program is free software: you can redistribute it and/or modify\n
 * it under the terms of the GNU General Public License as published by\n
 * the Free Software Foundation, either version 3 of the License, or\n
 * any later version.\n\n
 * 
 * This program is distributed in the hope that it will be useful,\n
 * but WITHOUT ANY WARRANTY; without even the implied warranty of\n
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n
 * GNU General Public License for more details.\n\n
 * 
 * You should have received a copy of the GNU General Public License\n
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.\n
 *  
 */
#include <stdlib.h>
#include "hardware_conf.h"
#include "firmware_conf.h"
#include <screen/screen.h>
#include <screen/font.h>
#include <debug/trace.h>
#include <utils/macros.h>
#include "usb_drv.h"
#include "usb_msd.h"
#include "usb_sbc.h"
#include "usb_lun.h"
#include "usb_std.h"
#include "usb_bot.h"
#include "usb_sbc_func.h"

extern s_usb_request *pSetup;
extern s_usb_endpoint *pEndpoint[];

S_bot bot_struct;   //!< Structure holding MSD driver state variables
S_bot *pBot = &bot_struct;    //!< Pointer to Bulk-Only Transport (BOT) driver instance


/**
 * Resets the state of the BOT driver
 * 
 */
void bot_reset(void)
{
    TRACE_BOT("MSDReset   ");

    pBot->bState = BOT_STATE_READ_CBW;
    pBot->isWaitResetRecovery = false;
    pBot->sCommandState.bState = false;
}


/**
 * This function is to be used as a callback for USB or LUN transfers.
 * 
 * A S_bot_transfer structure is updated with the method results.
 * \param   pTransfer         Pointer to USB transfer result structure 
 * \param   bStatus           Operation result code
 * \param   dBytesTransferred Number of bytes transferred by the command
 * \param   dBytesRemaining   Number of bytes not transferred
 * 
 */
void bot_callback(S_bot_transfer *pTransfer,
                    unsigned char  bStatus,
                    unsigned int   dBytesTransferred,
                    unsigned int   dBytesRemaining)
{
    TRACE_BOT("Cbk ");
    pTransfer->bSemaphore++;
    pTransfer->bStatus = bStatus;
    pTransfer->dBytesTransferred = dBytesTransferred;
    pTransfer->dBytesRemaining = dBytesRemaining;
}


/**
 * Returns the expected transfer length and direction (IN, OUT or don't
 * care) from the host point-of-view.
 * 
 * \param  pCbw    Pointer to the CBW to examinate
 * \param  pLength Expected length of command
 * \param  pType   Expected direction of command
 * 
 */
static void bot_get_command_information(S_msd_cbw     *pCbw,
                                        unsigned int  *pLength,
                                        unsigned char *pType)
{
    // Expected host transfer direction and length
    (*pLength) = pCbw->dCBWDataTransferLength;

    if (*pLength == 0)
    {
        (*pType) = BOT_NO_TRANSFER;
    }
    else if (ISSET(pCbw->bmCBWFlags, MSD_CBW_DEVICE_TO_HOST))
    {
        (*pType) = BOT_DEVICE_TO_HOST;
    }
    else
    {
        (*pType) = BOT_HOST_TO_DEVICE;
    }
}


/**
 * Pre-processes a command by checking the differences between the
 * host and device expectations in term of transfer type and length.
 * 
 * Once one of the thirteen cases is identified, the actions to do
 * during the post-processing phase are stored in the dCase variable
 * of the command state.
 * 
 * \return  True if the command is supported, false otherwise
 * 
 */
static char bot_pre_process_command(void)
{
    unsigned int        dHostLength;
    unsigned int        dDeviceLength;
    unsigned char       bHostType;
    unsigned char       bDeviceType;
    char                isCommandSupported = false;
    S_bot_command_state *pCommandState = &(pBot->sCommandState);
    S_msd_csw           *pCsw = &(pCommandState->sCsw);
    S_msd_cbw           *pCbw = &(pCommandState->sCbw);

    // Get information about the command
    // Host-side
    bot_get_command_information(pCbw, &dHostLength, &bHostType);

    // Device-side
    isCommandSupported = sbc_get_command_information(pCbw->pCommand,
                                                   &dDeviceLength,
                                                   &bDeviceType);

    // Initialize data residue and result status
    pCsw->dCSWDataResidue = 0;
    pCsw->bCSWStatus = MSD_CSW_COMMAND_PASSED;

    // Check if the command is supported
    if (isCommandSupported)
    {
        // Identify the command case
        // Case 1  (Hn = Dn)
        if ((bHostType == BOT_NO_TRANSFER)
            && (bDeviceType == BOT_NO_TRANSFER))
        {

            pCommandState->bCase = 0;
            pCommandState->dLength = 0;
        }
        // Case 2  (Hn < Di)
        else
        if ((bHostType == BOT_NO_TRANSFER)
                 && (bDeviceType == BOT_DEVICE_TO_HOST))
        {
            TRACE_BOT("BOT_PreProcessCommand: Case 2\n");
            pCommandState->bCase = BOT_CASE_PHASE_ERROR;
            pCommandState->dLength = 0;
        }
        // Case 3  (Hn < Do)
        else
        if ((bHostType == BOT_NO_TRANSFER)
                 && (bDeviceType == BOT_HOST_TO_DEVICE))
        {
            TRACE_BOT("BOT_PreProcessCommand: Case 3\n");
            pCommandState->bCase = BOT_CASE_PHASE_ERROR;
            pCommandState->dLength = 0;
        }
        // Case 4  (Hi > Dn)
        else 
        if ((bHostType == BOT_DEVICE_TO_HOST)
                 && (bDeviceType == BOT_NO_TRANSFER))
        {
            TRACE_BOT("BOT_PreProcessCommand: Case 4\n");
            pCommandState->bCase = BOT_CASE_STALL_IN;
            pCommandState->dLength = 0;
            pCsw->dCSWDataResidue = dHostLength;
        }
        // Case 5  (Hi > Di)
        else 
        if ((bHostType == BOT_DEVICE_TO_HOST)
                 && (bDeviceType == BOT_DEVICE_TO_HOST)
                 && (dHostLength > dDeviceLength))
        {
            TRACE_BOT("BOT_PreProcessCommand: Case 5\n");
            pCommandState->bCase = BOT_CASE_STALL_IN;
            pCommandState->dLength = dDeviceLength;
            pCsw->dCSWDataResidue = dHostLength - dDeviceLength;
        }
        // Case 6  (Hi = Di)
        if ((bHostType == BOT_DEVICE_TO_HOST)
            && (bDeviceType == BOT_DEVICE_TO_HOST)
            && (dHostLength == dDeviceLength))
        {
            pCommandState->bCase = 0;
            pCommandState->dLength = dDeviceLength;
        }
        // Case 7  (Hi < Di)
        else
        if ((bHostType == BOT_DEVICE_TO_HOST)
                 && (bDeviceType == BOT_DEVICE_TO_HOST)
                 && (dHostLength < dDeviceLength))
        {
            TRACE_BOT("BOT_PreProcessCommand: Case 7\n");
            pCommandState->bCase = BOT_CASE_PHASE_ERROR;
            pCommandState->dLength = dHostLength;
        }
        // Case 8  (Hi <> Do)
        else 
        if ((bHostType == BOT_DEVICE_TO_HOST)
                 && (bDeviceType == BOT_HOST_TO_DEVICE))
        {
            TRACE_BOT("BOT_PreProcessCommand: Case 8\n");
            pCommandState->bCase = BOT_CASE_STALL_IN | BOT_CASE_PHASE_ERROR;
            pCommandState->dLength = 0;
        }
        // Case 9  (Ho > Dn)
        else 
        if ((bHostType == BOT_HOST_TO_DEVICE)
                 && (bDeviceType == BOT_NO_TRANSFER))
        {
            TRACE_BOT("BOT_PreProcessCommand: Case 9\n");
            pCommandState->bCase = BOT_CASE_STALL_OUT;
            pCommandState->dLength = 0;
            pCsw->dCSWDataResidue = dHostLength;
        }
        // Case 10 (Ho <> Di)
        else
        if ((bHostType == BOT_HOST_TO_DEVICE)
                 && (bDeviceType == BOT_DEVICE_TO_HOST))
        {
            TRACE_BOT("BOT_PreProcessCommand: Case 10\n");
            pCommandState->bCase = BOT_CASE_STALL_OUT | BOT_CASE_PHASE_ERROR;
            pCommandState->dLength = 0;
        }
        // Case 11 (Ho > Do)
        else
        if ((bHostType == BOT_HOST_TO_DEVICE)
                 && (bDeviceType == BOT_HOST_TO_DEVICE)
                 && (dHostLength > dDeviceLength))
        {
            TRACE_BOT("BOT_PreProcessCommand: Case 11\n");
            pCommandState->bCase = BOT_CASE_STALL_OUT;
            pCommandState->dLength = dDeviceLength;
            pCsw->dCSWDataResidue = dHostLength - dDeviceLength;
        }
        // Case 12 (Ho = Do)
        else
        if ((bHostType == BOT_HOST_TO_DEVICE)
                 && (bDeviceType == BOT_HOST_TO_DEVICE)
                 && (dHostLength == dDeviceLength))
        {
            pCommandState->bCase = 0;
            pCommandState->dLength = dDeviceLength;
        }
        // Case 13 (Ho < Do)
        else
        if ((bHostType == BOT_HOST_TO_DEVICE)
                 && (bDeviceType == BOT_HOST_TO_DEVICE)
                 && (dHostLength < dDeviceLength))
        {
            TRACE_BOT("BOT_PreProcessCommand: Case 13\n");
            pCommandState->bCase = BOT_CASE_PHASE_ERROR;
            pCommandState->dLength = dHostLength;
        }
    }

    return isCommandSupported;
}


/**
 * Post-processes a command given the case identified during the
 * pre-processing step.
 * 
 * Depending on the case, one of the following actions can be done:
 * - Bulk IN endpoint is stalled
 * - Bulk OUT endpoint is stalled
 * - CSW status set to phase error
 * 
 */
static void bot_post_process_command(void)
{
    S_bot_command_state *pCommandState = &(pBot->sCommandState);
    S_msd_csw           *pCsw = &(pCommandState->sCsw);

    // STALL Bulk IN endpoint ?
    if (ISSET(pCommandState->bCase, BOT_CASE_STALL_IN))
    {
        TRACE_BOT("StallIn ");
        usb_halt(BOT_EPT_BULK_IN, USB_SET_FEATURE);
    }

    // STALL Bulk OUT endpoint ?
    if (ISSET(pCommandState->bCase, BOT_CASE_STALL_OUT))
    {
        TRACE_BOT("StallOut ");
        usb_halt(BOT_EPT_BULK_OUT, USB_SET_FEATURE);
    }

    // Set CSW status code to phase error ?
    if (ISSET(pCommandState->bCase, BOT_CASE_PHASE_ERROR))
    {
        TRACE_BOT("PhaseErr ");
        pCsw->bCSWStatus = MSD_CSW_PHASE_ERROR;
    }
}


/**
 * Processes the latest command received by the device.
 * 
 * \return  True if the command has been completed, false otherwise.
 * 
 */
static bool bot_process_command(void)
{
    unsigned char       bStatus;
    S_bot_command_state *pCommandState = &(pBot->sCommandState);
    S_msd_cbw           *pCbw = &(pCommandState->sCbw);
    S_msd_csw           *pCsw = &(pCommandState->sCsw);
    bool                isCommandComplete = false;

    // Check if LUN is valid
    if (pCbw->bCBWLUN > pBot->bMaxLun)
    {
        TRACE_BOT("BOT_ProcessCommand: Requested LUN does not exist\n");
        bStatus = BOT_STATUS_ERROR;
    }
    else
    {
        // Process command
        if (pBot->bMaxLun > 0) {
            TRACE_BOT("LUN%d ", pCbw->bCBWLUN);
        }
        bStatus = sbc_process_command(pCommandState);
    }

    // Check command result code
    if (bStatus == BOT_STATUS_PARAMETER)
    {
        TRACE_BOT("BOT_ProcessCommand: Unknown command 0x%02X\n",
                      pCbw->pCommand[0]);

        // Update sense data
        sbc_update_sense_data(&(pLun->sRequestSenseData),
                            SBC_SENSE_KEY_ILLEGAL_REQUEST,
                            SBC_ASC_INVALID_COMMAND_OPERATION_CODE,
                            /* SBC_ASC_INVALID_FIELD_IN_CDB, */
                            0);

        // Result codes
        pCsw->bCSWStatus = MSD_CSW_COMMAND_FAILED;
        isCommandComplete = true;

        // stall the request, IN or OUT
        if (!ISSET(pCbw->bmCBWFlags, MSD_CBW_DEVICE_TO_HOST)
            && (pCbw->dCBWDataTransferLength > 0))
        {

            // Stall the OUT endpoint : host to device
            usb_halt(BOT_EPT_BULK_OUT, USB_SET_FEATURE);
            TRACE_BOT("StaOUT ");
        }
        else
        {

            // Stall the IN endpoint : device to host
            usb_halt(BOT_EPT_BULK_IN, USB_SET_FEATURE);
            TRACE_BOT("StaIN ");
        }
    }
    else
    if (bStatus == BOT_STATUS_ERROR)
    {
        TRACE_BOT("MSD_ProcessCommand: Command failed\n");

        // Update sense data
        sbc_update_sense_data(&(pLun->sRequestSenseData),
                            SBC_SENSE_KEY_MEDIUM_ERROR,
                            SBC_ASC_INVALID_FIELD_IN_CDB,
                            0);

        // Result codes
        pCsw->bCSWStatus = MSD_CSW_COMMAND_FAILED;
        isCommandComplete = true;
    }
    else
    {
        // Update sense data
        sbc_update_sense_data(&(pLun->sRequestSenseData),
                            SBC_SENSE_KEY_NO_SENSE,
                            0,
                            0);

        // Is command complete ?
        if (bStatus == BOT_STATUS_SUCCESS)
        {
            isCommandComplete = true;
        }
    }

    // Check if command has been completed
    if (isCommandComplete)
    {
        TRACE_BOT("Cplt ");

        // Adjust data residue
        if (pCommandState->dLength != 0)
        {
            pCsw->dCSWDataResidue += pCommandState->dLength;

            // STALL the endpoint waiting for data
            if (!ISSET(pCbw->bmCBWFlags, MSD_CBW_DEVICE_TO_HOST))
            {
                // Stall the OUT endpoint : host to device
                usb_halt(BOT_EPT_BULK_OUT, USB_SET_FEATURE);
                TRACE_BOT("StaOUT ");
            }
            else
            {
                // Stall the IN endpoint : device to host
                usb_halt(BOT_EPT_BULK_IN, USB_SET_FEATURE);
                TRACE_BOT("StaIN ");
            }
        }

        // Reset command state
        pCommandState->bState = 0;
    }

    return isCommandComplete;
}


/**
 * Handler for incoming SETUP requests on default Control endpoint 0.
 * 
 * Standard requests are forwarded to the standard request handler.
 * 
 */
void bot_request_handler(void)
{
    TRACE_BOT("BotReq   ");
    
    // Handle requests
    switch (pSetup->bRequest)
    {
        case USB_CLEAR_FEATURE:
            TRACE_BOT("ClrFeat   ");
    
            switch (pSetup->wValue)
            {
                case USB_ENDPOINT_HALT:
                    TRACE_BOT("Hlt ");
        
                    // Do not clear the endpoint halt status if the device is waiting
                    // for a reset recovery sequence
                    if (!pBot->isWaitResetRecovery) {
        
                        // Forward the request to the standard handler
                        usb_std_req_handler();
                    }
                    else {
                        TRACE_BOT("No ");
                    }
                    usb_send_zlp0(0,0);
                    break;
        
                default:
                    // Forward the request to the standard handler
                    usb_std_req_handler();
            }
            break;
    
        case MSD_GET_MAX_LUN:
            TRACE_BOT("gMaxLun   ");
    
            // Check request parameters
            if ((pSetup->wValue == false)
                && (pSetup->wIndex == false)
                && (pSetup->wLength == true)) {
    
                usb_write(0, &(pBot->bMaxLun), 1, 0, 0);
            }
            else {
                TRACE_BOT("Bot_RequestHandler: GetMaxLUN(%d,%d,%d)\n",
                              pSetup->wValue, pSetup->wIndex, pSetup->wLength);
                usb_stall(0);
            }
            break;
    
        case MSD_BULK_ONLY_RESET:
            TRACE_BOT("Rst   ");
            // Check parameters
            if ((pSetup->wValue == 0)
                && (pSetup->wIndex == 0)
                && (pSetup->wLength == 0)) {
    
                // Reset the MSD driver
                bot_reset();
                usb_send_zlp0(0,0);
            }
            else {
                TRACE_BOT("Bot_RequestHandler: Reset(%d,%d,%d)\n",
                              pSetup->wValue, pSetup->wIndex, pSetup->wLength);
                usb_stall(0);
            }
            break;
    
        default:
            // Forward request to standard handler
            usb_std_req_handler();
            break;
    }
}


/**
 * Initializes a BOT driver and the associated USB driver.
 * 
 * \param   bNumLun Number of LUN in list
 * 
 */
void bot_init(unsigned char bNumLun)
{
    TRACE_BOT("MSD init\n");

    // Command state initialization
    pBot->sCommandState.bState = 0;
    pBot->sCommandState.bCase = 0;
    pBot->sCommandState.dLength = 0;
    pBot->sCommandState.sTransfer.bSemaphore = 0;

    // LUNs
    pBot->pLun = pLun;
    pBot->bMaxLun = (unsigned char) (bNumLun - 1);

    // Reset BOT driver
    bot_reset();
}


/**
 * State machine for the BOT driver
 * 
 */
void bot_state_machine(void)
{
    S_bot_command_state *pCommandState = &(pBot->sCommandState);
    S_msd_cbw           *pCbw = &(pCommandState->sCbw);
    S_msd_csw           *pCsw = &(pCommandState->sCsw);
    S_bot_transfer      *pTransfer = &(pCommandState->sTransfer);
    unsigned char       bStatus = USB_STATUS_UNKOWN;

    // Identify current driver state
    switch (pBot->bState)
    {
        case BOT_STATE_READ_CBW:
            // Start the CBW read operation
            pTransfer->bSemaphore = 0;
            bStatus = usb_read(BOT_EPT_BULK_OUT,
                               pCbw,
                               MSD_CBW_SIZE,
                              (Callback_f) bot_callback,
                              (void *) pTransfer);
            // Check operation result code
            if (bStatus == USB_STATUS_SUCCESS) {
                pBot->bState = BOT_STATE_WAIT_CBW;
            }
            break;

        case BOT_STATE_WAIT_CBW:
            // Check transfer semaphore
            if (pTransfer->bSemaphore > 0)
            {
                // Take semaphore and terminate transfer
                pTransfer->bSemaphore--;
    
                // Check if transfer was successful
                if (pTransfer->bStatus == USB_STATUS_SUCCESS)
                {
                    TRACE_BOT("-\n");
                    // Process received command
                    pBot->bState = BOT_STATE_PROCESS_CBW;
                }
                else if (pTransfer->bStatus == USB_STATUS_RESET)
                {
                    TRACE_BOT("BOT_StateMachine: Endpoint resetted\n");
                    pBot->bState = BOT_STATE_READ_CBW;
                }
                else
                {
                    TRACE_BOT("BOT_StateMachine: Failed to read CBW\n");
                    pBot->bState = BOT_STATE_READ_CBW;
                }
            }
            break;

        case BOT_STATE_PROCESS_CBW:
            // Check if this is a new command
            if (pCommandState->bState == 0) {
                // Copy the CBW tag
                pCsw->dCSWTag = pCbw->dCBWTag;
                // Check that the CBW is 31 bytes long
                if ((pTransfer->dBytesTransferred != MSD_CBW_SIZE) ||
                    (pTransfer->dBytesRemaining != 0))
                {
                    TRACE_BOT("BOT_StateMachine: Invalid CBW (too short or too long)\n");
    
                    // Wait for a reset recovery
                    pBot->isWaitResetRecovery = true;
    
                    // Halt the Bulk-IN and Bulk-OUT pipes
                    usb_halt(BOT_EPT_BULK_OUT, USB_SET_FEATURE);
                    usb_halt(BOT_EPT_BULK_IN, USB_SET_FEATURE);
    
                    pCsw->bCSWStatus = MSD_CSW_COMMAND_FAILED;
                    pBot->bState = BOT_STATE_READ_CBW;
                }
                // Check the CBW Signature
                else if (pCbw->dCBWSignature != MSD_CBW_SIGNATURE)
                {
                    TRACE_BOT("MSD_BOTStateMachine: Invalid CBW (Bad signature)\n");
    
                    // Wait for a reset recovery
                    pBot->isWaitResetRecovery = true;
    
                    // Halt the Bulk-IN and Bulk-OUT pipes
                    usb_halt(BOT_EPT_BULK_OUT, USB_SET_FEATURE);
                    usb_halt(BOT_EPT_BULK_IN, USB_SET_FEATURE);
    
                    pCsw->bCSWStatus = MSD_CSW_COMMAND_FAILED;
                    pBot->bState = BOT_STATE_READ_CBW;
                }
                else
                {
                    // Pre-process command
                    bot_pre_process_command();
                }
            }
    
            // Process command
            if (pCsw->bCSWStatus == BOT_STATUS_SUCCESS) {
    
                if (bot_process_command())
                {
                    // Post-process command if it is finished
                    bot_post_process_command();
                    pBot->bState = BOT_STATE_SEND_CSW;
                }
            }
    
            break;

        case BOT_STATE_SEND_CSW:
            // Set signature
            pCsw->dCSWSignature = MSD_CSW_SIGNATURE;
    
            // Start the CSW write operation
            bStatus = usb_write(BOT_EPT_BULK_IN,
                                pCsw,
                                MSD_CSW_SIZE,
                                (Callback_f) bot_callback,
                                (void *) pTransfer);
    
            // Check operation result code
            if (bStatus == USB_STATUS_SUCCESS)
            {
                TRACE_BOT("SendCSW ");
                // Wait for end of transfer
                pBot->bState = BOT_STATE_WAIT_CSW;
            }
            break;

        case BOT_STATE_WAIT_CSW:
            // Check transfer semaphore
            if (pTransfer->bSemaphore > 0)
            {
                // Take semaphore and terminate transfer
                pTransfer->bSemaphore--;
    
                // Check if transfer was successful
                if (bStatus == USB_STATUS_RESET)
                {
                    TRACE_BOT("BOT_StateMachine: Endpoint resetted\n");
                }
                else
                if (bStatus == USB_STATUS_ABORTED)
                {
                    TRACE_BOT("BOT_StateMachine: Failed to send CSW\n");
                }
                else
                {
                    TRACE_BOT("ok");
                }
    
                // Read new CBW
                pBot->bState = BOT_STATE_READ_CBW;
            }
            break;
    }
}
