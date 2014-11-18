// littleBits
// MP3 Player
//
// FAT library
//
// Copyright 2014 littleBits Electronics
//
// This file is part of i25-mp3player.
//
// i25-mp3player is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// i25-mp3player is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License at <http://www.gnu.org/licenses/> for more details.

#ifndef __FAT_H__
#define __FAT_H__

#define S8	char
#define U8	unsigned char
#define U16	unsigned int
#define U32	unsigned long

#define FIX_DIRECTORY 0		/* 1 means use fix directory, 0 for any directory */

#define MAX_TRACKS 250

#define  RECORD_ADDR_START 0	/* eeprom start address */
#define  RECORD_ADDR_END  512	/* eeprom end address */

#include<avr/eeprom.h>
#include<avr/pgmspace.h>
#include<stdio.h>


//#include "UART.H"
#include <stdlib.h>

//external hardware operating function
extern U8 SD_ReadBlock(U32 sector, U8* buffer);
extern U32 SD_ReadCapacity(void);


#define MSDOSFSROOT     0               // cluster 0 means the root dir
#define CLUST_FREE      0               // cluster 0 also means a free cluster
#define MSDOSFSFREE     CLUST_FREE
#define CLUST_FIRST     2             	// first legal cluster number
#define CLUST_RSRVD     0xfff6      	// reserved cluster range
#define CLUST_BAD       0xfff7     		// a cluster with a defect
#define CLUST_EOFS      0xfff8     		// start of eof cluster range
#define CLUST_EOFE      0xffff      	// end of eof cluster range




struct partrecord // length 16 bytes
{			
	U8	prIsActive;					// 0x80 indicates active partition
	U8	prStartHead;				// starting head for partition
	U16	prStartCylSect;				// starting cylinder and sector
	U8	prPartType;					// partition type (see above)
	U8	prEndHead;					// ending head for this partition
	U16	prEndCylSect;				// ending cylinder and sector
	U32	prStartLBA;					// first LBA sector for this partition
	U32	prSize;						// size of this partition (bytes or sectors ?)
};

        
struct partsector
{
	U8	psPartCode[446];		// pad so struct is 512b
	U8	psPart[64];				// four partition records (64 bytes)
	U16 signature;				//2 byte signature of sector
#define BOOTSIG0        0x55
#define BOOTSIG1        0xaa
};

struct extboot {
	S8	exDriveNumber;				// drive number (0x80)//0x00 for floopy disk 0x80 for hard disk
	S8	exReserved1;				// reserved should always set 0
	S8	exBootSignature;			// ext. boot signature (0x29)
#define EXBOOTSIG       0x29
	S8	exVolumeID[4];				// volume ID number
	S8	exVolumeLabel[11];			// volume label "NO NAME"
	S8	exFileSysType[8];			// fs type (FAT12 or FAT)
};

struct bootsector50 {
	U8	bsJump[3];					// jump inst E9xxxx or EBxx90
	S8	bsOemName[8];				// OEM name and version
	S8	bsBPB[25];					// BIOS parameter block
	S8	bsExt[26];					// Bootsector Extension
	S8	bsBootCode[448];			// pad so structure is 512b
	U8	bsBootSectSig0;				// boot sector signature byte 0x55 
	U8	bsBootSectSig1;				// boot sector signature byte 0xAA
#define BOOTSIG0        0x55
#define BOOTSIG1        0xaa
};


struct bpb50 {
        U16	bpbBytesPerSec; // bytes per sector				//512 1024 2048 or 4096
        U8	bpbSecPerClust; // sectors per cluster			// power of 2
        U16	bpbResSectors;  // number of reserved sectors	//1 is recommend
        U8	bpbFATs;        // number of FATs				// 2 is recommend
        U16	bpbRootDirEnts; // number of root directory entries
        U16	bpbSectors;     // total number of sectors
        U8	bpbMedia;       // media descriptor				//0xf8 match the fat[0]
        U16	bpbFATsecs;     // number of sectors per FAT
        U16	bpbSecPerTrack; // sectors per track
        U16	bpbHeads;       // number of heads
        U32	bpbHiddenSecs;  // # of hidden sectors
        U32	bpbHugeSectors; // # of sectors if bpbSectors == 0
};

struct bootsector710 {
	U8	bsJump[3];					// jump inst E9xxxx or EBxx90
	S8	bsOemName[8];				// OEM name and version
	S8	bsBPB[53];					// BIOS parameter block
	S8	bsExt[26];					// Bootsector Extension
	S8	bsBootCode[418];			// pad so structure is 512b
	U8	bsBootSectSig2;				// boot sector signature byte 0x00 
	U8	bsBootSectSig3;				// boot sector signature byte 0x00
	U8	bsBootSectSig0;				// boot sector signature byte 0x55 
	U8	bsBootSectSig1;				// boot sector signature byte 0xAA
#define BOOTSIG0        0x55
#define BOOTSIG1        0xaa
#define BOOTSIG2        0x00
#define BOOTSIG3        0x00
};

