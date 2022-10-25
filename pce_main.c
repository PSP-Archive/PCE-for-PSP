/*
	Portable PC-Engine Emulator
	1998 by BERO bero@geocities.co.jp

    Modified 1998 by hmmx hmmx@geocities.co.jp
*/

#include	"stdinc.h"
//#include	<stdio.h>
#include	"m6502.h"
#include	"pce_main.h"
#include	"_clib.h"
#include	"pg.h"
#include	"menu.h"
#include	"iyanbakan.h"
#include	"sound.h"
#include	"mp3.h"

#include "iso_ent.h"

#ifndef TRUE
#define	TRUE	1
#define	FALSE	0
#endif

int exit_flag;
	static unsigned int rapid1;
	static unsigned int rapid2;

#define	USE_SPRITE_CACHE 1

		extern	int	state_slot;

/* write */
	M6502 M;

	ctrl_data_t ctl;
void	sound_put(void);

void	counter(int x,int y,unsigned int c,int k,int col);

byte	RAM[0x8000];
byte	PopRAM[0x10000];
byte	*Page[8],*ROMMap[512];
#define	VRAMSIZE	0x20000
#define	ROMSIZE		(2621952+10)
byte	VRAM[VRAMSIZE];
byte	ROM_buff[ROMSIZE];
byte	*ROM;
byte	vchange[VRAMSIZE/32];
byte	WRAM[0x2000];
byte	DMYROM[0x2000];
byte	PCM[0x10000];
byte	IOAREA[0x2000];
short	SPRAM[64*4];
unsigned long VRAM2[VRAMSIZE / sizeof(unsigned long)];
#if defined(USE_SPRITE_CACHE)
byte vchanges[VRAMSIZE/128];
unsigned long VRAMS[VRAMSIZE / sizeof(unsigned long)];
#endif //defined(USE_SPRITE_CACHE)
int ROM_size;
int Country=0;
int IPeriod;
int BaseClock = 7160000, UPeriod = 0;
static int TimerCount,CycleOld;
int TimerPeriod;
int scanlines_per_frame  = 263;
int scanline, prevline;
#define MinLine	io.minline
#define MaxLine io.maxline
#define MAXDISP	227
byte BGONSwitch = 1, SPONSwitch = 1;
byte populus = 0;
byte sf2ce = 0;
int scroll = 0;
int skip_next_frame = 0;

byte Pal[512];

IO io;






// =-=- for Movie
short	*pusMovie[5];
int		iMoviePtr[5];
int		iMaxMoviePtr[5];
short	joy_before[5];
short	joy_counter[5];
int		iMode;
//CRITICAL_SECTION	cs;
void	SetPlayMode();
int	iMovieFilePtr;
int	iModeChange;

// =-=-=- For CDROM^2
#define CD_FRAMES 75
#define CD_SECS 60

byte minimum_bios_hooking = 1;
//BYTE can_write_debug = 0;
#define CD_BUF_LENGTH 8
//byte *cd_buf = NULL;
byte	cd_buf[CD_BUF_LENGTH*2048];
byte CDBIOS_replace[0x4d][2];
//char short_iso_name[80];
byte hook_start_cd_system = 0;
char ISO_filename[256] = "";
byte CD_emulation = 0;
// Do we emulate CD ( == 1)
//            or  ISO file   ( == 2)
//            or  plain BIN file ( == 3)
byte builtin_system_used = 0;
// Have we used the .dat included rom or no ?
Track CD_track[0x100];
// Track
// beg_min -> beginning in minutes since the begin of the CD(BCD)
// beg_sec -> beginning in seconds since the begin of the CD(BCD)
// beg_fr -> beginning in frames   since the begin of the CD(BCD)
// type -> 0 = audio, 4 = data
// beg_lsn -> beginning in number of sector (2048 bytes)
// length -> number of sector
byte cd_fade;
// the byte set by the fade function

byte cd_extra_mem[0x10000];
// extra ram provided by the system CD card

unsigned char binbcd[0x100] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
};


unsigned char bcdbin[0x100] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0, 0, 0, 0, 0, 0,
	0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0, 0, 0, 0, 0, 0,
	0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0, 0, 0, 0, 0, 0,
	0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0, 0, 0, 0, 0, 0,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0, 0, 0, 0, 0, 0,
	0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0, 0, 0, 0, 0, 0,
	0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0, 0, 0, 0, 0, 0,
	0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0, 0, 0, 0, 0, 0,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0, 0, 0, 0, 0, 0,
	0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0, 0, 0, 0, 0, 0,
};


byte cd_port_1800 = 0;
byte cd_port_1801 = 0;
byte cd_port_1802 = 0;
byte cd_port_1804 = 0;
byte cd_port_180b = 0;
byte pce_cd_adpcm_trans_done = 0;
byte pce_cd_curcmd;
byte pce_cd_cmdcnt;
byte cd_sectorcnt;
//FILE *iso_FILE = NULL;
DWORD packed_iso_filesize = 0;
byte cd_sector_buffer[0x2000];	// contain really data
// DWORD pce_cd_read_datacnt;
byte cd_extra_super_mem[0x30000];
DWORD pce_cd_sectoraddy;
DWORD pce_cd_read_datacnt;
byte pce_cd_sectoraddress[3];
byte pce_cd_temp_dirinfo[4];
byte pce_cd_temp_play[4];
byte pce_cd_temp_stop[4];
byte *cd_read_buffer;
byte pce_cd_dirinfo[4];
int pce_enable_cdrom = FALSE;
// struct cdrom_tocentry pce_cd_tocentry;
byte nb_max_track = 24;	//(NO MORE BCD!!!!!)
//char cdsystem_path[256];
//extern char   *pCartName;
//extern char snd_bSound;
// Pre declaration of reading function routines
void read_sector_CD (unsigned char *, DWORD);







int snd_use_sound=1;

#define	VRR	2
enum _VDC_REG {
	MAWR,MARR,VWR,vdc3,vdc4,CR,RCR,BXR,
	BYR,MWR,HSR,HDR,VPR,VDW,VCR,DCR,
	SOUR,DISTR,LENR,SATB};
#define	NODATA	0xff
#define	ENABLE	1
#define	DISABLE	0

#define	VDC_CR	0x01
#define	VDC_OR	0x02
#define	VDC_RR	0x04
#define	VDC_DS	0x08
#define	VDC_DV	0x10
#define	VDC_VD	0x20
#define	VDC_BSY	0x40
#define	VDC_SpHit	VDC_CR
#define	VDC_Over	VDC_OR
#define	VDC_RasHit	VDC_RR
#define	VDC_InVBlank VDC_VD
#define	VDC_DMAfinish	VDC_DV
#define	VDC_SATBfinish	VDC_DS

#define	SpHitON		(io.VDC[CR].W&0x01)
#define	OverON		(io.VDC[CR].W&0x02)
#define	RasHitON	(io.VDC[CR].W&0x04)
#define	VBlankON	(io.VDC[CR].W&0x08)
#define	SpriteON	(io.VDC[CR].W&0x40)
#define	ScreenON	(io.VDC[CR].W&0x80)

#define SATBIntON	(io.VDC[DCR].W&0x01)
#define DMAIntON	(io.VDC[DCR].W&0x02)

#define	IRQ2	1
#define	IRQ1	2
#define	TIRQ	4

////#define ScrollX	io.scroll_x
////#define ScrollY io.scroll_y
#define	ScrollX	io.VDC[BXR].W
#define	ScrollY	io.VDC[BYR].W
int ScrollYDiff;
int	oldScrollX;
int	oldScrollY;
int	oldScrollYDiff;

typedef struct {
	short y,x,no,atr;
} SPR;


int CheckSprites(void);
int JoyStick(byte *JS);

void  IO_write(word A,byte V);
byte  IO_read(word A);
void RefreshLine(int Y1,int Y2);
void RefreshSprite(int Y1,int Y2);
void RefreshScreen(void);

void bank_set(byte P,byte V)
{

	if (ROMMap[V]==IOAREA)
		Page[P]=IOAREA;
	else
		Page[P]=ROMMap[V]-P*0x2000;

}


byte _Rd6502(word A)
{
	if (Page[A>>13]!=IOAREA) return Page[A>>13][A];
	else return IO_read(A);
}

void _Wr6502(word A,byte V)
{
	if(sf2ce) {
		if (Page[A >> 13] == IOAREA)
			IO_write (A, V);
		else if ((A & 0x1ffc) == 0x1ff0) {
			// support for SF2CE silliness //
			int i;
			ROMMap[0x40] = ROMMap[0] + 0x80000;
			ROMMap[0x40] += (A & 3) * 0x80000;
			for (i = 0x41; i <= 0x7f; i++) {
				ROMMap[i] = ROMMap[i - 1] + 0x2000;
			}
		} else {
			Page[A >> 13][A] = V;
		}
	} else {
		if (Page[A>>13]!=IOAREA)	{
			Page[A>>13][A]=V;
		}else IO_write(A,V);
	}
}


int cd_track_search(int m,int s,int f)
{
	int i;
	for(i=0;i<0x100;i++){
		if(	cd_toc[i].min==m &&
			cd_toc[i].sec==s &&
			cd_toc[i].fra==f
		){
			return	i;
		}
	}
	return 0;
}

int cd_toc_read(void)
{
	char	toc_buf[2048];
	char	*p;
	int	size,fd;
	
	_memset( cd_toc, 0, sizeof(struct_cd_toc)*0x100 );
	
	fd = sceIoOpen( ini_data.path_cd, O_RDONLY, 0777);
	if(fd<0){
		Error_mes("cd toc read error");
		return 1;
	}
	size = sceIoRead(fd, toc_buf, 2048);
	sceIoClose(fd);
	p=toc_buf;
	{
		int	n=0,f=0,c=0,s=0;
		while(size>0){
			if( *p>='0' && *p<='9' ){
				s*=10;
				s+=*p-'0';
				f=1;
			}else{
				if( f ){
					switch(c){
					case	0:				n = s;	c++;	break;
					case	1:	cd_toc[n].min = s;	c++;	break;
					case	2:	cd_toc[n].sec = s;	c++;	break;
					case	3:	cd_toc[n].fra = s;	c++;	break;
					case	4:	cd_toc[n].LBA = s;	c=0;	
						n++;
						if( n>=0x100 )size=0;
						break;
					}
				}
				if( !_memcmp( p, "Data",  4 ) )cd_toc[n].type=4;
				if( !_memcmp( p, "Audio", 5 ) )cd_toc[n].type=0;
				f=0;
				s=0;
			}
			p++;
			size--;
		}
		nb_max_track=n-1;
	}
	
	return 0;
}
void fill_cd_info()
{

  byte Min, Sec, Fra;
  byte current_track;

  // Track 1 is almost always a audio avertising track
  // 30 sec. seems usual

  CD_track[1].beg_min = binbcd[00];
  CD_track[1].beg_sec = binbcd[02];
  CD_track[1].beg_fra = binbcd[00];

  CD_track[1].type = 0;
  CD_track[1].beg_lsn = 0;	// Number of sector since the
  // beginning of track 1

  CD_track[1].length = 47 * CD_FRAMES + 65;

  // CD_track[0x01].length=53 * CD_FRAMES + 65;

  // CD_track[0x01].length=0 * CD_FRAMES + 16;

  nb_sect2msf (CD_track[1].length, &Min, &Sec, &Fra);

// Fra = CD_track[0x01].length % CD_FRAMES;
// Sec = (CD_track[0x01].length) % (CD_FRAMES * CD_SECS) / CD_SECS;
// Min = (CD_track[0x01].length) (CD_FRAMES * CD_SECS);

// Second track is the main code track

  CD_track[2].beg_min = binbcd[bcdbin[CD_track[1].beg_min] + Min];
  CD_track[2].beg_sec = binbcd[bcdbin[CD_track[1].beg_sec] + Sec];
  CD_track[2].beg_fra = binbcd[bcdbin[CD_track[1].beg_fra] + Fra];

  CD_track[2].type = 4;
  CD_track[2].beg_lsn =
    msf2nb_sect (bcdbin[CD_track[2].beg_min] - bcdbin[CD_track[1].beg_min],
		 bcdbin[CD_track[2].beg_sec] - bcdbin[CD_track[1].beg_sec],
		 bcdbin[CD_track[2].beg_fra] - bcdbin[CD_track[1].beg_fra]);
/*
  switch (CD_emulation)
    {
    case 2:
      CD_track[0x02].length = filesize (iso_FILE) / 2048;
      break;
    case 3:
      CD_track[0x02].length = 140000;
      break;
    default:
		break;
    }
*/
      CD_track[0x02].length = 140000;



  // Now most track are audio

  for (current_track = 3; current_track < bcdbin[nb_max_track];
       current_track++)
    {

      Fra = (byte) (CD_track[current_track - 1].length % CD_FRAMES);
      Sec = (byte) ((CD_track[current_track - 1].length / CD_FRAMES) % CD_SECS);
      Min = (byte) ((CD_track[current_track - 1].length / CD_FRAMES) / CD_SECS);

      CD_track[current_track].beg_min =
	binbcd[bcdbin[CD_track[current_track - 1].beg_min] + Min];
      CD_track[current_track].beg_sec =
	binbcd[bcdbin[CD_track[current_track - 1].beg_sec] + Sec];
      CD_track[current_track].beg_fra =
	binbcd[bcdbin[CD_track[current_track - 1].beg_fra] + Fra];

      CD_track[current_track].type = 0;
      CD_track[current_track].beg_lsn =
	msf2nb_sect (bcdbin[CD_track[current_track].beg_min] -
		     bcdbin[CD_track[1].beg_min],
		     bcdbin[CD_track[current_track].beg_sec] -
		     bcdbin[CD_track[1].beg_sec],
		     bcdbin[CD_track[current_track].beg_fra] -
		     bcdbin[CD_track[1].beg_fra]);
      // 1 min for all
      CD_track[current_track].length = 1 * CD_SECS * CD_FRAMES;

    }

  // And the last one is generally also code


  Fra = (byte) (CD_track[nb_max_track - 1].length % CD_FRAMES);
  Sec = (byte) ((CD_track[nb_max_track - 1].length / CD_FRAMES) % CD_SECS);
  Min = (byte) ((CD_track[nb_max_track - 1].length / CD_FRAMES) / CD_SECS);

  CD_track[nb_max_track].beg_min =
    binbcd[bcdbin[CD_track[nb_max_track - 1].beg_min] + Min];
  CD_track[nb_max_track].beg_sec =
    binbcd[bcdbin[CD_track[nb_max_track - 1].beg_sec] + Sec];
  CD_track[nb_max_track].beg_fra =
    binbcd[bcdbin[CD_track[nb_max_track - 1].beg_fra] + Fra];

  CD_track[nb_max_track].type = 4;
  CD_track[nb_max_track].beg_lsn =
    msf2nb_sect (bcdbin[CD_track[nb_max_track].beg_min] -
		 bcdbin[CD_track[1].beg_min],
		 bcdbin[CD_track[nb_max_track].beg_sec] -
		 bcdbin[CD_track[1].beg_sec],
		 bcdbin[CD_track[nb_max_track].beg_fra] -
		 bcdbin[CD_track[1].beg_fra]);

  /* Thank to Nyef for having localised a little bug there */
/*
  switch (CD_emulation)
    {
    case 2:
      CD_track[nb_max_track].length = filesize (iso_FILE) / 2048;
      break;
    case 3:
      CD_track[nb_max_track].length = 14000;
      break;
    default:
		break;
    }
*/

      CD_track[nb_max_track].length = 14000;


  return;
}



/*
int cd_track_search(int m,int s,int f)
{
	int i;
	for(i=0;i<0x100;i++){
		if(	cd_toc[i].min==m &&
			cd_toc[i].sec==s &&
			cd_toc[i].fra==f
					case	3:	cd_toc[n].LBA = s;	c=0;	
		){
			return	i+1;
		}
	}
	return 0;
}

int cd_toc_read(void)
{
	char	toc_buf[2048];
	char	*p;
	int	size,fd;
	
	_memset( cd_toc, 0, sizeof(struct_cd_toc)*0x100 );
	
	fd = sceIoOpen( ini_data.path_cd, O_RDONLY, 0777);
*/
//char	cd_name[]="ms0:/スーパーダライアス/Track No02.iso";
//char	cd_name[]="ms0:/1.iso";
void	cd_test_read(char *p,DWORD s)
{
	int fd;
	int i;
	char	n[1024];
	char	tn[3];
	char *wp;

	for(i=1;i<0x100;i++){
		if( cd_toc[i].LBA>s )break;
	}
	i--;
	tn[0]='0'+(i/10);
	tn[1]='0'+(i%10);
	tn[2]=0;
	_strcpy( n, ini_data.path_cd);
	wp=_strrchr( n, '/' );wp[1]=0;
	_strcat( n, tn);
	_strcat( n, ".iso");
//Error_mes(n);	
	fd = sceIoOpen( n,O_RDONLY, 0777);
	if(fd>=0){
		DWORD w;
		long long a;
		a = (s-cd_toc[i].LBA)*2048;
		w = sceIoLseek( fd, 0, 2);
		if( a<0 || w<a+2048 ){
			Error_mes("CDシークエラー");
		}else{
			w = sceIoLseek( fd, a, 0);
			sceIoRead(fd, p, 2048);
		}
		sceIoClose(fd);
	}else{
		Error_mes("CDオープンエラー");
		Error_mes(n);
	}
}
//	2トラック目 3590
DWORD first_sector = 0;
void read_sector_CD(unsigned char *p, DWORD sector)
{
  int i;
//Error_count(sector);

	mp3_play_stop();

  if (cd_buf != NULL)
    if ((sector >= first_sector) &&
	(sector <= first_sector + CD_BUF_LENGTH - 1))
      {
	_memcpy (p, cd_buf + 2048 * (sector - first_sector), 2048);
	return;
      }
    else
      {
	for (i = 0; i < CD_BUF_LENGTH; i++)
//	  osd_cd_read (cd_buf + 2048 * i, sector + i);
		cd_test_read(cd_buf + 2048 * i, sector + i);
	first_sector = sector;
	_memcpy (p, cd_buf, 2048);
      }
  else
    {
//      cd_buf = (byte *) malloc (CD_BUF_LENGTH * 2048);

      for (i = 0; i < CD_BUF_LENGTH; i++){
//		osd_cd_read (cd_buf + 2048 * i, sector + i);
		cd_test_read(cd_buf + 2048 * i, sector + i);
		}
      first_sector = sector;
      _memcpy (p, cd_buf, 2048);
    }

	
}


