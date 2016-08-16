/**
 * \file usb_sbc_func.h
 * Header: SCSI block commands
 * 
 * SCSI block command functions
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
#ifndef USB_SBC_FUNC_H_
#define USB_SBC_FUNC_H_

/**
 * \name Possible states of a SBC command.
 * 
 */
//@{
#define SBC_STATE_READ                          0x01
#define SBC_STATE_WAIT_READ                     0x02
#define SBC_STATE_WRITE                         0x03
#define SBC_STATE_WAIT_WRITE                    0x04
#define SBC_STATE_NEXT_BLOCK                    0x05
//@}

char sbc_get_command_information(void          *pCommand,
                                 unsigned int  *pLength,
                                 unsigned char *pType);
unsigned char sbc_process_command(S_bot_command_state *pCommandState);
void sbc_update_sense_data(S_sbc_request_sense_data *pRequestSenseData,
                         unsigned char bSenseKey,
                         unsigned char bAdditionalSenseCode,
                         unsigned char bAdditionalSenseCodeQualifier);
                         
#endif /*USB_SBC_FUNC_H_*/
