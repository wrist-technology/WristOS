/**
 * \file fat.c
 * FAT functions
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <hardware_conf.h>
#include <firmware_conf.h>
#include <screen/screen.h>
#include <screen/font.h>
#include <debug/trace.h>
#include <utils/macros.h>
#include <sdcard/sdcard.h>
#include "fat.h"

/*
 * 4 byte uint8_t to uint32_t conversion union
 * 
 */
typedef union
{
    uint32_t ulong;     //!< uint32_t result
    uint8_t uchar[4];   //!< 4 byte uint8_t to convert
}LBCONV;

uint8_t     isFAT32;                //!< FAT32 / FAT16 indicator
uint16_t    sectors_per_cluster;    //!< Number of sectors in each disk cluster
uint32_t    first_data_sector;      //!< LBA index of first sector of the dataarea on the disk
uint32_t    first_fat_sector;       //!< LBA index of first FAT sector on the disk
uint32_t    first_dir_sector;       //!< LBA index of first Root Directory sector on the disk
uint32_t    root_dir_cluster;       //!< First Cluster of Directory (FAT32)
uint16_t    root_dir_sectors;       //!< Number of Sectors in Root Dir (FAT16)

uint32_t    num_clusters;           //!< Number of data clusters on partition
uint32_t    num_fat_sectors;        //!< Number of sectors per FAT

uint32_t    current_dir_cluster;    //!< Current Cluster of Directory

static uint8_t sector_buffer[512];  //!< Sector buffer

uint16_t    bytes_per_sector;       //!< Number of bytes per sector
uint16_t    bytes_per_cluster;      //!< Number of bytes per cluster

fhandle_t   *fd_handles[MAX_FILES]; //!< Array of file handles

static uint32_t sector_in_buffer = -2;  //!< number of sector located in buffer

/**
 * Read sector
 * 
 * Read card sector from SD card in buffer.
 * \param   lba     Sector number
 * \param   buffer  Buffer to store sector content
 * \return  Error code
 */
static uint8_t readsector(uint32_t lba, uint8_t *buffer)
{
    // check if sector is still in buffer
    // don't read sector multiple times in series
    if (lba == sector_in_buffer)
        return 0;

    sector_in_buffer = lba;
    TRACE_FAT("read sector %08lX\n", lba);
   
    return sd_readsector(lba,buffer, 0, 0);    
}


/**
 * Cluster to sector
 * 
 * Converting a cluster number to a LBA sector number.
 * \param clust Cluster number
 * \return Sector number
 * 
 */
static uint32_t cluster2sector(uint32_t clust)
{
    if (clust == 0) // if this is a cluster request for the rootdir, point to it
        return first_dir_sector;

    return ( (uint32_t) (clust-2) * sectors_per_cluster) + first_data_sector;    
}


/**
 * Next cluster
 * 
 * Find next cluster in the FAT chain.
 * 
 * \param   cluster     Actual cluster
 * \return  Next cluster in chain
 * 
 */
static uint32_t fat_nextcluster(uint32_t cluster)
{
    uint32_t sector, offset, newcluster, mask;
    
    uint32_t conv_val;
    LBCONV *conv;
    conv = (LBCONV*)&conv_val;
    
    //
    // calculate the sector number and sector offset 
    // in the FAT for this cluster number
    
    if (isFAT32)                        // if FAT32 
    {
        offset = cluster << 2;          // there is 4 bytes per FAT entry in FAT32  
        mask = FAT32_MASK;
    }
    else
    {
        offset = cluster << 1;          // there is 2 bytes per FAT entry in FAT16
        mask = FAT16_MASK;
    }

    sector  = first_fat_sector + (offset / bytes_per_sector);
    offset  = offset % bytes_per_sector;

    //
    // get the sector
    //
    readsector(sector, (uint8_t*) sector_buffer);
    
    //
    // get the data into offset
    //
    if (isFAT32)
    {
        conv->uchar[0] = sector_buffer[offset]; 
        conv->uchar[1] = sector_buffer[offset+1];  
        conv->uchar[2] = sector_buffer[offset+2];  
        conv->uchar[3] = sector_buffer[offset+3];  

        newcluster = conv->ulong & mask;
    }
    else
    {
        conv->uchar[0] = sector_buffer[offset]; 
        conv->uchar[1] = sector_buffer[offset+1];  
        conv->uchar[2] = 0;  
        conv->uchar[3] = 0;  

        newcluster = conv->ulong & mask;
    }

    //
    // mask the entry
    //      
    if (newcluster >= (CLUST_EOFS & mask) )
        return 0;

    return newcluster & mask;
}