void pce_cd_read_sector(void)
{
	
	read_sector_CD( cd_sector_buffer, pce_cd_sectoraddy );
	
  /* Avoid sound jiggling when accessing some sectors */
  pce_cd_sectoraddy++;


  pce_cd_read_datacnt = 2048;
  cd_read_buffer = cd_sector_buffer;

  /* restore sound volume */
}

void	issue_ADPCM_dma (void)
{

  while (cd_sectorcnt--)
    {
      _memcpy (PCM + io.adpcm_dmaptr, cd_read_buffer, pce_cd_read_datacnt);

      cd_read_buffer = NULL;

      io.adpcm_dmaptr += (unsigned short) pce_cd_read_datacnt;

      pce_cd_read_datacnt = 0;

      pce_cd_read_sector ();

    }

  pce_cd_read_datacnt = 0;

  pce_cd_adpcm_trans_done = 1;

  cd_read_buffer = NULL;

}


void	pce_cd_set_sector_address (void)
{
  pce_cd_sectoraddy = pce_cd_sectoraddress[0] << 16;
  pce_cd_sectoraddy += pce_cd_sectoraddress[1] << 8;
  pce_cd_sectoraddy += pce_cd_sectoraddress[2];
}

/*
 *  convert logical_block_address to m-s-f_number (3 bytes only)
 *  lifted from the cdrom test program in the Linux kernel docs.
 *  hacked up to convert to BCD.
 */
#ifndef LINUX
#define CD_MSF_OFFSET 150
#endif

void lba2msf(int lba, unsigned char *msf)
{
  lba += CD_MSF_OFFSET;
  msf[0] = binbcd[lba / (CD_SECS * CD_FRAMES)];
  lba %= CD_SECS * CD_FRAMES;
  msf[1] = binbcd[lba / CD_FRAMES];
  msf[2] = binbcd[lba % CD_FRAMES];
}


DWORD msf2nb_sect(byte min, byte sec, byte frm)
{
  DWORD result = frm;
  result += sec * CD_FRAMES;
  result += min * CD_FRAMES * CD_SECS;
  return result;
}

void nb_sect2msf(DWORD lsn, byte * min, byte * sec, byte * frm)
{

  (*frm) = (byte) (lsn % CD_FRAMES);
  lsn /= CD_FRAMES;
  (*sec) = (byte) (lsn % CD_SECS);
  (*min) = (byte) (lsn / CD_SECS);

  return;
}

void pce_cd_handle_command(void)
{

	if (pce_cd_cmdcnt) {
		if (--pce_cd_cmdcnt)
			cd_port_1800 = 0xd0;
		else
			cd_port_1800 = 0xc8;

		switch (pce_cd_curcmd) {
		case 0x08:
			if (!pce_cd_cmdcnt) {
				cd_sectorcnt = cd_port_1801;
				pce_cd_set_sector_address ();
				pce_cd_read_sector ();


	      /* TEST */
	      // cd_port_1800 = 0xD0; // Xanadu 2 doesn't block but still crash
	      /* TEST */

/* TEST ZEO
    if (Rd6502(0x20ff)==0xfe)
     cd_port_1800 = 0x98;
    else
     cd_port_1800 = 0xc8;
 ******** */

			} else
				pce_cd_sectoraddress[3 - pce_cd_cmdcnt] = cd_port_1801;

			break;
		case 0xd8:

			pce_cd_temp_play[pce_cd_cmdcnt] = cd_port_1801;

			if (!pce_cd_cmdcnt) {
				cd_port_1800 = 0xd8;
			}
			break;

		case 0xd9:
			pce_cd_temp_stop[pce_cd_cmdcnt] = cd_port_1801;
			if (!pce_cd_cmdcnt) {
				cd_port_1800 = 0xd8;
/*
               if (pce_cd_temp_stop[3] == 1)
                 osd_cd_play_audio_track(bcdbin[pce_cd_temp_play[2]]);
               else
*/

				//	音楽再生？
				if ((pce_cd_temp_play[0] | pce_cd_temp_play[1] | pce_cd_temp_stop[0] | pce_cd_temp_stop[1]) == 0) {
					mp3_play_read_track( bcdbin[pce_cd_temp_play[2]] );
					mp3_play_init();
				}else{
					int s;
					if( (s=cd_track_search(bcdbin[pce_cd_temp_play[2]], bcdbin[pce_cd_temp_play[1]], bcdbin[pce_cd_temp_play[0]]))>0 ){
				//	if( (s=cd_track_search( pce_cd_temp_play[2], pce_cd_temp_play[1], pce_cd_temp_play[0] ))>0 ){
						mp3_play_read_track( s );
						mp3_play_init();
					}
				}

/*
				if ((pce_cd_temp_play[0] | pce_cd_temp_play[1] | pce_cd_temp_stop[0] | pce_cd_temp_stop[1]) == 0) {
					osd_cd_play_audio_track (bcdbin[pce_cd_temp_play[2]]);
				} else {
					osd_cd_play_audio_range (
						bcdbin[pce_cd_temp_play[2]], bcdbin[pce_cd_temp_play[1]], bcdbin[pce_cd_temp_play[0]], 
						bcdbin[pce_cd_temp_stop[2]], bcdbin[pce_cd_temp_stop[1]], bcdbin[pce_cd_temp_stop[0]]);
				}
*/


			//	LogDump("play from %d:%d:%d:(%d) to %d:%d:%d:(%d)\nloop = %d\n", bcdbin[pce_cd_temp_play[2]], bcdbin[pce_cd_temp_play[1]], bcdbin[pce_cd_temp_play[0]], pce_cd_temp_play[3], bcdbin[pce_cd_temp_stop[2]], bcdbin[pce_cd_temp_stop[1]], bcdbin[pce_cd_temp_stop[0]], pce_cd_temp_stop[3], pce_cd_temp_stop[3] == 1);
			}
			break;

		case 0xde:
			if (pce_cd_cmdcnt)
				pce_cd_temp_dirinfo[pce_cd_cmdcnt] = cd_port_1801;
			else {
	      // We have received two arguments in pce_cd_temp_dirinfo
	      // We can use only one
	      // There's an argument indicating the kind of info we want
	      // and an optional argument for track number

				pce_cd_temp_dirinfo[0] = cd_port_1801;

				switch (pce_cd_temp_dirinfo[1]) {
				case 0:
					// We want info on number of first and last track
/*
					switch (CD_emulation) {
					case 2:
					case 3:
						pce_cd_dirinfo[0] = binbcd[01];	// Number of first track  (BCD)
						pce_cd_dirinfo[1] = binbcd[nb_max_track];	// Number of last track (BCD)
						break;
					case 1:
						{
							int first_track, last_track;
//							osd_cd_nb_tracks (&first_track, &last_track);
							pce_cd_dirinfo[0] = binbcd[first_track];
							pce_cd_dirinfo[1] = binbcd[last_track];
						}
						break;
					}		// switch CD emulation
*/
					pce_cd_dirinfo[0] = binbcd[1];
					pce_cd_dirinfo[1] = binbcd[nb_max_track];
//Error_mes("binbcd");

				cd_read_buffer = pce_cd_dirinfo;
				pce_cd_read_datacnt = 2;
				break;

				case 2:
					// We want info on the track whose number is pce_cd_temp_dirinfo[0]
/*
					switch (CD_emulation) {
					case 2:
					case 3:
						pce_cd_dirinfo[0] = CD_track[bcdbin[pce_cd_temp_dirinfo[0]]].beg_min;
						pce_cd_dirinfo[1] = CD_track[bcdbin[pce_cd_temp_dirinfo[0]]].beg_sec;
						pce_cd_dirinfo[2] = CD_track[bcdbin[pce_cd_temp_dirinfo[0]]].beg_fra;
						pce_cd_dirinfo[3] = CD_track[bcdbin[pce_cd_temp_dirinfo[0]]].type;
						break;
					case 1:
						{
							int Min, Sec, Fra, Ctrl;
							byte *buffer = (byte *) alloca (7);
//							osd_cd_track_info (bcdbin[pce_cd_temp_dirinfo[0]], &Min, &Sec, &Fra, &Ctrl);
							pce_cd_dirinfo[0] = binbcd[Min];
							pce_cd_dirinfo[1] = binbcd[Sec];
							pce_cd_dirinfo[2] = binbcd[Fra];
							pce_cd_dirinfo[3] = Ctrl;
						//	LogDump("The control byte of the audio track #%d is 0x%02X\n", bcdbin[pce_cd_temp_dirinfo[0]], pce_cd_dirinfo[3]);
							break;
						}		// case CD emulation = 1
					}		// switch CD emulation
*/
						pce_cd_dirinfo[0] = binbcd[cd_toc[bcdbin[pce_cd_temp_dirinfo[0]]].min];
						pce_cd_dirinfo[1] = binbcd[cd_toc[bcdbin[pce_cd_temp_dirinfo[0]]].sec];
						pce_cd_dirinfo[2] = binbcd[cd_toc[bcdbin[pce_cd_temp_dirinfo[0]]].fra];
						pce_cd_dirinfo[3] = binbcd[cd_toc[bcdbin[pce_cd_temp_dirinfo[0]]].type];
//Error_mes("pce_cd_temp_dirinfo");
					pce_cd_read_datacnt = 3;
					cd_read_buffer = pce_cd_dirinfo;
					break;

				case 1:
/*
					switch (CD_emulation) {
					case 1:
						{
							int min, sec, fra;
							osd_cd_length (&min, &sec, &fra);
							pce_cd_dirinfo[0] = binbcd[min];
							pce_cd_dirinfo[1] = binbcd[sec];
							pce_cd_dirinfo[2] = binbcd[fra];
							break;
						}		// case Cd emulation = 1

					default:
						pce_cd_dirinfo[0] = 0x25;
						pce_cd_dirinfo[1] = 0x06;
						pce_cd_dirinfo[2] = 0x00;
					}		// switch CD emulation
*/
						pce_cd_dirinfo[0] = 0x25;
						pce_cd_dirinfo[1] = 0x06;
						pce_cd_dirinfo[2] = 0x00;
//Error_mes("pce_cd_dirinfo");

					pce_cd_read_datacnt = 3;
					cd_read_buffer = pce_cd_dirinfo;
					break;
				}		// switch command of request 0xde
			}			// end if of request 0xde (receiving command or executing them)
		}			// switch of request
	}				// end if of command arg or new request
	else
	{
		// it's an command ID we're receiving
		switch (cd_port_1801) {
		case 0x00:
			cd_port_1800 = 0xD8;
			break;
		case 0x08:
			pce_cd_curcmd = cd_port_1801;
			pce_cd_cmdcnt = 4;
			break;
		case 0xD8:
			pce_cd_curcmd = cd_port_1801;
			pce_cd_cmdcnt = 4;
			break;
		case 0xD9:
			pce_cd_curcmd = cd_port_1801;
			pce_cd_cmdcnt = 4;
			break;
		case 0xDA:
			pce_cd_curcmd = cd_port_1801;
			pce_cd_cmdcnt = 0;
/*
			if (CD_emulation == 1)
				osd_cd_stop_audio ();
*/
		//	if (CD_emulation == 1)
				mp3_play_stop();
			break;
		case 0xDE:
			/* Get CD directory info */
			/* First arg is command? */
			/* Second arg is track? */
			cd_port_1800 = 0xd0;
			pce_cd_cmdcnt = 2;
			pce_cd_read_datacnt = 3;	/* 4 bytes */
			pce_cd_curcmd = cd_port_1801;
			break;
		}

/*
        if (cd_port_1801 == 0x00) {
            cd_port_1800 = 0xd8;
        } else if (cd_port_1801 == 0x08) {
            pce_cd_curcmd = cd_port_1801;
            pce_cd_cmdcnt = 4;
        } else if (cd_port_1801 == 0xd8) {
            pce_cd_cmdcnt = 4;
            pce_cd_curcmd = cd_port_1801;
        } else if (cd_port_1801 == 0xd9) {
            pce_cd_cmdcnt = 4;
            pce_cd_curcmd = cd_port_1801;
        } else if (cd_port_1801 == 0xde) {
            // Get CD directory info
            // First arg is command?
            // Second arg is track?
            cd_port_1800 = 0xd0;
            pce_cd_cmdcnt = 2;
            pce_cd_read_datacnt = 3; // 4 bytes
            pce_cd_curcmd = cd_port_1801;
        }
*/

	}
}







