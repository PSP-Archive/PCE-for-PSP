// primitive graphics for Hello World PSP

#include "pg.h"

#define NULL  0

//system call
void pspDisplayWaitVblankStart();
void pspDisplaySetMode(long,long,long);
void pspDisplaySetFrameBuf(char *topaddr,long linesize,long pixelsize,long);


//constants

//480*272 = 60*38
#define CMAX_X 60
#define CMAX_Y 38
#define CMAX2_X 30
#define CMAX2_Y 19
#define CMAX4_X 15
#define CMAX4_Y 9


//variables
char *pg_vramtop=(char *)0x04000000;
long pg_screenmode;
long pg_showframe;
long pg_drawframe;



void pgWaitVn(unsigned long count)
{
	for (; count>0; --count) {
		pspDisplayWaitVblankStart();
	}
}


void pgWaitV()
{
	pspDisplayWaitVblankStart();
}


char *pgGetVramAddr(unsigned long x,unsigned long y)
{
	return pg_vramtop+(pg_drawframe?FRAMESIZE:0)+x*PIXELSIZE*2+y*LINESIZE*2+0x40000000;
}


void pgInit()
{
	pspDisplaySetMode(0,SCREEN_WIDTH,SCREEN_HEIGHT);
	pgScreenFrame(0,0);
}

/*
void pgPrint(unsigned long x,unsigned long y,unsigned long color,const char *str)
{
	while (*str!=0 && x<CMAX_X && y<CMAX_Y) {
		pgPutChar(x*8,y*8,color,0,*str,1,0,1);
		str++;
		x++;
		if (x>=CMAX_X) {
			x=0;
			y++;
		}
	}
}


void pgPrint2(unsigned long x,unsigned long y,unsigned long color,const char *str)
{
	while (*str!=0 && x<CMAX2_X && y<CMAX2_Y) {
		pgPutChar(x*16,y*16,color,0,*str,1,0,2);
		str++;
		x++;
		if (x>=CMAX2_X) {
			x=0;
			y++;
		}
	}
}


void pgPrint4(unsigned long x,unsigned long y,unsigned long color,const char *str)
{
	while (*str!=0 && x<CMAX4_X && y<CMAX4_Y) {
		pgPutChar(x*32,y*32,color,0,*str,1,0,4);
		str++;
		x++;
		if (x>=CMAX4_X) {
			x=0;
			y++;
		}
	}
}
*/

void pgFillvram(unsigned long color)
{
	unsigned long *vr;
	int x,y;
	color+=color<<16;
	vr = (void *)pgGetVramAddr(0,0);
	for(y=0;y<272;y++){
		for(x=0;x<480/2;x++){
			*vr++=color;
		}
		vr+=(LINESIZE-480)/2;
	}
}

void pgBitBlt(unsigned long x,unsigned long y,unsigned long w,unsigned long h,unsigned long mag,const unsigned short *d)
{
	unsigned char *vptr0;		//pointer to vram
	unsigned char *vptr;		//pointer to vram
	unsigned long xx,yy,mx,my;
	const unsigned short *dd;
	
	vptr0=pgGetVramAddr(x,y);
	for (yy=0; yy<h; yy++) {
		for (my=0; my<mag; my++) {
			vptr=vptr0;
			dd=d;
			for (xx=0; xx<w; xx++) {
				for (mx=0; mx<mag; mx++) {
					*(unsigned short *)vptr=*dd;
					vptr+=PIXELSIZE*2;
				}
				dd++;
			}
			vptr0+=LINESIZE*2;
		}
		d+=w;
	}
	
}

/*
void pgPutChar(unsigned long x,unsigned long y,unsigned long color,unsigned long bgcolor,unsigned char ch,char drawfg,char drawbg,char mag)
{
	unsigned char *vptr0;		//pointer to vram
	unsigned char *vptr;		//pointer to vram
	const unsigned char *cfont;		//pointer to font
	unsigned long cx,cy;
	unsigned long b;
	char mx,my;

	if (ch>255) return;
	cfont=font+ch*8;
	vptr0=pgGetVramAddr(x,y);
	for (cy=0; cy<8; cy++) {
		for (my=0; my<mag; my++) {
			vptr=vptr0;
			b=0x80;
			for (cx=0; cx<8; cx++) {
				for (mx=0; mx<mag; mx++) {
					if ((*cfont&b)!=0) {
						if (drawfg) *(unsigned short *)vptr=color;
					} else {
						if (drawbg) *(unsigned short *)vptr=bgcolor;
					}
					vptr+=PIXELSIZE*2;
				}
				b=b>>1;
			}
			vptr0+=LINESIZE*2;
		}
		cfont++;
	}
}
*/

