// littleBits
// MP3 Player
//
// SD library
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

#ifndef __MMC_SD_h__
#define __MMC_SD_h__

#define U8  unsigned char
#define U16 unsigned int
#define U32 unsigned long


#define MMC_SD_PORT       PORTB                    //Òý½Å¶¨Òå
#define MMC_SD_CS_PIN     2     //mega8
#define DDR_INI() DDRB |= _BV(2)|_BV(3)|_BV(5)  //mega8
#define SD_Assert()   MMC_SD_PORT &= ~_BV(MMC_SD_CS_PIN)  
#define SD_Deassert() MMC_SD_PORT |=  _BV(MMC_SD_CS_PIN)


extern void SPI_Init(void);
extern U8 SD_Init(void);
extern U8 SD_ReadBlock(U32 sector, U8* buffer);
extern U32 SD_ReadCapacity();
extern void SPI_High_Speed(void);


#endif