void  IO_write(word A,byte V)
{
	//printf("w%04x,%02x ",A&0x3FFF,V);
  switch(A&0x1C00) {
  case 0x0000:	/* VDC */
	switch(A&3){
	case 0: io.vdc_reg = V&31; return;
	case 1: return;
	case 2:
		//printf("vdc_l%d,%02x ",io.vdc_reg,V);
		switch(io.vdc_reg){
		case VWR:
			/*VRAM[io.VDC[MAWR].W*2] = V;*/ io.vdc_ratch = V;
			//io.VDC_ratch[VWR] = V;
/*			if (0x1000 <= io.VDC[MAWR].W && io.VDC[MAWR].W < 0x1008)
			{
				TRACE("L: PC = %X, ", M.PC.W);
				for (int i = -10; i < 4; i++)
					TRACE("%02X ", Page[(M.PC.W+i)>>13][M.PC.W+i]);
				TRACE("\nL: V[%X] = %X\n", io.VDC[MAWR].W, V);
			}
*/			return;
		case HDR:
			io.screen_w = (V+1)*8;
			//printf("screen:%dx%d %d\n",io.screen_w,256,V);
			//TRACE("HDRl: %X\n", V);
			break;
		case MWR:
			{static byte bgw[]={32,64,128,128};
			io.bg_h=(V&0x40)?64:32;
			io.bg_w=bgw[(V>>4)&3];}
		//	TRACE("bg:%dx%d, V:%X\n",io.bg_w,io.bg_h, V);
			//TRACE("MWRl: %02X\n", V);
			break;
		case BYR:
			if (!scroll) {
				oldScrollX = ScrollX;
				oldScrollY = ScrollY;
				oldScrollYDiff = ScrollYDiff;
			}
			io.VDC[BYR].B.l = V;
			scroll=1;
//			io.vdc_iswrite_h = 0;
//			if (scanline <= MaxLine)
//			{
				//TRACE("BYRl = %d, scanline = %d, h = %d\n", io.VDC[BYR].W, scanline, io.screen_h);
			//if (RasHitON)
				ScrollYDiff=scanline-1;
//			}
//			else
//				ScrollYDiff=0;
			//TRACE("BYRl = %d, scanline = %d, h = %d\n", io.VDC[BYR].W, scanline, io.screen_h);
			return;
		case BXR:
			if (!scroll) {
				oldScrollX = ScrollX;
				oldScrollY = ScrollY;
				oldScrollYDiff = ScrollYDiff;
			}
			io.VDC[BXR].B.l = V;
			scroll=1;
			return;
/*			
//#define PRINT_VDC_L(REG)	case REG: TRACE(#REG "l: %X\n",V);break;
#define PRINT_VDC_L(REG)	case REG: Error_count(V);break;
			PRINT_VDC_L(VPR);
			PRINT_VDC_L(VDW);
			PRINT_VDC_L(VCR);
			PRINT_VDC_L(DCR);
		case	VPR:	break;
		case	VDW:	break;
		case	VCR:	break;
		case	DCR:	break;
*/
		}
		io.VDC[io.vdc_reg].B.l = V; //io.VDC_ratch[io.vdc_reg] = V;
//		if (io.vdc_reg != CR)
//			TRACE("vdc_l: %02X,%02X\n", io.vdc_reg, V);
/*
		if (io.vdc_reg>19) {
			TRACE("ignore write lo vdc%d,%02x\n",io.vdc_reg,V);
		}
*/
		return;
	case 3:
		//printf("vdc_h%d,%02x ",io.vdc_reg,V);
		switch(io.vdc_reg){
		case VWR:
			//printf("v%04x\n",io.VDC[MAWR].W);
			VRAM[io.VDC[MAWR].W*2]=io.vdc_ratch;
			VRAM[io.VDC[MAWR].W*2+1]=V;
/*			if (0x1000<=io.VDC[MAWR].W&&io.VDC[MAWR].W<0x1008)
			{
				TRACE("adr=%04X,val=%02X%02X\n", io.VDC[MAWR].W, V, io.vdc_ratch);
				//DebugDumpFp(4, TRUE);
			}
*/
/*			if (0x1000 <= io.VDC[MAWR].W && io.VDC[MAWR].W < 0x1100)
			{
				TRACE("PC = %X, ", M.PC.W);
				for (int i = -10; i < 4; i++)
					TRACE("%02X ", Page[(M.PC.W+i)>>13][M.PC.W+i]);
				TRACE("\nPage = %d", (Page[(M.PC.W)>>13] - ROM)/0x2000);
				TRACE("\nV[%X] = %02X%02X\n", io.VDC[MAWR].W, VRAM[io.VDC[MAWR].W*2+1], VRAM[io.VDC[MAWR].W*2]);
				TRACE("Page: ");
				for (i = 0; i < 8; i++)
					TRACE("%02X ", M.MPR[i]);
				TRACE("\nZero: ");
				for (i = 0; i < 32; i++)
					TRACE("%02X ", RAM[i]);
				TRACE("\n");
			}
*/			vchange[io.VDC[MAWR].W/16]=1;
#if defined(USE_SPRITE_CACHE)
			vchanges[io.VDC[MAWR].W/64]=1;
#endif //defined(USE_SPRITE_CACHE)
			io.VDC[MAWR].W+=io.vdc_inc;
			io.vdc_ratch=0;
			return;
		case VDW:
			//io.VDC[VDW].B.l = io.VDC_ratch[VDW];
			io.VDC[VDW].B.h = V;
			io.screen_h = (io.VDC[VDW].W&511)+1;
			MaxLine = io.screen_h-1;
		//	TRACE("VDWh: %X\n", io.VDC[VDW].W);
			return;
		case LENR:
			//io.VDC[LENR].B.l = io.VDC_ratch[LENR];
			io.VDC[LENR].B.h = V;
		//	TRACE("DMA:%04x %04x %04x\n",io.VDC[DISTR].W,io.VDC[SOUR].W,io.VDC[LENR].W);
			/* VRAM to VRAM DMA */
			_memcpy(VRAM+io.VDC[DISTR].W*2,VRAM+io.VDC[SOUR].W*2,(io.VDC[LENR].W+1)*2);
			_memset(vchange+io.VDC[DISTR].W/16,1,(io.VDC[LENR].W+1)/16);
			_memset(vchange+io.VDC[DISTR].W/64,1,(io.VDC[LENR].W+1)/64);
			io.VDC[DISTR].W += io.VDC[LENR].W+1;
			io.VDC[SOUR].W += io.VDC[LENR].W+1;
			io.vdc_status|=VDC_DMAfinish;
//System_mes("LENR");
			return;
		case CR :
			{static byte incsize[]={1,32,64,128};
			io.vdc_inc = incsize[(V>>3)&3];
			//TRACE("CRh: %02X\n", V);
			}
			break;
		case HDR:
			//io.screen_w = (io.VDC_ratch[HDR]+1)*8;
		//	TRACE0("HDRh\n");
			break;
		case BYR:
			if (!scroll) {
				oldScrollX = ScrollX;
				oldScrollY = ScrollY;
				oldScrollYDiff = ScrollYDiff;
			}
			io.VDC[BYR].B.h = V&1;
			scroll=1;
//			io.vdc_iswrite_h = 1;
//			if (scanline <= MaxLine)
//			{
				//TRACE("BYRh = %d, scanline = %d, h = %d\n", io.VDC[BYR].W, scanline, io.screen_h);
			//if (RasHitON)
			{
				ScrollYDiff=scanline-1;
				//TRACE("BYRh = %d, scanline = %d, h = %d\n", io.VDC[BYR].W, scanline, io.screen_h);
				//DebugDumpFp(4, TRUE);
			}
//			}
//			else
//				ScrollYDiff=0;
			//ScrollY = 51-35;
			return;
		case SATB:
			io.VDC[SATB].B.h = V;
			//TRACE("SATB=%X,scanline=%d\n", io.VDC[SATB].W, scanline);
			//memcpy(SPRAM,VRAM+io.VDC[SATB].W*2,64*8);
			io.vdc_satb=1;
			io.vdc_status&=~VDC_SATBfinish;
			return;
		case BXR:
			if (!scroll) {
				oldScrollX = ScrollX;
				oldScrollY = ScrollY;
				oldScrollYDiff = ScrollYDiff;
			}
			io.VDC[BXR].B.h = V & 3;
			scroll=1;
//			ScrollX = io.VDC[BXR].W;
//			TRACE("BXRh = %d, scanline = %d\n", io.VDC[BXR].W, scanline);
//			io.VDC[BXR].W = 256;
			return;
/*			
//#define PRINT_VDC_H(REG)	case REG: TRACE(#REG "h: %X\n",V);break;
			PRINT_VDC_H(VPR);
			PRINT_VDC_H(VCR);
			PRINT_VDC_H(DCR);
		case	VPR:	break;
		case	VCR:	break;
		case	DCR:	break;
		//case RCR: TRACE("RCR: %02X%02X\n", V, io.VDC[io.vdc_reg].B.l);break;
*/
		}
		//io.VDC[io.vdc_reg].B.l = io.VDC_ratch[io.vdc_reg];
		io.VDC[io.vdc_reg].B.h = V;
//		if (io.vdc_reg != CR)
//			TRACE("vdc_h: %02X,%02X\n", io.vdc_reg, V);
/*
		if (io.vdc_reg>19) {
			TRACE("ignore write hi vdc%d,%02x\n",io.vdc_reg,V);
		}
*/
		return;
	}
	break;

  case 0x0400:	/* VCE */
	switch(A&7) {
	case 0: /*io.vce_reg.W = 0; io.vce_ratch=0;*/ /*??*/
		io.vce_cr = V;
	//	TRACE("VCE 0, V=%X\n", V); 
		return;
	case 1:			return;
	case 2: io.vce_reg.B.l = V; return;
	case 3: io.vce_reg.B.h = V&1; return;
	case 4: io.VCE[io.vce_reg.W].B.l= V;
		{
			byte c; int i,n;
		n = io.vce_reg.W;
		c = io.VCE[n].W>>1;
		if (n==0) {
			for(i=0;i<256;i+=16) Pal[i]=c;
		}else if (n&15) Pal[n] = c;
		}
		return;
	case 5:
		/*io.VCE[io.vce_reg.W].B.l = io.vce_ratch;*/
//	DebugDumpFp(4, TRUE);
		io.VCE[io.vce_reg.W].B.h = V;
		{
			byte c; int i,n;
			n = io.vce_reg.W;
			c = io.VCE[n].W>>1;
			if (n==0) {
				for(i=0;i<256;i+=16) Pal[i]=c;
			}else{
				if (n&15) Pal[n] = c;
			}
		}
		io.vce_reg.W=(io.vce_reg.W+1)&0x1FF;
		return;
	//case 6: /* ?? */ return;
	case 6:			return;
	case 7:			return;

	}
	break;

  case 0x0800:	/* PSG */
	if( ini_data.pcm )
		if (snd_use_sound && io.psg_ch < 6) {
			if ((A&15) <= 1) {
				int	i;
				for (i = 0; i < 6; i++)
					write_psg(i);
			} else
				write_psg(io.psg_ch);
		}
	
	//	System_mes("PSG");

  	switch(A&15){
	case 0: io.psg_ch = V&7; return;
	case 1: io.psg_volume = V; return;
	case 2: io.PSG[io.psg_ch][2] = V; break;
	case 3: io.PSG[io.psg_ch][3] = V&15; break;
	case 4: io.PSG[io.psg_ch][4] = V; break;
	case 5: io.PSG[io.psg_ch][5] = V; break;
	case 6: if (io.PSG[io.psg_ch][4]&0x40){
				io.wave[io.psg_ch][0]=V&31;
			}else {
				io.wave[io.psg_ch][io.wavofs[io.psg_ch]]=V&31;
				io.wavofs[io.psg_ch]=(io.wavofs[io.psg_ch]+1)&31;
			} break;
	case 7: io.PSG[io.psg_ch][7] = V; break;
	case 8: io.psg_lfo_freq = V; break;
	case 9: io.psg_lfo_ctrl = V; break;
	default: //TRACE("ignored PSG write\n");
				System_mes("ignored PSG write");
    }
    return;

  case 0x0c00:	/* timer */
	//TRACE("Timer Access: A=%X,V=%X\n", A, V);
	switch(A&1){
	case 0: io.timer_reload = V&127; return;
	case 1: 
		V&=1;
		if (V && !io.timer_start)
			io.timer_counter = io.timer_reload;
		io.timer_start = V;
		//		System_mes("タイマー");
		return;
	}
	break;

  case 0x1000:	/* joypad */
//	  TRACE("V=%02X\n", V);
		io.joy_select = V&1;
		if (V&2) io.joy_counter = 0;
		return;

  case 0x1400:	/* IRQ */
	switch(A&3){
	case 2: io.irq_mask = V;/*TRACE("irq_mask = %02X\n", V);*/ return;
	case 3: io.irq_status= (io.irq_status&~TIRQ)|(V&0xF8); return;
	}
	break;
/*
  case 0x1800:	// CD-ROM extention 
	switch(A&15){
	case 7: io.backup = ENABLE; return;
	case 8: io.adpcm_ptr.B.l = V; return;
	case 9: io.adpcm_ptr.B.h = V; return;
	case 0xa: PCM[io.adpcm_wptr++] = V; return;
	case 0xd:
		if (V&4) io.adpcm_wptr = io.adpcm_ptr.W;
		else { io.adpcm_rptr = io.adpcm_ptr.W; io.adpcm_firstread = TRUE; }
		return;
	}
	break;
*/
	case 0x1800:	/* CD-ROM extention */
		switch(A&15){
		case 7: io.backup = ENABLE; return;
	/*	case 8: io.adpcm_ptr.B.l = V; return;
		case 9: io.adpcm_ptr.B.h = V; return;
		case 0xa: PCM[io.adpcm_wptr++] = V; return;
		case 0xd:
			if (V&4) io.adpcm_wptr = io.adpcm_ptr.W;
			else { io.adpcm_rptr = io.adpcm_ptr.W; io.adpcm_firstread = TRUE; }
			return;
	*/

		case 0: if (V == 0x81) cd_port_1800 = 0xD0; return;

		case 1:
			cd_port_1801 = V;
			if (!pce_cd_cmdcnt) {
				switch (V) {
				case 0x81:	// Another Reset?
					cd_port_1800 = 0x40;
					return;
				case 0:		// RESET?
				case 3:		// Get System Status?
				case 8:		// Read Sector
				case 0xD8:	// Play Audio?
				case 0xD9:	// Play Audio?
				case 0xDA:	// Pause Audio?
				case 0xDD:	// Read Q Channel?
				case 0xDE:	// Get Directory Info?
				default:
					return;
				}
			}
			return;
		case 2:
//Error_mes("cd read2");
			if ((!(cd_port_1802 & 0x80)) && (V & 0x80)) {
				cd_port_1800 &= ~0x40;
			} else if ((cd_port_1802 & 0x80) && (!(V & 0x80))) {
				cd_port_1800 |= 0x40;
				if (pce_cd_adpcm_trans_done) {
					cd_port_1800 |= 0x10;
					pce_cd_curcmd = 0x00;
					pce_cd_adpcm_trans_done = 0;
				}

				if (cd_port_1800 & 0x08) {
					if (cd_port_1800 & 0x20) {
						cd_port_1800 &= ~0x80;
					} else if (!pce_cd_read_datacnt) {
						if (pce_cd_curcmd == 0x08) {
							if (!--cd_sectorcnt) {
								cd_port_1800 |= 0x10;	/* wrong */
								pce_cd_curcmd = 0x00;
							} else {
								pce_cd_read_sector ();
							}
						} else {
							if (cd_port_1800 & 0x10) {
								cd_port_1800 |= 0x20;
							} else {
								cd_port_1800 |= 0x10;
							}
						}
					} else {
						pce_cd_read_datacnt--;
					}
				} else {
					pce_cd_handle_command ();
				}
			}

			cd_port_1802 = V;
			return;

		case 4:
			if (V & 2) {
				// Reset asked
				// do nothing for now
				switch (CD_emulation) {
				case 1:
/*
					if (osd_cd_init (ISO_filename) != 0) {
					//	LogDump("CD rom drive couldn't be initialised\n");
					//	SendNotifyMessage(mainwnd_hwnd, WM_CLOSE, 0, 0);
					}
*/
					break;
				case 2:
				case 3:
					fill_cd_info ();
					break;
	/*
	                case 5:
	                     fill_HCD_info(ISO_);
	                     break;
	*/
				}
					CD_emulation=1;
					cd_toc_read();
					fill_cd_info ();
//					Error_mes("fill_cd_info");

				_Wr6502 (0x222D, 1);
		      // This byte is set to 1 if a disc if present

		      //cd_port_1800 &= ~0x40;
				cd_port_1804 = V;
			} else {
				// Normal utilisation
				cd_port_1804 = V;
				// cd_port_1800 |= 0x40; // Maybe the previous reset is enough
				// cd_port_1800 |= 0xD0;
				// Indicates that the Hardware is ready after such a reset
			}
			return;

		case 8:
			io.adpcm_ptr.B.l = V;
			return;

		case 9:
			io.adpcm_ptr.B.h = V;
			return;

		case 0x0A:
			PCM[io.adpcm_wptr++] = V;
			return;

		case 0x0B:		// DMA enable ?
			if ((V & 2) && (!(cd_port_180b & 2))) {
				issue_ADPCM_dma ();
				cd_port_180b = V;
				return;
			}
			/* TEST */
			if (!V) {
				cd_port_1800 &= ~0xF8;
				cd_port_1800 |= 0xD8;
			}
			cd_port_180b = V;
			return;

		case 0x0C:		/* TEST, not nyef code */
			// well, do nothing
			return;

		case 0x0D:
			if ((V & 0x03) == 0x03) {
				io.adpcm_dmaptr = io.adpcm_ptr.W;	// set DMA pointer
			}

			if (V & 0x04) {
				io.adpcm_wptr = io.adpcm_ptr.W;	// set write pointer
			}

			if (V & 0x08) {		// set read pointer
				io.adpcm_rptr = io.adpcm_ptr.W;
				io.adpcm_firstread = 2;
			}

			if (V & 0x80) {			// ADPCM reset
			} else {			// Normal ADPCM utilisation
		    }
			return;

		case 0xe:		// Set ADPCM playback rate
			io.adpcm_rate = 32 / (16 - (V & 15));
			return;

		case 0xf:		// don't know how to use it
			cd_fade = V;
		counter( 50, 70, cd_fade, 7, -1);
			return;
		}			// A&15 switch, i.e. CD ports
		break;
	}
//	TRACE("ignore I/O write %04x,%02x\n",A,V);
//	DebugDumpTrace(4, TRUE);
}


/* read */
byte	IO_read(word A)
{
	byte ret;
	//printf("r%04x ",A&0x3FFF);
  switch(A&0x1CC0){
  case 0x0000: /* VDC */
  	switch(A&3){
	case 0:
		ret = io.vdc_status;
		io.vdc_status=0;//&=VDC_InVBlank;//&=~VDC_BSY;
		return ret;
	case 1:
		return 0;
	case 2:
		if (io.vdc_reg==VRR)
			return VRAM[io.VDC[MARR].W*2];
		else return io.VDC[io.vdc_reg].B.l;
	case 3:
		if (io.vdc_reg==VRR) {
			ret = VRAM[io.VDC[MARR].W*2+1];
			io.VDC[MARR].W+=io.vdc_inc;
//			TRACE0("VRAM read\n");
			return ret;
		} else return io.VDC[io.vdc_reg].B.h;
	}
	break;

  case 0x0400:/* VCE */
  	switch(A&7){
	case 4: return io.VCE[io.vce_reg.W].B.l;
	case 5: return io.VCE[io.vce_reg.W++].B.h;
	}
	break;

  case 0x0800:	/* PSG */
	switch(A&15){
	case 0: return io.psg_ch;
	case 1: return io.psg_volume;
	case 2: return io.PSG[io.psg_ch][2];
	case 3: return io.PSG[io.psg_ch][3];
	case 4: return io.PSG[io.psg_ch][4];
	case 5: return io.PSG[io.psg_ch][5];
	case 6:
		{
			int	ofs=io.wavofs[io.psg_ch];
			io.wavofs[io.psg_ch]=(io.wavofs[io.psg_ch]+1)&31;
			return io.wave[io.psg_ch][ofs];
		}
	case 7: return io.PSG[io.psg_ch][7];
	case 8: return io.psg_lfo_freq;
	case 9: return io.psg_lfo_ctrl;
	default: return NODATA;
	}
	break;

  case 0x0c00:	/* timer */
	switch(A&1){
	case 0:
		return io.timer_counter;
	}
	break;

	case 0x1000:	/* joypad */
//	  TRACE("js=%d\n", io.joy_counter)
//Error_count(io.joy_counter);
		ret = io.JOY[io.joy_counter]^0xff;
		if (io.joy_select&1) ret>>=4;
		else { ret&=15; io.joy_counter=(io.joy_counter+1)%5; }
		return ret;//|Country; /* country 0:JPN 1=US */

  case 0x1400:	/* IRQ */
	switch(A&3){
	case 2: return io.irq_mask;
	case 3: ret = io.irq_status;io.irq_status=0;return ret;
	}
	break;

	case 0x18C0:		// Memory management ?
		switch (A & 15) {
		case 5:
		case 1:
			return 0xAA;
		case 2:
		case 6:
			return 0x55;
		case 3:
		case 7:
			return 0x03;
		}
		break;
/*
  case 0x1800:	// CD-ROM extention 
	switch(A&15){
	case 3: return io.backup = DISABLE;
//	case 0xa: 
//		if (!io.adpcm_firstread) return PCM[io.adpcm_rptr++];
//		else {io.adpcm_firstread=FALSE; return NODATA;}
	}
	break;
*/
	case 0x1800:	/* CD-ROM extention */
		switch(A&15){
		case 0: return cd_port_1800;	// return 0x40; // ready ?
		case 1:
			{
				byte retval;

				if (cd_read_buffer) {
					retval = *cd_read_buffer++;
					if (pce_cd_read_datacnt == 2048) {
						pce_cd_read_datacnt--;
					}
					if (!pce_cd_read_datacnt)
						cd_read_buffer = 0;
				} else
					retval = 0;
				return retval;
			}

		case 2: return cd_port_1802;	// Test
	//	case 3: return io.backup = DISABLE;
		case 3:
			{
				static byte tmp_res = 0x02;
				tmp_res = 0x02 - tmp_res;
				io.backup = DISABLE;
	/* TEST */// return 0x20;
				return tmp_res | 0x20;
			}
		case 4: return cd_port_1804;	// Test
		case 5: return 0x50;			// Test
		case 6: return 0x05;			// Test
		case 0x0A:
//Error_mes("pcm");
			if (!io.adpcm_firstread)
				return PCM[io.adpcm_rptr++];
			else {
				io.adpcm_firstread--;
				return NODATA;
			}

		case 0x0B: return 0x00;			// Test
		case 0x0C: return 0x01;			// Test
		case 0x0D: return 0x00;			// Test
		case 8:
//Error_mes("cd read");
			if (pce_cd_read_datacnt) {
				byte retval;
				if (cd_read_buffer) {
					retval = *cd_read_buffer++;
				} else
					retval = 0;
				if (!--pce_cd_read_datacnt) {
					cd_read_buffer = 0;
					if (!--cd_sectorcnt) {
						cd_port_1800 |= 0x10;
						pce_cd_curcmd = 0;
					} else {
						pce_cd_read_sector ();
					}
				}
				return retval;
			}
			break;
		}
	}
	return NODATA;
}

