
#include	<stdio.h>
#include "stdinc.h"
#include "pce_main.h"
#include	"_clib.h"
#include	"pg.h"

extern IO	io;
extern M6502	M;

#define double	int

#define SOUND_BUF_MS	200
DWORD	dwOldPos[6];
WORD	sbuf[6][44100*SOUND_BUF_MS/1000*sizeof(WORD)*2];
DWORD	CycleOld;
extern int	BaseClock;

int	ds_channels = 1;
int	ds_sample_rate = 15360;//44100;//22050;

int AdpcmIndexAdjustTable[16] = {
  -1, -1, -1, -1,		/* +0 - +3, decrease the step size */
  2, 4, 6, 8,			/* +4 - +7, increase the step size */
  -1, -1, -1, -1,		/* -0 - -3, decrease the step size */
  2, 4, 6, 8,			/* -4 - -7, increase the step size */
};
#define ADPCM_MAX_INDEX 88
int AdpcmStepSizeTable[ADPCM_MAX_INDEX + 1] = {
            7,     8,     9,    10,    11,    12,    13,    14,
           16,    17,    19,    21,    23,    25,    28,    31,
           34,    37,    41,    45,    50,    55,    60,    66,
           73,    80,    88,    97,   107,   118,   130,   143,
          157,   173,   190,   209,   230,   253,   279,   307,
          337,   371,   408,   449,   494,   544,   598,   658,
          724,   796,   876,   963,  1060,  1166,  1282,  1411,
         1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,
         3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,
         7132,  7845,  8630,  9493, 10442, 11487, 12635, 13899,
        15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};
DWORD AdpcmFilledBuf = 0;
byte new_adpcm_play = 0;

#define max(a, b)  (((a) > (b)) ? (a) : (b))
static int mseq(DWORD *rand_val)
{
	if (*rand_val & 0x00080000)
	{
		*rand_val = ((*rand_val ^ 0x0004) << 1) + 1;
		return 1;
	}
	else
	{
		*rand_val <<= 1;
		return 0;
	}
}


