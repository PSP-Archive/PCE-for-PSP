

int mp3_play_read_track(int t);
int	mp3_play_init(void);
int	mp3_play_1sync(void);
int	mp3_play_stop(void);

typedef struct
{
	int	LBA;
	unsigned char	min;
	unsigned char	sec;
	unsigned char	fra;
	unsigned char	type;

}struct_cd_toc;
extern	struct_cd_toc	cd_toc[0x100];

