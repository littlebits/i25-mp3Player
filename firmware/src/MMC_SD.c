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




#include <avr/io.h>
#include "MMC_SD.h"


volatile U8 SDHC_flag, cardType;

//spi low speed for initialization
void SPI_Low_Speed(void)
{
	SPCR =   _BV(SPE)|_BV(MSTR)|_BV(SPR1)|_BV(SPR0);
	SPSR &= ~_BV(SPI2X);
}

//spi full speed
void SPI_High_Speed(void)
{
	SPCR =  _BV(SPE)|_BV(MSTR);
	SPSR |= _BV(SPI2X);
}

//port initialize


//send an SPI byte
U8 SPI_Send(U8 val)
{
	SPDR = val;
	while(!(SPSR & _BV(SPIF)));
	return SPDR;
}

//receive an SPI byte
U8 SPI_Receive(void)
{
	SPDR = 0xff;
	while(!(SPSR & _BV(SPIF)));
	return SPDR;
}

void SPI_Init(void)
{
	DDR_INI();
	SPI_Low_Speed();
	SD_Deassert();
}

//sd send command
U8 SD_Send_CMD(U8 cmd, U32 arg)
{
	U8 r1, status;
	U8 retry=0;
	
	SPI_Receive();
	SD_Assert();
	
	SPI_Send(cmd | 0x40); //send command
	SPI_Send(arg>>24);
	SPI_Send(arg>>16);
	SPI_Send(arg>>8);
	SPI_Send(arg);
	
	if (cmd == 8)
		SPI_Send(0x87);
	else
		SPI_Send(0x95);
	
	while((r1 = SPI_Receive()) == 0xff) //wait response
		if(retry++ > 0xfe) break; //time out error

	if(r1 == 0x00 && cmd == 58)  //checking response of CMD58
	{
		status = SPI_Receive() & 0x40;     //first byte of the OCR register (bit 31:24)
		if(status == 0x40) SDHC_flag = 1;  //we need it to verify SDHC card
		else SDHC_flag = 0;

		SPI_Receive(); //remaining 3 bytes of the OCR register are ignored here
		SPI_Receive(); //one can use these bytes to check power supply limits of SD
		SPI_Receive();
	}

	SD_Deassert();
	SPI_Receive(); // extra 8 CLK

	return r1; //return state
}


U8 SD_Init(void)
{
	U8 i, SD_version;
	U8 retry;
	U8 r1=0;
	retry = 0;
	SPI_Low_Speed();
	do
	{
		for(i=0;i<10;i++) SPI_Receive();
		r1 = SD_Send_CMD(0, 0); //send idle command
		retry++;
		if(retry>0xfe) return 1; //time out
	} while(r1 != 0x01);	

	SPI_Receive();
	SPI_Receive();

	SD_version = 2;

	retry = 0;
	
	//TODO: Place SD init guide website link for future readers
	
	do
	{
		r1 = SD_Send_CMD(8, 0x1aa); //send active command
		retry++;
		if(retry>0xf0)
		{
			SD_version = 1;
			cardType = 1;
			break;
		} //time out
	} while(r1 != 0x01);
	
	
	retry = 0;
	do
	{
		r1 = SD_Send_CMD(55,0);
		r1 = SD_Send_CMD(41, 0x40000000); //send active command
		retry++;
		if(retry>0xf0)
		{
			return 1;
		} //time out
	} while(r1 != 0x00);
	
	retry = 0;
	SDHC_flag = 0;
	
	if (SD_version == 2)
	{
		do
		{
			r1 = SD_Send_CMD(58,0);
			retry++;
			if(retry>0xfe)
			{
				cardType = 0;
				break;
			}
		}while(r1 != 0x00);

		if(SDHC_flag == 1) cardType = 2;
		else cardType = 3;
	}
	
	SPI_High_Speed();
	
	
	r1 = SD_Send_CMD(59, 0); //disable CRC
	r1 = SD_Send_CMD(16, 512); //set sector size to 512
	
	return 0; //normal return
}

//read one sector
U8 SD_ReadBlock(U32 sector, U8* buffer)
{
	U8 r1;
	U16 i;
	U16 retry=0;

	if (SDHC_flag == 0)	//SD standard is usually FAT16 so read direct address
		r1 = SD_Send_CMD(17, sector<<9);
	else				//SDHC has to be format FAT32 so read the sector instead
		r1 = SD_Send_CMD(17, sector);
		
	if(r1 != 0x00)
		return r1;

	SD_Assert();
	
	//wait for data
	while(SPI_Receive() != 0xfe)if(retry++ > 0xfffe){SD_Deassert();return 1;}

	for(i=0; i<512; i++) //read the sector
	{
		*buffer++ = SPI_Receive();
	}

	SPI_Receive();
	SPI_Receive();
	
	SD_Deassert();
	SPI_Receive(); //SPI stall

	return 0;
}


U32 SD_ReadCapacity()
{
	U8 r1;
	U16 i;
	U16 temp;
	U8 buffer[16];
	U32 Capacity;
	U16 retry =0;

	r1 = SD_Send_CMD(9, 0);	//send command  //READ CSD
	if(r1 != 0x00)
		return r1;

	SD_Assert();
	while(SPI_Receive() != 0xfe)if(retry++ > 0xfffe){SD_Deassert();return 1;}

	
	for(i=0;i<16;i++)
	{
		buffer[i]=SPI_Receive();
	}	

	SPI_Receive();
	SPI_Receive();
	SPI_Receive();
	
	SD_Deassert();

	SPI_Receive(); //Extra 8 to assure clear of buffers

/*********************************/
//	C_SIZE
	i = buffer[6]&0x03;
	i<<=8;
	i += buffer[7];
	i<<=2;
	i += ((buffer[8]&0xc0)>>6);

/**********************************/
//  C_SIZE_MULT

	r1 = buffer[9]&0x03;
	r1<<=1;
	r1 += ((buffer[10]&0x80)>>7);


/**********************************/
// BLOCKNR

	r1+=2;

	temp = 1;
	while(r1)
	{
		temp*=2;
		r1--;
	}
	
	Capacity = ((U32)(i+1))*((U32)temp);

/////////////////////////
// READ_BL_LEN

	i = buffer[5]&0x0f;

/*************************/
//BLOCK_LEN

	temp = 1;
	while(i)
	{
		temp*=2;
		i--;
	}
/************************/


/************** formula of the capacity ******************/
//
//  memory capacity = BLOCKNR * BLOCK_LEN
//	
//	BLOCKNR = (C_SIZE + 1)* MULT
//
//           C_SIZE_MULT+2
//	MULT = 2
//
//               READ_BL_LEN
//	BLOCK_LEN = 2
/**********************************************/

//The final result
	
	Capacity *= (U32)temp;	 
	return Capacity;		
}