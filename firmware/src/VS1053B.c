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

#include <avr/io.h>
#include <avr/wdt.h>
#include "VS1053B.h"
#include "MMC_SD.h"
#include <util/delay.h>

//Write to VS Chip
void VS_WriteCMD(U8 addr, U16 dat)
{
	VS_XD_Deassert();
	VS_X_Assert();
	SPI_Send(SPI_WRITE);
	SPI_Send(addr);
	SPI_Send(dat>>8);
	SPI_Send(dat);
	VS_X_Deassert();
}

//read register
U16 VS_ReadCMD(U8 addr)
{
	U16 temp;
	VS_XD_Deassert();
	VS_X_Assert();
	SPI_Send(SPI_READ);
	SPI_Send(addr);
	temp = SPI_Receive();
	temp <<= 8;
	temp += SPI_Receive();
	VS_X_Deassert();
	return temp;
}

//write data (music data)
void VS1053B_WriteDAT(U8 dat)
{
	VS_XD_Assert();
	SPI_Send(dat);
	VS_XD_Deassert();
	VS_X_Deassert();
}

// Initialize the IC
U8 VS1053B_Init()
{
	wdt_reset();
	U8 retry = 0;
	PORT_INI();
	VS1053B_DDR &= ~_BV(VS1053B_DREQ);
	VS_X_Deassert();
	VS_XD_Deassert();
	//VS_RESET_L();
	//_delay_ms(20);
	VS_RESET_H();		//chip select
	SPI_Low_Speed();	//low speed
	_delay_ms(20);		//delay

	wdt_reset();
	while(VS_ReadCMD(0x03) != CLOCK_REG)	//set PLL register
	{
		VS_WriteCMD(0x03, CLOCK_REG);
		if(retry++ >10 )return 1;
	}

	_delay_ms(20);
	
	VS_WriteCMD(0x05, 0x000a);

	VS_WriteCMD(0x05, 0xac45);

	VS_SetVolume(HEADPHONE_VOLUME_LEVEL);
	VS_SetBassTreble(BASS_TREBLE);
	VS_SetSCIMode(SET_NATIVE_SPI);
	wdt_reset();
	_delay_ms(20);

	VS_WriteCMD(REG_MODE_CONTROL, 0x0804); //reset
	_delay_ms(40);
	wdt_reset();
	SPI_High_Speed();
	return 0;
}

//VS1053 soft reset
void VS1053B_SoftReset()
{
	VS_WriteCMD(REG_MODE_CONTROL, 0x0804); //reset
	_delay_ms(20);
}

void VS_StartSineTest(unsigned char index, unsigned char skipSpeed)
{
	uint8_t tmp = 0;
	
	VS_WriteCMD(0x00, 0x0820); //allow SDI tests
	
	VS1053B_WriteDAT(0x53);
	VS1053B_WriteDAT(0xef);
	VS1053B_WriteDAT(0x6e);
	
	// sine wave test set
	tmp = (index << 5) | skipSpeed;
	VS1053B_WriteDAT(tmp);
	
	VS1053B_WriteDAT(0x00);
	VS1053B_WriteDAT(0x00);
}

void VS_WriteSetting(U8 reg, U16 value)
{
	U8 retry = 0;
	while(VS_ReadCMD(reg) != value) //set value
	{
		VS_WriteCMD(reg, value);
		if(retry++ > 10) return;
	}
}

U16 VS_SetVolume(U16 volume)
{
	VS_WriteSetting(REG_VOLUME, volume);
	return volume;
}

void VS_SetBassTreble(U16 bass_treble)
{
	VS_WriteSetting(REG_BASS_TREBLE, bass_treble);
}

void VS_SetSCIMode(U16 mode)
{
	VS_WriteSetting(REG_MODE_CONTROL, mode);
}