/**
 * Init FAT
 * 
 * Initialize the file system by reading some
 * basic information from the disk
 * 
 */
void fat_init(void)
{
    uint32_t tot_sectors;
    uint32_t data_sectors;
    uint32_t first_sector;
    int i;

    struct  partrecord *pr = NULL;
    struct  bpb710 *bpb;

    // first check if there is a partition sector
    // by checking the first byte of the sector
    TRACE_FAT("FAT init\n");

    readsector(0, sector_buffer);   // read MBR

    if (*sector_buffer == 0xE9 || *sector_buffer == 0xEB) // if a jump instruction here
    {
        // there's no partition sector
        // set default sector start
        first_sector = 0;
    }
    else
    {
        // get the first partition record from the data
        pr = (struct partrecord *) ((struct partsector *) sector_buffer)->psPart;

        // get partition start sector
        first_sector = pr->prStartLBA; // get partition start sector
    }

    TRACE_FAT("Filesystem is a ");
    TRACE_SCR("FAT");

    isFAT32 = 0;

    /** \todo Add more FAT16 types here. */
    if ( pr->prPartType == PART_TYPE_FAT16 )
    {
        TRACE_FAT("FAT16\n");
        TRACE_SCR("16");
    }
    else
    if ( (pr->prPartType == PART_TYPE_FAT32) ||
         (pr->prPartType == PART_TYPE_FAT32LBA)
       )
    {
        TRACE_FAT("FAT32\n");
        TRACE_SCR("32");
        isFAT32 = 1;
    }
    else
    {
        TRACE_FAT("Unknown\n");
    }

    // Read Partition Boot Record (Volume ID)
    readsector(first_sector, sector_buffer); 

    // point to partition boot record
    bpb = (struct bpb710 *)  &((struct bootsector710 *) sector_buffer)->bsBPB;

    // Number of bytes per sector
    bytes_per_sector = bpb->bpbBytesPerSec;

    // Number of sectors per FAT
    if (bpb->bpbFATsecs != 0)
        num_fat_sectors = bpb->bpbFATsecs;
    else
        num_fat_sectors = bpb->bpbBigFATsecs;

    // Number of sectors in root dir (will be 0 for FAT32)
    root_dir_sectors  = ((bpb->bpbRootDirEnts * 32) + (bpb->bpbBytesPerSec-1)) / bpb->bpbBytesPerSec; 

    // First data sector on the volume (partition compensation will be added later)
    first_data_sector = bpb->bpbResSectors + bpb->bpbFATs * num_fat_sectors + root_dir_sectors;

    // calculate total amount of sectors on the volume
    if (bpb->bpbSectors != 0)
        tot_sectors = bpb->bpbSectors;
    else
        tot_sectors = bpb->bpbHugeSectors;
        
    // Total number of data sectors
    data_sectors = tot_sectors - first_data_sector;

    // Total number of clusters
    num_clusters = data_sectors / bpb->bpbSecPerClust;

    // check if this is a FAT32 or FAT16 filesystem     
    TRACE_FAT("data_sectors = %ld\n",data_sectors);
    TRACE_FAT("num_clusters = %ld\n",num_clusters);

    // calculate global variable values
    first_data_sector += first_sector;
    sectors_per_cluster = bpb->bpbSecPerClust;
    first_fat_sector = bpb->bpbResSectors + first_sector;

    // FirstDirSector is only used for FAT16
    first_dir_sector = bpb->bpbResSectors + bpb->bpbFATs * bpb->bpbFATsecs + first_sector;

    if (isFAT32)
        root_dir_cluster      = bpb->bpbRootClust;
    else        
        root_dir_cluster      = 0; // special case

    current_dir_cluster = root_dir_cluster;

    bytes_per_cluster = sectors_per_cluster * bytes_per_sector;

    TRACE_FAT("Bytes per sector= %ld\n",bytes_per_sector);
    TRACE_FAT("FirstSector     = %ld\n",first_sector);
    TRACE_FAT("Sectors/Cluster = %ld\n",sectors_per_cluster);
    TRACE_FAT("FirstFATSector  = %ld\n",first_fat_sector);
    TRACE_FAT("FirstDataSector = %ld\n",first_data_sector);
    TRACE_FAT("FirstDirSector  = %ld\n",first_dir_sector);
    TRACE_FAT("root_dir_cluster  = %ld\n",root_dir_cluster);
    TRACE_SCR(": %ldMB\n\r", (unsigned long)(((long long)data_sectors*bytes_per_sector)/(1024*1024)) );
    

    // clear filehandles
    for (i=0;i<MAX_FILES;i++)
        fd_handles[i] = 0;
}