void pgScreenFrame(long mode,long frame)
{
	pg_screenmode=mode;
	frame=(frame?1:0);
	pg_showframe=frame;
	if (mode==0) {
		//screen off
		pg_drawframe=frame;
		pspDisplaySetFrameBuf(0,0,0,1);
	} else if (mode==1) {
		//show/draw same
		pg_drawframe=frame;
		pspDisplaySetFrameBuf(pg_vramtop+(pg_showframe?FRAMESIZE:0),LINESIZE,PIXELSIZE,1);
	} else if (mode==2) {
		//show/draw different
		pg_drawframe=(frame?0:1);
		pspDisplaySetFrameBuf(pg_vramtop+(pg_showframe?FRAMESIZE:0),LINESIZE,PIXELSIZE,1);
	}
}


void pgScreenFlip()
{
	pg_showframe=(pg_showframe?0:1);
	pg_drawframe=(pg_drawframe?0:1);
	pspDisplaySetFrameBuf(pg_vramtop+(pg_showframe?FRAMESIZE:0),LINESIZE,PIXELSIZE,0);
}


void pgScreenFlipV()
{
	pgWaitV();
	pgScreenFlip();
}


/******************************************************************************/



#define PGA_CHANNELS 3
#define PGA_SAMPLES 256
#define MAXVOLUME 0x8000

int pga_ready=0;
int pga_handle[PGA_CHANNELS];

short pga_sndbuf[PGA_CHANNELS][2][PGA_SAMPLES][2];


void (*pga_channel_callback[PGA_CHANNELS])(void *buf, unsigned long reqn);

int pga_threadhandle[PGA_CHANNELS];


volatile int pga_terminate=0;


static int pga_channel_thread(int args, void *argp)
{
	volatile int bufidx=0;
	int channel=*(int *)argp;
	
	while (pga_terminate==0) {
		void *bufptr=&pga_sndbuf[channel][bufidx];
		void (*callback)(void *buf, unsigned long reqn);
		callback=pga_channel_callback[channel];
		if (callback) {
			callback(bufptr,PGA_SAMPLES);
		} else {
			unsigned long *ptr=bufptr;
			int i;
			for (i=0; i<PGA_SAMPLES; ++i) *(ptr++)=0;
		}
		pgaOutBlocking(channel,0x8000,0x8000,bufptr);
		bufidx=(bufidx?0:1);
	}
	sceKernelExitThread(0);
	return 0;
}


void pga_channel_thread_callback(int channel, void *buf, unsigned long reqn)
{
	void (*callback)(void *buf, unsigned long reqn);
	callback=pga_channel_callback[channel];
}


void pgaSetChannelCallback(int channel, void *callback)
{
	pga_channel_callback[channel]=callback;
}



/******************************************************************************/



int pgaInit()
{
	int i,ret;
	int failed=0;
	char str[32];

	pga_terminate=0;
	pga_ready=0;

	for (i=0; i<PGA_CHANNELS; i++) {
		pga_handle[i]=-1;
		pga_threadhandle[i]=-1;
		pga_channel_callback[i]=0;
	}
	for (i=0; i<PGA_CHANNELS; i++) {
		if ((pga_handle[i]=sceAudio_3(-1,PGA_SAMPLES,0))<0) failed=1;
	}
	if (failed) {
		for (i=0; i<PGA_CHANNELS; i++) {
			if (pga_handle[i]!=-1) sceAudio_4(pga_handle[i]);
			pga_handle[i]=-1;
		}
		return -1;
	}
	pga_ready=1;

	_strcpy(str,"pgasnd0");
	for (i=0; i<PGA_CHANNELS; i++) {
		str[6]='0'+i;
		pga_threadhandle[i]=sceKernelCreateThread(str,(pg_threadfunc_t)&pga_channel_thread,0x12,0x10000,0,NULL);
		if (pga_threadhandle[i]<0) {
			pga_threadhandle[i]=-1;
			failed=1;
			break;
		}
		ret=sceKernelStartThread(pga_threadhandle[i],sizeof(i),&i);
		if (ret!=0) {
			failed=1;
			break;
		}
	}
	if (failed) {
		pga_terminate=1;
		for (i=0; i<PGA_CHANNELS; i++) {
			if (pga_threadhandle[i]!=-1) {
				sceKernelWaitThreadEnd(pga_threadhandle[i],NULL);
				sceKernelDeleteThread(pga_threadhandle[i]);
			}
			pga_threadhandle[i]=-1;
		}
		pga_ready=0;
		return -1;
	}
	return 0;
}


void pgaTermPre()
{
	pga_ready=0;
	pga_terminate=1;
}


void pgaTerm()
{
	int i;
	pga_ready=0;
	pga_terminate=1;

	for (i=0; i<PGA_CHANNELS; i++) {
		if (pga_threadhandle[i]!=-1) {
			sceKernelWaitThreadEnd(pga_threadhandle[i],NULL);
			sceKernelDeleteThread(pga_threadhandle[i]);
		}
		pga_threadhandle[i]=-1;
	}

	for (i=0; i<PGA_CHANNELS; i++) {
		if (pga_handle[i]!=-1) {
			sceAudio_4(pga_handle[i]);
			pga_handle[i]!=-1;
		}
	}
}