inline int min(int x1, int x2){ return (x1<x2)? x1: x2; }


	static int skip_frame;
byte Loop6502(M6502 *R)
{
	static int UCount = 0;
	static int ACount = 0;
	int dispmin, dispmax;
	int ret;
	//printf("PC:%04x ",R->PC.W);

	dispmin = (MaxLine-MinLine>MAXDISP ? MinLine+((MaxLine-MinLine-MAXDISP+1)>>1) : MinLine);
	dispmax = (MaxLine-MinLine>MAXDISP ? MaxLine-((MaxLine-MinLine-MAXDISP+1)>>1) : MaxLine);

	scanline=(scanline+1)%scanlines_per_frame;
	//printf("scan %d\n",scanline);
	ret = INT_NONE;
	io.vdc_status&=~VDC_RasHit;
	if (scanline>MaxLine)
		io.vdc_status|=VDC_InVBlank;
//	if (scanline==MinLine+scanlines_per_frame-1)
//	else 
	if (scanline==MinLine) {
		io.vdc_status&=~VDC_InVBlank;
		prevline=dispmin;
		ScrollYDiff = 0;
		oldScrollYDiff = 0;
//		skip_frame = skip_next_frame;
//		skip_next_frame = 0;
//		if (io.vdc_iswrite_h)
//		{
//			io.vdc_iswrite_h = 0;
//			ScrollY = io.VDC[BYR].W;
//		}
//		TRACE("\nFirstLine\n");
	}else
	if (scanline==MaxLine) {
		if (CheckSprites()) io.vdc_status|=VDC_SpHit;
		else io.vdc_status&=~VDC_SpHit;
		if (UCount) UCount--;
		else if (!skip_frame) {
			if (prevline<dispmax) {
				RefreshLine(prevline,dispmax);
				if (SpriteON && SPONSwitch) RefreshSprite(prevline,dispmax+1);
			}
			prevline=dispmax+1;
			UCount=UPeriod;

//	if( ScreenON )
	{
			RefreshScreen();
			{
				static int c=0;
				if( (c++)&63 )
					pgScreenFlip();
				else
					pgScreenFlipV();
			}
	}

		}
		exit_flag=1;
		io.vdc_status|=VDC_InVBlank;
	}
	if (scanline>=MinLine && scanline<=MaxLine) {
		if (scanline==(io.VDC[RCR].W&1023)-64) {
			if (RasHitON && !UCount && dispmin<=scanline && scanline<=dispmax && !skip_frame) {
				if (prevline<dispmax) {
					RefreshLine(prevline,scanline-1);
					if (SpriteON && SPONSwitch) RefreshSprite(prevline,scanline);
				}
				prevline=scanline;
			}
			io.vdc_status|=VDC_RasHit;
			if (RasHitON) {
				//TRACE("rcr=%d\n", scanline);
				ret = INT_IRQ;
			}
		} else if (scroll) {
			if (scanline-1>prevline && !UCount && !skip_frame) {
				int	tmpScrollX, tmpScrollY, tmpScrollYDiff;
				tmpScrollX=ScrollX;
				tmpScrollY=ScrollY;
				tmpScrollYDiff=ScrollYDiff;
				ScrollX=oldScrollX;
				ScrollY=oldScrollY;
				ScrollYDiff=oldScrollYDiff;
				RefreshLine(prevline,scanline-2);
				if (SpriteON && SPONSwitch) RefreshSprite(prevline,scanline-1);
				prevline=scanline-1;
				ScrollX = tmpScrollX;
				ScrollY = tmpScrollY;
				ScrollYDiff = tmpScrollYDiff;
			}
		}
	} else {
		int rcr = (io.VDC[RCR].W&1023)-64;
		if (scanline==rcr){
//			ScrollYDiff = scanline;
			if (RasHitON) {
				//TRACE("rcr=%d\n", scanline);
				io.vdc_status |= VDC_RasHit;
				ret = INT_IRQ;
			}
		}
	}
	scroll=0;
	if (scanline==MaxLine+1) {
//	if (scanline==scanlines_per_frame-63) {
	//	int result=JoyStick(io.JOY);
	//	if (result&0x10000) return INT_QUIT;

		/* VRAM to SATB DMA */
		if (io.vdc_satb==1 || io.VDC[DCR].W&0x0010)	{
			_memcpy(SPRAM,VRAM+io.VDC[SATB].W*2,64*8);
			io.vdc_satb=1;
			io.vdc_status&=~VDC_SATBfinish;
		}
		if (ret==INT_IRQ)
			io.vdc_pendvsync = 1;
		else {
		//	System_mes("vsync");
			//TRACE("vsync\n");
			//io.vdc_status|=VDC_InVBlank;
			if (VBlankON) {
				//TRACE("vsync=%d\n", scanline);
				ret = INT_IRQ;
			}
		}
	}
	else 
	if (scanline==min(MaxLine+5, scanlines_per_frame-1)) {
		if (io.vdc_satb) {
			io.vdc_status|=VDC_SATBfinish;
			io.vdc_satb = 0;
			if (SATBIntON) {
				//TRACE("SATB=%d\n", scanline);
				ret = INT_IRQ;
			}
/*		} else {
			io.vdc_status&=~VDC_SATBfinish;
			io.vdc_satb = 0;
*/		}
	} else if (io.vdc_pendvsync && ret!=INT_IRQ) {
		io.vdc_pendvsync = 0;
		//io.vdc_status|=VDC_InVBlank;
		if (VBlankON) {
			//TRACE("vsync=%d\n", scanline);
			ret = INT_IRQ;
		}
	}
	if (ret == INT_IRQ) {
		if (!(io.irq_mask&IRQ1)) {
			io.irq_status|=IRQ1;
			//if (io.vdc_status&0x20)
			//TRACE("status=%02X\n", io.vdc_status);
			//TRACE("irq:scan %d\n ",scanline);
			return ret;
		}
	}
	return INT_NONE;
}

byte TimerInt(M6502 *R)
{
	if (io.timer_start) {
		io.timer_counter--;
		if (io.timer_counter > 128) {
			io.timer_counter = io.timer_reload;
			//io.irq_status &= ~TIRQ;
			if (!(io.irq_mask&TIRQ)) {
				io.irq_status |= TIRQ;
				//TRACE("tirq=%d\n",scanline);
				//TRACE("tirq\n");
				return INT_TIMER;
			}
		}
	}
	return INT_NONE;
}

/*
	Hit Chesk Sprite#0 and others
*/
int CheckSprites(void)
{
	int i,x0,y0,w0,h0,x,y,w,h;
	SPR *spr;
	spr = (SPR*)SPRAM;
	x0 = spr->x;
	y0 = spr->y;
	w0 = (((spr->atr>>8 )&1)+1)*16;
	h0 = (((spr->atr>>12)&3)+1)*16;
	spr++;
	for(i=1;i<64;i++,spr++) {
		x = spr->x;
		y = spr->y;
		w = (((spr->atr>>8 )&1)+1)*16;
		h = (((spr->atr>>12)&3)+1)*16;
		if ((x<x0+w0)&&(x+w>x0)&&(y<y0+h0)&&(y+h>y0)) return TRUE;
	}
	return FALSE;
}

static void plane2pixel(int no)
{
	unsigned long M;
	byte *C=VRAM+no*32;
	unsigned long L,*C2 = VRAM2+no*8;
	int j;
  for(j=0;j<8;j++,C+=2,C2++) {
    M=C[0];
    L =((M&0x88)>>3)|((M&0x44)<<6)|((M&0x22)<<15)|((M&0x11)<<24);
    M=C[1];
    L|=((M&0x88)>>2)|((M&0x44)<<7)|((M&0x22)<<16)|((M&0x11)<<25);
    M=C[16];
    L|=((M&0x88)>>1)|((M&0x44)<<8)|((M&0x22)<<17)|((M&0x11)<<26);
    M=C[17];
    L|=((M&0x88))|((M&0x44)<<9)|((M&0x22)<<18)|((M&0x11)<<27);
    C2[0] = L; //37261504
  }
}


#if defined(USE_SPRITE_CACHE)
static void sprite2pixel(int no)
{
	byte M;
	byte *C;
	unsigned long *C2;
	C=&VRAM[no*128];
	C2=&VRAMS[no*32];
	int i;
	for(i=0;i<32;i++,C++,C2++){
		long L;
		M=C[0];
		L =((M&0x88)>>3)|((M&0x44)<<6)|((M&0x22)<<15)|((M&0x11)<<24);
		M=C[32];
		L|=((M&0x88)>>2)|((M&0x44)<<7)|((M&0x22)<<16)|((M&0x11)<<25);
		M=C[64];
		L|=((M&0x88)>>1)|((M&0x44)<<8)|((M&0x22)<<17)|((M&0x11)<<26);
		M=C[96];
		L|=((M&0x88))|((M&0x44)<<9)|((M&0x22)<<18)|((M&0x11)<<27);
		C2[0]=L;
	}
}
#else //defined(USE_SPRITE_CACHE)
static long sp2pixel(byte *C)
{
	long L;
	byte M;
    M=C[0];
    L =((M&0x88)>>3)|((M&0x44)<<6)|((M&0x22)<<15)|((M&0x11)<<24);
    M=C[32];
    L|=((M&0x88)>>2)|((M&0x44)<<7)|((M&0x22)<<16)|((M&0x11)<<25);
    M=C[64];
    L|=((M&0x88)>>1)|((M&0x44)<<8)|((M&0x22)<<17)|((M&0x11)<<26);
    M=C[96];
    L|=((M&0x88))|((M&0x44)<<9)|((M&0x22)<<18)|((M&0x11)<<27);
	return L;
}
#endif //defined(USE_SPRITE_CACHE)

#define	PAL(c)	R[c]
#define	SPal	(Pal+256)
byte *XBuf;
byte Black = 0;
#define	FC_W	io.screen_w
#define	FC_H	256

void RefreshLine(int Y1,int Y2)
{
	int X1,XW,Line;
	int x,y,h,offset;

	byte *PP;//,*ZP;

//	System_mes("RefreshLine");
//	System_count(Y1);
//	System_count(Y2);

	//TRACE("%d-%d,Scroll=%d\n",Y1,Y2,ScrollY-ScrollYDiff);
	Y2++;
	PP=XBuf+WIDTH*(HEIGHT-FC_H)/2+(WIDTH-FC_W)/2+WIDTH*Y1;
	if(!ScreenON ){
		_memset(XBuf+((HEIGHT-FC_H)/2+Y1)*WIDTH,Pal[0],(Y2-Y1)*WIDTH);
		return;
	}else
	if(!BGONSwitch){
		_memset(XBuf+((HEIGHT-FC_H)/2+Y1)*WIDTH,Pal[0],(Y2-Y1)*WIDTH);
		return;
	}else {
		//TRACE("ScrollY=%d,diff=%d\n", ScrollY, ScrollYDiff);
		//TRACE("ScrollX=%d\n", ScrollX);
	y = Y1+ScrollY-ScrollYDiff;
	offset = y&7;
	h = 8-offset;
	if (h>Y2-Y1) h=Y2-Y1;
	y>>=3;
	PP-=ScrollX&7;
	XW=io.screen_w/8+1;
//	Shift = ScrollX&7;
//	{byte *Z=ZBuf+Y1*ZW;
//	for(Line=Y1;Line<Y2;Line++,Z+=ZW) Z[0]=0;
//	}

  for(Line=Y1;Line<Y2;y++) {
//    ZP = ZBuf+Line*ZW;
	x = ScrollX/8;
	y &= io.bg_h-1;
	for(X1=0;X1<XW;X1++,x++,PP+=8/*,ZP++*/){
		byte *R,*P,*C;//,*Z;
		unsigned long *C2;
		int no,i;
		x&=io.bg_w-1;
		no = ((word*)VRAM)[x+y*io.bg_w];
		R = &Pal[(no>>12)*16];
		no&=0xFFF;
		if (vchange[no]) { vchange[no]=0; plane2pixel(no);}
		C2 = &VRAM2[no*8+offset];
		C = &VRAM[no*32+offset*2];
		P = PP;
//		Z = ZP;
		for(i=0;i<h;i++,P+=WIDTH,C2++,C+=2/*,Z+=ZW*/) {
			unsigned long L;

			L=C2[0];
			P[0]=PAL((L>>4)&15);
			P[1]=PAL((L>>12)&15);
			P[2]=PAL((L>>20)&15);
			P[3]=PAL((L>>28));
			P[4]=PAL((L)&15);
			P[5]=PAL((L>>8)&15);
			P[6]=PAL((L>>16)&15);
			P[7]=PAL((L>>24)&15);
		}
	}
	Line+=h;
	PP+=WIDTH*h-XW*8;
	offset = 0;
	h = Y2-Line;
	if (h>8) h=8;
  }
  	}
}

#define	V_FLIP	0x8000
#define	H_FLIP	0x0800
#define	SPBG	0x80
#define	CGX		0x100

static void PutSprite(byte *P,byte *C,unsigned long *C2,byte *R,int h,int inc)
{
	int i,J;
	unsigned long L;
	for(i=0;i<h;i++,C+=inc,C2+=inc,P+=WIDTH) {
		J = ((word*)C)[0]|((word*)C)[16]|((word*)C)[32]|((word*)C)[48];
		if (!J) continue;
#if defined(USE_SPRITE_CACHE)
		L = C2[1];
#else //defined(USE_SPRITE_CACHE)
		L = sp2pixel(C+1);
#endif //defined(USE_SPRITE_CACHE)
		if (J&0x8000) P[0]=PAL((L>>4)&15);
		if (J&0x4000) P[1]=PAL((L>>12)&15);
		if (J&0x2000) P[2]=PAL((L>>20)&15);
		if (J&0x1000) P[3]=PAL((L>>28));
		if (J&0x0800) P[4]=PAL((L)&15);
		if (J&0x0400) P[5]=PAL((L>>8)&15);
		if (J&0x0200) P[6]=PAL((L>>16)&15);
		if (J&0x0100) P[7]=PAL((L>>24)&15);
#if defined(USE_SPRITE_CACHE)
		L = C2[0];
#else //defined(USE_SPRITE_CACHE)
		L = sp2pixel(C);
#endif //defined(USE_SPRITE_CACHE)
		if (J&0x80) P[8 ]=PAL((L>>4)&15);
		if (J&0x40) P[9 ]=PAL((L>>12)&15);
		if (J&0x20) P[10]=PAL((L>>20)&15);
		if (J&0x10) P[11]=PAL((L>>28));
		if (J&0x08) P[12]=PAL((L)&15);
		if (J&0x04) P[13]=PAL((L>>8)&15);
		if (J&0x02) P[14]=PAL((L>>16)&15);
		if (J&0x01) P[15]=PAL((L>>24)&15);
	}
}

static void PutSpriteHflip(byte *P,byte *C,unsigned long *C2,byte *R,int h,int inc)
{
	int i,J;
	unsigned long L;
	for(i=0;i<h;i++,C+=inc,C2+=inc,P+=WIDTH) {
		J = ((word*)C)[0]|((word*)C)[16]|((word*)C)[32]|((word*)C)[48];
		if (!J) continue;
#if defined(USE_SPRITE_CACHE)
		L = C2[0];
#else //defined(USE_SPRITE_CACHE)
		L = sp2pixel(C);
#endif //defined(USE_SPRITE_CACHE)
		if (J&0x01) P[0]=PAL((L>>24)&15);
		if (J&0x02) P[1]=PAL((L>>16)&15);
		if (J&0x04) P[2]=PAL((L>>8)&15);
		if (J&0x08) P[3]=PAL((L)&15);
		if (J&0x10) P[4]=PAL((L>>28));
		if (J&0x20) P[5]=PAL((L>>20)&15);
		if (J&0x40) P[6]=PAL((L>>12)&15);
		if (J&0x80) P[7]=PAL((L>>4)&15);
#if defined(USE_SPRITE_CACHE)
		L = C2[1];
#else //defined(USE_SPRITE_CACHE)
		L = sp2pixel(C+1);
#endif //defined(USE_SPRITE_CACHE)
		if (J&0x0100) P[8]=PAL((L>>24)&15);
		if (J&0x0200) P[9]=PAL((L>>16)&15);
		if (J&0x0400) P[10]=PAL((L>>8)&15);
		if (J&0x0800) P[11]=PAL((L)&15);
		if (J&0x1000) P[12]=PAL((L>>28));
		if (J&0x2000) P[13]=PAL((L>>20)&15);
		if (J&0x4000) P[14]=PAL((L>>12)&15);
		if (J&0x8000) P[15]=PAL((L>>4)&15);
	}
}

