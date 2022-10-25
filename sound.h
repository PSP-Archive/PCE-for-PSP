

#define SND1_MAXSLOT 8



typedef struct {
	unsigned long channels;
	unsigned long samplerate;
	unsigned long samplecount;
	unsigned long datalength;
	char *wavdata;
	unsigned long rateratio;		// samplerate / 44100 * 0x10000
	unsigned long playptr;
	unsigned long playptr_frac;
	int playloop;
} wavout_wavinfo_t;
int wavoutStartPlay1(wavout_wavinfo_t *wi);
extern	wavout_wavinfo_t wavout_snd1_wavinfo[SND1_MAXSLOT];
extern	int wavout_snd1_playing[SND1_MAXSLOT];

int wavoutLoadWav(const char *filename, wavout_wavinfo_t *wi, void *buf, unsigned long buflen);
void wavoutClearWavinfoPtr(wavout_wavinfo_t *wi);
void wavoutStopPlay0();
void wavoutStartPlay0(wavout_wavinfo_t *wi);
int wavoutWaitEnd0();

//extern	wavout_wavinfo_t wavinfo_se[SND1_MAXSLOT];
extern	unsigned short fcpsoundbuff[SND1_MAXSLOT][735*60];

