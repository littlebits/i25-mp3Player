// littleBits
// MP3 Player
//
// VS1053 library
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

#ifndef __VS1053B_H__
#define __VS1053B_H__

#define U8  unsigned char
#define U16 unsigned int
#define U32 unsigned long

#define CLOCK_REG			0xc000          //0xc00 is for VS1053 or higher version

#define VS1053B_PORT		PORTC
#define VS1053B_DDR			DDRC
#define VS1053B_PIN			PINC

#define SPI_WRITE			0x02
#define SPI_READ			0x03

#define REG_MODE_CONTROL	0x00
#define REG_VOLUME			0x0b
#define REG_BASS_TREBLE		0x02

#define SET_NATIVE_SPI		0x0800
#define HEADPHONE_VOLUME_LEVEL		0x2a2a // -21.0 dB
#define SPEAKER_VOLUME_LEVEL		0x1212 // -9.0 dB
#define BASS_TREBLE			0x8000 // bass and treble off

#define VS_XRESET			0
#define VS1053B_DREQ		2
#define VS_XDCS				1
#define VS_XCS				3

#define PORT_INI()			VS1053B_DDR |= 1<<VS_XCS | 1<<VS_XRESET | 1<<VS_XDCS

#define VS_X_Deassert()		VS1053B_PORT |=  _BV(VS_XCS)
#define VS_X_Assert()		VS1053B_PORT &= ~_BV(VS_XCS)

#define VS_RESET_H()		VS1053B_PORT |=  _BV(VS_XRESET)
#define VS_RESET_L()		VS1053B_PORT &= ~_BV(VS_XRESET)

#define VS_XD_Deassert()	VS1053B_PORT |=  _BV(VS_XDCS)
#define VS_XD_Assert()		VS1053B_PORT &= ~_BV(VS_XDCS)

extern void VS_WriteCMD(unsigned char addr, unsigned int dat);
extern unsigned int VS_ReadCMD(unsigned char addr);
extern void VS1053B_WriteDAT(unsigned char dat);
extern unsigned char VS1053B_Init();
extern void VS1053B_SoftReset();
extern void VS_StartSineTest(unsigned char index, unsigned char skipSpeed);

extern void VS_WriteSetting(U8 reg, U16 value);
extern U16 VS_SetVolume(U16 volume);
extern void VS_SetBassTreble(U16 basstreble);
extern void VS_SetSCIMode(U16 mode);
#endif
