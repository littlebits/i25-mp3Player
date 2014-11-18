// littleBits
// MP3 player
//
// main file
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
#define F_CPU 8000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include "MMC_SD.h"			//head files
#include "FAT.h"
#include "VS1053B.h"

// Define our data types
#define U8 unsigned char
#define U16 unsigned int
#define U32 unsigned long

// Define how long a high signal for a latch
#define HOW_LONG_TO_CONSIDER_A_LOGIC_HIGH_INPUT_TO_BE_INTERPRETED_AS_A_LATCH_IN_MILLISECONDS 700

// Define our input buttons
#define PUSH_PLAY	(PIND & 0b00100000) >> 5	// PLAY/PAUSE BUTTON (SIG IN) set to PD5
#define PLAY_MODE	( (PIND & 0b01000000) >> 4 ) | ( (PIND & 0b00011000) >> 3) 	// THREE MODE SWITCH set to PD4/PD3
#define PREV		(PIND & 0b00000010) >> 1	// PREV FILE TACT BUTTON set to PD1
#define NEXT		(PIND & 0b00000100) >> 2	// NEXT FILE TACT BUTTON set to PD2

#define RESET_PIN	(0b00000001) //defines the RESET_PIN register space.

#define MP3 1
#define WMA 2
#define MID 3

//mode
#define ONCE_MODE	7
#define LOOP_MODE	3
#define NEXT_MODE	5
#define ALL_MODE	6


extern U16 SectorsPerClust;
extern U16 FirstDataSector;  //struct of file information
extern U8  FAT32_Enable;

struct FileInfoStruct FileInfo;

struct direntry MusicInfo;	//file to play

volatile unsigned int P = 0;

U8 trackOrderFlag = 0; //If this is set we won't try to order the tracks again.
U16 trackToPlay = 0; //this is to solve pointer issue.
U8 buttonPush = 0;
U16 TotalTracks;				//number of TrackNumber on cards
U8 type;					//file type
U8 SDHC_flag = 0;			//set if SD card is type HC
volatile U8 trackOrder[MAX_TRACKS];				//store the track play state; 1 = played
U8 maxNumTrack = 0;
U8 lastDataByte = 0;
U8 trackEnded = 0;
U16 testlooper = 0;
U8 flag;	//flag of pause
U8 toDREQ = 0;
U8 resetFlag = 0;
U8 resample = 0;
U8 initpush = 1;
U16 count; //data counting
U8 i; //loop variable
U16 j; //loop variable
U32 p; //cluster
U32 totalsect; //total sectors in file
U16 leftbytes; //total bytes left
U8 *buffer; //buffer
U32 sector; //record the current sector to judge the end sector
U16 TrackNumber=1;	//play the fist TrackNumber by default
U8 mode = 0;
U8 bPlay; //monitor for changes on the play/pause pint
U16 VolumeLevel = HEADPHONE_VOLUME_LEVEL;
U16 Q = 0; //Stall after dB change for no accidental track changes
U8 VolumeChanged = 0;
U8 ChangeOK = 0;
U8 PrevOK = 0;
U8 NextOK = 0;
U8 NextPushed = 0;
U8 PrevPushed = 0;
U8 volumeWait = 0;
U16 latchCount = 0xFFFF;
U8 isLatched = 1;
U8 loopOrAllInitialState = 0;
U8 latchDirection = 0;
U16 latchCompare = 0;
	
int PlayTrack();
void AudioCallback();
void ReorderTracks();
void initTimerISR();

void ReorderTracks()
{
	U8 *buffer;
	volatile U8 done = 0;
	volatile U8 maxtrack = 0;
	volatile U8 trackIndex = 0;
	volatile U8 trackcount = 0;
	buffer = malloc(MAX_TRACKS);
	
	for (i = 0; i < MAX_TRACKS; i++) //find the highest numbered track so we know we can stop at some point
	{
		if (trackOrder[i] < 250)
		{
			if (trackOrder[i] > maxtrack) maxtrack = trackOrder[i];
		}
	}
	trackcount = 1;
	done = 0;
	while (!done) //find and order all numbered tracks
	{
		
		wdt_reset();
		for (i = 0; i < MAX_TRACKS; i++)
		{
			if (trackOrder[i] == trackcount)
			{
				buffer[trackIndex] = i;
				trackIndex++;
			}
		}
		trackcount++;
		if (trackcount > maxtrack)
		{
			done = 1;
		}
	}
	
	//Now find all of the non-numbered tracks
	for (i = 0; i < MAX_TRACKS; i++)
	{
		if (trackOrder[i] == 250)
		{
			buffer[trackIndex] = i;
			trackIndex++;
		}
	}
	for (i = 0; i <trackIndex; i++)
	{
		trackOrder[i] = buffer[i] + 1;
	}
	
	free(buffer);
}



