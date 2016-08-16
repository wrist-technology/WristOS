/**
 * \file usb_bot.h
 * Header: Bulk only tranfer 
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
#ifndef USB_BOT_H_
#define USB_BOT_H_

//! \brief  Address of Bulk OUT endpoint
#define BOT_EPT_BULK_OUT    1

//! \brief  Address of Bulk IN endpoint
#define BOT_EPT_BULK_IN     2

#define BOT_IN_EP_SIZE      64  //!< Bulk IN enpoint size
#define BOT_OUT_EP_SIZE     64  //!< Bulk OUT endpoint size

//! \brief  Possible states of the MSD driver
//! \brief  Driver is expecting a command block wrapper
#define BOT_STATE_READ_CBW              (1 << 0)

//! \brief  Driver is waiting for the transfer to finish
#define BOT_STATE_WAIT_CBW              (1 << 1)

//! \brief  Driver is processing the received command
#define BOT_STATE_PROCESS_CBW           (1 << 2)

//! \brief  Driver is starting the transmission of a command status wrapper
#define BOT_STATE_SEND_CSW              (1 << 3)

//! \brief  Driver is waiting for the CSW transmission to finish
#define BOT_STATE_WAIT_CSW              (1 << 4)

//! \brief  Result codes for MSD functions
//! \brief  Method was successful
#define BOT_STATUS_SUCCESS              0x00

//! \brief  There was an error when trying to perform a method
#define BOT_STATUS_ERROR                0x01

//! \brief  No error was encountered but the application should call the
//!         method again to continue the operation
#define BOT_STATUS_INCOMPLETE           0x02

//! \brief  A wrong parameter has been passed to the method
#define BOT_STATUS_PARAMETER            0x03

//! \brief  Actions to perform during the post-processing phase of a command
//! \brief  Indicates that the CSW should report a phase error
#define BOT_CASE_PHASE_ERROR            (1 << 0)

//! \brief  The driver should halt the Bulk IN pipe after the transfer
#define BOT_CASE_STALL_IN               (1 << 1)

//! \brief  The driver should halt the Bulk OUT pipe after the transfer
#define BOT_CASE_STALL_OUT              (1 << 2)

//! \name Possible direction values for a data transfer
//@{
#define BOT_DEVICE_TO_HOST              0
#define BOT_HOST_TO_DEVICE              1
#define BOT_NO_TRANSFER                 2
//@}

//! \brief  Structure for holding the result of a USB transfer
//! \see    MSD_Callback
typedef struct {

    unsigned int  dBytesTransferred; //!< Number of bytes transferred
    unsigned int  dBytesRemaining;   //!< Number of bytes not transferred
    unsigned char bSemaphore;        //!< Semaphore to indicate transfer completion
    unsigned char bStatus;           //!< Operation result code

} S_bot_transfer;

//! \brief  Status of an executing command
//! \see    S_msd_cbw
//! \see    S_msd_csw
//! \see    S_bot_transfer
typedef struct {

    S_bot_transfer sTransfer; //!< Current transfer status
    S_msd_cbw      sCbw;      //!< Received CBW
    S_msd_csw      sCsw;      //!< CSW to send
    unsigned char  bState;    //!< Current command state
    unsigned char  bCase;     //!< Actions to perform when command is complete
    unsigned int   dLength;   //!< Remaining length of command

} S_bot_command_state;

//! \brief  MSD driver state variables
//! \see    S_bot_command_state
//! \see    S_std_class
//! \see    S_lun
typedef struct {

    S_lun               *pLun;               //!< Pointer to a list of LUNs
    S_bot_command_state sCommandState;       //!< State of the currently executing command
    unsigned char       bMaxLun;             //!< Maximum LUN index
    unsigned char       bState;              //!< Current state of the driver
    unsigned char       isWaitResetRecovery; //!< Indicates if the driver is
                                             //!< waiting for a reset recovery
} S_bot;

void bot_request_handler(void);
void bot_state_machine(void);
void bot_init(unsigned char bNumLun);
void bot_callback(S_bot_transfer *pTransfer,
                                unsigned char  bStatus,
                                unsigned int   dBytesTransferred,
                                unsigned int   dBytesRemaining);

#endif /*USB_BOT_H_*/
