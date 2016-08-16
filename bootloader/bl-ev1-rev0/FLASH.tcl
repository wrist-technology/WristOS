#  ----------------------------------------------------------------------------
#          ATMEL Microcontroller Software Support  -  ROUSSET  -
#  ----------------------------------------------------------------------------
#  Copyright (c) 2006, Atmel Corporation
#
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#
#  - Redistributions of source code must retain the above copyright notice,
#  this list of conditions and the disclaiimer below.
#
#  - Redistributions in binary form must reproduce the above copyright notice,
#  this list of conditions and the disclaimer below in the documentation and/or
#  other materials provided with the distribution. 
#
#  Atmel's name may not be used to endorse or promote products derived from
#  this software without specific prior written permission. 
#
#  DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
#  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
#  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
#  DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
#  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
#  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
#  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#  ----------------------------------------------------------------------------


################################################################################
################################################################################
## NAMESPACE FLASH
################################################################################
################################################################################
namespace eval FLASH {
    
    # FLASH Features fl020
    variable AT91C_NB_OF_PAGES            1024
    variable AT91C_NB_OF_SECTORS          16
    variable AT91C_NB_OF_PAGES_BY_SECTOR  64
    variable AT91C_PAGE_SIZE              256
    variable AT91C_SECTOR_SIZE            [expr $AT91C_NB_OF_PAGES_BY_SECTOR * $AT91C_PAGE_SIZE]
    variable AT91C_FLASH_SIZE             [expr $AT91C_NB_OF_PAGES * $AT91C_PAGE_SIZE]
    variable AT91C_ADDR_MASK              [expr $AT91C_FLASH_SIZE - 1]
    variable AT91C_PAGE_SHIFT             8
    variable AT91C_SECTOR_SHIFT           16
    variable AT91C_SECTOR_MASK            0xFFFF0000
    variable AT91C_MC_CORRECT_KEY         [expr 0x5A << 24]
    variable AT91C_FLASH_TIMEOUT          1000000
    variable baseAddressFLASH             [expr 0x100000]
    variable StartAddress                 [expr 0x100000]
    variable MemorySize                   [expr 256*1024]

    # Page Buffer & Page Number Address used by the C binary project
    variable comType                      0x2013FC
    variable pageBuf                      0x201400
    variable pageNb                       [expr $pageBuf + $AT91C_PAGE_SIZE ]

    # Link address & Binary name in the C project
    variable monitorAddr                  0x201600
    variable monitorName                  "SAM-BA-Flash.bin"
    
    variable ForceUnlockBeforeWrite       0
    variable ForceLockAfterWrite          0
}

global target
set dummy_err 0
if {$target(comType) == 1} {
    # Retrieve MCK through DBGU baudrate programmation
    global AT91C_DBGU_BRGR
    set fmcn [expr [TCL_Read_Int $target(handle) $AT91C_DBGU_BRGR dummy_err] & 0xFFFF]
    
    set fmcn [expr 16 * 115200 * $fmcn]
    set fmcn [expr $fmcn / 1000000]
    
    #set GPNVM_FMR [expr $fmcn /10]
    #set GPNVM_FMR [expr 0x00000100 | [expr $GPNVM_FMR << 16]]
    #set PROG_FMR [expr $fmcn *3/2]
    #set PROG_FMR [expr 0x00000100 | [expr $PROG_FMR << 16]]
    
    # Set GPNVM_FMR configuration for GPNVM bits operations
    set GPNVM_FMR 0x00480100
    # Set PROG_FMR configuration for Programming operations
    set PROG_FMR 0x00480100
    
} else  {
    global AT91C_PMC_MCKR
    global AT91C_PMC_SR
    # Switch on MCK = PLLB/2 =48MHz
    TCL_Write_Int $target(handle) 5 $AT91C_PMC_MCKR dummy_err
    while {[expr [TCL_Read_Int $target(handle) $AT91C_PMC_SR dummy_err] & 8] != 8} { }
    TCL_Write_Int $target(handle) 7 $AT91C_PMC_MCKR dummy_err
    while {[expr [TCL_Read_Int $target(handle) $AT91C_PMC_SR dummy_err] & 8] != 8} { }
    
    # Set GPNVM_FMR configuration for GPNVM bits operations
    set GPNVM_FMR 0x00480100
    # Set PROG_FMR configuration for Programming operations
    set PROG_FMR 0x00480100
}


