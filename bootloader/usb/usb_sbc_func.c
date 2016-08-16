/**
 * \file usb_sbc_func.c
 * SCSI Block Commands 
 * 
 * SCSI block commands handling functions
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
#include "hardware_conf.h"
#include "firmware_conf.h"
#include <screen/screen.h>
#include <screen/font.h>
#include <debug/trace.h>
#include <utils/macros.h>
#include <sdcard/sdcard.h>
#include "usb_drv.h"
#include "usb_msd.h"
#include "usb_sbc.h"
#include "usb_lun.h"
#include "usb_std.h"
#include "usb_bot.h"
#include "usb_sbc_func.h"

/**
 * Header for the mode pages data
 * 
 */
static const S_sbc_mode_parameter_header_6 sModeParameterHeader6 =
{
    sizeof(S_sbc_mode_parameter_header_6) - 1,  //!< Length of mode page data is 0x03
    SBC_MEDIUM_TYPE_DIRECT_ACCESS_BLOCK_DEVICE, //!< Direct-access block device
    0,                                          //!< Reserved bits
    false,                                      //!< DPO/FUA not supported
    0,                                          //!< Reserved bits
    false,                                      //!< Medium is not write-protected
    0                                           //!< No block descriptor
};

//extern S_lun pLun[];  //!< LUNs used by the BOT driver

/**
 * Handles an INQUIRY command.
 * 
 * This function operates asynchronously and must be called multiple
 * times to complete. A result code of BOT_STATUS_INCOMPLETE indicates
 * that at least another call of the method is necessary.
 * 
 * \param   pCommandState   Current state of the command
 * \return  Operation result code (SUCCESS, ERROR, INCOMPLETE or PARAMETER)
 * 
 */
static unsigned char sbc_inquiry(S_bot_command_state *pCommandState)
{
    unsigned char  bResult = BOT_STATUS_INCOMPLETE;
    unsigned char  bStatus;
    S_bot_transfer *pTransfer = &(pCommandState->sTransfer);

    // Check if required length is 0
    if (pCommandState->dLength == 0)
    {
        // Nothing to do
        bResult = BOT_STATUS_SUCCESS;
    }
    // Initialize command state if needed
    else
    if (pCommandState->bState == 0)
    {
        pCommandState->bState = SBC_STATE_WRITE;

        // Change additional length field of inquiry data
        pLun->pInquiryData->bAdditionalLength
            = (unsigned char) (pCommandState->dLength - 5);
    }

    // Identify current command state
    switch (pCommandState->bState)
    {
        case SBC_STATE_WRITE:
            // Start write operation
            bStatus = usb_write(BOT_EPT_BULK_IN,
                                (void *) pLun->pInquiryData,
                                pCommandState->dLength,
                                (Callback_f) bot_callback,
                                (void *) pTransfer);
    
            // Check operation result code
            if (bStatus != USB_STATUS_SUCCESS)
            {
                TRACE_SBC("SPC_Inquiry: Cannot start sending data\n");
                bResult = BOT_STATUS_ERROR;
            }
            else
            {
                // Proceed to next state
                TRACE_SBC("Sending   ");
                pCommandState->bState = SBC_STATE_WAIT_WRITE;
            }
            break;

        case SBC_STATE_WAIT_WRITE:
            // Check the semaphore value
            if (pTransfer->bSemaphore > 0)
            {
                // Take semaphore and terminate command
                pTransfer->bSemaphore--;
    
                if (pTransfer->bStatus != USB_STATUS_SUCCESS)
                {
                    TRACE_SBC("SPC_Inquiry: Data transfer failed\n");
                    bResult = BOT_STATUS_ERROR;
                }
                else
                {
                    TRACE_SBC("Sent   ");
                    bResult = BOT_STATUS_SUCCESS;
                }
    
                // Update dLength field
                pCommandState->dLength -= pTransfer->dBytesTransferred;
            }
            break;
    }

    return bResult;
}


/**
 * Performs a READ CAPACITY (10) command.
 * 
 * This function operates asynchronously and must be called multiple
 * times to complete. A result code of BOT_STATUS_INCOMPLETE indicates
 * that at least another call of the method is necessary.
 * 
 * \param   pCommandState   Current state of the command
 * \return  Operation result code (SUCCESS, ERROR, INCOMPLETE or PARAMETER)
 * 
 */
