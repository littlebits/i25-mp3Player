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

#include"FAT.h"
#include <avr/wdt.h>

extern U8 trackOrder[MAX_TRACKS];
extern U8 trackOrderFlag;

U32 FirstDirClust;    //first directory cluster
U32 FirstDataSector;	// The first sector number of data
U16 BytesPerSector;	// Bytes per sector
U16 FATsectors;		// The amount sector a FAT occupied
U16 SectorsPerClust;	// Sector per cluster
U32 FirstFATSector;	// The first FAT sector
U32 FirstDirSector;	// The first Dir sector
U32 RootDirSectors;	// The sector number a Root dir occupied 
U32 RootDirCount;		// The count of directory in root dir
U8 FAT32_Enable;

U8 (* FAT_ReadSector)(U32,U8 *);
U8 (* FAT_WriteSector)(U32,U8 *);


// function pointer to the sd card read & write single block
// write sector are not use in this player
U8 (* FAT_ReadSector)(U32 sector, U8 * buffer)=SD_ReadBlock;//device read

struct FileInfoStruct FileInfo;//temporarily buffer for file information

unsigned char FAT_Init() //Initialize of FAT need initialize SD first
{
	struct bootsector710 *bs  = 0;
	struct bpb710        *bpb = 0;
	struct partsector    *ps  = 0;
	struct partrecord    *pr  = 0;

	U8 buffer[512];
	U32 hidsec=0;
	U32 Capacity;

	Capacity = SD_ReadCapacity();
	if(Capacity<0xff)return 1;


	if(FAT_ReadSector(0,buffer))return 1;
	bs = (struct bootsector710 *)buffer;

	hidsec = pr->prStartLBA;//the hidden sectors
	
		
		if(bs->bsJump[0]!=0xE9 && bs->bsJump[0]!=0xEB)
		{
			ps = (struct partsector *)buffer;
			if(ps->signature != 0xaa55)return 1;
			pr = (struct partrecord *)(ps->psPart);
			
			FAT_ReadSector(pr->prStartLBA,buffer);
			bs = (struct bootsector710 *)buffer;	
			
			if(bs->bsJump[0]!=0xE9 && bs->bsJump[0]!=0xEB) return 1;
		}

	if(bs->bsJump[0]!=0xE9 && bs->bsJump[0]!=0xEB)	//dead with the card which has no bootsect
	{
		ps = (struct partsector *)buffer;
		if(ps->signature != 0xaa55)return 1;
		pr = (struct partrecord *)(ps->psPart);
		FAT_ReadSector(pr->prStartLBA,buffer);
		bs = (struct bootsector710 *)buffer;
	}
	bpb = (struct bpb710 *)bs->bsBPB;

	
	if(bpb->bpbFATsecs)//detemine thd FAT type  //do not support FAT12
	{
		FAT32_Enable=0;	//FAT16
		FATsectors		= bpb->bpbFATsecs;//FAT占用的扇区数	//the sectors number occupied by one fat talbe
		FirstDirClust = 2;
	}
	else
	{
		FAT32_Enable=1;	//FAT32
		FATsectors		= bpb->bpbBigFATsecs; //the sectors number occupied by one fat talbe
		FirstDirClust = bpb->bpbRootClust;
	}

	BytesPerSector	= bpb->bpbBytesPerSec;
	SectorsPerClust	= (U8)bpb->bpbSecPerClust;
	FirstFATSector	= bpb->bpbResSectors+bpb->bpbHiddenSecs;
	RootDirCount	= bpb->bpbRootDirEnts;
	RootDirSectors	= (RootDirCount*32)>>9;
	FirstDirSector	= FirstFATSector+bpb->bpbFATs*FATsectors;
	FirstDataSector	= FirstDirSector+RootDirSectors;
	return 0;
}


//read one sector of one cluster, parameter part indicate which sector
unsigned char FAT_LoadPartCluster(unsigned long cluster,unsigned part,U8 * buffer)
{
	U32 sector;
	sector=FirstDataSector+(U32)(cluster-2)*(U32)SectorsPerClust;//calculate the actual sector number
	if(FAT_ReadSector(sector+part,buffer))return 1;
	else return 0;
}



