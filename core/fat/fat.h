/**
 * \file fat.h
 * Header: FAT functions
 * 
 * FAT file system handling functions
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
#ifndef FAT_H_
#define FAT_H_


/**
 * \name Some useful cluster numbers.
 * 
*/
//@{
#define MSDOSFSROOT     0               //!< Cluster 0 means the root dir 
#define CLUST_FREE      0               //!< Cluster 0 also means a free cluster 
#define MSDOSFSFREE     CLUST_FREE      //!< Cluster 0 also means a free cluster
#define CLUST_FIRST     2               //!< First legal cluster number 
#define CLUST_RSRVD     0xfffffff6      //!< Reserved cluster range 
#define CLUST_BAD       0xfffffff7      //!< A cluster with a defect 
#define CLUST_EOFS      0xfffffff8      //!< Start of eof cluster range 
#define CLUST_EOFE      0xffffffff      //!< End of eof cluster range 
//@}


#define FAT12_MASK      0x00000fff      //!< Mask for 12 bit cluster numbers 
#define FAT16_MASK      0x0000ffff      //!< Mask for 16 bit cluster numbers 
#define FAT32_MASK      0x0fffffff      //!< Mask for FAT32 cluster numbers 


/**
 * \name Partition Types
 *  
 */
//@{ 
#define PART_TYPE_UNKNOWN   0x00
#define PART_TYPE_FAT12     0x01
#define PART_TYPE_XENIX     0x02
#define PART_TYPE_DOSFAT16  0x04
#define PART_TYPE_EXTDOS    0x05
#define PART_TYPE_FAT16     0x06
#define PART_TYPE_NTFS      0x07
#define PART_TYPE_FAT32     0x0B
#define PART_TYPE_FAT32LBA  0x0C
#define PART_TYPE_FAT16LBA  0x0E
#define PART_TYPE_EXTDOSLBA 0x0F
#define PART_TYPE_ONTRACK   0x33
#define PART_TYPE_NOVELL    0x40
#define PART_TYPE_PCIX      0x4B
#define PART_TYPE_PHOENIXSAVE   0xA0
#define PART_TYPE_CPM       0xDB
#define PART_TYPE_DBFS      0xE0
#define PART_TYPE_BBT       0xFF
//@}


/**
 * This is the format of the contents of the deTime field in the direntry
 * structure.
 * We don't use bitfields because we don't know how compilers for
 * arbitrary machines will lay them out.
 * 
 */
#define DT_2SECONDS_MASK        0x1F    //!< seconds divided by 2 
#define DT_2SECONDS_SHIFT       0       //!< -
#define DT_MINUTES_MASK         0x7E0   //!< minutes 
#define DT_MINUTES_SHIFT        5       //!< -
#define DT_HOURS_MASK           0xF800  //!< hours 
#define DT_HOURS_SHIFT          11      //!< -


/**
 * This is the format of the contents of the deDate field in the direntry
 * structure.
 */
#define DD_DAY_MASK             0x1F    //!< day of month 
#define DD_DAY_SHIFT            0       //!< -
#define DD_MONTH_MASK           0x1E0   //!< month 
#define DD_MONTH_SHIFT          5       //!< -
#define DD_YEAR_MASK            0xFE00  //!< year - 1980 
#define DD_YEAR_SHIFT           9       //!< -


/**
 * Definition of access modes for open files
 * 
 */
#define _O_RDONLY       0x01  //!< open for reading only
#define _O_WRONLY       0x02  //!< open for writing only
#define _O_RDWR         0x04  //!< open for reading and writing
#define _O_APPEND       0x08  //!< writes done at eof
#define _O_WRITEMODES   ( _O_WRONLY | _O_RDWR | _O_APPEND ) //!< open in all write modes 

#define _O_CREAT        0x10  //!< create and open file


/**
 * O_TEXT files have CRLF sequences translated to LF on read()'s,
 * and LF sequences translated to CRLF on write()'s
 * 
*/
#define _O_TEXT         0x40  //!< file mode is text (translated) 
#define _O_BINARY       0x80  //!< file mode is binary (untranslated) 