/**
 * TBD
 * 
 * \todo Add function description here
 * 
 */
static uint8_t szWildMatch8(char * pat, char * str) 
{
   char *s, *p;
   uint8_t star = 0;

loopStart:
   for (s = str, p = pat; *s; ++s, ++p) 
   {
      switch (*p) 
      {
         case '?':
            if (*s == '.') goto starCheck;
            break;
         case '*':
            star = 1;
            str = s, pat = p;
            //do { ++pat; } while (*pat == '*');    // can be skipped
            if (!*(++pat)) return 1;
            goto loopStart;
         default:
            if (toupper(*s) != toupper(*p))
               goto starCheck;
            break;
      } // endswitch 
   } // endfor 
   if (*p == '*') ++p;
   return (!*p);

starCheck:
   if (!star) return 0;
   str++;
   goto loopStart;
}


/**
 * Make valid filename from DOS short name
 * 
 * \param str Filename buffer
 * \param dos DOS filename
 * \param ext Extention
 * 
 */
void dos2str(char *str, char *dos, char *ext)
{
    uint8_t i;

    for (i=0;i<8 && *dos && *dos != ' ';i++)
        *str++ = *dos++;

    if (*ext && *ext != ' ')
    {
        *str++ = '.';
        for (i=0;i<3 && *ext && *ext != ' ';i++)
            *str++ = *ext++;
    }
    *str = 0;
}


/**
 * Scan through directory entries to find the given file or directory
 * 
 * \param ff_data FatFind helper structure
 * \return Error code
 * 
 */