// Music is played from the tracks discovered on the SD Card
void AudioCallback()
{
		count=0; //clear count
		if (initpush == 1) 
		{
			initpush = 0;
			flag = 0;
		}
		else if ( trackEnded && (mode == NEXT_MODE) ) flag = 0;
		else if (mode == ONCE_MODE && resample == 0) flag = 0;
		else if (mode == ALL_MODE && flag == 0) flag = 0;
		else flag=1;
	
		trackEnded = 0;
		resample = 0;
		
		wdt_reset();
		PlayTrack(); //Enter the track until a change of track is requested.
}

int PlayTrack()
{
	while(count<2048)  //recommend 2048 zeros honoring DREQ before soft reset
	{
		if((VS1053B_PIN & 1<<VS1053B_DREQ)!=0)
		{
			for(j=0;j<32;j++)
			{
				VS1053B_WriteDAT(0x00); //fill0
				count++;
			}
			if(count == 2047)break;
		}
	}
	wdt_reset();
	
	VS1053B_SoftReset(); 
	trackToPlay = trackOrder[TrackNumber-1];
	Search(&MusicInfo,&trackToPlay,&type); //find the file
	
	p = MusicInfo.deStartCluster+(((U32)MusicInfo.deHighClust)<<16);	//the first cluster of the file
	
	totalsect = MusicInfo.deFileSize / 512; //calculate the total sectors
	leftbytes = MusicInfo.deFileSize % 512; //calculate the bytes left
	i=0;
	sector=0;
	
	while(1)
	{
		
		//mode = PLAY_MODE; //modes
		for(; i<SectorsPerClust; i++)	//a cluster
		{
			
			wdt_reset();
			buffer=malloc(512);
			FAT_LoadPartCluster(p,i,buffer); //read a sector
			count=0;
			while(count<512)
			{
				wdt_reset();
				if (resetFlag == 1)
				{
					resetFlag = 0;
					if(VS1053B_Init()) //initialize vs1053
					return 0;
				}	
				
				if((VS1053B_PIN & _BV(VS1053B_DREQ))!=0 && flag) //send data DREQ
				{
					
					wdt_reset();
					toDREQ = 0;
					for(j=0;j<32;j++) //32 Bytes each time
					{
						VS1053B_WriteDAT(buffer[count]);
						count++;
					}
					if(sector == totalsect && count >= leftbytes) //if this is the end of the file
					{
						i=SectorsPerClust;
						break;
					} //end of file
					if(count == 511 && flag)
					{
						break;
					} //break if a sector was sent
					if(bPlay ^ (PUSH_PLAY) && buttonPush == 0)	//signal play/pause, pulse or switch
					{
						if (mode == ONCE_MODE)
						{
							if (!bPlay)
							{
								resample = 1;
								free(buffer);
								return 0;
							}
						}
					}
				}
				
				if(bPlay ^ (PUSH_PLAY) && buttonPush == 0 )	//signal play/pause, pulse or switch
				{
					if (mode == NEXT_MODE)
					{	
						if (!bPlay)
						{
							buttonPush = 1;
							P = 0;
							//bPlay = (PUSH_PLAY);
							TrackNumber++; //Goto next song
							if(TrackNumber > TotalTracks) TrackNumber=1; //If it is the last song then go to the first song
							free(buffer);
							buttonPush = 1;
							P = 0;
							return 0;
						}
					}
					else if (mode == LOOP_MODE)
					{
						wdt_reset();
						bPlay = (PUSH_PLAY);
						if ( (!isLatched) && (bPlay != latchDirection) ) //When there is no perceived latch state
						{
							buttonPush = 1;
							P = 0;
							latchCount = 0; //Starts the latch timer
							if(flag) flag = 0; //if we are playing then pause
							else flag=1; //if we are paused then play
							loopOrAllInitialState = bPlay; //set out previous state
						}
						else if ( isLatched )
						{
							buttonPush = 1;
							P = 0;
							isLatched = 0;
							latchCount = 0;
							//latchDirection = bPlay;
							if(flag) flag = 0; //if we are playing then pause
							else flag=1; //if we are paused then play
							loopOrAllInitialState = bPlay; //set out previous state
						}
						else
						{
							buttonPush = 1;
							P = 0;
							latchCount = 0;
							loopOrAllInitialState = bPlay;
						}
					}
					else if (mode == ONCE_MODE)
					{
						if (!bPlay)
						{
							flag = 1;
							resample = 1;
							free(buffer);
							return 0;
						}
						//bPlay = (PUSH_PLAY);
					}
					else if (mode == ALL_MODE)
					{
						wdt_reset();
						bPlay = (PUSH_PLAY);
						if ( (!isLatched) && (bPlay != latchDirection) ) //When there is no perceived latch state
						{
							buttonPush = 1;
							P = 0;
							latchCount = 0; //Starts the latch timer
							if(flag) flag = 0; //if we are playing then pause
							else flag=1; //if we are paused then play
							loopOrAllInitialState = bPlay; //set out previous state
						}
						else if ( isLatched )
						{
							buttonPush = 1;
							P = 0;
							isLatched = 0;
							latchCount = 0;
							//latchDirection = bPlay;
							if(flag) flag = 0; //if we are playing then pause
							else flag=1; //if we are paused then play
							loopOrAllInitialState = bPlay; //set out previous state
						}
						else
						{
							buttonPush = 1;
							P = 0;
							latchCount = 0;
							loopOrAllInitialState = bPlay;
						}
					}
				}
				if (!(PREV) && !(NEXT)  && VolumeChanged == 0)
				{
						if (VolumeLevel == HEADPHONE_VOLUME_LEVEL)
						VolumeLevel = VS_SetVolume(SPEAKER_VOLUME_LEVEL);
						else VolumeLevel = VS_SetVolume(HEADPHONE_VOLUME_LEVEL);
						Q = 0;
						volumeWait = 0;
						VolumeChanged = 1;
				}
				else if ( !(PREV) ) 
				{
					Q = 0;
					PrevPushed = 1;
				}
				else if ( !(NEXT) ) 
				{
					Q = 0;
					NextPushed = 1;
				}
				else if ( PrevOK )
				{
					PrevOK = 0;
					PrevPushed = 0;
					if (sector <= 30 ) //If song just started go to previous track, else we repeat the track
					{
						if(TrackNumber == 1) TrackNumber=TotalTracks; //If it is the first song then go to the last song
						else TrackNumber--; //else go to the previous song in the list
					}
					sector = 0;
					free(buffer);
					return 0;
				}
				else if ( NextOK )
				{
					NextOK = 0;
					NextPushed = 0;
					TrackNumber++; //Goto next song
					if(TrackNumber > TotalTracks) TrackNumber=1; //If it is the last song then go to the first song
					free(buffer);
					return 0;
				}
				
			}
			sector++;
			free(buffer);
		}
		i=0;
		p=FAT_NextCluster(p);			//read next cluster
		if(p == 0x0fffffff || p == 0x0ffffff8 || (FAT32_Enable == 0 && p == 0xffff))	//no more cluster
		{
			wdt_reset();
			trackEnded = 1;
			if(mode==ALL_MODE) 
			{
				TrackNumber++;
				if(TrackNumber > TotalTracks) TrackNumber=1;
			}
			return 0;
		}
	}
}

