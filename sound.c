
#include "_clib.h"
#include "sound.h"


//=============================================================================
//	サウンド関係



//channel 0 for wave output, 1 for sound effect
wavout_wavinfo_t *wavout_snd0_wavinfo=0;
int wavout_snd0_ready=0;
int wavout_snd0_playend=0;
unsigned long wavout_snd0_playptr=0;


static void wavout_snd0_callback(short *_buf, unsigned long _reqn)
{
	static int power[128];
	
	unsigned long i;
	unsigned long ptr,frac,rr,max;
	int channels;
	char *src;
	short *buf=_buf;
	unsigned long reqn=_reqn;
	
	wavout_wavinfo_t *wi=wavout_snd0_wavinfo;

	if (wi==0) {
		wavout_snd0_ready=1;
		_memset(buf,0,reqn*4);
		return;
	}
	
	wavout_snd0_ready=0;
	
	ptr=wi->playptr;
	frac=wi->playptr_frac;
	rr=wi->rateratio;
	max=wi->samplecount;
	channels=wi->channels;
	src=wi->wavdata;

	for (; reqn>0; --reqn) {
		frac+=rr;
		ptr+=(frac>>16);
		frac&=0xffff;
		if (ptr>=max) {
			if (wi->playloop) {
				ptr=0;
			} else {
				for (; reqn>0; --reqn) {
					*(buf++)=0;
					*(buf++)=0;
				}
				goto playend;
			}
		}
		if (channels==1) {
			buf[0]=buf[1]=*(short *)(src+ptr*2);
			buf+=2;
		} else {
			buf[0]=*(short *)(src+ptr*4);
			buf[1]=*(short *)(src+ptr*4+2);
			buf+=2;
		}
	}

//	powercalc(_buf);	//単にwaveを出すだけなら不要

	wavout_snd0_playptr=ptr;
	wi->playptr=ptr;
	wi->playptr_frac=frac;
	return;
	
playend:
	wavout_snd0_playend=1;
	return;

}





wavout_wavinfo_t wavout_snd1_wavinfo[SND1_MAXSLOT];
int wavout_snd1_playing[SND1_MAXSLOT];

static void wavout_snd1_callback(short *_buf, unsigned long _reqn)
{
	unsigned long i,slot;
	wavout_wavinfo_t *wi;
	unsigned long ptr,frac;
	short *buf=_buf;
	
	for (i=0; i<_reqn; i++) {
		int outr=0,outl=0;
		for (slot=0; slot<SND1_MAXSLOT; slot++) {
			if (!wavout_snd1_playing[slot]) continue;
			wi=&wavout_snd1_wavinfo[slot];
			frac=wi->playptr_frac+wi->rateratio;
			wi->playptr=ptr=wi->playptr+(frac>>16);
			wi->playptr_frac=(frac&0xffff);
			if (ptr>=wi->samplecount) {
				if( wi->playloop ){
					wi->playptr=0;
				}else{
					wavout_snd1_playing[slot]=0;
				}
				break;
			}
			short *src=(short *)wi->wavdata;
			if (wi->channels==1) {
				outl+=src[ptr];
				outr+=src[ptr];
			} else {
				outl+=src[ptr*2];
				outr+=src[ptr*2+1];
			}
		}

		if (outl<-32768) outl=-32768;
		else if (outl>32767) outl=32767;
		if (outr<-32768) outr=-32768;
		else if (outr>32767) outr=32767;
		*(buf++)=outl;
		*(buf++)=outr;

	}
}





void wavoutStopPlay0()
{
	if (wavout_snd0_wavinfo!=0) {
		while (wavout_snd0_ready) pgWaitV();
		wavout_snd0_wavinfo=0;
		while (!wavout_snd0_ready) pgWaitV();
	}
}

void wavoutStartPlay0(wavout_wavinfo_t *wi)
{
	wavoutStopPlay0();
	while (!wavout_snd0_ready) pgWaitV();
	wavout_snd0_playptr=0;
	wavout_snd0_playend=0;
	wavout_snd0_wavinfo=wi;
	while (wavout_snd0_ready) pgWaitV();
}

int wavoutWaitEnd0()
{
	if (wavout_snd0_wavinfo==0) return -1;
	if (wavout_snd0_wavinfo->playloop) return -1;
	while (!wavout_snd0_playend) pgWaitV();
	return 0;
}


unsigned short fcpsoundbuff[SND1_MAXSLOT][735*60];
wavout_wavinfo_t wavinfo_bg[SND1_MAXSLOT];
void	counter(int x,int y,unsigned int c,int k,int col);
#define	soundloopc	(735*60)
//wavout_wavinfo_t wavinfo_se[SND1_MAXSLOT];
void	wave_set(int i,char *a)
{
}

//stop all
void wavoutStopPlay1()
{
	int i;
	for (i=0; i<SND1_MAXSLOT; i++) wavout_snd1_playing[i]=0;
}

 //return 0 if success
/*
int wavoutStartPlay1(wavout_wavinfo_t *wi)
{
	int i;
	wavout_wavinfo_t *wid;
	for (i=0; i<SND1_MAXSLOT; i++) if (wavout_snd1_playing[i]==0) break;
	if (i==SND1_MAXSLOT) return -1;
	wid=&wavout_snd1_wavinfo[i];
	wid->channels=wi->channels;
	wid->samplerate=wi->samplerate;
	wid->samplecount=wi->samplecount;
	wid->datalength=wi->datalength;
	wid->wavdata=wi->wavdata;
	wid->rateratio=wi->rateratio;
	wid->playptr=0;
	wid->playptr_frac=0;
	wid->playloop=1;
	wavout_snd1_playing[i]=1;
	return 0;
}
*/
void wavout_1secInits(int i)
{
	{
		wavout_wavinfo_t *w=&wavout_snd1_wavinfo[i];//wavinfo_se[i];
		w->channels = 1;
		w->samplerate = 15360;
		w->samplecount = (256*60);
		w->datalength = (256*60)*2;
		w->wavdata = (void *)&fcpsoundbuff[i][0];
		w->rateratio = (15360*0x4000)/11025;
		w->playptr = 0;
		w->playptr_frac = 0;
		w->playloop = 1;
		wavout_snd1_playing[i]=1;
	//	wavoutStartPlay1(w);
	}
}
void	wavout_1secInit(void)
{
//		InfoNES_StartTimer();
	int i;
	for(i=0;i<6;i++){
		wavout_snd1_playing[i]=1;
		wavout_snd1_wavinfo[i].playptr=0;
	}

}

//return 0 if success
int wavoutInit()
{
	int i;
	
	wavout_snd0_wavinfo=0;
	
	for (i=0; i<SND1_MAXSLOT; i++) {
		wavout_snd1_playing[i]=0;
	}

//	pgaSetChannelCallback(0,wavout_snd0_callback);
	pgaSetChannelCallback(1,wavout_snd1_callback);

	wavout_1secInits(0);
	wavout_1secInits(1);
	wavout_1secInits(2);
	wavout_1secInits(3);
	wavout_1secInits(4);
	wavout_1secInits(5);
	wavout_1secInits(6);

	return 0;
}

void wavoutClearWavinfoPtr(wavout_wavinfo_t *wi)
{
	wi->playptr=0;
	wi->playptr_frac=0;
}

