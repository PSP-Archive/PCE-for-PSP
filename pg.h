// primitive graphics for Hello World PSP

void pgInit();
void pgWaitV();
void pgWaitVn(unsigned long count);
void pgScreenFrame(long mode,long frame);
void pgScreenFlip();
void pgScreenFlipV();
/*
void pgPrint(unsigned long x,unsigned long y,unsigned long color,const char *str);
void pgPrint2(unsigned long x,unsigned long y,unsigned long color,const char *str);
void pgPrint4(unsigned long x,unsigned long y,unsigned long color,const char *str);
*/
void pgFillvram(unsigned long color);
void pgBitBlt(unsigned long x,unsigned long y,unsigned long w,unsigned long h,unsigned long mag,const unsigned short *d);
//void pgPutChar(unsigned long x,unsigned long y,unsigned long color,unsigned long bgcolor,unsigned char ch,char drawfg,char drawbg,char mag);
void	Error_mes(char *t);
void	Error_count(int i);


#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 272

#define		PIXELSIZE	1				//in short
#define		LINESIZE	512				//in short
#define		FRAMESIZE	0x44000			//in byte

/* Index for the two analog directions */ 
#define CTRL_ANALOG_X   0 
#define CTRL_ANALOG_Y   1 

/* Button bit masks */ 
#define CTRL_SQUARE      0x8000 
#define CTRL_TRIANGLE   0x1000 
#define CTRL_CIRCLE      0x2000 
#define CTRL_CROSS      0x4000 
#define CTRL_UP         0x0010 
#define CTRL_DOWN      0x0040 
#define CTRL_LEFT      0x0080 
#define CTRL_RIGHT      0x0020 
#define CTRL_START      0x0008 
#define CTRL_SELECT      0x0001 
#define CTRL_LTRIGGER   0x0100 
#define CTRL_RTRIGGER   0x0200 

/* Returned control data */ 
// buttons
// 00000000
// 00000010  up
// 00000040  down
// 00000080  left
// 00000020  right
// 00000001  SELECT
// 00000008  START
// 00000100  L
// 00000200  R
// 00001000  triangle
// 00008000  square
// 00002000  circle
// 00004000  cross

typedef struct _ctrl_data 
{ 
	unsigned long frame; 			// ŽžŠÔ(tick)
   unsigned long buttons;
   unsigned char  analog[4]; 		// [0]:X [1]:Y
   unsigned long unused; 
} ctrl_data_t; 



enum { 
    TYPE_DIR=0x10, 
    TYPE_FILE=0x20 
}; 

#define OF_RDONLY    0x0001 
#define O_RDONLY    0x0001 
#define O_WRONLY    0x0002 
#define O_RDWR      0x0003 
#define O_NBLOCK    0x0010 
#define O_APPEND    0x0100 
#define O_CREAT     0x0200 
#define O_TRUNC     0x0400 
#define O_NOWAIT    0x8000 


typedef struct dirent { 
    unsigned long unk0; 
    unsigned long type; 
    unsigned long size; 
    unsigned long unk[19]; 
    char name[0x108]; 
	unsigned char dmy[128];
} dirent_t;


//
void  sceCtrlInit(int unknown); 
void  sceCtrlSetAnalogMode(int on); 
void  sceCtrlRead(ctrl_data_t* paddata, int unknown); 

//
int		sceIoOpen(const char* file, int mode, int nazo);
void sceIoClose(int fd); 
int  sceIoRead(int fd, void *data, int size); 
int  sceIoWrite(int fd, void *data, int size); 
int sceIoLseek(int fd, long long offset, int whence); 
int  sceIoRemove(const char *file); 
int  sceIoMkdir(const char *dir, int mode); 
int  sceIoRmdir(const char *dir); 
int  sceIoRename(const char *oldname, const char *newname); 
//int sceIoDevctl(const char *name int cmd, void *arg, size_t arglen, void *buf, size_t *buflen); 
int  sceIoDopen(const char *fn); 
int  sceIoDread(int fd, dirent_t *de); 
void sceIoDclose(int fd); 


void sceKernelExitGame(void); 
void SetExitCallback(int cbid); 
void PowerSetCallback(int zero, int cbid); 
void KernelPollCallbacks(void); 
int  sceKernelCreateCallbacks(const char *name, void *func); 
int  sceKernelCreateThread(const char *name, void *func, int a,int b,int c,int d);

void pgaSetChannelCallback(int channel, void *callback);

typedef int (*pg_threadfunc_t)(int args, void *argp);
int sceKernelStartThread(int hthread, int arg0, void *arg1);
void sceKernelExitThread(int ret);
int sceKernelWaitThreadEnd(int hthread, void *unk);
int sceKernelDeleteThread(int hthread);


void	System_mes_init(void);
void	System_mes_put(void);
void	System_mes(char *t);
void	System_count(int c);