static unsigned char sbc_read_capacity10(S_bot_command_state *pCommandState)
{
    unsigned char bResult = BOT_STATUS_INCOMPLETE;
    unsigned char bStatus;
    S_bot_transfer *pTransfer = &(pCommandState->sTransfer);

    // Initialize command state if needed
    if (pCommandState->bState == 0)
    {
        pCommandState->bState = SBC_STATE_WRITE;
    }

    // Identify current command state
    switch (pCommandState->bState)
    {
        case SBC_STATE_WRITE:
            // Start the write operation
            bStatus = usb_write(BOT_EPT_BULK_IN,
                                &(pLun->sReadCapacityData),
                                pCommandState->dLength,
                                (Callback_f) bot_callback,
                                (void *) pTransfer);
    
            // Check operation result code
            if (bStatus != USB_STATUS_SUCCESS)
            {
                TRACE_SBC("RBC_ReadCapacity: Cannot start sending data\n");
                bResult = BOT_STATUS_ERROR;
            }
            else
            {
                // Proceed to next command state
                TRACE_SBC("Sending   ");
                pCommandState->bState = SBC_STATE_WAIT_WRITE;
            }
            break;

        case SBC_STATE_WAIT_WRITE:
            // Check semaphore value
            if (pTransfer->bSemaphore > 0)
            {
                // Take semaphore and terminate command
                pTransfer->bSemaphore--;
    
                if (pTransfer->bStatus != USB_STATUS_SUCCESS)
                {
                    TRACE_SBC("RBC_ReadCapacity: Cannot send data\n");
                    bResult = BOT_STATUS_ERROR;
                }
                else
                {
                    TRACE_SBC("Sent   ");
                    bResult = BOT_STATUS_SUCCESS;
                }
                pCommandState->dLength -= pTransfer->dBytesTransferred;
            }
            break;
    }

    return bResult;
}


/**
 * Performs a WRITE (10) command on the specified LUN.
 * 
 * The data to write is first received from the USB host and then
 * actually written on the media.
 * 
 * This function operates asynchronously and must be called multiple
 * times to complete. A result code of BOT_STATUS_INCOMPLETE indicates
 * that at least another call of the method is necessary.
 * 
 * \param   pCommandState   Current state of the command
 * \return Operation result code (SUCCESS, ERROR, INCOMPLETE or PARAMETER)
 * 
 */