static int ff_scan(fatffdata_t * ff_data)
{
    uint16_t i,j, eindex;
    uint16_t sectoroffset, entryoffset, maxsector;
    uint32_t cluster;
    uint32_t sector;
    uint8_t match,longmatch;
    char matchspace[13];
    fatdirentry_t *de;
    uint8_t n;
    uint8_t nameoffset;

    eindex  = ff_data->ff_offset;
    cluster = ff_data->ff_cluster;

    sectoroffset = eindex / (bytes_per_sector/sizeof(fatdirentry_t));
    entryoffset  = eindex % (bytes_per_sector/sizeof(fatdirentry_t));

    if (isFAT32 == 0 && cluster == root_dir_cluster ) // if not FAT32 and this is the root dir
        maxsector = root_dir_sectors;                 // set max sector count for root dir
    else
        maxsector = sectors_per_cluster;

    longmatch = 0;
    // loop until entry found or end of list
    while (1)                               
    {
        sector = cluster2sector(cluster);

        for (i=sectoroffset;i<maxsector;i++)
        {
            readsector(sector+i, (uint8_t*) sector_buffer);      // request a sector
            de = (fatdirentry_t *) sector_buffer;
            de += entryoffset;

            for (j=entryoffset;j<bytes_per_sector/sizeof(fatdirentry_t);j++)  // 32 entries per sector 
            {

                if (*de->deName == SLOT_EMPTY)
                {
                    return -1;
                }

                if (*de->deName != SLOT_DELETED)
                {
                    if ((de->deAttributes & ATTR_LONG_FILENAME) == ATTR_LONG_FILENAME)
                    {
                        // found long name entry

                        if ( ((struct winentry *)de)->weCnt & WIN_LAST )            // first part of a long name
                            memset(ff_data->ff_name,0,sizeof(ff_data->ff_name));    // clear name

                        // piece together a fragment of the long name
                        // and place it at the correct spot
                        nameoffset = ((((struct winentry *)de)->weCnt & WIN_CNT)-1) * WIN_CHARS;

                        for (n=0;n<5;n++)
                            ff_data->ff_name[nameoffset+n] = ((struct winentry *)de)->wePart1[n*2];
                        for (n=0;n<6;n++)
                            ff_data->ff_name[nameoffset+5+n] = ((struct winentry *)de)->wePart2[n*2];
                        for (n=0;n<2;n++)
                            ff_data->ff_name[nameoffset+11+n] = ((struct winentry *)de)->wePart3[n*2];

                        if ( (((struct winentry *)de)->weCnt & WIN_CNT) == 1 )  // long name complete
                        {
                            // set match flag
                            longmatch = szWildMatch8(ff_data->ff_search, ff_data->ff_name);
                        }
                    }
                    else
                    {
                        // found short name entry, determine a match 
                        dos2str(matchspace,(char*)de->deName,(char*)de->deExtension);
                        match = szWildMatch8(ff_data->ff_search, matchspace);

                        // special!
                        // skip the single dot entry
                        if (strcmp(matchspace,".") != 0)
                        {
                            if ((match || longmatch) && ((de->deAttributes & ATTR_VOLUME) == 0))
                            {
                                // found
                                ff_data->ff_cluster = cluster;
                                ff_data->ff_offset = eindex;    // save index of file
                                memcpy(&ff_data->ff_de,de,sizeof(fatdirentry_t));
                                if (!longmatch)
                                {
                                    strcpy(ff_data->ff_name, matchspace);
                                }
                                return 0;
                            }
                        }
                    }

                }//if !SLOT_DELETED

                eindex++;
                de++;   // next entry
            }//for (j=entryoffset

            entryoffset = 0;
        }//for (i=sectoroffset

        sectoroffset = 0;
        cluster = fat_nextcluster(cluster);
 
        if (cluster == 0)   // if end of chain
        {
            TRACE_FAT("ff_scan end of chain\n");
            return -1;
        }
        eindex = 0;
    }//while

}


/**
 * Scan through disk directory to find the given file or directory
 * 
 * \param name File or directory name
 * \param ff_data FatFind helper structure
 * \return Error code
 * 
 */
int find_first(char *name, fatffdata_t * ff_data)
{
    char *q;

    TRACE_FAT("_find_first for %s\r\n",name);

    memset(ff_data->ff_name,0,sizeof(ff_data->ff_name));
    memset(ff_data->ff_short,0,sizeof(ff_data->ff_short));

    // determine if it's a long or short name
    ff_data->ff_islong = 0;         // assume short name    

    if ((q = strchr(name,'.')) != 0)    // if there is an extension dot 
    {
        if (q-name > 8)                 // if the base part is longer than 8 chars,
            ff_data->ff_islong = 1; // it's a long name 

        if (strlen(q+1) > 3)            // if the extension is longer than 3 chars,
            ff_data->ff_islong = 1; // it's a long name 
    }
    else
    {
        if (strlen(name) > 8)               // if name is longer than 8 chars, 
            ff_data->ff_islong = 1;     // it's a long name 
    }

    ff_data->ff_search = name;
    ff_data->ff_cluster = current_dir_cluster;    // first cluster to search
    ff_data->ff_offset = 0;                     // initial offset in cluster

    return ff_scan(ff_data);    
}


/**
 * Skip to next entry in cluster
 * 
 * \param ff_data FatFind helper structure
 * \return Error code
 * 
 */
int find_next(fatffdata_t * ff_data)
{
    ff_data->ff_offset++;
    return ff_scan(ff_data);
}


/**
 * Find number of entries for specific filename
 * 
 * \param match Filename to count
 * \return Number of matching directory entries
 * 
 */