################################################################################
#  proc FLASH::OFFSET_PAGE
################################################################################
proc FLASH::OFFSET_PAGE { X } {
    variable AT91C_PAGE_SIZE
    
    return [expr $X * $AT91C_PAGE_SIZE]
}

################################################################################
#  proc FLASH::PerformCmd
################################################################################
proc FLASH::PerformCmd { transfer_cmd } {
    global AT91C_MC_FCR
    global target
    set dummy_err 0

    TCL_Write_Int $target(handle) $transfer_cmd $AT91C_MC_FCR dummy_err
}

################################################################################
#  proc FLASH::CfgModeRegister
################################################################################
proc FLASH::CfgModeRegister { mode } {
    global AT91C_MC_FMR
    variable AT91C_FLASH_TIMEOUT

    global target
    set dummy_err 0

    # Write to the FMR register
    TCL_Write_Int $target(handle) $mode $AT91C_MC_FMR dummy_err
    # Wait for FLASH Ready
    return [FLASH::WaitReady $AT91C_FLASH_TIMEOUT]
}

################################################################################
#  proc FLASH::WaitReady
################################################################################
proc FLASH::WaitReady {timeout} {
    global AT91C_MC_FSR AT91C_MC_FRDY
    
    global target
    set dummy_err 0

    set Status 0
    while {$Status == 0} {
        set timeout [expr $timeout - 1]
        if {$timeout == 0} {
            puts stderr "-E- TimeOut error"
            return -1
        }
        set Status [expr [TCL_Read_Int $target(handle) $AT91C_MC_FSR dummy_err] & $AT91C_MC_FRDY]
    }
    return 1
}

################################################################################
#  proc FLASH::ProgramPage
################################################################################
proc FLASH::ProgramPage { page File } {
    global AT91C_MC_PAGEN AT91C_MC_FCMD_START_PROG
    global valueOfDataForSendFile
    variable AT91C_MC_CORRECT_KEY
    variable AT91C_PAGE_SIZE
    variable AT91C_PAGE_SHIFT
    variable baseAddressFLASH

    global target
    set dummy_err 0
    
    # Load the buffer
    for {set i 0} {$i < [expr $AT91C_PAGE_SIZE / 4]} {incr i} {
        set valueOfDataForSendFile [read $File 4]
        binary scan $valueOfDataForSendFile i1 data
        TCL_Write_Int $target(handle) $data [expr $baseAddressFLASH + [FLASH::OFFSET_PAGE $page] + [expr $i *4]] dummy_err
    }
    
    # Send Page Program Command
    set cmd [expr $AT91C_MC_PAGEN & [expr $page << $AT91C_PAGE_SHIFT]]
    set cmd [expr $AT91C_MC_CORRECT_KEY | $AT91C_MC_FCMD_START_PROG | $cmd ]
    FLASH::PerformCmd $cmd
}

################################################################################
#  proc FLASH::SetGP
################################################################################
proc FLASH::SetGP { gp } {    
    global AT91C_MC_FCMD_SET_GP_NVM AT91C_MC_PAGEN
    variable AT91C_MC_CORRECT_KEY
    variable    AT91C_PAGE_SHIFT
    
    # Send Page Program Command
    set cmd [expr $AT91C_MC_PAGEN & [expr $gp << $AT91C_PAGE_SHIFT]]
    FLASH::PerformCmd [expr $AT91C_MC_CORRECT_KEY | $AT91C_MC_FCMD_SET_GP_NVM | $cmd]
}

