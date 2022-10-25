
#include	"mp3.h"
#include	"clipmp3.h"
#include	"pg.h"
#include	"sound.h"
#include	"menu.h"

#if 0 /*0:êÆêîî≈/1:é¿êîî≈*/
#define MP3DRIVER		MP3RDRIVER
#define mp3_init		mp3r_init
#define mp3_decode_frame	mp3r_decode_frame
#endif

MP3DRIVER mp3;

int	mp3_flag=0;

#define	mp3_size_max	(65536*16*5)
char	mp3_buffer[mp3_size_max];
int		mp3_size;
//char	mp3_name[]="ms0:/í¥åZãM/28.mp3";
//char	mp3_name[]="ms0:/í¥åZãM/test.mp3";
//	21 0 68






struct_cd_toc	cd_toc[0x100];






int mp3_play_read_track(int t)
{
	int fd;
	char	n[1024];
	char	tn[3];
	char *wp;
	
	if( t<0 || t>99 ){
		Error_mes("mp3 track nb error");
		return 1;
	}
	_strcpy( n, ini_data.path_cd);
	wp=_strrchr( n, '/' );wp[1]=0;
	tn[0]='0'+(t/10);
	tn[1]='0'+(t%10);
	tn[2]=0;
	_strcat( n, tn);
	_strcat( n, ".mp3");
	fd = sceIoOpen( n, O_RDONLY, 0777);
	if(fd<0){
	//	Error_mes(n);
	//	Error_mes("mp3 open error");
		return	1;
	}
	mp3_size = sceIoRead(fd, mp3_buffer, mp3_size_max);
	sceIoClose(fd);

	return	0;
}
int	mp3_decode_size;
int	out_size;
int	sound_c;
int	mp3_play_init(void)
{
	if(mp3_init(&mp3, mp3_buffer, mp3_size) != 0){
		Error_mes("mp3 init error");
		return	1;
	}
	
	out_size=0;
	mp3_decode_size = mp3.c_fbuff * sizeof(short);
	if( !mp3_decode_size ){
		Error_mes("mp3 buff size error");
		return	1;
	}
	{
		int i=7;
		wavout_wavinfo_t *w=&wavout_snd1_wavinfo[i];//wavinfo_se[i];
		w->channels = 1;
		w->samplerate = mp3.sampling_frequency;
		w->samplecount = mp3_decode_size*16;
		w->datalength = mp3_decode_size*32;
		w->wavdata = (void *)&fcpsoundbuff[i][0];
		w->rateratio = (mp3.sampling_frequency*0x4000)/11025;
		w->playptr = 0;
		w->playptr_frac = 0;
		w->playloop = 1;
		wavout_snd1_playing[i]=1;
	//	wavoutStartPlay1(w);
	}
	sound_c=5*mp3_decode_size;
	
	mp3_flag=1;

	return	0;
}
int	mp3_play_stop(void)
{
	if( mp3_flag ){
		mp3_flag=0;
		wavout_snd1_playing[7]=0;
		_memset( &fcpsoundbuff[7], 0, 735*60*2);
	}
}
extern	unsigned char  cd_fade;
int	mp3_play_1sync(void)
{
	if( !mp3_flag )return 0;
	if( cd_fade >= 12 ){
		_memset((void*)fcpsoundbuff[7]+sound_c, 0, mp3_decode_size);

		sound_c += mp3_decode_size;
		if( (sound_c)>=(mp3_decode_size*32) ){
			sound_c-=mp3_decode_size*32;
		}

		return 0;
	}

	out_size-=mp3_decode_size*100;
	while( out_size<=mp3_decode_size*156 ){
		while( mp3_decode_frame(&mp3) ){
			mp3_play_stop();
			return	1;
/*
			if(mp3_init(&mp3, mp3_buffer, mp3_size) != 0){
				Error_mes("mp3_init error");
				return	1;
			}
*/
		}
		out_size += mp3_decode_size*156;

		if( cd_fade == 0 ){
			_memcpy((void*)fcpsoundbuff[7]+sound_c, mp3.fbuff, mp3_decode_size);
		}else{
			int i;
			static int	f[]={
				0,
				120,
				140,
				170,
				200,
				300,
				400,
				500,
				600,
				700,
				800,
				900,
			};
			for(i=0;i<mp3_decode_size/2;i++ ){
				int s=(mp3.fbuff[i]*100)/f[cd_fade];
				fcpsoundbuff[7][(sound_c/2)+i]=s;
			}
		}


		sound_c += mp3_decode_size;
		if( (sound_c)>=(mp3_decode_size*32) ){
			sound_c-=mp3_decode_size*32;
		}

	}
//		counter( 50, 70, mp3_decode_size, 7, -1);
//		counter( 50, 80, mp3.c_fbuff, 7, -1);
//	out_size += mp3_decode_size;
//Error_count(mp3_decode_size);	//	2304
/*	
	while(out_size){
		fcpsoundbuff[7][sound_c]=mp3.fbuff
		out_size--;
	}
*/
//	_memcpy(wavefile.out_buf + out_size, mp3.fbuff, mp3_decode_size);
//	fcpsoundbuff[i][soundc]

	return	0;
}