int num_direntries(char *match)
{
    int res, max = 0;
    fatffdata_t ff; /** \todo ! no good with this static alloc, dynamic is also crap, see if other solution can be used. */

    res = find_first(match,&ff);
    while (res != -1)
    {
        res = find_next(&ff);
        max++;
    }
    return max; 
}


/**
 * Map from index to fatfind structure.
 * Return the fatfind struct for the file with the given index in the current directory.
 * 
 * \param match     The filename mask to apply in the search
 * \param fileindex Index of file to find
 * \return FatFind helper structure
 */
fatffdata_t * index_to_ff(char *match, int fileindex)
{
    int res;
    fatffdata_t *ff = (fatffdata_t*) malloc(sizeof(fatffdata_t));
    if (ff)
    {
        res = find_first(match,ff);
        while (res != -1)
        {
            if (fileindex == 0)
                break;
            fileindex--;
            res = find_next(ff);
        }
        if (res == -1)
        {
            free(ff);
            return NULL;
        }
    }
    return ff;
}


/**
 * Change directory
 * 
 * name may be a path like /joe/ben/moses
 * 
 * \param name Directory or path name
 * \return Current directory cluster
 *  
 */
int fat_chdir(const char *name)
{
    fatffdata_t *ff;
    char *p,*q;
    char tmp[32];/** TODO !! Argh ! I HATE these large variables */

    if (name == NULL)
        return 1;

    ff = malloc(sizeof(fatffdata_t));

    p = (char *) name;

    // if first char is /, go to root
    if (*p == '/')
    {
        TRACE_FAT("_chdir: changing to root\n");
       
        current_dir_cluster = root_dir_cluster;
        p++;
    }
    
    while (*p)
    {
        q = tmp;
        while (*p && *p != '/')
            *q++ = *p++;
        *q = 0;
        if (*p == '/')
            p++;
        if (*tmp)
        {
            TRACE_FAT("_chdir: changing to %s\r\n",tmp);

            if ( find_first(tmp, ff) != 0)  // if not found
            {
                TRACE_FAT("_chdir:ERROR: %s not found\r\n",tmp);
                
                free(ff);   
                return (uint32_t) -1;                           // fail
            }

            if ((ff->ff_de.deAttributes & ATTR_DIRECTORY) != ATTR_DIRECTORY)    // if this is not a directory
            {
                TRACE_FAT("_chdir:ERROR: %s is not a directory\r\n",tmp);
               
                free(ff);   
                return (uint32_t) -1;           // fail     TODO, this is not necessarily a fault
            }

            // set new directory as active
            
            current_dir_cluster = ff->ff_de.deHighClust;
            current_dir_cluster <<= 16;
            current_dir_cluster |= ff->ff_de.deStartCluster;
        }
    }
    TRACE_FAT("_chdir: all done\r\n");

    free(ff);   
    return current_dir_cluster;
}


/**
 * Return the size of a file
 * 
 * \param   fd  File handle
 * \return  Size of file
 * 
 */
uint32_t fat_size(int fd)
{
    fhandle_t *handle = fd_handles[fd];   
    
    if ( (fd > MAX_FILES) || (fd < 0) ) // if not valid index
        return (uint32_t) -1;           // return failure
    
    if (handle == NULL)                 // if slot is not in use
        return (uint32_t) -1;           // return failure

    return handle->highwater;           // return filesize          
}


/**
 * Open a file
 * 
 * \param name File name
 * \param mode Mode to open file
 * 
 * - _O_BINARY Raw mode.
 * - _O_TEXT End of line translation. 
 * - _O_EXCL Open only if it does not exist. 
 * - _O_RDONLY Read only. 
    
      any of the write modes will (potentially) modify the file or directory entry

 * - _O_CREAT Create file if it does not exist. 
 * - _O_APPEND Always write at the end. 
 * - _O_RDWR Read and write. 
 * - _O_WRONLY Write only.

 * - _O_TRUNC Truncate file if it exists. This is currently not supported
 *
 * \return File handle 
 */