/**
 * \def MAX_FNAME
 * Maximum length of a filename/path
 * 
 */
#define MAX_FNAME   256 


/** \struct partrecord 
 * Partition Record Structure
 * 
 */ 
struct partrecord /* length 16 uint8_ts */
{           
    uint8_t     prIsActive;                 //!< 0x80 indicates active partition 
    uint8_t     prStartHead;                //!< Atarting head for partition 
    uint16_t    prStartCylSect;             //!< Atarting cylinder and sector 
    uint8_t     prPartType;                 //!< Partition type (see above) 
    uint8_t     prEndHead;                  //!< Ending head for this partition 
    uint16_t    prEndCylSect;               //!< Ending cylinder and sector 
    uint32_t    prStartLBA;                 //!< First LBA sector for this partition 
    uint32_t    prSize;                     //!< Size of this partition (uint8_ts or sectors ?)
#ifndef _DOXYGEN_    
}__attribute__((packed));
#else
}
#endif

        
/** \struct partsector
 *  Partition Sector
 * 
 */        
struct partsector
{
    char            psPartCode[512-64-2];   //!< Pad so struct is 512b 
    // offset 446
    struct          partrecord psPart[4];   //!< Four partition records (64 bytes)
    uint8_t         psBootSectSig0;         //!< First signature   
    uint8_t         psBootSectSig1;         //!< Second signature
#define BOOTSIG0    0x55                    //!< Signature constant 0
#define BOOTSIG1    0xaa                    //!< Signature constant 1
#ifndef _DOXYGEN_
}__attribute__((packed));
#else
}
#endif


/** \struct bpb710 
 * BPB for DOS 7.10 (FAT32).  This one has a few extensions to bpb50.
 * 
 */
struct bpb710 
{
        uint16_t        bpbBytesPerSec; //!< Bytes per sector 
        uint8_t         bpbSecPerClust; //!< Sectors per cluster 
        uint16_t        bpbResSectors;  //!< Number of reserved sectors 
        uint8_t         bpbFATs;        //!< Number of FATs 
        uint16_t        bpbRootDirEnts; //!< Number of root directory entries 
        uint16_t        bpbSectors;     //!< Total number of sectors 
        uint8_t         bpbMedia;       //!< Media descriptor 
        uint16_t        bpbFATsecs;     //!< Number of sectors per FAT
        uint16_t        bpbSecPerTrack; //!< Sectors per track
        uint16_t        bpbHeads;       //!< Number of heads 
        uint32_t        bpbHiddenSecs;  //!< Number of hidden sectors
// 3.3 compat ends here 
        uint32_t        bpbHugeSectors; //!< Number of sectors if bpbSectors == 0
// 5.0 compat ends here 
        uint32_t        bpbBigFATsecs;  //!< Like bpbFATsecs for FAT32
        uint16_t        bpbExtFlags;    //!< Extended flags:
#define FATNUM          0xf             //!< Mask for numbering active FAT
#define FATMIRROR       0x80            //!< FAT is mirrored (like it always was)
        uint16_t        bpbFSVers;      //!< Filesystem version
#define FSVERS          0               //!< Currently only 0 is understood
        uint32_t        bpbRootClust;   //!< Start cluster for root directory
        uint16_t        bpbFSInfo;      //!< Filesystem info structure sector
        uint16_t        bpbBackup;      //!< Backup boot sector 
        uint8_t         bppFiller[12];  //!< Reserved
#ifndef _DOXYGEN_
}__attribute__((packed));
#else
}
#endif


/** \struct extboot
 * Fat12 and Fat16 boot block structure
 * 
 */ 