//Return the cluster number of next cluster of file
//Suitable for system which has limited RAM
unsigned long FAT_NextCluster(unsigned long cluster)
{
	U8 buffer[512];
	U32 sector;
	U32 offset;
	if(FAT32_Enable)offset = cluster/128;
	else offset = cluster/256;
	if(cluster<2)return 0x0ffffff8;
	sector=FirstFATSector+offset;//calculate the actual sector
	if(FAT_ReadSector(sector,buffer))return 0x0ffffff8;//read fat table / return 0xfff8 when error occured

	if(FAT32_Enable)
	{
		offset=cluster%128;//find the position
		sector=((unsigned long *)buffer)[offset];	
	}
	else
	{
		offset=cluster%256;//find the position
		sector=((unsigned int *)buffer)[offset];
	}
	return (unsigned long)sector;//return the cluste number
}



//copy item
void CopyDirentruyItem(struct direntry *Desti,struct direntry *Source)
{
	U8 i;
	for(i=0;i<8;i++)Desti->deName[i] = Source->deName[i];
	for(i=0;i<3;i++)Desti->deExtension[i] = Source->deExtension[i];
	Desti->deAttributes = Source->deAttributes;
	Desti->deLowerCase = Source->deLowerCase;
	Desti->deCHundredth = Source->deCHundredth;
	for(i=0;i<2;i++)Desti->deCTime[i] = Source->deCTime[i];
	for(i=0;i<2;i++)Desti->deCDate[i] = Source->deCDate[i];
	for(i=0;i<2;i++)Desti->deADate[i] = Source->deADate[i];
	Desti->deHighClust = Source->deHighClust;
	for(i=0;i<2;i++)Desti->deMTime[i] = Source->deMTime[i];
	for(i=0;i<2;i++)Desti->deMDate[i] = Source->deMDate[i];
	Desti->deStartCluster = Source->deStartCluster;
	Desti->deFileSize = Source->deFileSize;
}



void WriteFolderCluster(U16 addr,U32 cluster)
{
	eeprom_write_byte(addr,cluster>>24);
	eeprom_write_byte(addr+1,cluster>>16);
	eeprom_write_byte(addr+2,cluster>>8);
	eeprom_write_byte(addr+3,cluster>>0);
}

U32 GetFolderCluster(U16 addr)
{
	U32 temp;
	temp = eeprom_read_byte(addr);
	temp <<= 8;
	temp += eeprom_read_byte(addr+1);
	temp <<= 8;
	temp += eeprom_read_byte(addr+2);
	temp <<= 8;
	temp += eeprom_read_byte(addr+3);

	return temp;
}