################################################################################
#  proc FLASH::ClearGP
################################################################################
proc FLASH::ClearGP { gp } {   
    global AT91C_MC_FCMD_CLR_GP_NVM AT91C_MC_PAGEN
    variable AT91C_MC_CORRECT_KEY
    variable    AT91C_PAGE_SHIFT
    
    # Send Page Program Command
    set cmd [expr $AT91C_MC_PAGEN & [expr $gp << $AT91C_PAGE_SHIFT]]
    FLASH::PerformCmd [expr $AT91C_MC_CORRECT_KEY | $AT91C_MC_FCMD_CLR_GP_NVM | $cmd]
}

################################################################################
#  proc FLASH::ScriptGPNMV
################################################################################
proc FLASH::ScriptGPNMV { index } {   
    switch $index {
        0 {
            if { [catch {FLASH::SetGP 0}] } {
                puts stderr "-E- Enable BrownOut failed"
                return -1
            } else  {puts "-I- BrownOut Enabled"}
        }
        1 {
            if { [catch {FLASH::ClearGP 0}] } {
                puts stderr "-E- Disable BrownOut failed"
                return -1
            } else  {puts "-I- BrownOut Disabled"}
        }
        2 {
            if { [catch {FLASH::SetGP 1}] } {
                puts stderr "-E- Enable BrownOut Reset failed"
                return -1
            } else  {puts "-I- BrownOut Reset Enabled"}
        }
        3 {
            if { [catch {FLASH::ClearGP 1}] } {
                puts stderr "-E- Disable BrownOut Reset failed"
                return -1
            } else  {puts "-I- BrownOut Reset Disabled"}
        }
        4 {
            if { [catch {FLASH::SetGP 2}] } {
                puts stderr "-E- Boot From Flash failed"
                return -1
            } else  {puts "-I- Boot from Flash"}
        }
        5 {
            if { [catch {FLASH::ClearGP 2}] } {
                puts stderr "-E- Boot From ROM failed"
                return -1
            } else  {puts "-I- Boot from ROM"}
        }
    }
}

################################################################################
#  proc FLASH::SetSecurityBit
################################################################################
proc FLASH::SetSecurityBit { } {
    global AT91C_MC_FCMD_SET_SECURITY
    variable AT91C_MC_CORRECT_KEY
    global GPNVM_FMR
    
    # Restore configuration for GPNVM bits operations
    FLASH::CfgModeRegister $GPNVM_FMR
    
    # Send Page Program Command
    FLASH::PerformCmd [expr $AT91C_MC_CORRECT_KEY | $AT91C_MC_FCMD_SET_SECURITY]
}

################################################################################
#  proc FLASH::ScriptSetSecurityBit
################################################################################
proc FLASH::ScriptSetSecurityBit { } {

    if { [catch {FLASH::SetSecurityBit}] } {
        puts stderr "-E- Set Security Bit failed"
        return -1
    }
    puts "-I- Security Bit Set"
}

################################################################################
#  proc FLASH::LockSector
################################################################################
proc FLASH::LockSector { PageInSector } {
    global AT91C_MC_FCMD_LOCK AT91C_MC_PAGEN AT91C_MC_FMCN AT91C_MC_FMR
    variable AT91C_MC_CORRECT_KEY
    variable AT91C_PAGE_SHIFT
    global GPNVM_FMR
    global target
    set dummy_err 0

    
    # Restore configuration for GPNVM bits operations
    FLASH::CfgModeRegister $GPNVM_FMR
    
    # Send Lock Command
    set cmd [expr $AT91C_MC_PAGEN & [expr $PageInSector << $AT91C_PAGE_SHIFT]]
    FLASH::PerformCmd [expr $AT91C_MC_CORRECT_KEY | $AT91C_MC_FCMD_LOCK | $cmd]
}