static void PutSpriteBack(byte *P,byte *C,unsigned long *C2,byte *R,int h,int inc,
						  int cx,int cy,int xoffset,int yoffset)
{
	int i,J;
	unsigned long L;
	unsigned long M;
	int tx;
	byte *CC1,*CC2,*CC3;
	int no;

	yoffset+=8;	// 最初のifに入れる
	cy--;
	for(i=0;i<h;i++,C+=inc,C2+=inc,P+=WIDTH,yoffset++,CC1+=2,CC2+=2,CC3+=2) {
		if (yoffset>=8) {
			cy++;
			cy = cy&(io.bg_h-1);
			yoffset -= 8;

			no = ((word*)VRAM)[cx+cy*io.bg_w];
			no&=0xFFF;
			CC1 = &VRAM[no*32+yoffset*2];
			tx=(cx+1)&(io.bg_w-1);
			no = ((word*)VRAM)[tx+cy*io.bg_w];
			no&=0xFFF;
			CC2 = &VRAM[no*32+yoffset*2];
			tx=(tx+1)&(io.bg_w-1);
			no = ((word*)VRAM)[tx+cy*io.bg_w];
			no&=0xFFF;
			CC3 = &VRAM[no*32+yoffset*2];
		}

		M=CC1[0]|CC1[1]|CC1[16]|CC1[17];
		M=(M<<8)|CC2[0]|CC2[1]|CC2[16]|CC2[17];
		M=(M<<8)|CC3[0]|CC3[1]|CC3[16]|CC3[17];
		M>>=(8-xoffset);
		M=~M;

		J = ((word*)C)[0]|((word*)C)[16]|((word*)C)[32]|((word*)C)[48];
		if (!J) continue;
#if defined(USE_SPRITE_CACHE)
		L = C2[1];
#else //defined(USE_SPRITE_CACHE)
		L = sp2pixel(C+1);
#endif //defined(USE_SPRITE_CACHE)
		if (J&M&0x8000) P[0]=PAL((L>>4)&15);
		if (J&M&0x4000) P[1]=PAL((L>>12)&15);
		if (J&M&0x2000) P[2]=PAL((L>>20)&15);
		if (J&M&0x1000) P[3]=PAL((L>>28));
		if (J&M&0x0800) P[4]=PAL((L)&15);
		if (J&M&0x0400) P[5]=PAL((L>>8)&15);
		if (J&M&0x0200) P[6]=PAL((L>>16)&15);
		if (J&M&0x0100) P[7]=PAL((L>>24)&15);
#if defined(USE_SPRITE_CACHE)
		L = C2[0];
#else //defined(USE_SPRITE_CACHE)
		L = sp2pixel(C);
#endif //defined(USE_SPRITE_CACHE)
		if (J&M&0x80) P[8 ]=PAL((L>>4)&15);
		if (J&M&0x40) P[9 ]=PAL((L>>12)&15);
		if (J&M&0x20) P[10]=PAL((L>>20)&15);
		if (J&M&0x10) P[11]=PAL((L>>28));
		if (J&M&0x08) P[12]=PAL((L)&15);
		if (J&M&0x04) P[13]=PAL((L>>8)&15);
		if (J&M&0x02) P[14]=PAL((L>>16)&15);
		if (J&M&0x01) P[15]=PAL((L>>24)&15);
	}
}

static void PutSpriteHflipBack(byte *P,byte *C,unsigned long *C2,byte *R,int h,int inc,
						  int cx,int cy,int xoffset,int yoffset)
{
	int i,J;
	unsigned long L;
	unsigned long M;
	int tx;
	byte *CC1,*CC2,*CC3;
	int no;

	yoffset+=8;	// 最初のifに入れる
	cy--;
	for(i=0;i<h;i++,C+=inc,C2+=inc,P+=WIDTH,yoffset++,CC1+=2,CC2+=2,CC3+=2) {
		if (yoffset>=8) {
			cy++;
			cy = cy&(io.bg_h-1);
			yoffset -= 8;

			no = ((word*)VRAM)[cx+cy*io.bg_w];
			no&=0xFFF;
			CC1 = &VRAM[no*32+yoffset*2];
			tx=(cx+1)&(io.bg_w-1);
			no = ((word*)VRAM)[tx+cy*io.bg_w];
			no&=0xFFF;
			CC2 = &VRAM[no*32+yoffset*2];
			tx=(tx+1)&(io.bg_w-1);
			no = ((word*)VRAM)[tx+cy*io.bg_w];
			no&=0xFFF;
			CC3 = &VRAM[no*32+yoffset*2];
		}

		M=CC1[0]|CC1[1]|CC1[16]|CC1[17];
		M=(M<<8)|CC2[0]|CC2[1]|CC2[16]|CC2[17];
		M=(M<<8)|CC3[0]|CC3[1]|CC3[16]|CC3[17];
		M>>=(8-xoffset);
		//M=~M;

		J = ((word*)C)[0]|((word*)C)[16]|((word*)C)[32]|((word*)C)[48];
		if (!J) continue;
#if defined(USE_SPRITE_CACHE)
		L = C2[0];
#else //defined(USE_SPRITE_CACHE)
		L = sp2pixel(C);
#endif //defined(USE_SPRITE_CACHE)
		if ((J&0x01|M&0x8000)==0x01) P[0]=PAL((L>>24)&15);
		if ((J&0x02|M&0x4000)==0x02) P[1]=PAL((L>>16)&15);
		if ((J&0x04|M&0x2000)==0x04) P[2]=PAL((L>>8)&15);
		if ((J&0x08|M&0x1000)==0x08) P[3]=PAL((L)&15);
		if ((J&0x10|M&0x0800)==0x10) P[4]=PAL((L>>28));
		if ((J&0x20|M&0x0400)==0x20) P[5]=PAL((L>>20)&15);
		if ((J&0x40|M&0x0200)==0x40) P[6]=PAL((L>>12)&15);
		if ((J&0x80|M&0x0100)==0x80) P[7]=PAL((L>>4)&15);
#if defined(USE_SPRITE_CACHE)
		L = C2[1];
#else //defined(USE_SPRITE_CACHE)
		L = sp2pixel(C+1);
#endif //defined(USE_SPRITE_CACHE)
		if ((J&0x0100|M&0x80)==0x0100) P[8]=PAL((L>>24)&15);
		if ((J&0x0200|M&0x40)==0x0200) P[9]=PAL((L>>16)&15);
		if ((J&0x0400|M&0x20)==0x0400) P[10]=PAL((L>>8)&15);
		if ((J&0x0800|M&0x10)==0x0800) P[11]=PAL((L)&15);
		if ((J&0x1000|M&0x08)==0x1000) P[12]=PAL((L>>28));
		if ((J&0x2000|M&0x04)==0x2000) P[13]=PAL((L>>20)&15);
		if ((J&0x4000|M&0x02)==0x4000) P[14]=PAL((L>>12)&15);
		if ((J&0x8000|M&0x01)==0x8000) P[15]=PAL((L>>4)&15);
	}
}

static void PutSpriteBack2(byte *P,byte *C,unsigned long *C2,byte *R,int h,int inc,
						   int cx,int cy,int xoffset,int yoffset)
{
	int i;
	unsigned long J;
	unsigned long L;
	unsigned long M;
	unsigned long CL;
	int tx;
	byte *CC1,*CC2,*CC3;
	unsigned long *CC21,*CC22,*CC23;
	byte *CR1, *CR2, *CR3;
	byte *CP;
	int no;

	yoffset+=8;	// 最初のifに入れる
	cy--;
	for(i=0;i<h;i++,C+=inc,C2+=inc,P+=WIDTH,yoffset++,CC1+=2,CC2+=2,CC3+=2,CC21++,CC22++,CC23++) {
		if (yoffset>=8) {
			cy++;
			cy = cy&(io.bg_h-1);
			yoffset -= 8;

			no = ((word*)VRAM)[cx+cy*io.bg_w];
			CR1 = &Pal[(no>>12)*16];
			no&=0xFFF;
			CC1 = &VRAM[no*32+yoffset*2];
			CC21 = &VRAM2[no*8+yoffset];
			tx=(cx+1)&(io.bg_w-1);
			no = ((word*)VRAM)[tx+cy*io.bg_w];
			CR2 = &Pal[(no>>12)*16];
			no&=0xFFF;
			CC2 = &VRAM[no*32+yoffset*2];
			CC22 = &VRAM2[no*8+yoffset];
			tx=(tx+1)&(io.bg_w-1);
			no = ((word*)VRAM)[tx+cy*io.bg_w];
			CR3 = &Pal[(no>>12)*16];
			no&=0xFFF;
			CC3 = &VRAM[no*32+yoffset*2];
			CC23 = &VRAM2[no*8+yoffset];
		}

		M=CC1[0]|CC1[1]|CC1[16]|CC1[17];
		M=(M<<8)|CC2[0]|CC2[1]|CC2[16]|CC2[17];
		M=(M<<8)|CC3[0]|CC3[1]|CC3[16]|CC3[17];
		M>>=(8-xoffset);
		M=~M;

		J = ((word*)C)[0]|((word*)C)[16]|((word*)C)[32]|((word*)C)[48];
		if (!J) continue;
#if defined(USE_SPRITE_CACHE)
		L = C2[1];
#else //defined(USE_SPRITE_CACHE)
		L = sp2pixel(C+1);
#endif //defined(USE_SPRITE_CACHE)
		if (J&M&0x8000) P[0]=PAL((L>>4)&15);
		if (J&M&0x4000) P[1]=PAL((L>>12)&15);
		if (J&M&0x2000) P[2]=PAL((L>>20)&15);
		if (J&M&0x1000) P[3]=PAL((L>>28));
		if (J&M&0x0800) P[4]=PAL((L)&15);
		if (J&M&0x0400) P[5]=PAL((L>>8)&15);
		if (J&M&0x0200) P[6]=PAL((L>>16)&15);
		if (J&M&0x0100) P[7]=PAL((L>>24)&15);
#if defined(USE_SPRITE_CACHE)
		L = C2[0];
#else //defined(USE_SPRITE_CACHE)
		L = sp2pixel(C);
#endif //defined(USE_SPRITE_CACHE)
		if (J&M&0x80) P[8 ]=PAL((L>>4)&15);
		if (J&M&0x40) P[9 ]=PAL((L>>12)&15);
		if (J&M&0x20) P[10]=PAL((L>>20)&15);
		if (J&M&0x10) P[11]=PAL((L>>28));
		if (J&M&0x08) P[12]=PAL((L)&15);
		if (J&M&0x04) P[13]=PAL((L>>8)&15);
		if (J&M&0x02) P[14]=PAL((L>>16)&15);
		if (J&M&0x01) P[15]=PAL((L>>24)&15);

		M<<=8-xoffset;
		M=~M;
		J<<=8-xoffset;
		CL=CC21[0];
		CP=P-xoffset;
		/*switch (xoffset) {
		case 0: if (J&M&0x800000) CP[0]=CR1[(CL>>4)&15];
		case 1: if (J&M&0x400000) CP[1]=CR1[(CL>>12)&15];
		case 2: if (J&M&0x200000) CP[2]=CR1[(CL>>20)&15];
		case 3: if (J&M&0x100000) CP[3]=CR1[(CL>>28)];
		case 4: if (J&M&0x080000) CP[4]=CR1[(CL)&15];
		case 5: if (J&M&0x040000) CP[5]=CR1[(CL>>8)&15];
		case 6: if (J&M&0x020000) CP[6]=CR1[(CL>>16)&15];
		case 7: if (J&M&0x010000) CP[7]=CR1[(CL>>24)&15];
		}*/
		if (xoffset<=0&&(J&M&0x800000)) CP[0]=CR1[(CL>>4)&15];
		if (xoffset<=1&&(J&M&0x400000)) CP[1]=CR1[(CL>>12)&15];
		if (xoffset<=2&&(J&M&0x200000)) CP[2]=CR1[(CL>>20)&15];
		if (xoffset<=3&&(J&M&0x100000)) CP[3]=CR1[(CL>>28)];
		if (xoffset<=4&&(J&M&0x080000)) CP[4]=CR1[(CL)&15];
		if (xoffset<=5&&(J&M&0x040000)) CP[5]=CR1[(CL>>8)&15];
		if (xoffset<=6&&(J&M&0x020000)) CP[6]=CR1[(CL>>16)&15];
		if (            (J&M&0x010000)) CP[7]=CR1[(CL>>24)&15];
		CL=CC22[0];
		if (J&M&0x8000) CP[8 ]=CR2[(CL>>4)&15];
		if (J&M&0x4000) CP[9 ]=CR2[(CL>>12)&15];
		if (J&M&0x2000) CP[10]=CR2[(CL>>20)&15];
		if (J&M&0x1000) CP[11]=CR2[(CL>>28)];
		if (J&M&0x0800) CP[12]=CR2[(CL)&15];
		if (J&M&0x0400) CP[13]=CR2[(CL>>8)&15];
		if (J&M&0x0200) CP[14]=CR2[(CL>>16)&15];
		if (J&M&0x0100) CP[15]=CR2[(CL>>24)&15];
		CL=CC23[0];
		/*switch (xoffset) {
		case  7: if (J&M&0x02) CP[22]=CR3[(CL>>16)&15];
		case  6: if (J&M&0x04) CP[21]=CR3[(CL>>8)&15];
		case  5: if (J&M&0x08) CP[20]=CR3[(CL)&15];
		case  4: if (J&M&0x10) CP[19]=CR3[(CL>>28)];
		case  3: if (J&M&0x20) CP[18]=CR3[(CL>>20)&15];
		case  2: if (J&M&0x40) CP[17]=CR3[(CL>>12)&15];
		case  1: if (J&M&0x80) CP[16]=CR3[(CL>>4)&15];
		}*/
		if (xoffset>=1&&(J&M&0x80)) CP[16]=CR3[(CL>>4)&15];
		if (xoffset>=2&&(J&M&0x40)) CP[17]=CR3[(CL>>12)&15];
		if (xoffset>=3&&(J&M&0x20)) CP[18]=CR3[(CL>>20)&15];
		if (xoffset>=4&&(J&M&0x10)) CP[19]=CR3[(CL>>28)];
		if (xoffset>=5&&(J&M&0x08)) CP[20]=CR3[(CL)&15];
		if (xoffset>=6&&(J&M&0x04)) CP[21]=CR3[(CL>>8)&15];
		if (xoffset>=7&&(J&M&0x02)) CP[22]=CR3[(CL>>16)&15];
	}
}

static void PutSpriteHflipBack2(byte *P,byte *C,unsigned long *C2,byte *R,int h,int inc,
								int cx,int cy,int xoffset,int yoffset)
{
	int i;
	unsigned long J;
	unsigned long L;
	unsigned long M;
	unsigned long CL;
	int tx;
	byte *CC1,*CC2,*CC3;
	unsigned long *CC21,*CC22,*CC23;
	byte *CR1, *CR2, *CR3;
	byte *CP;
	int no;

	yoffset+=8;	// 最初のifに入れる
	cy--;
	for(i=0;i<h;i++,C+=inc,C2+=inc,P+=WIDTH,yoffset++,CC1+=2,CC2+=2,CC3+=2,CC21++,CC22++,CC23++) {
		if (yoffset>=8) {
			cy++;
			cy = cy&(io.bg_h-1);
			yoffset -= 8;

			no = ((word*)VRAM)[cx+cy*io.bg_w];
			CR1 = &Pal[(no>>12)*16];
			no&=0xFFF;
			CC1 = &VRAM[no*32+yoffset*2];
			CC21 = &VRAM2[no*8+yoffset];
			tx=(cx+1)&(io.bg_w-1);
			no = ((word*)VRAM)[tx+cy*io.bg_w];
			CR2 = &Pal[(no>>12)*16];
			no&=0xFFF;
			CC2 = &VRAM[no*32+yoffset*2];
			CC22 = &VRAM2[no*8+yoffset];
			tx=(tx+1)&(io.bg_w-1);
			no = ((word*)VRAM)[tx+cy*io.bg_w];
			CR3 = &Pal[(no>>12)*16];
			no&=0xFFF;
			CC3 = &VRAM[no*32+yoffset*2];
			CC23 = &VRAM2[no*8+yoffset];
		}

		M=CC1[0]|CC1[1]|CC1[16]|CC1[17];
		M=(M<<8)|CC2[0]|CC2[1]|CC2[16]|CC2[17];
		M=(M<<8)|CC3[0]|CC3[1]|CC3[16]|CC3[17];
		M>>=(8-xoffset);

		J = ((word*)C)[0]|((word*)C)[16]|((word*)C)[32]|((word*)C)[48];
		if (!J) continue;
#if defined(USE_SPRITE_CACHE)
		L = C2[0];
#else //defined(USE_SPRITE_CACHE)
		L = sp2pixel(C);
#endif //defined(USE_SPRITE_CACHE)
		if ((J&0x01|M&0x8000)==0x01) P[0]=PAL((L>>24)&15);
		if ((J&0x02|M&0x4000)==0x02) P[1]=PAL((L>>16)&15);
		if ((J&0x04|M&0x2000)==0x04) P[2]=PAL((L>>8)&15);
		if ((J&0x08|M&0x1000)==0x08) P[3]=PAL((L)&15);
		if ((J&0x10|M&0x0800)==0x10) P[4]=PAL((L>>28));
		if ((J&0x20|M&0x0400)==0x20) P[5]=PAL((L>>20)&15);
		if ((J&0x40|M&0x0200)==0x40) P[6]=PAL((L>>12)&15);
		if ((J&0x80|M&0x0100)==0x80) P[7]=PAL((L>>4)&15);
#if defined(USE_SPRITE_CACHE)
		L = C2[1];
#else //defined(USE_SPRITE_CACHE)
		L = sp2pixel(C+1);
#endif //defined(USE_SPRITE_CACHE)
		if ((J&0x0100|M&0x80)==0x0100) P[8]=PAL((L>>24)&15);
		if ((J&0x0200|M&0x40)==0x0200) P[9]=PAL((L>>16)&15);
		if ((J&0x0400|M&0x20)==0x0400) P[10]=PAL((L>>8)&15);
		if ((J&0x0800|M&0x10)==0x0800) P[11]=PAL((L)&15);
		if ((J&0x1000|M&0x08)==0x1000) P[12]=PAL((L>>28));
		if ((J&0x2000|M&0x04)==0x2000) P[13]=PAL((L>>20)&15);
		if ((J&0x4000|M&0x02)==0x4000) P[14]=PAL((L>>12)&15);
		if ((J&0x8000|M&0x01)==0x8000) P[15]=PAL((L>>4)&15);

		M<<=8-xoffset;
		J<<=xoffset;
		CL=CC21[0];
		CP=P-xoffset;
		if (xoffset<=0&&(J&0x01|M&0x800000)==0x800001) CP[0]=CR1[(CL>>4)&15];
		if (xoffset<=1&&(J&0x02|M&0x400000)==0x400002) CP[1]=CR1[(CL>>12)&15];
		if (xoffset<=2&&(J&0x04|M&0x200000)==0x200004) CP[2]=CR1[(CL>>20)&15];
		if (xoffset<=3&&(J&0x08|M&0x100000)==0x100008) CP[3]=CR1[(CL>>28)];
		if (xoffset<=4&&(J&0x10|M&0x080000)==0x080010) CP[4]=CR1[(CL)&15];
		if (xoffset<=5&&(J&0x20|M&0x040000)==0x040020) CP[5]=CR1[(CL>>8)&15];
		if (xoffset<=6&&(J&0x40|M&0x020000)==0x020040) CP[6]=CR1[(CL>>16)&15];
		if (            (J&0x80|M&0x010000)==0x010080) CP[7]=CR1[(CL>>24)&15];
		CL=CC22[0];
		if ((J&0x0100|M&0x8000)==0x8100) CP[8 ]=CR2[(CL>>4)&15];
		if ((J&0x0200|M&0x4000)==0x4200) CP[9 ]=CR2[(CL>>12)&15];
		if ((J&0x0400|M&0x2000)==0x2400) CP[10]=CR2[(CL>>20)&15];
		if ((J&0x0800|M&0x1000)==0x1800) CP[11]=CR2[(CL>>28)];
		if ((J&0x1000|M&0x0800)==0x1800) CP[12]=CR2[(CL)&15];
		if ((J&0x2000|M&0x0400)==0x2400) CP[13]=CR2[(CL>>8)&15];
		if ((J&0x4000|M&0x0200)==0x4200) CP[14]=CR2[(CL>>16)&15];
		if ((J&0x8000|M&0x0100)==0x8100) CP[15]=CR2[(CL>>24)&15];
		CL=CC23[0];
		if (xoffset>=1&&(J&0x010000|M&0x80)==0x010080) CP[16]=CR3[(CL>>4)&15];
		if (xoffset>=2&&(J&0x020000|M&0x40)==0x020040) CP[17]=CR3[(CL>>12)&15];
		if (xoffset>=3&&(J&0x040000|M&0x20)==0x040020) CP[18]=CR3[(CL>>20)&15];
		if (xoffset>=4&&(J&0x080000|M&0x10)==0x080010) CP[19]=CR3[(CL>>28)];
		if (xoffset>=5&&(J&0x100000|M&0x08)==0x100008) CP[20]=CR3[(CL)&15];
		if (xoffset>=6&&(J&0x200000|M&0x04)==0x200004) CP[21]=CR3[(CL>>8)&15];
		if (xoffset>=7&&(J&0x400000|M&0x02)==0x400002) CP[22]=CR3[(CL>>16)&15];
	}
}