void WriteBuffer(WORD *buf, int ch, DWORD dwSize)
{
	static DWORD	n[6] = {0,0,0,0,0,0};
#define	N	32
	static DWORD	k[6] = {0,0,0,0,0,0};
	static DWORD	t;
	static DWORD	r[6];
	static DWORD	rand_val[6] = {0,0,0,0,0x51F631E4,0x51F631E4};
	static WORD		wave[32];
	static int		vol_tbl[32] =
	{
		100,451,508,573,646,728,821,925,
		1043,1175,1325,1493,1683,1898,2139,2411,
		2718,3064,3454,3893,4388,4947,5576,6285,
		7085,7986,9002,10148,11439,12894,14535,16384,
	};
	DWORD	dwPos;
	int		lvol, rvol;
	DWORD	Tp;

	if (!(io.PSG[ch][4]&0x80)){
		n[ch] = k[ch] = 0;
		for (dwPos = 0; dwPos < dwSize; dwPos++)
			*buf++ = 0;
		return;
	}

	if (io.PSG[ch][4]&0x40){
		wave[0] = ((short)io.wave[ch][0]-16)*702;
		if (ds_channels == 2){
			lvol = ((io.psg_volume>>3)&0x1E) + (io.PSG[ch][4]&0x1F) + ((io.PSG[ch][5]>>3)&0x1E);
			lvol = lvol-60;
			if (lvol < 0) lvol = 0;
			lvol = (int)(short)wave[0]*vol_tbl[lvol]/16384;
			rvol = ((io.psg_volume<<1)&0x1E) + (io.PSG[ch][4]&0x1F) + ((io.PSG[ch][5]<<1)&0x1E);
			rvol = rvol-60;
			if (rvol < 0) rvol = 0;
			rvol = (int)(short)wave[0]*vol_tbl[rvol]/16384;
			for (dwPos = 0; dwPos < dwSize; dwPos += 2)	{
				*buf++ = (WORD)lvol;
				*buf++ = (WORD)rvol;
			}
		}else{
			lvol = max((io.psg_volume>>3)&0x1E, (io.psg_volume<<1)&0x1E) + (io.PSG[ch][4]&0x1F) +
						max((io.PSG[ch][5]>>3)&0x1E, (io.PSG[ch][5]<<1)&0x1E);
			lvol = lvol-60;
			if (lvol < 0) lvol = 0;
			lvol = (int)(short)wave[0]*vol_tbl[lvol]/16384;
			for (dwPos = 0; dwPos < dwSize; dwPos++){
				*buf++ = (WORD)lvol;
			}
		}
	}else{
		if (ch >= 4 && (io.PSG[ch][7]&0x80)){
			DWORD Np = (io.PSG[ch][7]&0x1F);
			if (ds_channels == 2){
				lvol = ((io.psg_volume>>3)&0x1E) + (io.PSG[ch][4]&0x1F) + ((io.PSG[ch][5]>>3)&0x1E);
				lvol = lvol-60;
				if (lvol < 0) lvol = 0;
				lvol = vol_tbl[lvol];
				rvol = ((io.psg_volume<<1)&0x1E) + (io.PSG[ch][4]&0x1F) + ((io.PSG[ch][5]<<1)&0x1E);
				rvol = rvol-60;
				if (rvol < 0) rvol = 0;
				rvol = vol_tbl[rvol];
				for (dwPos = 0; dwPos < dwSize; dwPos += 2)	{
					k[ch] += 3000+Np*512;
					t = k[ch] / (DWORD)ds_sample_rate;
					if (t >= 1)	{
						r[ch] = mseq(&rand_val[ch]);
						k[ch] -= ds_sample_rate*t;
					}
					*buf++ = (WORD)((r[ch] ? 10*702 : -10*702)*lvol/16384);
					*buf++ = (WORD)((r[ch] ? 10*702 : -10*702)*rvol/16384);
				}
			}else{
				lvol = max((io.psg_volume>>3)&0x1E, (io.psg_volume<<1)&0x1E) + (io.PSG[ch][4]&0x1F) +
							max((io.PSG[ch][5]>>3)&0x1E, (io.PSG[ch][5]<<1)&0x1E);
				lvol = lvol-60;
				if (lvol < 0) lvol = 0;
				lvol = vol_tbl[lvol];
				for (dwPos = 0; dwPos < dwSize; dwPos++){
					k[ch] += 3000+Np*512;
					t = k[ch] / (DWORD)ds_sample_rate;
					if (t >= 1){
						r[ch] = mseq(&rand_val[ch]);
						k[ch] -= ds_sample_rate*t;
					}
					*buf++ = (WORD)((r[ch] ? 10*702 : -10*702)*lvol/16384);
				}
			}
			return;
		}
		{
			int i;
			for ( i = 0; i < 32; i++)
				wave[i] = ((short)io.wave[ch][i]-16)*702;
		}
		Tp = io.PSG[ch][2]+((DWORD)io.PSG[ch][3]<<8);
		if (Tp == 0){
			for (dwPos = 0; dwPos < dwSize; dwPos++)
				*buf++ = 0;
			return;
		}

		if (ds_channels == 2){
			lvol = ((io.psg_volume>>3)&0x1E) + (io.PSG[ch][4]&0x1F) + ((io.PSG[ch][5]>>3)&0x1E);
			lvol = lvol-60;
			if (lvol < 0) lvol = 0;
			lvol = vol_tbl[lvol];
			rvol = ((io.psg_volume<<1)&0x1E) + (io.PSG[ch][4]&0x1F) + ((io.PSG[ch][5]<<1)&0x1E);
			rvol = rvol-60;
			if (rvol < 0) rvol = 0;
			rvol = vol_tbl[rvol];
			for (dwPos = 0; dwPos < dwSize; dwPos += 2){
				*buf++ = (WORD)((int)(short)wave[n[ch]]*lvol/16384);
				*buf++ = (WORD)((int)(short)wave[n[ch]]*rvol/16384);
				k[ch] += N*1118608/Tp;
				t = k[ch] / (10*(DWORD)ds_sample_rate);
				n[ch] = (n[ch]+t)%N;
				k[ch] -= 10*ds_sample_rate*t;
			}
		}else{
			lvol = max((io.psg_volume>>3)&0x1E, (io.psg_volume<<1)&0x1E) + (io.PSG[ch][4]&0x1F) +
						max((io.PSG[ch][5]>>3)&0x1E, (io.PSG[ch][5]<<1)&0x1E);
			lvol = lvol-60;
			if (lvol < 0) lvol = 0;
			lvol = vol_tbl[lvol];
			for (dwPos = 0; dwPos < dwSize; dwPos++){
				*buf++ = (WORD)((int)(short)wave[n[ch]]*lvol/16384);
				k[ch] += N*1118608/Tp;
				t = k[ch] / (10*(DWORD)ds_sample_rate);
				n[ch] = (n[ch]+t)%N;
				k[ch] -= 10*ds_sample_rate*t;
			}
		}
	}
}
/*
void WriteSoundData(WORD *buf, int ch, DWORD dwSize)
{
	DWORD	dwNewPos;

//	EnterCriticalSection(&cs);
	dwNewPos = dwSize/ds_channels;
	if (dwOldPos[ch] < dwNewPos)
		WriteBuffer(&sbuf[ch][dwOldPos[ch]*ds_channels], ch, (dwNewPos-dwOldPos[ch])*ds_channels);
	CycleOld = (DWORD)M.User;
	_memcpy(buf, sbuf[ch], dwSize*sizeof(WORD));
	if (dwOldPos[ch] >= dwNewPos)
	{
		if (dwOldPos[ch] >= (DWORD)ds_sample_rate*SOUND_BUF_MS/(1000*100/95))
		{
			DWORD	size = ds_sample_rate*SOUND_BUF_MS/4/1000;
			_memcpy(sbuf[ch], sbuf[ch]+dwNewPos*ds_channels, size*sizeof(WORD)*ds_channels);
			dwOldPos[ch] = size;
		}
		else
		{
			_memcpy(sbuf[ch], sbuf[ch]+dwNewPos*ds_channels, (dwOldPos[ch]-dwNewPos)*sizeof(WORD)*ds_channels);
			dwOldPos[ch] = dwOldPos[ch]-dwNewPos;
		}
	}
	else
		dwOldPos[ch] = 0;
//	LeaveCriticalSection(&cs);
}
*/
#include "menu.h"
void	wave_set(int i);
static unsigned short fcpsoundbuff[6][735*60];
void	WriteSoundData(WORD *buf, int ch, DWORD dwSize)
{
	DWORD	dwNewPos;
	int i;
	WORD	*p =buf;

	if( !ini_data.sound )return;

	dwNewPos = dwSize/ds_channels;
//	for(ch=0;ch<6;ch++){
		if (dwOldPos[ch] < dwNewPos)
			WriteBuffer(&sbuf[ch][dwOldPos[ch]*ds_channels], ch, (dwNewPos-dwOldPos[ch])*ds_channels);
		CycleOld = (DWORD)M.User;

//	}
	
//	for(ch=0;ch<6;ch++){
		_memcpy(buf,&sbuf[ch][0],256*2);
//		wave_set(ch);
//	}

//	for(ch=0;ch<6;ch++){

		if (dwOldPos[ch] >= dwNewPos){
			if (dwOldPos[ch] >= (DWORD)ds_sample_rate*SOUND_BUF_MS/(1000*100/95)){
				DWORD	size = ds_sample_rate*SOUND_BUF_MS/4/1000;
				_memcpy(sbuf[ch], sbuf[ch]+dwNewPos*ds_channels, size*sizeof(WORD)*ds_channels);
				dwOldPos[ch] = size;
			}else{
				_memcpy(sbuf[ch], sbuf[ch]+dwNewPos*ds_channels, (dwOldPos[ch]-dwNewPos)*sizeof(WORD)*ds_channels);
				dwOldPos[ch] = dwOldPos[ch]-dwNewPos;
			}
		}else
			dwOldPos[ch] = 0;

//	}

}