void initTimerISR(void)
{
	cli();
	
	TCCR1A = 0b00000000;
	TCCR1B = 0b00000101;
	OCR1A = 1650;            // compare match register 16MHz/256/2Hz
	TCCR0A = 0b00000010;
	TCCR0B = 0b00000101;
	OCR0A = 193;
	TIMSK0 = 0b00000010;
	TCCR1B |= (1 << WGM12);   // CTC mode
	TCCR1B |= (1 << CS12);    // 256 prescaler
	TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
	sei();
	
}

//main function
int main()
{
	wdt_enable(WDTO_4S);
	wdt_reset();
	for (i = 0; i < MAX_TRACKS; i++) trackOrder[i] = 255;
	
	latchCompare = HOW_LONG_TO_CONSIDER_A_LOGIC_HIGH_INPUT_TO_BE_INTERPRETED_AS_A_LATCH_IN_MILLISECONDS/12;
	
	initTimerISR(); //sets up the timer for resetting the unit if there is a crash
	U8 retry = 0;
	DDRD &= 0b00000000;		// set all PORTD pins as input
	PORTD |= 0b11111111;	// set pull-up resistors for all PORTD
	PORTD &= 0b11011111;	// clear pull-up resistor on PD5 (PLAY)
	
	OSCCAL = 0x00;	// Slow the clock in case it is too fast for some SD cards
	
	_delay_ms(100);
	wdt_reset();
	
	SPI_Init();	// Initialize the SPI port.

	_delay_ms(100);
	wdt_reset();
	
	if(VS1053B_Init()) //initialize vs1053
	return 0;
	
	_delay_ms(100);
	wdt_reset();
	
	while(SD_Init()) //SD card initialize
	{
		
		wdt_reset();
		retry++;
		
		if(retry>5)
		{
			while(1); //Will reset the watchdog timer to try whole process again.
		}
	}
	
	OSCCAL = 0xff;	// Maximum Internal Clock Frequency

	_delay_ms(100);	// Wait for the clock to stabilize
	wdt_reset();
	
	if(FAT_Init())	// initialize file system: FAT16 and FAT32 are supported
		return 0;   // Exit if FAT system initialization failed
	
	
	wdt_reset();
	SearchInit();
	
	wdt_reset();
	Search(&MusicInfo,&TotalTracks,&type);
	trackOrderFlag = 1;
	
	
	wdt_reset();
	ReorderTracks();
	
	if(TotalTracks==0)return 0; //if no music file return
	
	mode = PLAY_MODE;
	bPlay = PUSH_PLAY; //Set to PUSH_PLAY to auto-start and !PUSH_PLAY to halt.
	latchDirection = bPlay; //Needed for the latching detection.

	if (mode == NEXT_MODE) TrackNumber = TotalTracks; 
	
	while (1) AudioCallback(); //Now that the audio has been initialized, we enter this forever loop
	
	return 0;
}