int fat_open(const char *name, int mode)
{
    fatffdata_t *ff;
    fhandle_t *fd = 0;
    char *p;
    int handle_index;

    TRACE_FAT("fat_open %s\n",name);

    if (!name || !*name)
        return -1;

    if (strchr(name,'/'))       // if the name has a path component
        fat_chdir((char*)name);     // follow the path  

    p = (char*) name;
    while ( strchr(p,'/') != 0 )    // if the name has an initial path component
        p = strchr(p,'/') + 1;      // step past it


    // alloc for findfile struct
    ff = malloc(sizeof(fatffdata_t));
    if (ff == NULL)
    {
        TRACE_FAT("cannot alloc fatffdata_t in fat_open\n");
        return -1;  // fail
    }

    // alloc for file spec
    fd = malloc(sizeof(fhandle_t)); // get a struct
    if (fd == NULL)
    {
        free(ff);
        TRACE_FAT("cannot alloc fhandle_t in fat_open\n");
        return -1;  // fail
    }
    
    for (handle_index=0;handle_index<MAX_FILES;handle_index++)
    {
        if (fd_handles[handle_index] == NULL)
            break; 
    }
    if (handle_index == MAX_FILES)
    {
        free(ff);
        free(fd);
        TRACE_FAT("no free handle slots in fat_open\n");
        return -1;  // fail
    }
    
    memset(fd,0,sizeof(fhandle_t)); // clear all fields
    fd->mode = mode;    // save mode

    // expecting existing file
    // try find it
    if ( find_first(p, ff) == 0)
    {
        // found existing entry
        TRACE_FAT("found existing file\n");

        // NASTY !!!! TODO
//      fd->diroffset = ( ((char*)&ff->ff_de) - ((char*)sector_buffer)) / sizeof(fatdirentry_t);
//      fd->dirsector = sector_in_buffer;

        // setup pointers
        fd->firstcluster = ((uint32_t)ff->ff_de.deHighClust << 16) | (ff->ff_de.deStartCluster);    // starting cluster
        fd->cluster = fd->firstcluster;             // current cluster
        fd->foffset = 0;                            // offset in file
        fd->highwater = ff->ff_de.deFileSize;       // max position in file
        goto open_ok;
    }
    else
    {
        // file not found, this is always a fault as we do not support creating files
        free(ff);
        free(fd);
 
        TRACE_FAT("file not found: %s\n", name);
     
        return -1;          // fail
    }

open_ok:
    free(ff);
    fd_handles[handle_index] = fd;

    TRACE_FAT("fat_open returns %d\n",handle_index);
    return handle_index;
}


/**
 * Fast Fileopen from a previous fatfind_ff structure.
 * 
 * \param ff FatFind helper structure
 * \return File handle
 */
int fat_fastopen(fatffdata_t *ff)
{
    fhandle_t *fd = 0;
    int handle_index;

    TRACE_FAT("fat_fastopen %s\n",ff->ff_name);

    // alloc for file spec
    fd = malloc(sizeof(fhandle_t)); // get a struct
    if (fd == NULL)
    {
        TRACE_FAT("cannot alloc fhandle_t in fat_fastopen\n");
        return -1;  // fail
    }
    
    for (handle_index=0;handle_index<MAX_FILES;handle_index++)
    {
        if (fd_handles[handle_index] == NULL)
            break; 
    }
    if (handle_index == MAX_FILES)
    {
        free(fd);
        TRACE_FAT("no free handle slots in fat_fastopen\n");
        return -1;  // fail
    }
    
    
    memset(fd,0,sizeof(fhandle_t));         // clear all fields
    fd->mode = _O_RDONLY;                   // save mode

//  fd->diroffset = ( ((char*)&ff->ff_de) - ((char*)SectorBuffer)) / sizeof(fatdirentry_t);
//  fd->dirsector = sector_in_buffer;

    // setup pointers
    fd->firstcluster = ((uint32_t)ff->ff_de.deHighClust << 16) | (ff->ff_de.deStartCluster);    // starting cluster
    fd->cluster = fd->firstcluster;                 // current cluster
    fd->foffset = 0;                            // offset in file
    fd->highwater = ff->ff_de.deFileSize;       // max position in file

    fd_handles[handle_index] = fd;
    TRACE_FAT("fat_fastopen returns %d\n",handle_index);
    return handle_index;
}