struct extboot {
        char          exDriveNumber;        //!< Drive number (0x80)
        char          exReserved1;          //!< Reserved
        char          exBootSignature;      //!< Ext. boot signature (0x29)
#define EXBOOTSIG       0x29                //!< Extended boot signature                
        char          exVolumeID[4];        //!< Volume ID number
        char          exVolumeLabel[11];    //!< Volume label
        char          exFileSysType[8];     //!< File system type (FAT12 or FAT16)
#ifndef _DOXYGEN_
}__attribute__((packed));
#else
}
#endif


/** \struct bootsector710 
 * Boot Sector.
 * This is the first sector on a DOS floppy disk or the fist sector of a partition 
 * on a hard disk.  But, it is not the first sector of a partitioned hard disk.
 * 
 */
struct bootsector710 {
        uint8_t         bsJump[3];              //!< Jump inst E9xxxx or EBxx90 
        char            bsOEMName[8];           //!< OEM name and version 
        // 56
        struct bpb710   bsBPB;                  //!< BPB block
        // 26
        struct extboot  bsExt;                  //!< Bootsector Extension 

        char            bsBootCode[418];        //!< Pad so structure is 512b 
        uint8_t         bsBootSectSig2;         //!< 2 & 3 are only defined for FAT32? 
        uint8_t         bsBootSectSig3;         //!< 2 & 3 are only defined for FAT32?
        uint8_t         bsBootSectSig0;         //!< 2 & 3 are only defined for FAT32?
        uint8_t         bsBootSectSig1;         //!< 2 & 3 are only defined for FAT32?

#define BOOTSIG0        0x55                    //!< Signature constant 0
#define BOOTSIG1        0xaa                    //!< Signature constant 1
#define BOOTSIG2        0                       //!< Signature constant 2
#define BOOTSIG3        0                       //!< Signature constant 3

#ifndef _DOXYGEN_
}__attribute__((packed));
#else
}
#endif


/** \struct fsinfo
 *  FAT32 FSInfo block.
 * 
 */
struct fsinfo {
        uint8_t fsisig1[4];         //!< Lead signature
        uint8_t fsifill1[480];      //!< Reseved
        uint8_t fsisig2[4];         //!< Structure signature
        uint8_t fsinfree[4];        //!< Last known free cluster count on the volume
        uint8_t fsinxtfree[4];      //!< Indicates the cluster number at which the driver should start looking for free clusters
        uint8_t fsifill2[12];       //!< Reserved 
        uint8_t fsisig3[4];         //!< Trail signature
        uint8_t fsifill3[508];      //!< Reserved
        uint8_t fsisig4[4];         //!< Sector signature 0xAA55
#ifndef _DOXYGEN_
}__attribute__((packed));
#else
}
#endif


/** 
 * \struct fatdirentry
 * DOS Directory entry.
 * 
 */
struct fatdirentry {
        uint8_t         deName[8];      //!< Filename, blank filled 
#define SLOT_EMPTY      0x00            //!< Slot has never been used 
#define SLOT_E5         0x05            //!< The real value is 0xe5 
#define SLOT_DELETED    0xe5            //!< File in this slot deleted 
        uint8_t         deExtension[3]; //!< Extension, blank filled 
        uint8_t         deAttributes;   //!< File attributes 
#define ATTR_NORMAL     0x00            //!< Normal file 
#define ATTR_READONLY   0x01            //!< File is readonly 
#define ATTR_HIDDEN     0x02            //!< File is hidden 
#define ATTR_SYSTEM     0x04            //!< File is a system file 
#define ATTR_VOLUME     0x08            //!< Entry is a volume label 
#define ATTR_LONG_FILENAME  0x0f        //!< This is a long filename entry              
#define ATTR_DIRECTORY  0x10            //!< Entry is a directory name 
#define ATTR_ARCHIVE    0x20            //!< File is new or modified 
        uint8_t         deLowerCase;    //!< NT VFAT lower case flags 
#define LCASE_BASE      0x08            //!< Filename base in lower case 
#define LCASE_EXT       0x10            //!< Filename extension in lower case 
        uint8_t         deCHundredth;   //!< Hundredth of seconds in CTime 
        uint8_t         deCTime[2];     //!< Creation time 
        uint8_t         deCDate[2];     //!< Creation date 
        uint8_t         deADate[2];     //!< Last access date 
        uint16_t        deHighClust;    //!< High bytes of cluster number 
        uint8_t         deMTime[2];     //!< Last update time 
        uint8_t         deMDate[2];     //!< Last update date 
        uint16_t        deStartCluster; //!< Starting cluster of file 
        uint32_t        deFileSize;     //!< Size of file in bytes 

#ifndef _DOXYGEN_
}__attribute__((packed));
#else
};
#endif