static unsigned char sbc_write10(S_bot_command_state *pCommandState)
{
    unsigned char  bStatus;
    unsigned char  bResult = BOT_STATUS_INCOMPLETE;
    S_bot_transfer *pTransfer = &(pCommandState->sTransfer);
    S_sbc_write_10 *pCommand = (S_sbc_write_10 *) pCommandState->sCbw.pCommand ;

    // Init command state
    if (pCommandState->bState == 0)
    {
        pCommandState->bState = SBC_STATE_READ;
    }

    // Convert pLength from bytes to blocks
    pCommandState->dLength /= pLun->dBlockSize;

    // Check if pLength equals 0
    if (pCommandState->dLength == 0)
    {
        TRACE_SBC("End ");
        bResult = BOT_STATUS_SUCCESS;
    }
    else
    {
        // Current command status
        switch (pCommandState->bState)
        {
            case SBC_STATE_READ:
                TRACE_SBC("Receive ");
                // Read one block of data sent by the host
                bStatus = usb_read(BOT_EPT_BULK_OUT,
                                   pLun->pReadWriteBuffer,
                                   pLun->dBlockSize,
                                   (Callback_f) bot_callback,
                                   (void *) pTransfer);
    
                // Check operation result code
                if (bStatus != USB_STATUS_SUCCESS)
                {
                    TRACE_SBC("RBC_Write10: Failed to start receiving data\n");
                    sbc_update_sense_data(&(pLun->sRequestSenseData),
                                        SBC_SENSE_KEY_HARDWARE_ERROR,
                                        0,
                                        0);
                    bResult = BOT_STATUS_ERROR;
                }
                else
                {
                    // Prepare next device state
                    pCommandState->bState = SBC_STATE_WAIT_READ;
                }
                break;
    
            case SBC_STATE_WAIT_READ:
                TRACE_SBC("Wait ");
    
                // Check semaphore
                if (pTransfer->bSemaphore > 0)
                {
                    pTransfer->bSemaphore--;
                    pCommandState->bState = SBC_STATE_WRITE;
                }
                break;
    
            case SBC_STATE_WRITE:
                // Check the result code of the read operation
                if (pTransfer->bStatus != USB_STATUS_SUCCESS)
                {
                    TRACE_SBC("RBC_Write10: Failed to received data\n");
                    sbc_update_sense_data(&(pLun->sRequestSenseData),
                                        SBC_SENSE_KEY_HARDWARE_ERROR,
                                        0,
                                        0);
                    bResult = BOT_STATUS_ERROR;
                }
                else
                {
                    // Write the block to the media
                    bStatus = lun_write(DWORDB(pCommand->pLogicalBlockAddress),
                                        pLun->pReadWriteBuffer,
                                        1,
                                        (Callback_f) bot_callback,
                                        (void *) pTransfer);
    
                    // Check operation result code
                    if (bStatus != USB_STATUS_SUCCESS)
                    {
                        TRACE_SBC("RBC_Write10: Failed to start media write\n");
                        sbc_update_sense_data(&(pLun->sRequestSenseData),
                                            SBC_SENSE_KEY_NOT_READY,
                                            0,
                                            0);
                        bResult = BOT_STATUS_ERROR;
                    }
                    else
                    {
                        // Prepare next state
                        pCommandState->bState = SBC_STATE_WAIT_WRITE;
                    }
                }
                break;
    
            case SBC_STATE_WAIT_WRITE:
                TRACE_SBC("Wait ");
    
                // Check semaphore value
                if (pTransfer->bSemaphore > 0)
                {
                    // Take semaphore and move to next state
                    pTransfer->bSemaphore--;
                    pCommandState->bState = SBC_STATE_NEXT_BLOCK;
                }
                break;
    
            case SBC_STATE_NEXT_BLOCK:
                // Check operation result code
                if (pTransfer->bStatus != USB_STATUS_SUCCESS)
                {
                    TRACE_SBC("RBC_Write10: Failed to write media\n");
                    sbc_update_sense_data(&(pLun->sRequestSenseData),
                                        SBC_SENSE_KEY_RECOVERED_ERROR,
                                        SBC_ASC_TOO_MUCH_WRITE_DATA,
                                        0);
                    bResult = BOT_STATUS_ERROR;
                }
                else
                {
                    // Update transfer length and block address
                    pCommandState->dLength--;
                    STORE_DWORDB((DWORDB(pCommand->pLogicalBlockAddress) + 1),
                                 pCommand->pLogicalBlockAddress);
    
                    // Check if transfer is finished
                    if (pCommandState->dLength == 0)
                    {
                        bResult = BOT_STATUS_SUCCESS;
                    }
                    else
                    {
                        pCommandState->bState = SBC_STATE_READ;
                    }
                }
                break;
        }
    }

    // Convert dLength from blocks to bytes
    pCommandState->dLength *= pLun->dBlockSize;

    return bResult;
}



/**
 * Performs a READ (10) command on specified LUN.
 * 
 * The data is first read from the media and then sent to the USB host.
 * This function operates asynchronously and must be called multiple
 * times to complete. A result code of BOT_STATUS_INCOMPLETE indicates
 * that at least another call of the method is necessary.
 * 
 * \param   pCommandState   Current state of the command
 * \return  Operation result code (SUCCESS, ERROR, INCOMPLETE or PARAMETER)
 * 
 */