////////////////////////////////////////////////////////////////
// If sufficient time has gone by without a Data Request
// the VLSI chip is reset. This is to help mitigate brownouts
// on the VLSI when the Atmega has not browned out
////////////////////////////////////////////////////////////////

ISR(TIMER1_COMPA_vect)
{
	if (flag == 1)
	{
		if (toDREQ < 20) toDREQ++;
		else if (toDREQ == 20)
		{
			resetFlag = 1;
			toDREQ = 0;
		}
	}
	TCNT1 = 0;
}



//////////////////////////////////////////////////////////////////////
// This updates the mode read as well as times out the signal state
// to help determine whether the bit is being controlled by pulse or
// latch.
//////////////////////////////////////////////////////////////////////

ISR(TIMER0_COMPA_vect)
{
	TCNT0 = 0;
	P++;
	
	
	if (volumeWait > 10)
	{
		if (Q < 10)
		{
			Q++;
			if ( PrevPushed && !NextPushed && !(PREV) )
			{
				PrevPushed = 0;
				PrevOK = 1;
			}
		}
		else if (Q == 10)
		{
			VolumeChanged = 0;
			if ( PrevPushed ) PrevOK = 1;
			if ( NextPushed ) NextOK = 1;
			Q = 11;
		}
		if ( VolumeChanged )
		{
			PrevPushed = 0;
			NextPushed = 0;
		}
	}
	else
	{
		volumeWait++;
	}
	
	if (P == 25)
	{
		buttonPush = 0;
		P = 0;
		mode = PLAY_MODE;
		bPlay = (PUSH_PLAY);
	}
	if (latchCount < latchCompare)
	{
		latchCount++;
		if (latchCount == latchCompare)
		{
			isLatched = 1;
			latchCount = 0xFFFF;
			latchDirection = bPlay;
			//if(flag) flag = 0; //if we are playing then pause
			//else flag=1; //if we are paused then play
		}
	}
}