void RefreshSprite(int Y1,int Y2)
{
	int n;
	SPR *spr;
	int usespbg = 0;

	spr = (SPR*)SPRAM + 63;

//	if(!ScreenON )return;

	for(n=0;n<64;n++,spr--){
		int x,y,no,atr,inc,cgx,cgy;
		byte *R,*C;
		unsigned long *C2;
		int	pos;
		int h,t,i,j;
		int y_sum;
		int spbg;
		int cx;
		int cy;
		int yoffset;
		int xoffset;
		int tx;

		atr=spr->atr;
		spbg = (atr>>7)&1;
		//if (spbg != bg)
		//	continue;
		y = (spr->y&1023)-64;
		x = (spr->x&1023)-32;
		no= spr->no&2047;
		cgx = (atr>>8)&1;
		cgy = (atr>>12)&3;
		cgy |= cgy>>1;
		no = (no>>1)&~(cgy*2+cgx);
		if (y>=Y2 || y+(cgy+1)*16<Y1 || x>=FC_W || x+(cgx+1)*16<0) continue;

		R = &SPal[(atr&15)*16];
#if defined(USE_SPRITE_CACHE)
		for (i=0;i<cgy*2+cgx+1;i++) {
			if (vchanges[no+i]) {
				vchanges[no+i] = 0;
				sprite2pixel(no+i);
			}
			if (!cgx) i++;
		}
#endif //defined(USE_SPRITE_CACHE)
		C = &VRAM[no*128];
#if defined(USE_SPRITE_CACHE)
		C2 = &VRAMS[no*32];
#else //defined(USE_SPRITE_CACHE)
		C2 = (unsigned long *)VRAM;//NULL;
#endif //defined(USE_SPRITE_CACHE)
		pos = WIDTH*(HEIGHT-FC_H)/2+(WIDTH-FC_W)/2+WIDTH*y+x;
		inc = 2;
		if (atr&V_FLIP) { inc=-2; C+=15*2+cgy*256; C2+=15*2+cgy*64;}
		y_sum = 0;
		//printf("(%d,%d,%d,%d,%d)",x,y,cgx,cgy,h);
		//TRACE("Spr#%d,no=%d,x=%d,y=%d,CGX=%d,CGY=%d,xb=%d,yb=%d\n", n, no, x, y, cgx, cgy, atr&H_FLIP, atr&V_FLIP);

		cy = y+ScrollY-ScrollYDiff;
		yoffset = cy&7;
		cy>>=3;
		xoffset=(x+ScrollX)&7;
		cx = (x+ScrollX)/8;

		for(i=0;i<=cgy;i++) {
			cy = cy&(io.bg_h-1);
			t = Y1-y-y_sum;
			h = 16;
			if (t>0) {
				C+=t*inc;
				C2+=t*inc;
				h-=t;
				pos+=t*WIDTH;
				cy+=(yoffset+t)>>3;
				yoffset=(yoffset+t)&7;
			}
			if (h>Y2-y-y_sum) h = Y2-y-y_sum;
			if (spbg==1 || !ScreenON){
				usespbg=1;
				if (atr&H_FLIP){
				  for(j=0;j<=cgx;j++)
					PutSpriteHflip(XBuf+pos+(cgx-j)*16,C+j*128,C2+j*32,R,h,inc);
				}else{
				  for(j=0;j<=cgx;j++)
					PutSprite(XBuf+pos+j*16,C+j*128,C2+j*32,R,h,inc);
				}
			} else if (usespbg) {
				if (atr&H_FLIP){
					for(j=0,tx=cx;j<=cgx;j++,tx+=2) {
						tx&=io.bg_w-1;
						PutSpriteHflipBack2(XBuf+pos+j*16,C+(cgx-j)*128,C2+(cgx-j)*32,R,h,inc,tx,cy,xoffset,yoffset);
					}
				}else{
					for(j=0,tx=cx;j<=cgx;j++,tx+=2) {
						tx&=io.bg_w-1;
						PutSpriteBack2(XBuf+pos+j*16,C+j*128,C2+j*32,R,h,inc,tx,cy,xoffset,yoffset);
					}
				}
			} else {
				if (atr&H_FLIP){
					for(j=0,tx=cx;j<=cgx;j++,tx+=2) {
						tx&=io.bg_w-1;
						PutSpriteHflipBack(XBuf+pos+j*16,C+(cgx-j)*128,C2+(cgx-j)*32,R,h,inc,tx,cy,xoffset,yoffset);
					}
				}else{
					for(j=0,tx=cx;j<=cgx;j++,tx+=2) {
						tx&=io.bg_w-1;
						PutSpriteBack(XBuf+pos+j*16,C+j*128,C2+j*32,R,h,inc,tx,cy,xoffset,yoffset);
					}
				}
			}
			pos+=h*WIDTH;
			C+=h*inc+16*7*inc;
			C2+=h*inc+16*inc;
			y_sum+=16;
			cy+=(yoffset+h)>>3;
			yoffset=(yoffset+h)&7;
		}
	}
}



#define		Vtimer	16666
DWORD	syorioti=0;
DWORD	syorioti_c;
DWORD	fps;
int      VSyncTimer()
{
	int l=0;
	{
		static int f=0;
		static int t=0;
		int	c;

		sceCtrlRead(&ctl,1);

		if( ctl.frame+(Vtimer*30) < syorioti ){	//カウンタ折り返し	4294967295
			syorioti-=(DWORD)1073741824;
			syorioti-=(DWORD)1073741824;
			syorioti-=(DWORD)1073741824;
			syorioti-=(DWORD)1073741824;
		}

		if( (++t)==60 ){syorioti += 40;	t=0;}
		syorioti += Vtimer;
		if( ctl.frame < syorioti ){	//合格
			if( !f ){
			//	while( ctl.frame < syorioti ){
			//		sceCtrlRead(&ctl,1);
			//	}
			}else{
				f=0;
			}
			l++;
		}else{
			l++;
			f=1;
			while( ctl.frame >= syorioti ){
				l++;
				if( (++t)==60 ){syorioti += 40;	t=0;}
				syorioti += Vtimer;
			}
		}
	}
	//		FrameSkip2 = l;//FrameSkip;
	syorioti_c=l;
	fps = 60/l;
	return l;

}




char	path_state[MAXPATH];




int		palt[65536];
short	palt2[65536];
static unsigned int soundc=0;


short	thumbnail_buffer[128*96];
short	hantoumei(int c1,int c2)
{
	return
		((((c1&0x7c007c00)+(c2&0x7c007c00))>> 1)&0x7c007c00)+
		((((c1&0x001f001f)+(c2&0x001f001f))>> 1)&0x001f001f)+
		((((c1&0x03e003e0)+(c2&0x03e003e0))>> 1)&0x03e003e0);
}
void	thumbnail_buffer_set(void)
{
	int dispmin,dispmax;
	dispmin = (MaxLine-MinLine>MAXDISP ? MinLine+((MaxLine-MinLine-MAXDISP+1)>>1) : MinLine);
	dispmax = (MaxLine-MinLine>MAXDISP ? MaxLine-((MaxLine-MinLine-MAXDISP+1)>>1) : MaxLine);
	{
		int x,y, xx,xc, yy,yc, c, xf,yf;
		unsigned char *v = (void *)XBuf,*vb;
		unsigned short *p = thumbnail_buffer,*pb;
		
		v+=((WIDTH*dispmin)+((416-FC_W)/2));
		_memset(thumbnail_buffer,0,128*96*2);
	//	if( FC_W<256 || (dispmax-dispmin)<=192 )return;
		xx=xc= (((FC_W)*100)/128);
		yy=yc= (((dispmax-dispmin)*100)/96)+100;
	//	v+=(0)+((0)*WIDTH);
		for(y=0;y<96;y++){
			pb=p;
			yf=0;
			while((yc-=100)>0){
				vb=v;
				xc=xx;
				for(x=0;x<128;x++){
					c=palt[*v++];
					while((xc-=100)>0){
						c=hantoumei(palt[*v++],c);
					}
					if( !yf )	*p=c;
					else		*p=hantoumei( c, *p);
					xc+=xx;
					p+=1;
				}
				yf++;
				p=pb;
				v=vb+WIDTH;
			}
			yc+=yy;
			p=pb+128;
		}
	}
}

void WriteSoundData(WORD *buf, int ch, DWORD dwSize);
int	mes_f=0,mes_c=60,mes_b;
void	RefreshScreen(void)
{

	int dispmin,dispmax;
		static	int	c=0;
	int	mx,my;
	
	dispmin = (MaxLine-MinLine>MAXDISP ? MinLine+((MaxLine-MinLine-MAXDISP+1)>>1) : MinLine);
	dispmax = (MaxLine-MinLine>MAXDISP ? MaxLine-((MaxLine-MinLine-MAXDISP+1)>>1) : MaxLine);

//	PutImage((WIDTH-FC_W)/2,(HEIGHT-FC_H)/2+MinLine+dispmin,FC_W,dispmax-dispmin+1);

	//memset(XBuf+MinLine*WIDTH,Pal[0],(MaxLine-MinLine)*WIDTH);
	//memset(SPM+MinLine*WIDTH,0,(MaxLine-MinLine)*WIDTH);

	if( mes_b ){
		Bmp_put(1);
		mes_b--;
	}
	switch( ini_data.videomode ){
	case	0:
	normal:
		{
			int	x,y;
			unsigned int *vr;
			unsigned short *v = (void *)XBuf;
			mx=(480-FC_W)/2;
			my=20;
			vr = (void *)pgGetVramAddr(mx,my);
			v+=((WIDTH*dispmin)+((416-FC_W)/2))/2;
			for(y=0;y<dispmax-dispmin;y++){
				for(x=0;x<FC_W/2;x++){
					*vr++=palt[*v++];
				}
				vr+=(LINESIZE-FC_W)/2;
				v+=(WIDTH-FC_W)/2;
			}
		}







		break;
	case	1:
		if(FC_W>272)goto normal;
		{
			int x,y;
			unsigned int *vr;
			unsigned char *v = (void *)XBuf,*vb;
			unsigned	int g,s;
			int yy,yc;
			int xx,xc;
			mx=104;
			my=0;
			vr = (void *)pgGetVramAddr(mx,my);
			v+=((WIDTH*dispmin)+((416-FC_W)/2));
			yc=yy=2720/(272-(dispmax-dispmin));
			xc=xx=2720/(272-(FC_W));
			for(y=0;y<272;y++){
				if((yc-=10)<=0){
					yc+=yy;v-=(WIDTH);
				}
				vb=v;
				xc=xx;
				for(x=0;x<272/2;x++){
					if((xc-=10)<=0){
						xc+=xx;
						s=*v;
					}else s=(*v++);
					if((xc-=10)<=0){
						xc+=xx;
						s+=(s)<<8;
					}else s+=(*v++)<<8;
					
					*vr++=palt[s];
				}
				vr+=(LINESIZE-272)/2;
				v=vb+WIDTH;
			}
		}
		break;
	case	2:
		if(FC_W>272)goto normal;
		{
			int x,y;
			unsigned int *vr;
			unsigned char *v = (void *)XBuf,*vb;
			unsigned	int g,s,g2;
			int yy,yc;
			int xx,xc;
			unsigned int b[480/2],*bp,bf;
			mx=104;
			my=0;
			vr = (void *)pgGetVramAddr(mx,my);
			v+=((WIDTH*dispmin)+((416-FC_W)/2));
			yc=yy=2720/(272-(dispmax-dispmin));
			xc=xx=2720/(272-(FC_W));
			for(y=0;y<272;y++){
				bp=b;
				if((yc-=10)<=0){
					yc+=yy;	bf=1;
				}else bf=0;
				vb=v;	xc=xx;	g=0;
				for(x=0;x<272/2;x++){
					if((xc-=10)<=0){
						xc+=xx;	s=palt2[g+(*v<<8)];
					}else{
						g=(*v++);	s=palt[g];
					}
					if((xc-=10)<=0){
						xc+=xx;	s+=palt2[g+(*v<<8)]<<16;
					}else{
						g=(*v++);	s+=palt[g]<<16;
					}
					if( bf ){
						*vr++=
							((((s&0x7c007c00)+(*bp&0x7c007c00))>> 1)&0x7c007c00)+
							((((s&0x001f001f)+(*bp&0x001f001f))>> 1)&0x001f001f)+
							((((s&0x03e003e0)+(*bp&0x03e003e0))>> 1)&0x03e003e0);
						bp++;
					}else{
						*vr++=*bp++=s;//palt[s];
					}
				}
				if( bf )vb-=(WIDTH);
				vr+=(LINESIZE-272)/2;
				v=vb+WIDTH;
			}
		}
		break;
	case	3:
		{
			int x,y;
			unsigned int *vr;
			unsigned char *v = (void *)XBuf,*vb;
			unsigned	int g,s;
			int yy,yc;
			int xx,xc;
			mx=60;
			my=0;
			vr = (void *)pgGetVramAddr(mx,my);
			v+=((WIDTH*dispmin)+((416-FC_W)/2));
			yc=yy=2720/(272-(dispmax-dispmin));
			xc=xx=3600/(360-(FC_W));
			for(y=0;y<272;y++){
				if((yc-=10)<=0){
					yc+=yy;v-=(WIDTH);
				}
				vb=v;
				xc=xx;
				for(x=0;x<360/2;x++){
					if((xc-=10)<=0){
						xc+=xx;
						s=*v;
					}else s=(*v++);
					if((xc-=10)<=0){
						xc+=xx;
						s+=(s)<<8;
					}else s+=(*v++)<<8;
					
					*vr++=palt[s];
				}
				vr+=(LINESIZE-360)/2;
				v=vb+WIDTH;
			}
		}
		break;
	case	4:
		{
			int x,y;
			unsigned int *vr;
			unsigned char *v = (void *)XBuf,*vb;
			unsigned	int g,s,g2;
			int yy,yc;
			int xx,xc;
			unsigned int b[480/2],*bp,bf;
			mx=60;
			my=0;
			vr = (void *)pgGetVramAddr(mx,my);
			v+=((WIDTH*dispmin)+((416-FC_W)/2));
			yc=yy=2720/(272-(dispmax-dispmin));
			xc=xx=3600/(360-(FC_W));
			for(y=0;y<272;y++){
				bp=b;
				if((yc-=10)<=0){
					yc+=yy;	bf=1;
				}else bf=0;
				vb=v;	xc=xx;	g=0;
				for(x=0;x<360/2;x++){
					if((xc-=10)<=0){
						xc+=xx;	s=palt2[g+(*v<<8)];
					}else{
						g=(*v++);	s=palt[g];
					}
					if((xc-=10)<=0){
						xc+=xx;	s+=palt2[g+(*v<<8)]<<16;
					}else{
						g=(*v++);	s+=palt[g]<<16;
					}
					if( bf ){
						*vr++=
							((((s&0x7c007c00)+(*bp&0x7c007c00))>> 1)&0x7c007c00)+
							((((s&0x001f001f)+(*bp&0x001f001f))>> 1)&0x001f001f)+
							((((s&0x03e003e0)+(*bp&0x03e003e0))>> 1)&0x03e003e0);
						bp++;
					}else{
						*vr++=*bp++=s;//palt[s];
					}
				}
				if( bf )vb-=(WIDTH);
				vr+=(LINESIZE-360)/2;
				v=vb+WIDTH;
			}
		}
		break;
	case	5:
		{
			int x,y;
			unsigned int *vr;
			unsigned char *v = (void *)XBuf,*vb;
			unsigned	int g,s;
			int yy,yc;
			int xx,xc;
			mx=0;
			my=0;
			vr = (void *)pgGetVramAddr(mx,my);
			v+=((WIDTH*(dispmin+8))+((416-FC_W)/2));
			yc=yy=(2720)/((272+16)-(dispmax-dispmin));
			xc=xx=4800/(480-(FC_W));
			for(y=0;y<272;y++){
				if((yc-=10)<=0){
					yc+=yy;v-=(WIDTH);
				}
				vb=v;
				xc=xx;
				for(x=0;x<480/2;x++){
					if((xc-=10)<=0){
						xc+=xx;
						s=*v;
					}else s=(*v++);
					if((xc-=10)<=0){
						xc+=xx;
						s+=(s)<<8;
					}else s+=(*v++)<<8;
					
					*vr++=palt[s];
				}
				vr+=(LINESIZE-480)/2;
				v=vb+WIDTH;
			}
		}
		break;
	case	6:
		{
			int x,y;
			unsigned int *vr;
			unsigned char *v = (void *)XBuf,*vb;
			unsigned	int g,s,g2;
			int yy,yc;
			int xx,xc;
			unsigned int b[480/2],*bp,bf;
			mx=0;
			my=0;
			vr = (void *)pgGetVramAddr(mx,my);
			v+=((WIDTH*(dispmin+8))+((416-FC_W)/2));
			yc=yy=(2720)/((272+16)-(dispmax-dispmin));
			xc=xx=4800/(480-(FC_W));
			for(y=0;y<272;y++){
				bp=b;
				if((yc-=10)<=0){
					yc+=yy;	bf=1;
				}else bf=0;
				vb=v;	xc=xx;	g=0;
				for(x=0;x<480/2;x++){
					if((xc-=10)<=0){
						xc+=xx;	s=palt2[g+(*v<<8)];
					}else{
						g=(*v++);	s=palt[g];
					}
					if((xc-=10)<=0){
						xc+=xx;	s+=palt2[g+(*v<<8)]<<16;
					}else{
						g=(*v++);	s+=palt[g]<<16;
					}
					if( bf ){
						*vr++=
							((((s&0x7c007c00)+(*bp&0x7c007c00))>> 1)&0x7c007c00)+
							((((s&0x001f001f)+(*bp&0x001f001f))>> 1)&0x001f001f)+
							((((s&0x03e003e0)+(*bp&0x03e003e0))>> 1)&0x03e003e0);
						bp++;
					}else{
						*vr++=*bp++=s;//palt[s];
					}
				}
				if( bf )vb-=(WIDTH);
				vr+=(LINESIZE-480)/2;
				v=vb+WIDTH;
			}
		}
		break;
	}
	if( mes_c ){
		mx+=2;
		my+=2;
		switch( mes_f ){
		case	1:
			mh_print( mx, my,"state quick loed",rgb2col( 255,255,255),0,0);
			break;
		case	2:
			mh_print( mx, my,"state quick save",rgb2col( 255,255,255),0,0);
			break;
		case	3:
			mh_print( mx, my,"state file loed",rgb2col( 255,255,255),0,0);
			mh_print( mx, my+11, path_state,rgb2col( 255,255,255),0,0);
			break;
		case	4:
			mh_print( mx, my,"state file save",rgb2col( 255,255,255),0,0);
			mh_print( mx, my+11, path_state,rgb2col( 255,255,255),0,0);
			break;
		}
	}


	if(ini_data.Debug){
		System_mes_put();
		counter( 50, 20, fps, 2, -1);
		counter( 50, 30, syorioti_c, 5, -1);
		counter( 50, 40, ctl.frame, 10, -1);
		counter( 50, 50, BaseClock, 7, -1);
	//	counter( 50,100, wavout_snd1_wavinfo[0].playptr, 8, -1);
	//	counter( 50,110, soundc, 8, -1);
	//	counter( 50,120, wavout_snd1_wavinfo[0].playptr-soundc, 8, -1);
	//	counter( 50,200, dispmin, 4, -1);
	//	counter( 50,210, dispmax, 4, -1);

	}


//	if( ctl.buttons&CTRL_START )return INT_QUIT;

	return;
}