static unsigned char sbc_read10(S_bot_command_state *pCommandState)
{
    unsigned char bStatus;
    unsigned char bResult = BOT_STATUS_INCOMPLETE;
    S_sbc_read_10 *pCommand = (S_sbc_read_10 *) pCommandState->sCbw.pCommand;
    S_bot_transfer *pTransfer = &(pCommandState->sTransfer);

    // Init command state
    if (pCommandState->bState == 0)
    {
        pCommandState->bState = SBC_STATE_READ;
    }

    // Convert dLength from bytes to blocks
    pCommandState->dLength /= pLun->dBlockSize;

    // Check length
    if (pCommandState->dLength == 0)
    {
        bResult = BOT_STATUS_SUCCESS;
    }
    else
    {
        // Command state management
        switch (pCommandState->bState)
        {
            case SBC_STATE_READ:
                // Read one block of data from the media
                bStatus = lun_read(DWORDB(pCommand->pLogicalBlockAddress),
                                   pLun->pReadWriteBuffer,
                                   1,
                                   (Callback_f) bot_callback,
                                   (void *) pTransfer);
    
                // Check operation result code
                if (bStatus != LUN_STATUS_SUCCESS)
                {
                    TRACE_SBC("RBC_Read10: Failed to start reading media\n");
                    sbc_update_sense_data(&(pLun->sRequestSenseData),
                                          SBC_SENSE_KEY_NOT_READY,
                                          SBC_ASC_LOGICAL_UNIT_NOT_READY,
                                          0);
                    bResult = BOT_STATUS_ERROR;
                }
                else
                {
                    // Move to next command state
                    pCommandState->bState = SBC_STATE_WAIT_READ;
                }
                break;

            case SBC_STATE_WAIT_READ:
                // Check semaphore value
                if (pTransfer->bSemaphore > 0)
                {
                    TRACE_SBC("Ok ");
                    // Take semaphore and move to next state
                    pTransfer->bSemaphore--;
                    pCommandState->bState = SBC_STATE_WRITE;
                }
                break;
    
            case SBC_STATE_WRITE:
                // Check the operation result code
                if (pTransfer->bStatus != USB_STATUS_SUCCESS)
                {
                    TRACE_SBC("RBC_Read10: Failed to read media\n");
                    sbc_update_sense_data(&(pLun->sRequestSenseData),
                                          SBC_SENSE_KEY_RECOVERED_ERROR,
                                          SBC_ASC_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE,
                                          0);
                    bResult = BOT_STATUS_ERROR;
                }
                else
                {
                    // Send the block to the host
                    bStatus = usb_write(BOT_EPT_BULK_IN,
                                        pLun->pReadWriteBuffer,
                                        pLun->dBlockSize,
                                        (Callback_f) bot_callback,
                                        (void *) pTransfer);
    
                    // Check operation result code
                    if (bStatus != USB_STATUS_SUCCESS)
                    {
                        TRACE_SBC("RBC_Read10: Failed to start to send data\n");
                        sbc_update_sense_data(&(pLun->sRequestSenseData),
                                              SBC_SENSE_KEY_HARDWARE_ERROR,
                                              0,
                                              0);
                        bResult = BOT_STATUS_ERROR;
                    }
                    else
                    {
                        TRACE_SBC("Sending ");
                        // Move to next command state
                        pCommandState->bState = SBC_STATE_WAIT_WRITE;
                    }
                }
                break;
    
            case SBC_STATE_WAIT_WRITE:
                // Check semaphore value
                if (pTransfer->bSemaphore > 0)
                {
                    TRACE_SBC("Sent ");
 
                    // Take semaphore and move to next state
                    pTransfer->bSemaphore--;
                    pCommandState->bState = SBC_STATE_NEXT_BLOCK;
                }
                break;
    
            case SBC_STATE_NEXT_BLOCK:
                // Check operation result code
                if (pTransfer->bStatus != USB_STATUS_SUCCESS)
                {
                    TRACE_SBC("RBC_Read10: Failed to send data\n");
                    sbc_update_sense_data(&(pLun->sRequestSenseData),
                                          SBC_SENSE_KEY_HARDWARE_ERROR,
                                          0,
                                          0);
                    bResult = BOT_STATUS_ERROR;
                }
                else
                {
                    TRACE_SBC("Next ");
                    // Update transfer length and block address
                    STORE_DWORDB((DWORDB(pCommand->pLogicalBlockAddress) + 1),
                                 pCommand->pLogicalBlockAddress);
                    pCommandState->dLength--;
    
                    // Check if transfer is finished
                    if (pCommandState->dLength == 0)
                    {
                        bResult = BOT_STATUS_SUCCESS;
                    }
                    else
                    {
                        pCommandState->bState = SBC_STATE_READ;
                    }
                }
                break;
        }
    }

    // Convert dLength from blocks to bytes
    pCommandState->dLength *= pLun->dBlockSize;

    return bResult;
}