static void WriteBufferADPCM16(signed short * buf, DWORD dwMaxSize, DWORD *dwADPCMPtr, DWORD dwADPCMMax, signed char * Index, signed int * PreviousValue)
{
	DWORD ret_val = 0;
	DWORD dwRet = 0;

	/* TODO: use something else than ALLEGRO's fixed to make this portable */
	signed int step, difference, deltaCode;
	signed char index = *Index;
	signed int previousValue = *PreviousValue;
	double FixedIndex = 0, FixedInc;


	if (io.adpcm_rate)
		FixedInc = ((double) io.adpcm_rate * 1000 / (double) ds_sample_rate);
	else
		return;

	while (dwRet < dwMaxSize) {
		FixedIndex += FixedInc;

		while (FixedIndex > (double) 1 ) {
			FixedIndex -= 1;
			ret_val++;
			deltaCode = PCM[(*dwADPCMPtr) >> 1];
			if ((*dwADPCMPtr) & 1)
				deltaCode >>= 4;
			else
				deltaCode &= 0xF;

			step = AdpcmStepSizeTable[index];
			(*dwADPCMPtr)++;

			(*dwADPCMPtr) &= 0x1FFFF;
		// Make the adpcm repeat from beginning once finished

		/* Construct the difference by scaling the current step size */
		/* This is approximately: difference = (deltaCode+.5)*step/4 */
			difference = step >> 3;
			if (deltaCode & 1)
				difference += step >> 2;
			if (deltaCode & 2)
				difference += step >> 1;
			if (deltaCode & 4)
				difference += step;

			if (deltaCode & 8)
				difference = -difference;

		/* Build the new sample */
			previousValue += difference;

			if (previousValue > 32767)
				previousValue = 32767;
			else if (previousValue < -32768)
				previousValue = -32768;

			index += AdpcmIndexAdjustTable[deltaCode];
			if (index < 0)
				index = 0;
			else if (index > ADPCM_MAX_INDEX)
				index = ADPCM_MAX_INDEX;
		}
		/* TEST, was 5 */
		*(buf++) = previousValue;
		dwRet+=2;
		if(ds_channels == 2) {
			*(buf++) = previousValue;// Stereo 16Bit
			dwRet+=2;
		}
	}
	*Index = index;
	*PreviousValue = previousValue;
	return ;
}


