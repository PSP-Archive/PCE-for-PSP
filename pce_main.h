
#define USE_SPRITE_CACHE 1

#define	WIDTH	(352+64)
#define	HEIGHT	256

#include "M6502.h"

typedef struct tagIO {
	pair VDC[32];
//	byte VDC_ratch[32];
	pair VCE[0x200];
	pair vce_reg;
	byte vce_cr;	// add (ver0.12)
	/* VDC */
	word vdc_inc,vdc_raster_count;
	byte vdc_reg,vdc_status,vdc_ratch,vce_ratch;
//	byte vdc_iswrite_h;
	byte vdc_satb;
	byte vdc_pendvsync;
	int bg_h,bg_w;
	int screen_w,screen_h;
	int scroll_y;
	int minline, maxline;
	/* joypad */
	byte JOY[16];
	int shiftmode[16];
	byte joy_select,joy_counter;
	/* PSG */
	byte PSG[6][8],wave[6][32],wavofs[6];
	byte psg_ch,psg_volume,psg_lfo_freq,psg_lfo_ctrl;
	/* TIMER */
	byte timer_reload,timer_start,timer_counter;
	/* IRQ */
	byte irq_mask,irq_status;
	/* CDROM extention */
	int backup,adpcm_firstread;
	pair adpcm_ptr;
	word adpcm_rptr,adpcm_wptr;
/* CAREFUL, added variable */
   word adpcm_dmaptr;

/* CAREFUL, added variable */
   byte adpcm_rate;

/* CAREFUL, added variable */
   DWORD adpcm_pptr; /* to know where to begin playing adpcm (in nibbles) */

/* CAREFUL, added variable */
   DWORD adpcm_psize; /* to know how many 4-bit samples to play */
} IO;

extern IO io;
extern int UPeriod;
extern int BaseClock;

void	thumbnail_buffer_set(void);
extern	short	thumbnail_buffer[128*96];


// =-=- for CDROM
void pce_cd_read_sector (void);

extern byte PCM[];
// ADPCM buffer
extern byte cd_port_1800;
extern byte cd_port_1801;
extern byte cd_port_1802;
extern byte cd_port_1804;
extern byte bcdbin[0x100];
extern byte binbcd[0x100];
extern byte cd_sector_buffer[];
extern byte cd_extra_mem[];
// extra ram provided by the system CD card

extern byte *cd_read_buffer;
extern byte cd_extra_super_mem[];
extern DWORD pce_cd_read_datacnt;
extern DWORD pce_cd_sectoraddy;
extern byte cd_sectorcnt;
typedef struct
    {
     DWORD offset;
     byte  new_val;
     } PatchEntry;
typedef struct
    {
     DWORD StartTime;
     DWORD Duration;
     char data[32];

     } SubtitleEntry;

typedef enum {
	HCD_SOURCE_REGULAR_FILE,
        HCD_SOURCE_CD_TRACK
	} hcd_source_type;

typedef struct
    {
     byte beg_min;
     byte beg_sec;
     byte beg_fra;

     byte type;

     DWORD beg_lsn;
     DWORD length;

     hcd_source_type source_type;
     char filename[256];

     DWORD patch_number;
     DWORD subtitle_number;

     byte subtitle_synchro_type;

     PatchEntry *patch;
     SubtitleEntry *subtitle;

     } Track;
extern Track CD_track[0x100];

extern byte nb_max_track;

extern byte minimum_bios_hooking;
extern int pce_enable_cdrom;
extern char ISO_filename[256];
extern byte CD_emulation;
extern byte WRAM[];
extern byte CDBIOS_replace[0x4d][2];
extern byte *Page[];



DWORD msf2nb_sect(byte min, byte sec, byte frm);
void nb_sect2msf(DWORD lsn, byte * min, byte * sec, byte * frm);

void	wram_write(void);
void	wram_read(void);