void	state_file_patch(char *n)
{
	int i=0;
	unsigned char *s;
	_strcpy( path_state, path_main);


	s=n;
	while(*s){
		if( *s=='/' )i++;
	//	if( ((*s>=0x80) && (*s<0xa0)) || (*s>=0xe0) )s++;
		s++;
	}
	if( !i ){
		s=n;
	}else{
		s--;
		while(*s){
			if( *s=='/' ){
				s++;
				break;
			//	if( ((*s>=0x80) && (*s<0xa0)) || (*s>=0xe0) );else	break;
			}
			s--;
		}
	//	s++;
	}
	_strcat( path_state, "SAVE/");
	_strcat( path_state, s);
	s=path_state;
	while(*s){
	//	if( ((*s>=0x80) && (*s<0xa0)) || (*s>=0xe0) )s++;
		s++;
	}
	s--;
	while(*s){
		if( *s=='.' ){
			break;
		//	s--;
		//	if( ((*s>=0x80) && (*s<0xa0)) || (*s>=0xe0) );else	break;
		}
		s--;
	}
	s+=1;
	*s++='S';
	*s++='A';
	*s++='V';
	*s++=0;
	System_mes(n);
	System_mes(path_state);
//	Error_mes(n);
//	Error_mes(path_state);
}
int	state_flag=0;
int	state_read_chk(void)
{
	int fd;
	fd = sceIoOpen( path_state, O_RDONLY, 0777);
	if(fd>=0){
		sceIoClose(fd);
		return 1;
	}
	return 0;
}
char* _memcpy_g(void *d,  void *s, unsigned long n)
{
	char *g=s;
	char *p=d;
	while(n--)*p++=*g++;
	return g;
}
char* _memcpy_p(void *d,  void *s, unsigned long n)
{
	char *g=s;
	char *p=d;
	while(n--)*p++=*g++;
	return p;
}
///////////////////////////////////////////////////////////////////////////////
		char	state_q[ 192564+0x10000+0x30000+1000];
		char	state_b1[192564+0x10000+0x30000+1000];
		char	state_b2[192564+0x10000+0x30000+1000];
		int		state_f;
///////////////////////////////////////////////////////////////////////////////
void	state_quick_read(void)
{
			char 	*p;
	if( !state_f )return;
		p = state_q;
			p = _memcpy_g( RAM, p, 0x8000);
			p = _memcpy_g( VRAM, p, VRAMSIZE);
			p = _memcpy_g( SPRAM, p, 64*4*sizeof(short));
			p = _memcpy_g( Pal, p, 512);
			p = _memcpy_g( &scanline, p, sizeof(int));
			p = _memcpy_g( &prevline, p, sizeof(int));
			p = _memcpy_g( &TimerCount, p, sizeof(int));
			p = _memcpy_g( &io, p, sizeof(IO));
			p = _memcpy_g( &M, p, sizeof(M6502));
			p = _memcpy_g( &CycleOld, p, 4);
		if( CD_emulation ){
			p = _memcpy_g( cd_extra_mem, p, 0x10000);
			p = _memcpy_g( cd_extra_super_mem, p, 0x30000);
		}
			{
				int i;
				for (i = 0; i < 8; i++)
				bank_set(i, M.MPR[i]);
			}
}
void	state_quick_write(void)
{
			char 	*p;
	state_f=1;
		p = state_q;
		p = _memcpy_p( p, RAM, 0x8000);
		p = _memcpy_p( p, VRAM, VRAMSIZE);
		p = _memcpy_p( p, SPRAM, 64*4*sizeof(short));
		p = _memcpy_p( p, Pal, 512);
		p = _memcpy_p( p, &scanline, sizeof(int));
		p = _memcpy_p( p, &prevline, sizeof(int));
		p = _memcpy_p( p, &TimerCount, sizeof(int));
		p = _memcpy_p( p, &io, sizeof(IO));
		p = _memcpy_p( p, &M, sizeof(M6502));
		p = _memcpy_p( p, &CycleOld, 4);
	if( CD_emulation ){
		p = _memcpy_p( p, cd_extra_mem, 0x10000);
		p = _memcpy_p( p, cd_extra_super_mem, 0x30000);
	}
}
int		BmpRead(int i,char *fname);
void	state_read(void)
{
	int fd;

	state_name_change();

	fd = sceIoOpen( path_state, O_RDONLY, 0777);
	if(fd>=0){
		int	state_ver;
		char	menub[MAXPATH];
		char	gameb[MAXPATH];

		sceIoRead(fd, &state_ver, 4);

			_memcpy( menub, ini_data.path_WallPaper_menu, MAXPATH );
			_memcpy( gameb, ini_data.path_WallPaper_game, MAXPATH );

		if( state_ver == 6 ){
			int		s;
			char 	*p;
			p = state_b2;
			s=sceIoRead(fd, state_b1, 192564+0x10000+0x30000+1000 );
			s=iyanbakan_d( p, state_b1, s );
			if( s!=192564 ){
				Error_mes("state展開エラー");
				Error_count(s);
				sceIoClose(fd);
				return;
			}
			p = _memcpy_g( thumbnail_buffer, p, 128*96*2);
			p = _memcpy_g( RAM, p, 0x8000);
			p = _memcpy_g( VRAM, p, VRAMSIZE);
			p = _memcpy_g( SPRAM, p, 64*4*sizeof(short));
			p = _memcpy_g( Pal, p, 512);
			p = _memcpy_g( &scanline, p, sizeof(int));
			p = _memcpy_g( &prevline, p, sizeof(int));
			p = _memcpy_g( &TimerCount, p, sizeof(int));
			p = _memcpy_g( &io, p, sizeof(IO));
			p = _memcpy_g( &M, p, sizeof(M6502));
			p = _memcpy_g( &CycleOld, p, 4);
			p = _memcpy_g( &ini_data, p, sizeof(ini_info));

		}else
		if( state_ver == 7 ){
			int		s;
			char 	*p;
			p = state_b2;
			s=sceIoRead(fd, state_b1, 192564+0x10000+0x30000+1000 );
			s=iyanbakan_d( p, state_b1, s );
			if( s!=192564+0x10000+0x30000 ){
				Error_mes("state展開エラー");
				Error_count(s);
				sceIoClose(fd);
				return;
			}
			p = _memcpy_g( thumbnail_buffer, p, 128*96*2);
			p = _memcpy_g( RAM, p, 0x8000);
			p = _memcpy_g( VRAM, p, VRAMSIZE);
			p = _memcpy_g( SPRAM, p, 64*4*sizeof(short));
			p = _memcpy_g( Pal, p, 512);
			p = _memcpy_g( &scanline, p, sizeof(int));
			p = _memcpy_g( &prevline, p, sizeof(int));
			p = _memcpy_g( &TimerCount, p, sizeof(int));
			p = _memcpy_g( &io, p, sizeof(IO));
			p = _memcpy_g( &M, p, sizeof(M6502));
			p = _memcpy_g( &CycleOld, p, 4);
			p = _memcpy_g( &ini_data, p, sizeof(ini_info));
			p = _memcpy_g( cd_extra_mem, p, 0x10000);
			p = _memcpy_g( cd_extra_super_mem, p, 0x30000);

		}else{
			Error_mes("未対応の形式です");
			return;
		}
			_memcpy( ini_data.path_WallPaper_menu, menub, MAXPATH );
			_memcpy( ini_data.path_WallPaper_game, gameb, MAXPATH );

		{
			int i;
			for (i = 0; i < 8; i++)
				bank_set(i, M.MPR[i]);
		}
		sceIoClose(fd);
		System_mes("state read");
	}
}
void	state_write(void)
{
	int fd;

	state_name_change();

	fd = sceIoOpen( path_state, O_CREAT|O_WRONLY|O_TRUNC, 0777);
	if(fd>=0){
		int	state_ver;
		int		s;
		char 	*p;
		p = state_b1;
		
		thumbnail_buffer_set();

		if( !CD_emulation ){
			state_ver = 6;

			sceIoWrite(fd, &state_ver, 4);
			p = _memcpy_p( p, thumbnail_buffer, 128*96*2);
			p = _memcpy_p( p, RAM, 0x8000);
			p = _memcpy_p( p, VRAM, VRAMSIZE);
			p = _memcpy_p( p, SPRAM, 64*4*sizeof(short));
			p = _memcpy_p( p, Pal, 512);
			p = _memcpy_p( p, &scanline, sizeof(int));
			p = _memcpy_p( p, &prevline, sizeof(int));
			p = _memcpy_p( p, &TimerCount, sizeof(int));
			p = _memcpy_p( p, &io, sizeof(IO));
			p = _memcpy_p( p, &M, sizeof(M6502));
			p = _memcpy_p( p, &CycleOld, 4);
			p = _memcpy_p( p, &ini_data, sizeof(ini_info));
		//	Error_count( (int)p-(int)state_b1);	//	サイズチェック
			s = iyanbakan_e( state_b2, state_b1, 192564 );
		}else{
			state_ver = 7;

			sceIoWrite(fd, &state_ver, 4);
			p = _memcpy_p( p, thumbnail_buffer, 128*96*2);
			p = _memcpy_p( p, RAM, 0x8000);
			p = _memcpy_p( p, VRAM, VRAMSIZE);
			p = _memcpy_p( p, SPRAM, 64*4*sizeof(short));
			p = _memcpy_p( p, Pal, 512);
			p = _memcpy_p( p, &scanline, sizeof(int));
			p = _memcpy_p( p, &prevline, sizeof(int));
			p = _memcpy_p( p, &TimerCount, sizeof(int));
			p = _memcpy_p( p, &io, sizeof(IO));
			p = _memcpy_p( p, &M, sizeof(M6502));
			p = _memcpy_p( p, &CycleOld, 4);
			p = _memcpy_p( p, &ini_data, sizeof(ini_info));
			p = _memcpy_p( p, cd_extra_mem, 0x10000);
			p = _memcpy_p( p, cd_extra_super_mem, 0x30000);
		//	Error_count( (int)p-(int)state_b1);	//	サイズチェック
			s = iyanbakan_e( state_b2, state_b1, 192564+0x10000+0x30000 );
		}
		
		sceIoWrite(fd, state_b2, s);

		sceIoClose(fd);
		System_mes("state write");
	}
}
char	wram_filename[]="wram.dat";
void	wram_read(void)
{
	int fd;
	char	n[1024];
	_strcpy( n, path_main);
	_strcat( n, wram_filename);
	fd = sceIoOpen( n, O_RDONLY, 0777);
	if( fd<0 ){
	//	Error_mes("wram read error");
		_memset(WRAM,0,0x2000);
		return;
	}
	
	sceIoRead(fd, WRAM, 0x2000);
	sceIoClose(fd);
}
void	wram_write(void)
{
	int fd;
	char	n[1024];
	_strcpy( n, path_main);
	_strcat( n, wram_filename);
//Error_mes(n);
	fd = sceIoOpen( n, O_CREAT|O_WRONLY|O_TRUNC, 0777);
	if( fd<0 ){
		Error_mes("wram read error");
		return;
	}
	sceIoWrite(fd, WRAM, 0x2000);
	sceIoClose(fd);
}

#include "zlibInterface.h"
		int fsize;
int	zipchk(char *name)
{
	char *n = name;
	while(*n){
		if( n[0]=='.' &&
			(n[1]=='z'||n[1]=='Z') &&
			(n[2]=='i'||n[2]=='I') &&
			(n[3]=='p'||n[3]=='P') &&
			!n[4] 
		){
			return 1;
		}
		n++;
	}
	return 0;
}
int	bmpchk(char *name)
{
	char *n = name;
	while(*n){
		if( n[0]=='.' &&
			(n[1]=='b'||n[1]=='B') &&
			(n[2]=='m'||n[2]=='M') &&
			(n[3]=='p'||n[3]=='P') &&
			!n[4] 
		){
			return 1;
		}
		n++;
	}
	return 0;
}
int	pcechk(char *name)
{
	char *n = name;
	while(*n){
		if( n[0]=='.' &&
			(n[1]=='p'||n[1]=='P') &&
			(n[2]=='c'||n[2]=='C') &&
			(n[3]=='e'||n[3]=='E') &&
			!n[4] 
		){
			return 1;
		}
		n++;
	}
	return 0;
}
			extern	char	BmpBuffer[];
int funcUnzipCallback(int nCallbackId,
                      unsigned long ulExtractSize,
		      unsigned long ulCurrentPosition,
                      const void *pData,
                      unsigned long ulDataSize,
                      unsigned long ulUserData)
{
    const char *pszFileName;
    const unsigned char *pbData;

    switch(nCallbackId) {
    case UZCB_FIND_FILE:
	pszFileName = (const char *)pData;
		switch( ulUserData ){
		case	0:
			if( ulExtractSize < 0x2000 ){
				return  UZCBR_PASS;
			}else
			if( ulExtractSize < ROMSIZE ){
				char	patch[MAXPATH];
				fsize=0;
				memcpy( patch, (unsigned char*)pszFileName, MAXPATH);
				state_file_patch( patch);
				return  UZCBR_OK;
			}else{
				return  UZCBR_CANCEL;
			}
			break;
		case	1:
			if( ulExtractSize == 391736 ){
				fsize=0;
				return  UZCBR_OK;
			}else{
				return  UZCBR_PASS;
			}
			break;
		}
        break;
    case UZCB_EXTRACT_PROGRESS:
	pbData = (const unsigned char *)pData;
		switch( ulUserData ){
		case	0:
			{
				char *p=(void *)ulCurrentPosition;
				_memcpy( (void *)&ROM_buff[fsize], (void *)pData, ulDataSize);
				fsize += ulDataSize;
			}
			break;
		case	1:
			{
				char *p=(void *)ulCurrentPosition;
				_memcpy( (void *)&BmpBuffer[fsize], (void *)pData, ulDataSize);
				fsize += ulDataSize;
			}
			break;
		}
		return  UZCBR_OK;
        break;
    default: // unknown...
        break; 
    }
    return 1;
}
int CartLoad(char *name)
{
	unsigned int fsize2;
	if( zipchk(name) ){
 		if( Unzip_execExtract(name, 0) == UZEXR_OK ){
//Error_count(fsize);
			fsize2 = fsize;
			fsize2&=~0x1fff;
			ROM_size = fsize2/0x2000;
		}else{
			return -1;
		}
	}else
	if( pcechk(name) ){
		int fd1,len;
		fd1 = sceIoOpen(name,O_RDONLY,0777);
		if( !fd1 )return -1;
		state_file_patch(name);
		len = sceIoRead(fd1, (char *)ROM_buff, ROMSIZE);
		sceIoClose(fd1);
		fsize = len;
		fsize2 = fsize;
		fsize2&=~0x1fff;
		ROM_size = fsize2/0x2000;

		System_mes(name);
		System_count(fsize);
		System_mes(" size");
	}else{
		return -1;
	}
	
	ROM = &ROM_buff[fsize&0x1fff];
/*
//	ROM = &ROM_buff[0];
    if((fsize / 512) & 1)
    {
        fsize -= 0x200;
//        ROM += 0x200;
Error_mes("512Bヘッダー");
    }

    if(fsize == 0x60000)
    {
        _memcpy(ROM + 0x80000, ROM + 0x40000, 0x20000);
Error_mes("確認0x60000");
    }
    else // Split 512K images if requested 
    if( (fsize == 0x80000))
    {
        _memcpy(ROM + 0x80000, ROM + 0x40000, 0x40000);
Error_mes("確認0x80000");
    }
*/
 //       ROM -= 0x200;
//		ROM = &ROM_buff[fsize&0x1fff];

/*
	FILE *fp;
	int fsize;
	fp=fopen(name,"rb");
	if (fp==NULL) {
	//	TRACE("%s not found.\n",name);
		return -1;
	}
	fseek(fp,0,SEEK_END);
	fsize = ftell(fp);
	fseek(fp,fsize&0x1fff,SEEK_SET);
	//printf("seekptr %x",ftell(fp));
	fsize&=~0x1fff;
//	ROM = (BYTE *)malloc(fsize);
	ROM_size = fsize/0x2000;
	//printf("ROM size:%x\n",fsize);
	fread(ROM,1,fsize,fp);
	fclose(fp);
*/
	return 0;
}