/**
 * Close a file.
 * 
 * \param fd File handle
 * \return Error code
 * 
 */
int fat_close(int fd)
{
    TRACE_FAT("fat_close handle %d\n",fd);
    fhandle_t *handle = fd_handles[fd];   
    
    if ( (fd > MAX_FILES) || (fd < 0) ) // if not valid index
        return (uint32_t) -1;           // return failure
    
    if (handle == NULL)                 // if slot is not in use
        return (uint32_t) -1;           // return failure

    free(handle);                       // free memory
    fd_handles[fd] = NULL;            // clear slot to empty

    return 0;
}


/**
 * Read one complete file sector into buffer
 * 
 * This is a fast blockaligned read. 
 * 
 * \param fd File handle
 * \param buffer Buffer to store data
 * \return Number of bytes successful read.
 * 
 */

int fat_blockread(int fd, void *buffer)
{
    uint32_t    block;
    uint32_t    temp;
    fhandle_t *handle = fd_handles[fd];   
    
    if ( (fd > MAX_FILES) || (fd < 0) ) // if not valid index
        return (uint32_t) -1;           // return failure
    
    if (handle == NULL)                 // if slot is not in use
        return (uint32_t) -1;           // return failure

    if (handle->foffset > handle->highwater)
    {
        return 0;
    }
    
    block = cluster2sector(handle->cluster);    // get the start block

    temp = handle->foffset % bytes_per_cluster;        // for later use

    // add local block within a cluster 
    block += temp / bytes_per_sector;

    TRACE_FAT("blockread: %ld\n",block);

    // read the data
    sd_readsector(block, buffer, 0, 0);
    /*handle->foffset += bytes_per_sector;*/ 
    handle->foffset += bytes_per_sector - (handle->foffset%bytes_per_sector);

    if (handle->foffset % bytes_per_cluster == 0)      // cluster boundary
    {
        // need to get a new cluster
        handle->cluster = fat_nextcluster(handle->cluster); // follow FAT chain
        //if (fd->cluster == -1)                        // if we're at end of chain
        //{
            //break;                                // we're finished, there is no more data
        //}
    }

    return bytes_per_sector;
}


/**
 * Read specific number of bytes from file
 * 
 * \param fd File handle
 * \param buffer Buffer to store data
 * \param len Number of bytes to read
 * \return Number of bytes successful read.
 * 
 */
int fat_read(int fd, char *buffer, int len)
{
    uint16_t    nbytes;
    uint32_t    block;
    uint16_t    dcount;
    uint16_t    loffset;
    fhandle_t   *handle = fd_handles[fd]; 

    TRACE_FAT("fat_read %d,%08x,%d\n",fd,buffer,len);
    
    if ( (fd > MAX_FILES) || (fd < 0) ) // if not valid index
        return -1;           // return failure
    
    if (handle == NULL)                 // if slot is not in use
        return -1;           // return failure


    // limit max read to size of file
    if (len > (handle->highwater - handle->foffset) )
        len = handle->highwater - handle->foffset;

    dcount = len;

    TRACE_FAT("fat_read\n");

    while (dcount)
    {
        loffset = handle->foffset % bytes_per_sector; // local offset within a block 
        block = cluster2sector(handle->cluster);    // get the start block

        // add local block within a cluster 
        block += (handle->foffset % bytes_per_cluster) / bytes_per_sector;

        // limit to max one block
        nbytes = dcount < bytes_per_sector ? dcount : bytes_per_sector;

        //limit to remaining bytes in a block
        if (nbytes > (bytes_per_sector - loffset) )
            nbytes = bytes_per_sector - loffset;

        // read the data
        sd_read_n(block, loffset, nbytes, (uint8_t*) buffer);

        // bump buffer pointer
        buffer += nbytes;

        handle->foffset += nbytes;
        dcount -= nbytes;

        if (handle->foffset % bytes_per_cluster == 0)      // cluster boundary
        {
            // need to get a new cluster
            handle->cluster = fat_nextcluster(handle->cluster); // follow FAT chain
            if (handle->cluster == -1)                      // if we're at end of chain
            {
                break;                              // we're finished, there is no more data
            }
        }
    }
    TRACE_FAT("fat_read return %d\n",len - dcount);
    return len - dcount;
}