U8 SearchFolder(U32 cluster,U16 *addr)
{
	U8 *buffer;
	// U8 buff[3];
	U32 sector;
	// U32 cluster;
	U32 tempclust;
	unsigned char cnt;
	unsigned int offset;
	// unsigned int i=0;
	// unsigned char j;		//long name buffer offset;
	// unsigned char *p;	//long name buffer pointer
	struct direntry *item = 0;
	// struct winentry *we =0;
	
	// Handle root directory here:
	if(cluster==0 && FAT32_Enable==0)
	{
		//apply memory
		buffer=malloc(512);
		if(buffer==0)
			//if failed
			return 1;
			
		for(cnt=0;cnt<RootDirSectors;cnt++)
		{
			if(FAT_ReadSector(FirstDirSector+cnt,buffer))
			{
				free(buffer);
				return 1;
			}
			
			for(offset=0;offset<512;offset+=32)
			{
				// convert pointer
				item=(struct direntry *)(&buffer[offset]);
				
				// find a valid item, and write it to uC EEPROM
				// Check for the ATTR_HIDDEN flag to ignore folders that have been marked "hidden" by an OS
				if((item->deName[0] != '.') && (item->deName[0] != 0x00) && (item->deName[0] != 0xe5) && !(item->deAttributes & ATTR_HIDDEN))
				{
					if(item->deAttributes & ATTR_DIRECTORY)
					{
						if(*addr==RECORD_ADDR_END)
							return 0;
						else
						{
							WriteFolderCluster(*addr,item->deStartCluster+(((unsigned long)item->deHighClust)<<16));
							*addr+=4;
						}
					}
					
				} // end if((item->deName[0] != '.') && (item->deName[0] != 0x00) && (item->deName[0] != 0xe5))
				
			} // end for(offset=0;offset<512;offset+=32)
			
		} // end for(cnt=0;cnt<RootDirSectors;cnt++)
		
		// release mem buffer
		free(buffer);
	}
	else// else it's not root directory, so handle other folders with a cluster offset:
	{
		tempclust=cluster;
		while(1)
		{
			//calculate the actual sector number
			sector=FirstDataSector+(U32)(tempclust-2)*(U32)SectorsPerClust;
			
			// apply memory
			buffer=malloc(512);
			if(buffer==0)
				//if failed
				return 1;
				
				
			for(cnt=0;cnt<SectorsPerClust;cnt++)
			{
				if(FAT_ReadSector(sector+cnt,buffer))
				{
					free(buffer);
					return 1;
				}
				
				for(offset=0;offset<512;offset+=32)
				{
					item=(struct direntry *)(&buffer[offset]);
					
					// find a valid item, and write it to uC EEPROM
					// Check for the ATTR_HIDDEN flag to ignore folders that have been marked "hidden" by an OS
					if((item->deName[0] != '.') && (item->deName[0] != 0x00) && (item->deName[0] != 0xe5) && !(item->deAttributes & ATTR_HIDDEN))
					{				
						if(item->deAttributes & ATTR_DIRECTORY )
						{
							if(*addr==RECORD_ADDR_END)
								return 0;
							else
							{
								WriteFolderCluster(*addr,item->deStartCluster+(((unsigned long)item->deHighClust)<<16));
								*addr+=4;
							}	
						}
			
					} // end if((item->deName[0] != '.') && (item->deName[0] != 0x00) && (item->deName[0] != 0xe5))
			
				} // end for(offset=0;offset<512;offset+=32)
			
			} // end for(cnt=0;cnt<SectorsPerClust;cnt++)
			
			// release mem buffer
			free(buffer);
			
			//next cluster
			tempclust=FAT_NextCluster(tempclust);
			
			if(tempclust == 0x0fffffff || tempclust == 0x0ffffff8 || (FAT32_Enable == 0 && tempclust == 0xffff))
				break;
				
		} // end while(1)
	}
	
	return 0;	
		
} // end SearchFolder(U32 cluster,U16 *addr)



U8 SearchInit()
{	
	U16 addr = RECORD_ADDR_START;
	U16 temp_addr;
	U32 cluster;
	
	if(FAT32_Enable)
		WriteFolderCluster(addr,FirstDirClust);
	else 
		WriteFolderCluster(RECORD_ADDR_START,0);
	
	addr += 4;
	
	WriteFolderCluster(addr, 0xffffffff);
	temp_addr = addr;
	addr = RECORD_ADDR_START;
	while(1)
	{
		
		wdt_reset();
		cluster = GetFolderCluster(addr);
		if(cluster == 0xffffffff)
			return 0;
		else
		{
			SearchFolder(cluster,&temp_addr);
			
			if(GetFolderCluster(temp_addr) != 0xffffffff)
				WriteFolderCluster(temp_addr,0XFFFFFFFF);
				
			if(temp_addr == RECORD_ADDR_END)
			{
				WriteFolderCluster(temp_addr - 4,0XFFFFFFFF);
				break;
			}
		}
		addr+=4;
		
	} // end while(1)
	return 0;
} // end SearchInit()