/**
 * Performs a MODE SENSE (6) command.
 * 
 * This function operates asynchronously and must be called multiple
 * times to complete. A result code of BOT_STATUS_INCOMPLETE indicates
 * that at least another call of the method is necessary.
 * 
 * \param   pCommandState   Current state of the command
 * \return Operation result code (SUCCESS, ERROR, INCOMPLETE or PARAMETER)
 * 
 */
static unsigned char sbc_mode_sense6(S_bot_command_state *pCommandState)
{
    unsigned char      bResult = BOT_STATUS_INCOMPLETE;
    unsigned char      bStatus;
    S_bot_transfer     *pTransfer = &(pCommandState->sTransfer);

    // Initialize command state if needed
    if (pCommandState->bState == 0)
    {
        pCommandState->bState = SBC_STATE_WRITE;
    }

    // Check current command state
    switch (pCommandState->bState)
    {
        case SBC_STATE_WRITE:
            // Start transfer
            bStatus = usb_write(BOT_EPT_BULK_IN,
                                (void *) &sModeParameterHeader6,
                                pCommandState->dLength,
                                (Callback_f) bot_callback,
                                (void *) pTransfer);
    
            // Check operation result code
            if (bStatus != USB_STATUS_SUCCESS)
            {
                TRACE_SBC("SPC_ModeSense6: Cannot start data transfer\n");
                bResult = BOT_STATUS_ERROR;
            }
            else
            {
                // Proceed to next state
                pCommandState->bState = SBC_STATE_WAIT_WRITE;
            }
            break;
    
        case SBC_STATE_WAIT_WRITE:
            TRACE_SBC("Wait ");
    
            // Check semaphore value
            if (pTransfer->bSemaphore > 0)
            {
                // Take semaphore and terminate command
                pTransfer->bSemaphore--;
    
                if (pTransfer->bStatus != USB_STATUS_SUCCESS)
                {
                    TRACE_SBC("SPC_ModeSense6: Data transfer failed\n");
                    bResult = BOT_STATUS_ERROR;
                }
                else
                {
                    bResult = BOT_STATUS_SUCCESS;
                }
                // Update dLength field
                pCommandState->dLength -= pTransfer->dBytesTransferred;
    
            }
            break;
    }

    return bResult;
}


/**
 * Performs a REQUEST SENSE command.
 * 
 * This function operates asynchronously and must be called multiple
 * times to complete. A result code of BOT_STATUS_INCOMPLETE indicates
 * that at least another call of the method is necessary.
 * 
 * \param   pCommandState   Current state of the command
 * \return Operation result code (SUCCESS, ERROR, INCOMPLETE or PARAMETER)
 * 
 */
static unsigned char sbc_request_sense(S_bot_command_state *pCommandState)
{
    unsigned char bResult = BOT_STATUS_INCOMPLETE;
    unsigned char bStatus;
    S_bot_transfer *pTransfer = &(pCommandState->sTransfer);

    // Check if requested length is zero
    if (pCommandState->dLength == 0)
    {
        // Nothing to do
        bResult = BOT_STATUS_SUCCESS;
    }
    // Initialize command state if needed
    else
    if (pCommandState->bState == 0)
    {
        pCommandState->bState = SBC_STATE_WRITE;
    }

    // Identify current command state
    switch (pCommandState->bState)
    {
        case SBC_STATE_WRITE:
            // Start transfer
            bStatus = usb_write(BOT_EPT_BULK_IN,
                                &(pLun->sRequestSenseData),
                                pCommandState->dLength,
                                (Callback_f) bot_callback,
                                (void *) pTransfer);
            // Check result code
            if (bStatus != USB_STATUS_SUCCESS)
            {
                TRACE_SBC("RBC_RequestSense: Cannot start sending data\n");
                bResult = BOT_STATUS_ERROR;
            }
            else
            {
                // Change state
                pCommandState->bState = SBC_STATE_WAIT_WRITE;
            }
            break;
    
         case SBC_STATE_WAIT_WRITE:
             // Check the transfer semaphore
            if (pTransfer->bSemaphore > 0)
            {
                // Take semaphore and finish command
                pTransfer->bSemaphore--;
    
                if (pTransfer->bStatus != USB_STATUS_SUCCESS)
                {
                    bResult = BOT_STATUS_ERROR;
                }
                else
                {
                    bResult = BOT_STATUS_SUCCESS;
                }
    
                // Update pLength
                pCommandState->dLength -= pTransfer->dBytesTransferred;
            }
            break;
    }

    return bResult;
}