int fat_write(int fd, char *buffer, int len)
{
    uint16_t    nbytes;
    uint32_t    block;
    uint16_t    dcount;
    uint16_t    loffset;
    fhandle_t   *handle = fd_handles[fd]; 

    TRACE_FAT("fat_write %d,%08x,%d\n",fd,buffer,len);
    
    if ( (fd > MAX_FILES) || (fd < 0) ) // if not valid index
        return -1;           // return failure
    
    if (handle == NULL)                 // if slot is not in use
        return -1;           // return failure


    // limit max read to size of file
    if (len > (handle->highwater - handle->foffset) )
        len = handle->highwater - handle->foffset;

    dcount = len;

    TRACE_FAT("fat_write\n");

    //while (dcount)
    {
        loffset = handle->foffset % bytes_per_sector; // local offset within a block 
        block = cluster2sector(handle->cluster);    // get the start block

        // add local block within a cluster 
        block += (handle->foffset % bytes_per_cluster) / bytes_per_sector;

        // limit to max one block
        nbytes = dcount < bytes_per_sector ? dcount : bytes_per_sector;

        //limit to remaining bytes in a block
        if (nbytes > (bytes_per_sector - loffset) )
            nbytes = bytes_per_sector - loffset;

        // read the data
        sd_write_n(block, loffset, nbytes, (uint8_t*) buffer);

        // bump buffer pointer
        buffer += nbytes;

        handle->foffset += nbytes;
        dcount -= nbytes;
/*
        if (handle->foffset % bytes_per_cluster == 0)      // cluster boundary
        {
            // need to get a new cluster
            handle->cluster = fat_nextcluster(handle->cluster); // follow FAT chain
            if (handle->cluster == -1)                      // if we're at end of chain
            {
                break;                              // we're finished, there is no more data
            }
        }
        */
    }
    TRACE_FAT("fat_write return %d\n",len - dcount);
    return len - dcount;
}


/**
 * Seek to specific position within file.
 * 
 * \param fd File handle
 * \param offset Number of bytes to seek
 * \param origin Position from where to start
 * \return New position within file
 * 
 */
uint32_t fat_seek(int fd, long offset, uint8_t origin )
{
    long fpos = 0,npos,t, newcluster;
    fhandle_t *handle = fd_handles[fd];   
    
    if ( (fd > MAX_FILES) || (fd < 0) ) // if not valid index
    {
        TRACE_FAT("FATSeek failure, invalid index \n");
        return (uint32_t) -1;           // return failure
    }
    
    if (handle == NULL)                 // if slot is not in use
    {
        TRACE_FAT("FATSeek failure, slot not in use\n");
        return (uint32_t) -1;           // return failure
    }


    // setup position to seek from
    switch (origin)
    {
        case SEEK_SET : fpos = 0; break;
        case SEEK_CUR : fpos = handle->foffset; break;
        case SEEK_END : fpos = handle->highwater; break;
    }
    
    // adjust and apply limits
    npos = fpos + offset;
    if (npos < 0)
        npos = 0;
    if (npos > handle->highwater)
        npos = handle->highwater;

    // now set the new position
    t = npos;
    handle->foffset = t;            // real offset

    // calculate how many clusters from start cluster
    t = npos / bytes_per_cluster;

    //set start cluster
    handle->cluster = handle->firstcluster;

    // follow chain
    while (t--)
    {
        newcluster = fat_nextcluster(handle->cluster);


        //  TODO: is this really needed ?
        
        // this is a special kludge for handling seeking to the last byte+1 
        // in a file. This will return cluster 0, as we're on the end of the chain
        // typically happens when searching before writing to a file that ends on
        // a cluster bonudary (like the directory)
        //
        // as a temp fix, we just keep the last cluster in the file (which is actually
        // the one with the terminator tag).
        //
        if (newcluster != 0)
            handle->cluster = newcluster;
    }

    return npos; 
}