static DWORD WriteBufferAdpcm8(word * buf, DWORD begin, DWORD size, signed char * Index, signed int * PreviousValue, DWORD * bufsize)
{
	DWORD ret_val = 0;
	DWORD dwRet = 0;

	/* TODO: use something else than ALLEGRO's fixed to make this portable */
	signed int step, difference, deltaCode;
	signed char index = *Index;
	signed int previousValue = *PreviousValue;
	double FixedIndex = 0, FixedInc;


	if (io.adpcm_rate)
		FixedInc = ((double) io.adpcm_rate * 1000 / (double) ds_sample_rate);
	else
		return 0;

	while (size) {
		FixedIndex += FixedInc;

		while (FixedIndex > (double) 1 ) {
			FixedIndex -= 1;
			ret_val++;
			deltaCode = PCM[begin >> 1];
			if (begin & 1)
				deltaCode >>= 4;
			else
				deltaCode &= 0xF;

			step = AdpcmStepSizeTable[index];
			begin++;

			begin &= 0x1FFFF;
		// Make the adpcm repeat from beginning once finished

		/* Construct the difference by scaling the current step size */
		/* This is approximately: difference = (deltaCode+.5)*step/4 */
			difference = step >> 3;
			if (deltaCode & 1)
				difference += step >> 2;
			if (deltaCode & 2)
				difference += step >> 1;
			if (deltaCode & 4)
				difference += step;

			if (deltaCode & 8)
				difference = -difference;

		/* Build the new sample */
			previousValue += difference;

			if (previousValue > 32767)
				previousValue = 32767;
			else if (previousValue < -32768)
				previousValue = -32768;

			index += AdpcmIndexAdjustTable[deltaCode];
			if (index < 0)
				index = 0;
			else if (index > ADPCM_MAX_INDEX)
				index = ADPCM_MAX_INDEX;
		}
		/* TEST, was 5 */
		*(buf++) = (previousValue << 6);
		dwRet++;
		if(ds_channels == 2) {
			*(buf++) = (previousValue << 6);// Stereo 16Bit
			dwRet++;
		}
		size--;
	}
	*Index = index;
	*PreviousValue = previousValue;
	*bufsize = dwRet;
	return ret_val;
}
int	WriteADPCM(WORD *buf)
{
	DWORD AdpcmUsedNibbles;
	DWORD usebufsize;
	static signed char index;
	static signed int previousValue;

	AdpcmFilledBuf = io.adpcm_psize;

	if (new_adpcm_play) {
		index = 0;
		previousValue = 0;
	} else {
		return 0;
	}

	AdpcmUsedNibbles = WriteBufferAdpcm8((word*) buf, io.adpcm_pptr, AdpcmFilledBuf, &index, &previousValue, &usebufsize);

	io.adpcm_pptr += AdpcmUsedNibbles;
	io.adpcm_pptr &= 0x1FFFF;

	if (AdpcmUsedNibbles)
		io.adpcm_psize -= AdpcmUsedNibbles;
	else
		io.adpcm_psize = 0;
  /* If we haven't played even a nibbles, it problably mean we won't ever be
   * able to play one, so we stop the adpcm playing
   */

}

void write_psg(int ch)
{
	DWORD	Cycle;
	DWORD	dwNewPos;


	if ((int)((DWORD)M.User - CycleOld) < 0)
		CycleOld = (DWORD)M.User;
	Cycle = (DWORD)M.User - CycleOld;
	dwNewPos = (DWORD)ds_sample_rate/25*(Cycle/2)/(BaseClock/50);
//	dwNewPos=735;
	if (dwNewPos > (DWORD)ds_sample_rate*SOUND_BUF_MS/1000){
		dwNewPos = ds_sample_rate*SOUND_BUF_MS*3/4/1000;
//	dwNewPos=735;
	}
	if (dwNewPos > dwOldPos[ch]){
		WriteBuffer(&sbuf[ch][dwOldPos[ch]*ds_channels], ch, (dwNewPos-dwOldPos[ch])*ds_channels);
		dwOldPos[ch] = dwNewPos;
	}

}




