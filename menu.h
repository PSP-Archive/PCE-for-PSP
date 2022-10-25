


#define MAXDEPTH  16
#define MAXDIRNUM 1024
#define MAXPATH   0x108
#define PATHLIST_H 23
#define REPEAT_TIME 0x40000

extern	char			path_main[MAXPATH];

extern	char			target[MAXPATH];
extern	char			now_path[MAXPATH];

extern	unsigned long control_bef_ctl  ;
extern	unsigned long control_bef_tick ;

extern	dirent_t		dlist[MAXDIRNUM];
extern	int				dlist_curpos;


typedef struct {
	int		ver;
	char	path_rom[MAXPATH];
	char	path_cd[MAXPATH];
	int		BaseClock;
	int		frame_skip;
	int		Debug;
	int		key[13];
	int		WallPaper[2];
	char	path_WallPaper_menu[MAXPATH];
	char	path_WallPaper_game[MAXPATH];
	int		sound,pcm;
	int		autosave;
	int		videomode;
	int		CpuClock;
	int		rapid1;
	int		rapid2;
	int		dummy[100];
} ini_info;
extern	ini_info	ini_data;

int menu_Control(void) ;
void menu_Draw(void) ;
void menu_Get_DirList(char *path) ;
void	menu_init(void);

void	ini_read(void);
void	ini_write(void);

int		menu_ini(void);
int		menu_file(void);

void	Bmp_put(int i);