/**
 * Performs a TEST UNIT READY COMMAND command.
 * 
 * \return Operation result code (SUCCESS, ERROR, INCOMPLETE or PARAMETER)
 * 
 */
static unsigned char sbc_test_unit_ready(void)
{
    unsigned char bResult = BOT_STATUS_ERROR;
    
    // Send error if card is not present
    if(sd_card_detect() != SD_OK) pLun->bMediaStatus = SD_NOCARD;

    // Check current media state
    switch(pLun->bMediaStatus)
    {
        case SD_STATUS_READY:
            // Nothing to do
            TRACE_SBC("Rdy ");
            bResult = BOT_STATUS_SUCCESS;
            break;
    
        case SD_STATUS_BUSY:
            TRACE_SBC("Bsy ");
            sbc_update_sense_data(&(pLun->sRequestSenseData),
                                  SBC_SENSE_KEY_NOT_READY,
                                  0,
                                  0);
            break;
    
        //------
        default:
        //------
            TRACE_SBC("? ");
            sbc_update_sense_data(&(pLun->sRequestSenseData),
                                SBC_SENSE_KEY_NOT_READY,
                                SBC_ASC_MEDIUM_NOT_PRESENT,
                                0);
            break;
    }

    return bResult;
}


/**
 * Updates the sense data of a LUN with the given key and codes
 * 
 * \param   pRequestSenseData             Pointer to the sense data to update
 * \param   bSenseKey                     Sense key
 * \param   bAdditionalSenseCode          Additional sense code
 * \param   bAdditionalSenseCodeQualifier Additional sense code qualifier
 * 
 */
void sbc_update_sense_data(S_sbc_request_sense_data *pRequestSenseData,
                         unsigned char bSenseKey,
                         unsigned char bAdditionalSenseCode,
                         unsigned char bAdditionalSenseCodeQualifier)
{
    pRequestSenseData->bSenseKey = bSenseKey;
    pRequestSenseData->bAdditionalSenseCode = bAdditionalSenseCode;
    pRequestSenseData->bAdditionalSenseCodeQualifier
        = bAdditionalSenseCodeQualifier;
}


/**
 * Return information about the transfer length and direction expected
 * by the device for a particular command.
 * 
 * \param   pCommand    Pointer to a buffer holding the command to evaluate
 * \param   pLength     Expected length of the data transfer
 * \param   pType       Expected direction of data transfer
 * \return  Command support status
 */