/**
 * \typedef fatdirentry_t 
 * Type definition for fatdirentry
 * 
 */
typedef struct fatdirentry  fatdirentry_t;


/**
 * \struct fatffdata_t
 * FatFind helper structure.
 * 
 */ 
typedef struct
{
    uint32_t        ff_cluster;     //!< 4 - Cluster of a found entry   
    uint16_t        ff_offset;      //!< 2 - Offset of directory entry within cluster   
    char            *ff_search;     //!< 2 - Pointer to the name we're searching for
    uint8_t         ff_islong;      //!< 1 - Set to indicate a long name
    char            ff_short[12];   //!< 12 - DOS'ified version of the (short) name we're looking for   
    char            ff_name[256];   //!< 256 - Name entry found (long or short) 
    fatdirentry_t   ff_de;          //!< 32 - directory entry with file data (short entry)
} fatffdata_t;              

/**
 * \typedef fhandle_t
 * Filehandle structure.
 * 
 */
typedef struct fhandle 
{
    int             mode;           //!< file mode
    uint32_t        foffset;        //!< file offset
    uint32_t        highwater;      //!< max offset used and/or filesize when reading
    uint32_t        firstcluster;   //!< starting cluster of file
    uint32_t        cluster;        //!< current cluster 
} fhandle_t;


/** \struct winentry
 *  Win95 long name directory entry.
 */
struct winentry {
        uint8_t         weCnt;          //!< LFN sequence counter
#define WIN_LAST        0x40            //!< Last sequence indicator
#define WIN_CNT         0x3f            //!< Sequence number mask
        uint8_t         wePart1[10];    //!< Characters 1-5 of LFN
        uint8_t         weAttributes;   //!< Attributes, must be ATTR_LONG_FILENAME
#define ATTR_WIN95      0x0f            //!< Attribute of LFN for Win95
        uint8_t         weReserved1;    //!< Must be zero, reserved
        uint8_t         weChksum;       //!< Checksum of name in SFN entry
        uint8_t         wePart2[12];    //!< Character 6-11 of LFN
        uint16_t        weReserved2;    //!< Must be zero, reserved
        uint8_t         wePart3[4];     //!< Character 12-13 of LFN

#ifndef _DOXYGEN_
}__attribute__((packed));
#else
};
#endif

/** \def WIN_CHARS
 * Number of chars per winentry 
 * 
 */
#define WIN_CHARS       13


/** \def WIN_MAXLEN
 * Maximum filename length in Win95.
 * Note: Must be < sizeof(dirent.d_name)
 */
#define WIN_MAXLEN      255


/**
 * \def MAX_FILES
 * Maximum open files ???
 * 
 */
#define MAX_FILES   4

extern uint32_t current_dir_cluster;      //!< Cluster of Current Directory
extern uint16_t bytes_per_sector;

void fat_init(void);
uint32_t fat_size(int fd);
int fat_open(const char *name, int mode);
int fat_fastopen(fatffdata_t *ff);
int fat_close(int fd);
int fat_blockread(int fd, void *buffer);
int fat_read(int fd, char *buffer, int len);
uint32_t fat_seek(int fd, long offset, uint8_t origin );

int find_first(char *name, fatffdata_t * ff_data);
int find_next(fatffdata_t * ff_data);

int num_direntries(char *match);
fatffdata_t * index_to_ff(char *match, int index);

int fat_chdir(const char *name);

#endif /*FAT_H_*/