################################################################################
#  proc FLASH::AskForLockSector
################################################################################
proc FLASH::AskForLockSector { first_lockregion last_lockregion } {
    variable    AT91C_NB_OF_PAGES_BY_SECTOR
    variable    AT91C_FLASH_TIMEOUT
    variable    ForceLockAfterWrite
    global      commandLineMode
      
    if {$commandLineMode == 0} {
        # Ask for Lock Sector(s)
        set returnValue [messageDialg warning.gif "Do you want to lock involved lock region(s) ($first_lockregion to $last_lockregion) ?" "Lock region(s) to lock" yesno ]
        if {$returnValue} {
            return -1
        } else {
            set ForceLockAfterWrite 1
        }
    }

    if {$ForceLockAfterWrite} {
        # Lock all sectors involved in the write
        for {set i $first_lockregion} {$i <= $last_lockregion } {incr i} {
            FLASH::LockSector [expr $i * $AT91C_NB_OF_PAGES_BY_SECTOR]
            set returnValue [FLASH::WaitReady $AT91C_FLASH_TIMEOUT]
            puts "-I- Sector $i locked"
        }
    }

    return 1
}

################################################################################
#  proc FLASH::UnlockSector
################################################################################
proc FLASH::UnlockSector { PageInSector } {   
    global AT91C_MC_FCMD_UNLOCK AT91C_MC_PAGEN 
    variable AT91C_MC_CORRECT_KEY
    variable AT91C_PAGE_SHIFT
    global GPNVM_FMR
    global target
    set dummy_err 0

    # Restore configuration for GPNVM bits operations
    FLASH::CfgModeRegister $GPNVM_FMR
    
    # Send Unlock Command
    set cmd [expr $AT91C_MC_PAGEN & [expr $PageInSector << $AT91C_PAGE_SHIFT]]
    FLASH::PerformCmd [expr $AT91C_MC_CORRECT_KEY | $AT91C_MC_FCMD_UNLOCK | $cmd]
}

################################################################################
#  proc FLASH::IsSectorLocked
################################################################################
proc FLASH::IsSectorLocked { lockregion } {   
    global AT91C_MC_FSR
    variable AT91C_SECTOR_SHIFT
    variable AT91C_SECTOR_MASK
    
    global target
    set dummy_err 0

    set Status [expr [TCL_Read_Int $target(handle) $AT91C_MC_FSR dummy_err] & $AT91C_SECTOR_MASK]
    set Status [expr $Status & [expr 1 << [expr $lockregion + $AT91C_SECTOR_SHIFT] ] ]
    
    if {$Status == 0} {
            return 0
    }
        
    return 1
}

################################################################################
#  proc FLASH::AskForUnlockSector
################################################################################
proc FLASH::AskForUnlockSector { first_lockregion last_lockregion } {  
    variable    AT91C_NB_OF_PAGES_BY_SECTOR
    variable    AT91C_FLASH_TIMEOUT
    variable    ForceUnlockBeforeWrite
    global      commandLineMode
    
    # Test if at least one sector involved in the write is locked
    set OneSectorLocked 0
    for {set i $first_lockregion} {$i <= $last_lockregion } {incr i} {
        set returnValue [FLASH::IsSectorLocked $i]
        if {$returnValue} {
            puts "-I- Found sector $i locked"
            set OneSectorLocked 1
        }
    }
    
    # Ask for Unlock Sector(s)
    if {$OneSectorLocked} {
        if {$commandLineMode == 0} {
            set returnValue [messageDialg warning.gif "Do you want to unlock involved lock region(s) ($first_lockregion to $last_lockregion) ?" "At least one lock region is locked !" yesno ]
            if {$returnValue} {
                return -1
            } else {
                set ForceUnlockBeforeWrite 1
            }
        }
    } else {
        return 1
    }
    
    if {$ForceUnlockBeforeWrite} {
        # Unlock all sectors involved in the write
        for {set i $first_lockregion} {$i <= $last_lockregion } {incr i} {
            FLASH::UnlockSector [expr $i * $AT91C_NB_OF_PAGES_BY_SECTOR]
            set returnValue [FLASH::WaitReady $AT91C_FLASH_TIMEOUT]
            puts "-I- Sector $i unlocked"
        }
        return 1
    } else {
        return -1
    }
}