// Search the file, when *count = 0 it will bring the number whole TrackNumber, when *count != 0 the *MusicInfo will bring the infomation of the file
U8 Search(struct direntry *MusicInfo,U16 *Count,U8 *type)
{
	U8 trackNumIndex = 0;
	U16 trackNum = 0;
	U8 *buffer;
	U32 sector;
	U32 cluster;
	U32 tempclust;
	unsigned char cnt;
	unsigned int offset;
	unsigned int i=0;
	//unsigned char j;//long name buffer offset;
	//unsigned char *p;//long name buffer pointer
	struct direntry *item = 0;
	//struct winentry *we =0;
	//cluster = FAT_OpenDir(dir);
	//if(cluster == 1)return 1;
	
	U16 addr = RECORD_ADDR_START;
	
	while(1)
	{
		
		wdt_reset();
		cluster = GetFolderCluster(addr);
		addr += 4;
		if(cluster == 0xffffffff)
			break;
		else
		{
			// music_record_addr = addr - 4;	/* record in which record found the right file */
			// If we're in a root directory:
			if(cluster==0 && FAT32_Enable==0)
			{
				// apply memory
				buffer=malloc(512);
				if(buffer==0)
					// if failed
					return 1;
				
				for(cnt=0;cnt<RootDirSectors;cnt++)
				{
					if(FAT_ReadSector(FirstDirSector+cnt,buffer))
					{
						free(buffer);
						return 1;
					}
					
					for(offset=0;offset<512;offset+=32)
					{
						// Convert pointer
						item=(struct direntry *)(&buffer[offset]);
						
						// Now find a valid dos directory entry, and copy into our MusicInfo struct (type direntry), which we later play out of...
						// Note that we check for the ATTR_HIDDEN flag... if it's set high, then we skip the directory entry (because it's been marked as a hidden file by the OS)
						if((item->deName[0] != '.') && (item->deName[0] != 0x00) && (item->deName[0] != 0xe5) && (item->deAttributes != 0x0f) && !(item->deAttributes & ATTR_HIDDEN))
						{
							if((item->deExtension[0] == 'M')&&(item->deExtension[1] == 'P')&&(item->deExtension[2] == '3'))
							{
								CopyDirentruyItem(MusicInfo,item);
								*type=1;
								
								if (!trackOrderFlag)
								{
									trackNumIndex = 0;
																	
									while ( (item->deName[trackNumIndex] == 48) && (trackNumIndex < 8) )
									{
										trackNumIndex++; //ignore leading 0s in name
									}
																	
									trackNum = 0;
									if ( (item->deName[trackNumIndex] >= 48) && (item->deName[trackNumIndex] <= 57)  && (trackNumIndex < 8) )
									{
										while ( (item->deName[trackNumIndex] >= 48) && (item->deName[trackNumIndex] <= 57) && (trackNum < 250) && (trackNumIndex < 8) )
										{
											trackNum *= 10;
											trackNum += (item->deName[trackNumIndex] - 48);
											if (trackNum >= 250) trackNum = 250;
											trackNumIndex++;
										}
									}
									else
									{
										trackNum = 250;
									}
																	
									if ( trackNumIndex >= 8 ) trackNum = 250; //We reached the end without determining the number so 250
									trackOrder[i] = trackNum;
								}
								
								i++;
								if(i==*Count){free(buffer);return 0;}
							}
							//else if((item->deExtension[0] == 'W')&&(item->deExtension[1] == 'M')&&(item->deExtension[2] == 'A'))
							//{
							//	CopyDirentruyItem(MusicInfo,item);
							//	*type=2;
							//	i++;
							//	if(i==*Count){free(buffer);return 0;}
							//}
							//else if((item->deExtension[0] == 'M')&&(item->deExtension[1] == 'I')&&(item->deExtension[2] == 'D'))
							//{
							//	CopyDirentruyItem(MusicInfo,item);
							//	*type=3;
							//	i++;
							//	if(i==*Count){free(buffer);return 0;}
							//}
							//else if((item->deExtension[0] == 'W')&&(item->deExtension[1] == 'A')&&(item->deExtension[2] == 'V'))
							//{
							//	CopyDirentruyItem(MusicInfo,item);
							//	*type=4;
							//	i++;
							//	if(i==*Count){free(buffer);return 0;}
							//}
							
						} // end if((item->deName[0] != '.') && (item->deName[0] != 0x00) && (item->deName[0] != 0xe5) && (item->deAttributes != 0x0f))
						
					} // end for(offset=0;offset<512;offset+=32)
					
				} // end for(cnt=0;cnt<RootDirSectors;cnt++) 
				
				// release our mem buffer
				free(buffer);
				
			}
			else // else not in root directory, so all other (non-root) directories:
			{
				tempclust=cluster;
				while(1)
				{
					//calculate the actual sector number
					sector=FirstDataSector+(U32)(tempclust-2)*(U32)SectorsPerClust;

					//apply memory
					buffer=malloc(512);
					if(buffer==0)
						//if failed
						return 1;	
					
					for(cnt=0;cnt<SectorsPerClust;cnt++)
					{
						if(FAT_ReadSector(sector+cnt,buffer))
						{
							free(buffer);
							return 1;
						}
						
						for(offset=0;offset<512;offset+=32)
						{
							item=(struct direntry *)(&buffer[offset]);
							
							// Now find a valid dos directory entry, and copy into our MusicInfo struct (type direntry), which we later play out of...
							// Note that we check for the ATTR_HIDDEN flag... if it's set high, then we skip the directory entry (because it's been marked as a hidden file by the OS)
							if((item->deName[0] != '.') && (item->deName[0] != 0x00) && (item->deName[0] != 0xe5) && (item->deAttributes != 0x0f) && !(item->deAttributes & ATTR_HIDDEN))
							{				
								if((item->deExtension[0] == 'M')&&(item->deExtension[1] == 'P')&&(item->deExtension[2] == '3'))
								{
									CopyDirentruyItem(MusicInfo,item);
									if (!trackOrderFlag)
									{
										trackNumIndex = 0;
									
										while ( (item->deName[trackNumIndex] == 48) && (trackNumIndex < 8) ) 
										{
											trackNumIndex++; //ignore leading 0s in name
										}
									
										trackNum = 0;
										if ( (item->deName[trackNumIndex] >= 48) && (item->deName[trackNumIndex] <= 57)  && (trackNumIndex < 8) )
										{
											while ( (item->deName[trackNumIndex] >= 48) && (item->deName[trackNumIndex] <= 57) && (trackNum < 250) && (trackNumIndex < 8) )
											{
													trackNum *= 10;
													trackNum += (item->deName[trackNumIndex] - 48);
													if (trackNum >= 250) trackNum = 250;
													trackNumIndex++;
											}
										}
										else
										{
											trackNum = 250;
										}
									
										if ( trackNumIndex >= 8 ) trackNum = 250; //We reached the end without determining the number so 250
										trackOrder[i] = trackNum;
									}	
									
									*type = 1;
									i++;
									if(i==*Count){free(buffer);return 0;}
								}
								//else if((item->deExtension[0] == 'W')&&(item->deExtension[1] == 'M')&&(item->deExtension[2] == 'A'))
								//{
								//	CopyDirentruyItem(MusicInfo,item);
								//	*type = 2;
								//	i++;
								//	if(i==*Count){free(buffer);return 0;}	
								//}
								//else if((item->deExtension[0] == 'M')&&(item->deExtension[1] == 'I')&&(item->deExtension[2] == 'D'))
								//{
								//	CopyDirentruyItem(MusicInfo,item);
								//	*type = 3;
								//	i++;
								//	if(i==*Count){free(buffer);return 0;}
								//}
								//else if((item->deExtension[0] == 'W')&&(item->deExtension[1] == 'A')&&(item->deExtension[2] == 'V'))
								//{
								//	CopyDirentruyItem(MusicInfo,item);
								//	*type=4;
								//	i++;
								//	if(i==*Count){free(buffer);return 0;}
								//}
							} // end if((item->deName[0] != '.') && (item->deName[0] != 0x00) && (item->deName[0] != 0xe5) && (item->deAttributes != 0x0f))
							
						} // end for(offset=0;offset<512;offset+=32)
					
					} // end for(cnt=0;cnt<SectorsPerClust;cnt++)
					
					// release mem buffer
					free(buffer);
					
					//next cluster
					tempclust = FAT_NextCluster(tempclust);
					
					if(tempclust == 0x0fffffff || tempclust == 0x0ffffff8 || (FAT32_Enable == 0 && tempclust == 0xffff))
						break;
						
				} // end while(1)
				
			} // end else //other folders
			
		} // end else
		
	} // end while(1)
	
	if(*Count==0)
		*Count=i;
	return 0;
	
} // end Search(struct direntry *MusicInfo,U16 *Count,U8 *type)