char sbc_get_command_information(void          *pCommand,
                                 unsigned int  *pLength,
                                 unsigned char *pType)
{
    S_sbc_command *pSbcCommand = (S_sbc_command *) pCommand;
    char          isCommandSupported = true;

    // Identify command
    switch (pSbcCommand->bOperationCode)
    {
        case SBC_INQUIRY:
            (*pType) = BOT_DEVICE_TO_HOST;
    
            // Allocation length is stored in big-endian format
            (*pLength) = WORDB(pSbcCommand->sInquiry.pAllocationLength);
            break;
    
        case SBC_MODE_SENSE_6:
            (*pType) = BOT_DEVICE_TO_HOST;
            (*pLength) = MIN(sizeof(S_sbc_mode_parameter_header_6),
                             pSbcCommand->sModeSense6.bAllocationLength);
    
            // Only "return all pages" command is supported
            if (pSbcCommand->sModeSense6.bPageCode != SBC_PAGE_RETURN_ALL)
            {
                // Unsupported page
                TRACE_SBC("SBC_GetCommandInformation: Page code not supported (0x%02X)\n",
                              pSbcCommand->sModeSense6.bPageCode);
                isCommandSupported = false;
                (*pLength) = 0;
            }
            break;
    
        case SBC_PREVENT_ALLOW_MEDIUM_REMOVAL:
            (*pType) = BOT_NO_TRANSFER;
            break;
    
        case SBC_REQUEST_SENSE:
            (*pType) = BOT_DEVICE_TO_HOST;
            (*pLength) = pSbcCommand->sRequestSense.bAllocationLength;
            break;
    
        case SBC_TEST_UNIT_READY:
            (*pType) = BOT_NO_TRANSFER;
            break;
    
        case SBC_READ_CAPACITY_10:
            (*pType) = BOT_DEVICE_TO_HOST;
            (*pLength) = sizeof(S_sbc_read_capacity_10_data);
            break;
    
        case SBC_READ_10:
            (*pType) = BOT_DEVICE_TO_HOST;
            (*pLength) = WORDB(pSbcCommand->sRead10.pTransferLength)
                         * pLun->dBlockSize;
            break;
    
        case SBC_WRITE_10:
            (*pType) = BOT_HOST_TO_DEVICE;
            (*pLength) = WORDB(pSbcCommand->sWrite10.pTransferLength)
                         * pLun->dBlockSize;
            break;
    
        case SBC_VERIFY_10:
            (*pType) = BOT_NO_TRANSFER;
            break;
    
        default:
            isCommandSupported = false;
    }

    // If length is 0, no transfer is expected
    if ((*pLength) == 0)
    {
        (*pType) = BOT_NO_TRANSFER;
    }

    return isCommandSupported;
}


/**
 * Processes a SBC command by dispatching it to a subfunction.
 * 
 * \param   pCommandState   Pointer to the current command state
 * \return  Operation result code
 * 
 */
unsigned char sbc_process_command(S_bot_command_state *pCommandState)
{
    unsigned char bResult = BOT_STATUS_INCOMPLETE;
    S_sbc_command *pCommand = (S_sbc_command *) pCommandState->sCbw.pCommand;

    // Identify command
    switch (pCommand->bOperationCode)
    {
        case SBC_READ_10:
            TRACE_SBC("Read(10) ");
    
            // Perform the Read10 command
            bResult = sbc_read10(pCommandState);
            break;

        case SBC_WRITE_10:
            TRACE_SBC("Write(10) ");
    
            // Perform the Write10 command
            bResult = sbc_write10(pCommandState);
            break;

        case SBC_READ_CAPACITY_10:
            TRACE_SBC("RdCapacity(10) ");
            // Perform the ReadCapacity command
            bResult = sbc_read_capacity10(pCommandState);
            break;

        case SBC_VERIFY_10:
            TRACE_SBC("Verify(10) ");
            // Nothing to do
            bResult = BOT_STATUS_SUCCESS;
            break;

        case SBC_INQUIRY:
            TRACE_SBC("Inquiry   ");
            // Process Inquiry command
            bResult = sbc_inquiry(pCommandState);
            break;

        case SBC_MODE_SENSE_6:
            TRACE_SBC("ModeSense(6) ");
            // Process ModeSense6 command
            bResult = sbc_mode_sense6(pCommandState);
            break;

        case SBC_TEST_UNIT_READY:
            TRACE_SBC("TstUnitRdy ");
            // Process TestUnitReady command
            bResult = sbc_test_unit_ready();
            break;

        case SBC_REQUEST_SENSE:
            TRACE_SBC("ReqSense ");
            // Perform the RequestSense command
            bResult = sbc_request_sense(pCommandState);
            break;

        case SBC_PREVENT_ALLOW_MEDIUM_REMOVAL:
            TRACE_SBC("PrevAllowRem ");
            // Nothing to do
            bResult = BOT_STATUS_SUCCESS;
            break;

        default:
            bResult = BOT_STATUS_PARAMETER;
    }

    return bResult;
}