################################################################################
#  proc FLASH::WritePartialPage
################################################################################
proc FLASH::WritePartialPage { File dest sizeToWrite } {
    global AT91C_MC_PAGEN AT91C_MC_FCMD_START_PROG
    global valueOfDataForSendFile
    variable AT91C_MC_CORRECT_KEY
    variable AT91C_PAGE_SIZE
    variable AT91C_FLASH_TIMEOUT
    variable AT91C_PAGE_SHIFT
    variable baseAddressFLASH
  
    global target
    set dummy_err 0

    set page [expr $dest / $AT91C_PAGE_SIZE]
    set AdrInPage [expr $dest % $AT91C_PAGE_SIZE]
  
    #read corresponding page from target
    if { [catch {set result [TCL_Read_Data $target(handle) [expr $baseAddressFLASH + [FLASH::OFFSET_PAGE $page]] $AT91C_PAGE_SIZE dummy_err]}] } {
        puts stderr "-E- Can't read data, error in connection"
        return -1
    }

    # put data in a file
    if { [catch {set f [open "PartialPageWrite.tmp" w+]}] } {
        puts stderr "-E- Can't open file $name"
        return -1
    }
    fconfigure $f -translation binary
    puts -nonewline $f $result
    
    # Modify corresponding page
    seek $f $AdrInPage start
    set valueOfDataForSendFile [read $File $sizeToWrite]
    puts -nonewline $f $valueOfDataForSendFile
    seek $f 0 start
    
    # Write Modified File
    FLASH::ProgramPage $page $f
    set returnValue [FLASH::WaitReady $AT91C_FLASH_TIMEOUT]
    
    close $f
    file delete "PartialPageWrite.tmp"
}

################################################################################
#  proc FLASH::Write
################################################################################
proc FLASH::Write { dest sizeToWrite File } {
    variable AT91C_NB_OF_PAGES
    variable AT91C_PAGE_SIZE
    variable AT91C_FLASH_TIMEOUT
    variable AT91C_ADDR_MASK
    variable AT91C_FLASH_SIZE
    variable AT91C_SECTOR_SIZE
    global PROG_FMR

    if { [expr $dest < 0] || [expr $sizeToWrite < 0] } {
        error AT91C_ERR1
    }
    
    # mask on base flash address
    set dest [expr $dest & $AT91C_ADDR_MASK]
    
    if { [expr $dest + $sizeToWrite] > $AT91C_FLASH_SIZE } {
        error AT91C_ERR2
    }
    
    # Compute first and last lock regions
    set first_sector [expr $dest / $AT91C_SECTOR_SIZE]
    set last_sector [expr [expr $dest + $sizeToWrite] / $AT91C_SECTOR_SIZE]
    
    # Check for locked lock regions in order to unlock them, return if user refuses
    set returnValue [FLASH::AskForUnlockSector $first_sector $last_sector]
    if {$returnValue == -1} {
        puts stderr "-E- Send file failed: some lock regions are always locked !"
        return -1
    }
    
    # Restore configuration for programming operations
    FLASH::CfgModeRegister $PROG_FMR

    # 1st step: Address is not modulo AT91C_PAGE_SIZE
    set AdrInPage [expr $dest % $AT91C_PAGE_SIZE]
    if {$AdrInPage} {
        set length [expr $AT91C_PAGE_SIZE - $AdrInPage]
        if {$sizeToWrite < $length} {
            set length $sizeToWrite
        }
        FLASH::WritePartialPage $File $dest $length
        
        set sizeToWrite [expr $sizeToWrite - $length]
        set dest [expr $dest + $length]
    }
    
    # 2nd step: write whole page(s)
    while { [expr $sizeToWrite - $AT91C_PAGE_SIZE] >= 0 } {
        set page [expr $dest / $AT91C_PAGE_SIZE]
        FLASH::ProgramPage $page $File
        set returnValue [FLASH::WaitReady $AT91C_FLASH_TIMEOUT]
        
        set sizeToWrite [expr $sizeToWrite - $AT91C_PAGE_SIZE]
        set dest [expr $dest + $AT91C_PAGE_SIZE]
    }
    
    # 3rd step: write rest of file (size inferior to a page)
    if {$sizeToWrite > 0} {
        set returnValue [FLASH::WaitReady $AT91C_FLASH_TIMEOUT]
        FLASH::WritePartialPage $File $dest $sizeToWrite
    }

    # Lock all sectors involved in the write
    set returnValue [FLASH::AskForLockSector $first_sector $last_sector]
    
    return 1
}