int pgaOutBlocking(unsigned long channel,unsigned long vol1,unsigned long vol2,void *buf)
{
	if (!pga_ready) return -1;
	if (channel>=PGA_CHANNELS) return -1;
	if (vol1>MAXVOLUME) vol1=MAXVOLUME;
	if (vol2>MAXVOLUME) vol2=MAXVOLUME;
	return sceAudio_2(pga_handle[channel],vol1,vol2,buf);
}


volatile int pg_terminate=0;


void pgExit(int n)
{
	pg_terminate=1;
	
	// terminate subsystem preprocess
	pgaTermPre();
	
	// terminate subsystem
	pgaTerm();
	
	//__exit();
}

void pgErrorHalt(const char *str)
{
/*
	pgScreenFrame(1,0);
	pgcLocate(0,0);
	pgcColor(0xffff,0x0000);
	pgcDraw(1,1);
	pgcSetmag(1);
	pgcPuts(str);
*/
	while (1) { pgWaitV(); }
}


void pgiInit()
{
	sceCtrlInit(0);
	sceCtrlSetAnalogMode(1);
}
void pgMain(unsigned long args, void *argp)
{
	int ret;
	int n;
/*	
	n=args;
	if (n>sizeof(pg_mypath)-1) n=sizeof(pg_mypath)-1;
	_memcpy(pg_mypath,argp,n);
	pg_mypath[sizeof(pg_mypath)-1]=0;
	_strcpy(pg_workdir,pg_mypath);
	for (n=_strlen(pg_workdir); n>0 && pg_workdir[n-1]!='/'; --n) pg_workdir[n-1]=0;
	
	sceDisplaySetMode(0,SCREEN_WIDTH,SCREEN_HEIGHT);
	
	pgScreenFrame(0,1);
	pgcLocate(0,0);
	pgcColor(0xffff,0x0000);
	pgcDraw(1,1);
	pgcSetmag(1);
	pgScreenFrame(0,0);
	pgcLocate(0,0);
	pgcColor(0xffff,0x0000);
	pgcDraw(1,1);
	pgcSetmag(1);
*/	
	pgiInit();
	
	ret=pgaInit();
	if (ret) pgErrorHalt("pga subsystem initialization failed.");

	ret=xmain( args, argp);
	pgExit(ret);
}

char	system_mes_buf[22][50];
char	system_mes_f[22];
int		system_mes_x;
int		system_mes_y;
void	System_mes_init(void)
{
	int i;
	for(i=0;i<27;i++){
		system_mes_f[i]=0;
	}
	system_mes_x=0;
}
void	System_mes_put(void)
{
	int i;
	for( i=0;i<22;i++ ){
		if( system_mes_f[i] ){
			mh_print(120,((i)*10)+0, &system_mes_buf[i][0],rgb2col(255,255,255),0,0);
			system_mes_f[i]--;
		}
	}
}
int	System_mes_s(void)
{
	int i;
	_memcpy(&system_mes_buf[0][0],&system_mes_buf[1][0],21*50);
	_memcpy(&system_mes_f[0],&system_mes_f[1],(21));
	system_mes_f[21]=40;
	return	21;
}
void	System_mes(char *t)
{
	int i;
	int x=system_mes_x;
	char *p;
	if( !system_mes_x )i=system_mes_y=System_mes_s();else i=system_mes_y;
	p=&system_mes_buf[i][x];
	while(x<50){
		*p++=*t++;
		x++;
	}
	system_mes_x=0;
}
void	System_count(int c)
{
	int i;
	int k=10;
	int x;
	if( !system_mes_x )i=system_mes_y=System_mes_s();else i=system_mes_y;
	for(x=system_mes_x;x<system_mes_x+10;x++)system_mes_buf[i][x]=' ';
	x=system_mes_x+10;
	system_mes_buf[i][x--]=0;
	do{
		system_mes_buf[i][x--]='0'+(c%10);
		c/=10;
		if(!(--k))c=0;
	}while(c);
	system_mes_x+=10;
}




extern	int	psp_exit_flag;
void	Error_mes(char *t)
{
	pgFillvram(0);
	mh_print(0,100,t,rgb2col(255,0,0),0,0);
	pgScreenFlipV();
	
	{
		int i=30;
		while(i--)pgWaitV();
	}
	while( !psp_exit_flag ){
		unsigned long key;
		key = Read_Key();
		if (key ) break;
		pgWaitV();
	}
}

void	Error_count(int c)
	{
		pgFillvram(0);
		counter( 50, 0, c, 8, -1);
		pgScreenFlipV();
		{
			int i=30;
			while(i--)pgWaitV();
		}
		while( !psp_exit_flag ){
			unsigned long key;
			key = Read_Key();
			if (key ) break;
			pgWaitV();
		}
	}