void ResetPCE(M6502 *M)
{
	_memset(M,0,sizeof(*M));
	TimerCount = TimerPeriod;
	M->IPeriod = IPeriod;
	M->TrapBadOps = 1;
	_memset(&io, 0, sizeof(IO));
	scanline = 0;
	prevline = 0;
	io.vdc_status=0;//VDC_InVBlank;
	io.vdc_inc = 1;
	io.minline = 0;
	io.maxline = 255;
	io.irq_mask = 0;
	io.psg_volume = 0;
	io.psg_ch = 0;
	{
		int i;
		for (i = 0; i < 6; i++) {
			io.PSG[i][4] = 0x80;
		}
		for(i = 0; i < 5; i++) {
			io.shiftmode[i] = 0;
		}
	}
	CycleOld = 0;
	Reset6502(M);

	if (1) {
		WORD x;
	//	LogDumpDetail("Will hook cd functions\n");
		if (!minimum_bios_hooking)
			for (x = 0x01; x < 0x4D; x++)
				if (x != 0x22) {// the 0x22th jump is special, points to a one byte routine
					WORD dest;
					dest = Op6502 (0xE000 + x * 3 + 1);
					dest += 256 * Op6502 (0xE000 + x * 3 + 2);
					CDBIOS_replace[x][0] = Op6502 (dest);
					CDBIOS_replace[x][1] = Op6502 (dest + 1);
					_Wr6502 (dest, 0xFC);
					_Wr6502 (dest + 1, (byte) x);
				}
    }

	//printf("reset PC=%04x ",M->PC.W);
}

int InitPCE(char *filename)
{
	int i,ROMmask;
/*
	if (*name==1) {
		if (*(DWORD *)name!=1)
			return -1;
		cart_name = name+sizeof(DWORD)*3;
		if (CartLoadMem((char *)((DWORD *)name)[1], ((DWORD *)name)[2]))
			return -1;
	} else {
		cart_name = name;
		if (CartLoad(name))
			return -1;
	}
*/
	if( !state_flag )
		if( CartLoad(filename) )	return -1;

	mh_strncpy( ini_data.path_rom, now_path, MAXPATH);



/*
	if (strstr(cart_name, "populous.")) {
	//	TRACE("populus\n");
		populus = 1;
	} else
*/
		populus = 0;
	if(ROM_size > 256)
		sf2ce = 1;
	else
		sf2ce = 0;

	//	パレットテーブル作成
	{
		int i;
		for(i=0;i<65536;i++){
//			g=(((s&0x1c))+((s&0xe0)<<2)+((s&3)<<13));
			int s1=i&255;
			int s2=i/256;
			palt[i]=
				(((s1&0x1c))+((s1&0xe0)<<2)+((s1&3)<<13))+
				((((s2&0x1c))+((s2&0xe0)<<2)+((s2&3)<<13))<<16);
			palt2[i]=
				((((s1&0x03)+(s2&0x03))<<12)&0x7c00)+
				((((s1&0x1c)+(s2&0x1c))>> 1)&0x001f)+
				((((s1&0xe0)+(s2&0xe0))<< 1)&0x03e0);
				
		}
	}


//	DMYROM=(void *)&DMYROM_buff;//malloc(0x2000);
	_memset(DMYROM,NODATA,0x2000);
//	WRAM=(void *)&WRAM_buff;//malloc(0x2000);
//	_memset(WRAM,0,0x2000);
//	VRAM=(void *)&VRAM_buff;//malloc(VRAMSIZE);
//	VRAM2=(void *)&VRAM2_buff;//malloc(VRAMSIZE);
	_memset(IOAREA,0xFF,0x2000);
//	vchange = (void *)&vchange_buff;//malloc(VRAMSIZE/32);
	_memset(vchange,1,VRAMSIZE/32);
#if defined(USE_SPRITE_CACHE)
//	VRAMS=(void *)&VRAMS_buff;//malloc(VRAMSIZE);
//	vchanges = (void *)&vchanges_buff;//malloc(VRAMSIZE/128);
	_memset(vchanges,1,VRAMSIZE/128);
#endif //defined(USE_SPRITE_CACHE)



	CD_emulation = 0;



	if (1) {
		_memset (cd_extra_mem, 0, 0x10000);
		_memset (cd_extra_super_mem, 0, 0x30000);
	}


	ROMmask = 1;
	while(ROMmask<ROM_size) ROMmask<<=1;
	ROMmask--;
//	TRACE("ROMmask=%02X, ROM_size=%02X\n", ROMmask, ROM_size);
	for(i=0;i<0xF7;i++)	{
		if (ROM_size == 0x30){
		//	ROMMap[i]= ROM+(i&ROMmask)*0x2000;
			switch (i&0x70){
			case 0x00:
			case 0x10:
			case 0x50:
				ROMMap[i]= ROM+(i&ROMmask)*0x2000;
				break;
			case 0x20:
			case 0x60:
				ROMMap[i]= ROM+((i-0x20)&ROMmask)*0x2000;
				break;
			case 0x30:
			case 0x70:
				ROMMap[i]= ROM+((i-0x10)&ROMmask)*0x2000;
				break;
			case 0x40:
				ROMMap[i]= ROM+((i-0x20)&ROMmask)*0x2000;
				break;
			}
		}else
			ROMMap[i]= ROM+(i&ROMmask)*0x2000;
	}
	System_count(ROMmask);
	System_count(ROM_size);
	System_mes(" ROMmask ROMsize");
//		ROMMap[i]=ROM+(i%ROM_size+i/ROM_size*0x10)*0x2000;
/*		if (((i&ROMmask)+i/(ROMmask+1)) < ROM_size)
			ROMMap[i]=ROM+((i&ROMmask)+i/(ROMmask+1)*0x20)*0x2000;
		else
			ROMMap[i]=ROM;
*///		ROMMap[i]=ROM+(i&ROMmask)*0x2000;
	if (populus){
		ROMMap[0x40] = PopRAM + (0)*0x2000;
		ROMMap[0x41] = PopRAM + (1)*0x2000;
		ROMMap[0x42] = PopRAM + (2)*0x2000;
		ROMMap[0x43] = PopRAM + (3)*0x2000;
	}
	if (1) {
		for(i = 0; i < 8; i++)
			_memcpy(cd_extra_mem + i * 0x2000, ROMMap[0x80 + i], 0x2000);
		ROMMap[0x80] = cd_extra_mem;
		ROMMap[0x81] = cd_extra_mem + 0x2000;
		ROMMap[0x82] = cd_extra_mem + 0x4000;
		ROMMap[0x83] = cd_extra_mem + 0x6000;
		ROMMap[0x84] = cd_extra_mem + 0x8000;
		ROMMap[0x85] = cd_extra_mem + 0xA000;
		ROMMap[0x86] = cd_extra_mem + 0xC000;
		ROMMap[0x87] = cd_extra_mem + 0xE000;

		for (i = 0x68; i < 0x80; i++) {
			_memcpy(cd_extra_super_mem + 0x2000 * (i - 0x68), ROMMap[i], 0x2000);
			ROMMap[i] = cd_extra_super_mem + 0x2000 * (i - 0x68);
		}
	}
/*	ROMMap[0x80] = PopRAM + (0)*0x2000;
	ROMMap[0x81] = PopRAM + (1)*0x2000;
	ROMMap[0x82] = PopRAM + (2)*0x2000;
	ROMMap[0x83] = PopRAM + (3)*0x2000;
	ROMMap[0x84] = PopRAM + (4)*0x2000;
	ROMMap[0x85] = PopRAM + (5)*0x2000;
	ROMMap[0x86] = PopRAM + (6)*0x2000;
	ROMMap[0x87] = PopRAM + (7)*0x2000;
*/
	ROMMap[0xF7]=WRAM;
	ROMMap[0xF8]=RAM;
	ROMMap[0xF9]=RAM+0x2000;
	ROMMap[0xFA]=RAM+0x4000;
	ROMMap[0xFB]=RAM+0x6000;
	ROMMap[0xFF]=IOAREA; //NULL; /* NULL = I/O area */


	mp3_play_stop();


	return 0;
}

void		wavoutInit();
int	WriteADPCM(WORD *buf);
void	sound_put(void)
{
	{
		int i;

		for (  i = 0; i < 6; i++)
			WriteSoundData((void *)&fcpsoundbuff[i][soundc], i, 256);

		WriteADPCM((void *)&fcpsoundbuff[6][soundc]);

		if( (soundc+=256)>=(256*60) ){
			soundc=0;
		}
	}
	mp3_play_1sync();
}
void	pause_init(void)
{

	_memset(&fcpsoundbuff,0,SND1_MAXSLOT*735*60*2);
}
void	pause_end(void)
{

	mes_b=2;
		wavout_1secInit();
		sceCtrlRead(&ctl,1);
		syorioti=ctl.frame-(16666*3);
		syorioti_c=0;
		soundc=256*5;
}
extern	int	psp_exit_flag;
int RunPCE(void)
{

	ResetPCE(&M);
	io.VDC[CR].W|=0x80;	//	ScreenON	

	mes_b=2;

	//	wavoutInit();
		wavout_1secInit();
		sceCtrlRead(&ctl,1);
		syorioti=ctl.frame-(16666*3);
		syorioti_c=0;
		soundc=256*5;


	if( state_flag || ini_data.autosave ){
		if( state_flag==1 ){
		state_quick_read();
		}else{
			state_name_change();
			state_read();
			CpuClockSet(ini_data.CpuClock+3);
			if( state_f==0 )
				state_quick_write();
		}
		state_flag=0;

		BaseClock = ini_data.BaseClock;
		IPeriod = BaseClock/(scanlines_per_frame*60);
	//	UPeriod = 0;
		TimerPeriod = BaseClock/1000*3*1024/21480;
		TimerCount = TimerPeriod;
	//	M->IPeriod = IPeriod;

	}


	while(1){
		int i=VSyncTimer();
		static f=0;
/*
		if( i==1 ){
			if( BaseClock < 7160000 ){
				BaseClock+=16384;
				IPeriod = BaseClock/(scanlines_per_frame*60);
				TimerPeriod = BaseClock/1000*3*1024/21480;
			}
		}
		if( i>2 ){
			if( BaseClock > 1000000 ){
				BaseClock-=16384;
				IPeriod = BaseClock/(scanlines_per_frame*60);
				TimerPeriod = BaseClock/1000*3*1024/21480;
			}
		}
*/
		if( i>80 ){
			System_count(i);
//			System_mes(" フレームスキップ80超えてやばいっすよ");
			if( i>100 ){
//				Error_mes("100フレームスキップ超えたので強制終了");
//				return 1;
			}
		}
		while(i--){
			if(mes_c)mes_c--;
			exit_flag=0;
			skip_frame=i;

			if(i<ini_data.frame_skip){
				JoySticks(io.JOY);
				_Run6502(&M);
			}

			sound_put();
		}
		if( ctl.buttons ){
			if( !f ){
			//	if( (ctl.buttons&ini_data.key[8])==ini_data.key[8] && ini_data.key[8] ){
				if( ctl.buttons==ini_data.key[8] && ini_data.key[8] ){	//	quick load
					f=1;
					if( state_f ){
						mes_f=1;
						mes_c=60;
						state_flag=1;
						return 2;
					}
				}else
				if( ctl.buttons==ini_data.key[9] && ini_data.key[9] ){	//	quick save
					f=1;
					mes_f=2;
					mes_c=60;
					state_quick_write();
				}else
				if( ctl.buttons==ini_data.key[6] && ini_data.key[6] ){
					int r;
					f=1;
					pause_init();
					if( (r=menu_ini()) )return r;
					pause_end();
				}else
				if( ctl.buttons==ini_data.key[7] && ini_data.key[7] ){
					f=1;
					pause_init();
					if( menu_file() )return 1;
					pause_end();
				}else
				if( ctl.buttons==ini_data.key[10] && ini_data.key[10] ){	//	file load
					if( state_read_chk() ){
						f=1;
						mes_f=3;
						mes_c=60;
						state_flag=2;
						pause_end();
						return 2;
					}
				}else
				if( ctl.buttons==ini_data.key[11] && ini_data.key[11] ){	//	file save
					f=1;
					mes_f=4;
					mes_c=60;
					pause_init();
					state_write();
					pause_end();
				}else
				if( ctl.buttons==ini_data.key[12] && ini_data.key[12] ){
					f=1;
					ini_data.videomode=(ini_data.videomode+1)%7;
					mes_b=2;
				}
			}
		}else
			f=0;
		if( psp_exit_flag ){
			pause_init();
			return 1;
		}
	}

	return 1;
}

void TrashPCE()
{
/*
	FILE *fp;
	fp = fopen(backmemname, "wb");
	if (fp == NULL)
		LogDump("Can't open %s\n", backmemname);
	else
	{
		fwrite(WRAM, 0x2000, 1, fp);
		fclose(fp);
	}
*/
	//if (IOAREA) free(IOAREA);
/*
	if (vchange) free(vchange);
#if defined(USE_SPRITE_CACHE)
	if (vchanges) free(vchanges);
	if (VRAMS) free(VRAMS);
#endif //defined(USE_SPRITE_CACHE)
	if (DMYROM) free(DMYROM);
	if (WRAM) free(WRAM);
	if (VRAM) free(VRAM);
	if (VRAM2) free(VRAM2);
	if (ROM) free(ROM);
*/
}

#define	JOY_A	1
#define	JOY_B	2
#define	JOY_SELECT	4
#define	JOY_START	8
#define	JOY_UP	0x10
#define	JOY_DOWN	0x40
#define	JOY_LEFT	0x80
#define	JOY_RIGHT	0x20

int	JoySticks(byte *JS)
{
//	return Joysticks(JS);
	unsigned int i;
	for (i = 0; i < 1; i++) {
		
		JS[i]=0;

			if( (ctl.buttons & ini_data.key[2])  ){
				if( ((rapid1)%(ini_data.rapid1*2))<ini_data.rapid1 )JS[i] |= JOY_A;
				rapid1++;
			}else
				rapid1=0;
			if( (ctl.buttons & ini_data.key[3])  ){
				if( ((rapid2)%(ini_data.rapid2*2))<ini_data.rapid2 )JS[i] |= JOY_B;
				rapid2++;
			}else
				rapid2=0;

		if( ctl.buttons ){
			if( (ctl.buttons & ini_data.key[0])  )JS[i] |= JOY_A;
			if( (ctl.buttons & ini_data.key[1])  )JS[i] |= JOY_B;
			if( (ctl.buttons & ini_data.key[4])  )JS[i] |= JOY_SELECT;
			if( (ctl.buttons & ini_data.key[5])  )JS[i] |= JOY_START;
		}else{
		}
		if( ctl.buttons & CTRL_UP )JS[i] |= JOY_UP;
		if( ctl.buttons & CTRL_DOWN )JS[i] |= JOY_DOWN;
		if( ctl.buttons & CTRL_LEFT )JS[i] |= JOY_LEFT;
		if( ctl.buttons & CTRL_RIGHT )JS[i] |= JOY_RIGHT;
    
		if( ctl.analog[0] < 127-84 )JS[i] |= JOY_LEFT;
		if( ctl.analog[0] > 127+84 )JS[i] |= JOY_RIGHT;
		if( ctl.analog[1] < 127-84 )JS[i] |= JOY_UP;
		if( ctl.analog[1] > 127+84 )JS[i] |= JOY_DOWN;

	}
	return 0;
}

unsigned long	vram[((352+64) * (256))/4];

int pce_main(char *filename)
{
	int i;
	static int asf=0;

	Bmp_put(0);
	mh_print( 240-((_strlen(filename)*5)/2), 130, filename, rgb2col( 255,255,255),0,0);
	pgScreenFlipV();

	mes_f=0;

	asf++;
	XBuf = (unsigned char *)&vram;//(unsigned char *)pgGetVramAddr(0,0);
	System_mes_init();

	System_count(asf);System_mes("←ゲーム起動回数");


		BaseClock = ini_data.BaseClock;
		IPeriod = BaseClock/(scanlines_per_frame*60);
		UPeriod = 0;
		TimerPeriod = BaseClock/1000*3*1024/21480;


	if( !InitPCE(filename) ){

		i=RunPCE();

		TrashPCE();

	}

	if( ini_data.autosave && !state_flag ){
		state_slot = 0;
		state_name_change();
		state_write();
	}

//	wavoutStopPlay0();
	wavoutStopPlay1();

	_memset(&fcpsoundbuff,0,6*735*60*2);

	return i;
}