################################################################################
#  proc FLASH::FastWrite
################################################################################
proc FLASH::FastWrite { dest sizeToWrite File } {
    variable AT91C_NB_OF_PAGES
    variable AT91C_PAGE_SIZE
    variable AT91C_FLASH_TIMEOUT
    variable AT91C_ADDR_MASK
    variable AT91C_FLASH_SIZE
    variable AT91C_SECTOR_SIZE
    variable monitorName
    variable monitorAddr
    variable pageNb
    variable pageBuf
    variable comType    
    global PROG_FMR
    
    global libPath
    global target
    set dummy_err 0

    if { [expr $dest < 0] || [expr $sizeToWrite < 0] } {
        error AT91C_ERR1
    }
    
    # mask on base flash address
    set dest [expr $dest & $AT91C_ADDR_MASK]
    
    if { [expr $dest + $sizeToWrite] > $AT91C_FLASH_SIZE } {
        error AT91C_ERR2
    }
    # Open monitor file
    set fileName "$libPath(extLib)/$target(board)/$monitorName"
    if { [catch {set f [open $fileName r]}] } {
        puts stderr "-E- Can't open file $fileName"
        return -1
    }
    
    # Copy $fileName content into SRAM reserved memory (at address $monitorAddr)
    fconfigure $f -translation binary
    set size [file size $fileName]
    set valueOfDataForSendFile [read $f $size]
    catch {TCL_Write_Data $target(handle) $monitorAddr valueOfDataForSendFile $size dummy_err}
    close $f
    
    # Compute first and last lock regions
    set first_sector [expr $dest / $AT91C_SECTOR_SIZE]
    set last_sector [expr [expr $dest + $sizeToWrite] / $AT91C_SECTOR_SIZE]

    # Check for locked lock regions in order to unlock them, return if user refuses
    set returnValue [FLASH::AskForUnlockSector $first_sector $last_sector]
    if {$returnValue == -1} {
        puts stderr "-E- Send file failed: some lock regions are always locked !"
        return -1
    }
    
    # Restore configuration for programming operations
    FLASH::CfgModeRegister $PROG_FMR

    # 1st step: Address is not modulo AT91C_PAGE_SIZE
    set AdrInPage [expr $dest % $AT91C_PAGE_SIZE]
    if {$AdrInPage} {
        set length [expr $AT91C_PAGE_SIZE - $AdrInPage]
        if {$sizeToWrite < $length} {
            set length $sizeToWrite
        }
        FLASH::WritePartialPage $File $dest $length
        
        set sizeToWrite [expr $sizeToWrite - $length]
        set dest [expr $dest + $length]
    }
    
    # 2nd step: write whole page(s)
    while { [expr $sizeToWrite - $AT91C_PAGE_SIZE] >= 0 } {
        set page [expr $dest / $AT91C_PAGE_SIZE]
        
        TCL_Write_Int $target(handle) $target(comType) $comType dummy_err    
        # Copy corresponding page into SRAM reserved memory (at address $pageNb)
        TCL_Write_Int $target(handle) $page $pageNb dummy_err
        set valueOfDataForSendFile [read $File $AT91C_PAGE_SIZE]
        # Copy corresponding page content into SRAM reserved memory (at address $pageBuf)
        catch {TCL_Write_Data $target(handle) $pageBuf valueOfDataForSendFile $AT91C_PAGE_SIZE dummy_err}
        # Branch on C binary
        catch {TCL_Go $target(handle) $monitorAddr dummy_err}

        set sizeToWrite [expr $sizeToWrite - $AT91C_PAGE_SIZE]
        set dest [expr $dest + $AT91C_PAGE_SIZE]
        puts "Write page $page at offset $dest"
    }

    # 3rd step: write rest of file (size inferior to a page)
    if {$sizeToWrite > 0} {
        set returnValue [FLASH::WaitReady $AT91C_FLASH_TIMEOUT]
        FLASH::WritePartialPage $File $dest $sizeToWrite
    }
    
    # Lock all sectors involved in the write
    set returnValue [FLASH::AskForLockSector $first_sector $last_sector] 
 
    return 1
}