struct bpb710 {
		U16	bpbBytesPerSec;	// bytes per sector
		U8	bpbSecPerClust;	// sectors per cluster
		U16	bpbResSectors;	// number of reserved sectors
		U8	bpbFATs;		// number of FATs
		U16	bpbRootDirEnts;	// number of root directory entries
		U16	bpbSectors;		// total number of sectors
		U8	bpbMedia;		// media descriptor
		U16	bpbFATsecs;		// number of sectors per FAT
		U16	bpbSecPerTrack;	// sectors per track
		U16	bpbHeads;		// number of heads
		U32	bpbHiddenSecs;	// # of hidden sectors
// 3.3 compat ends here
		U32	bpbHugeSectors;	// # of sectors if bpbSectors == 0
// 5.0 compat ends here
		U32     bpbBigFATsecs;// like bpbFATsecs for FAT32
		U16      bpbExtFlags;	// extended flags:
#define FATNUM    0xf			// mask for numbering active FAT
#define FATMIRROR 0x80			// FAT is mirrored (like it always was)
		U16      bpbFSVers;	// filesystem version
#define FSVERS    0				// currently only 0 is understood
		U32     bpbRootClust;	// start cluster for root directory
		U16      bpbFSInfo;	// filesystem info structure sector
		U16      bpbBackup;	// backup boot sector
		// There is a 12 byte filler here, but we ignore it
};



// Structure of a dos directory entry.
struct direntry {
		U8		deName[8];      	// filename, blank filled
#define SLOT_EMPTY      0x00            // slot has never been used
#define SLOT_E5         0x05            // the real value is 0xE5
#define SLOT_DELETED    0xE5            // file in this slot deleted
#define SLOT_DIR		0x2E			// a directorymmm
		U8		deExtension[3]; 	// extension, blank filled
		U8		deAttributes;   	// file attributes
#define ATTR_NORMAL     0x00            // normal file
#define ATTR_READONLY   0x01            // file is readonly
#define ATTR_HIDDEN     0x02            // file is hidden
#define ATTR_SYSTEM     0x04            // file is a system file
#define ATTR_VOLUME     0x08            // entry is a volume label
#define ATTR_LONG_FILENAME	0x0F		// this is a long filename entry			    
#define ATTR_DIRECTORY  0x10            // entry is a directory name
#define ATTR_ARCHIVE    0x20            // file is new or modified
		U8        deLowerCase;    	// NT VFAT lower case flags  (set to zero)
#define LCASE_BASE      0x08            // filename base in lower case
#define LCASE_EXT       0x10            // filename extension in lower case
		U8        deCHundredth;   	// hundredth of seconds in CTime
		U8        deCTime[2];     	// create time
		U8        deCDate[2];     	// create date
		U8        deADate[2];     	// access date
		U16        deHighClust; 		// high bytes of cluster number
		U8        deMTime[2];     	// last update time
		U8        deMDate[2];     	// last update date
		U16        deStartCluster; 	// starting cluster of file
		U32       deFileSize;  		// size of file in bytes
};


// number of directory entries in one sector
#define DIRENTRIES_PER_SECTOR	0x10	//when the bpbBytesPerSec=512 

// Structure of a Win95 long name directory entry
struct winentry {
		U8			weCnt;			// 
#define WIN_LAST        0x40
#define WIN_CNT         0x3f
		U8		wePart1[10];
		U8		weAttributes;
#define ATTR_WIN95      0x0f
		U8		weReserved1;
		U8		weChksum;
		U8		wePart2[12];
		U16       	weReserved2;
		U8		wePart3[4];
};

#define WIN_ENTRY_S8S	13      // Number of chars per winentry

// Maximum filename length in Win95
// Note: Must be < sizeof(dirent.d_name)
#define WIN_MAXLEN      255

// This is the format of the contents of the deTime field in the direntry
// structure.
// We don't use bitfields because we don't know how compilers for
// arbitrary machines will lay them out.
#define DT_2SECONDS_MASK        0x1F    // seconds divided by 2
#define DT_2SECONDS_SHIFT       0
#define DT_MINUTES_MASK         0x7E0   // minutes
#define DT_MINUTES_SHIFT        5
#define DT_HOURS_MASK           0xF800  // hours
#define DT_HOURS_SHIFT          11

// This is the format of the contents of the deDate field in the direntry
// structure.
#define DD_DAY_MASK				0x1F	// day of month
#define DD_DAY_SHIFT			0
#define DD_MONTH_MASK			0x1E0	// month
#define DD_MONTH_SHIFT			5
#define DD_YEAR_MASK			0xFE00	// year - 1980
#define DD_YEAR_SHIFT			9



// Stuctures
struct FileInfoStruct
{
	unsigned long StartCluster;			//< file starting cluster for last file accessed
	unsigned long Size;					//< file size for last file accessed
	unsigned char Attr;					//< file attr for last file accessed
	//unsigned short CreateTime;			//< file creation time for last file accessed
	//unsigned short CreateDate;			//< file creation date for last file accessed
	unsigned long Sector;				//<file record place
	unsigned int Offset;				//<file record offset
};



extern unsigned char FAT_Init();//初始化
extern unsigned long FAT_NextCluster(unsigned long cluster);//查找下一簇号
extern unsigned int FAT_FindItem(unsigned long cluster, U8 *name, struct FileInfoStruct *FileInfo);//查找文件
extern unsigned long FAT_OpenDir(U8 * dir);//打开目录


	extern U8 SearchInit(void);
	extern unsigned char Search(struct direntry *MusicInfo,U16 *Count,U8 *type);//查找音乐文件


extern unsigned char FAT_LoadPartCluster(unsigned long cluster,unsigned part,U8 * buffer);//加载文件

#endif