################################################################################
#  proc FLASH::sendFile
################################################################################
proc FLASH::sendFile { name addr } {

    if { [catch {set f [open $name r]}] } {
        puts stderr "-E- Can't open file $name"
        return -1
    }
    
    fconfigure $f -translation binary
    
    set size [file size $name]
    puts "-I- File size = $size byte(s)"

    # Check for type of communication to branch on special high speed send file function for USB
    if { [catch {FLASH::FastWrite $addr $size $f} errInfo] } {
    switch $errInfo {
        "AT91C_ERR1" {puts stderr "-E- Negative Address or Size"}
        "AT91C_ERR2" {puts stderr "-E- Memory Overflow"}
        default {puts stderr "-E- Can't send data, error in connection"}
    }
    close $f
    return -1
    }       
    
    close $f
}

################################################################################
#  proc FLASH::receiveFile
################################################################################
proc FLASH::receiveFile {name addr size} {
    global target
    set dummy_err 0

    #read data from target
    if { [catch {set result [TCL_Read_Data $target(handle) $addr $size dummy_err]}] } {
        puts stderr "-E- Can't read data, error in connection"
        return -1
    }
    # put data in a file
    if { [catch {set f2 [open $name w+]}] } {
        puts stderr "-E- Can't open file $name"
        return -1
    }
    fconfigure $f2 -translation binary
    puts -nonewline $f2 $result
    close $f2
}

################################################################################
#  proc FLASH::ScriptEraseAllFlash
################################################################################
proc FLASH::ScriptEraseAllFlash { } {
    variable AT91C_NB_OF_SECTORS
    global PROG_FMR
    global commandLineMode
    
    # Check for locked lock regions in order to unlock them, return if user refuses
    catch {set returnValue [FLASH::AskForUnlockSector 0 [expr $AT91C_NB_OF_SECTORS - 1]]}
    if {$returnValue == -1} {
        puts stderr "-E- Erase All Flash failed: some lock regions are always locked !"
        return -1
    }

    # Wait window for loading
    if {$commandLineMode == 0} {
        waitWindows::createWindow "Erase Flash" "erase.gif"
        tkwait visibility .topWaitWindow
    }

    # Restore configuration for programming operations
    FLASH::CfgModeRegister $PROG_FMR

    if { [catch {FLASH::EraseAllFlash}] } {
        puts stderr "-E- Erase All Flash failed"
        if {$commandLineMode == 0} {
            catch { waitWindows::destroyWindow }
        }
        return -1
    }
    
    puts "-I- All Flash erased"

    if {$commandLineMode == 0} {
        catch { waitWindows::destroyWindow }
    }
}

################################################################################
#  proc FLASH::EraseAllFlash
################################################################################
proc FLASH::EraseAllFlash { } {
    global AT91C_MC_FCMD_ERASE_ALL
    variable AT91C_MC_CORRECT_KEY
    variable AT91C_FLASH_TIMEOUT
    
    # Send Page Program Command
    FLASH::PerformCmd [expr $AT91C_MC_CORRECT_KEY | $AT91C_MC_FCMD_ERASE_ALL]
    set returnValue [FLASH::WaitReady $AT91C_FLASH_TIMEOUT]
}


