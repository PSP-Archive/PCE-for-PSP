
#include "pg.h"
#include "menu.h"
#include "_clib.h"

char	title[]="PCE for PSP 0.7 動かないそふと多目です＞ｗ＜";

extern	int	psp_exit_flag;
// ---------------------------------------------------------------------------------
//#define BUFSIZE		(65536*16)

dirent_t		dlist[MAXDIRNUM];
int				dlist_curpos;
int				dlist_num;
char			now_path[MAXPATH];
char			path_tmp[MAXPATH];
int				dlist_start;
int				cbuf_start[MAXDEPTH];
int				cbuf_curpos[MAXDEPTH];
int				now_depth;
//char			buf[BUFSIZE];
char			target[MAXPATH];
char			now_path[MAXPATH];
char			path_main[MAXPATH];
unsigned long control_bef_ctl  = 0;
unsigned long control_bef_tick = 0;

char *mh_strncpy(char *s1,char *s2,int n) {
	char *rt = s1;

	while((*s2!=0) && ((n-1)>0)) {
		*s1 = *s2;
		s1++;
		s2++;
		n--;
	}
	*s1 = 0;

	return rt;
}

char *mh_strncat(char *s1,char *s2,int n) {
	char *rt = s1;

	while((*s1!=0) && ((n-1)>0)) {
		s1++;
		n--;
	}

	while((*s2!=0) && ((n-1)>0)) {
		*s1 = *s2;
		s1++;
		s2++;
		n--;
	}
	*s1 = 0;

	return rt;
}

typedef struct tagBITMAPINFOHEADER{
    unsigned long  biSize;
    long           biWidth;
    long           biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned long  biCompression;
    unsigned long  biSizeImage;
    long           biXPixPerMeter;
    long           biYPixPerMeter;
    unsigned long  biClrUsed;
    unsigned long  biClrImporant;
} BITMAPINFOHEADER;
#define		BmpBuffer_Max	(391736+1)
char	BmpBuffer[BmpBuffer_Max];
short	BmpBuffer2[2][480*272];
int	bmp_f[2];
int	bmp_i=-1;
void	BmpAkarusa(int i)
{
	if(!bmp_f[i])return;
	{
		int x,y;
		unsigned char *rb=(void *)&BmpBuffer;
		int	r,g,b;
		unsigned short *wb=(void *)&BmpBuffer2[i][0];
		int a=ini_data.WallPaper[i];
		rb+=14+40;
		wb+=(272-1)*480;
		for(y=0;y<272;y++){
			for(x=0;x<480;x++){
				b=*rb++;
				g=*rb++;
				r=*rb++;
				b=((b*a)+1)/101;
				g=((g*a)+1)/101;
				r=((r*a)+1)/101;
				*wb++=
					(((b>>3) & 0x1F)<<10)+
					(((g>>3) & 0x1F)<<5)+
					(((r>>3) & 0x1F)<<0);//+0x8000;
			}
			wb-=480*2;
		}
	}
}
int	bmpchk(char *name);
int	zipchk(char *name);
#include "zlibInterface.h"
int funcUnzipCallback(int nCallbackId,
                      unsigned long ulExtractSize,
		      unsigned long ulCurrentPosition,
                      const void *pData,
                      unsigned long ulDataSize,
                      unsigned long ulUserData);
int		BmpRead(int i,char *fname)
{
	int fd1,len;
	unsigned long	offset;
	BITMAPINFOHEADER	bm;
	int c;

	bmp_f[i]=0;
//	BmpBuffer[0]=0;
	
	if( bmpchk(fname) ){
		fd1 = sceIoOpen( fname, O_RDONLY, 0777);
		len = sceIoRead(fd1, BmpBuffer, BmpBuffer_Max);
		sceIoClose(fd1);
	}else
	if( zipchk(fname) ){
 		if( Unzip_execExtract(fname, 1) == UZEXR_OK ){
		}else return 0;
	}else
		return 0;

	if( BmpBuffer[0]!='B' || BmpBuffer[1]!='M' )return 0;

//	if( len!=BmpBuffer_Max ){Error_mes("ファイルサイズ変");return 0;}

	{
		int i=40;
		char *p=(char *)&bm,*g=(char *)&BmpBuffer[14];
		while(i--)*p++=*g++;
	}

	if(bm.biBitCount!=24){Error_mes("24ビット以外対応してねぇよ！");return 0;}
	if(bm.biWidth !=480){Error_mes("横のサイズ480以外対応してねぇよ！！");return 0;}
	if(bm.biHeight!=272){Error_mes("縦のサイズ272以外対応してねぇよ！！");return 0;}

	bmp_i=i;
	bmp_f[i]=1;
	BmpAkarusa(i);
	return 1;
}

void	Bmp_put(int i)
{
	unsigned long *vr;
	unsigned long *g=(void *)&BmpBuffer2[i][0];
	int x,y;

	if( !bmp_f[i] ){
		int a=ini_data.WallPaper[i]/4;
		pgFillvram(a+(a<<5)+(a<<10));
		return;
	}

	vr = (void *)pgGetVramAddr(0,0);
	for(y=0;y<272;y++){
		for(x=0;x<480/2;x++){
			*vr++=*g++;
		}
		vr+=(LINESIZE-480)/2;
	}
	
}


unsigned long Read_Key(void) {
	ctrl_data_t ctl;

	sceCtrlRead(&ctl,1);
	if (ctl.buttons == control_bef_ctl) {
		if ((ctl.frame - control_bef_tick) > REPEAT_TIME) {
			return control_bef_ctl;
		}
		return 0;
	}
	control_bef_ctl  = ctl.buttons;
	control_bef_tick = ctl.frame;
	return control_bef_ctl;
}

int	file_type=0;
int	kakutyousichk(char *n)
{
	char *p;
	p=n;
	while(*p){
		if( *p++=='.' )break;
	}
	if( !*p )return 1;
	p=n;
	while(*p){
		if(	p[0]=='.' &&
			( p[1]=='z' || p[1]=='Z' ) &&
			( p[2]=='i' || p[2]=='I' ) &&
			( p[3]=='p' || p[3]=='P' ) &&
			!p[4]
		)return 1;
		switch(file_type){
		case	0:
		if(	p[0]=='.' &&
			( p[1]=='p' || p[1]=='P' ) &&
			( p[2]=='c' || p[2]=='C' ) &&
			( p[3]=='e' || p[3]=='E' ) &&
			!p[4]
		)return 1;
			break;
		case	1:
		if(	p[0]=='.' &&
			( p[1]=='b' || p[1]=='B' ) &&
			( p[2]=='m' || p[2]=='M' ) &&
			( p[3]=='p' || p[3]=='P' ) &&
			!p[4]
		)return 1;
			break;
		case	2:
		if(	p[0]=='.' &&
			( p[1]=='t' || p[1]=='T' ) &&
			( p[2]=='o' || p[2]=='O' ) &&
			( p[3]=='c' || p[3]=='C' ) &&
			!p[4]
		)return 1;
			break;
		}
		p++;
	}
	return 0;
}

void menu_Get_DirList(char *path) {
	int ret,fd;

	// Directory read
	fd = sceIoDopen(path);
	dlist_num = 0;
	ret = 1;
	if( now_depth>0 ){
		dlist[dlist_num].name[0] = '.';
		dlist[dlist_num].name[1] = '.';
		dlist[dlist_num].name[2] = 0;
		dlist[dlist_num].type = TYPE_DIR;
		dlist_num++;
	}
	while((ret>0) && (dlist_num<MAXDIRNUM)) {
		ret = sceIoDread(fd, &dlist[dlist_num]);
		if (dlist[dlist_num].name[0] == '.') continue;
		if (!kakutyousichk(dlist[dlist_num].name)) continue;
		if (ret>0) dlist_num++;
	}
	sceIoDclose(fd);

	// ディレクトリリストエラーチェック
	if (dlist_start  >= dlist_num) { dlist_start  = dlist_num-1; }
	if (dlist_start  <  0)         { dlist_start  = 0;           }
	if (dlist_curpos >= dlist_num) { dlist_curpos = dlist_num-1; }
	if (dlist_curpos <  0)         { dlist_curpos = 0;           }
}





int	pcechk(char *name);
int	thumbnail_cf;
//						char	thumbnail_b1[(128*96*2)+1000];
extern		char	state_b1[];
extern		char	state_b2[];
extern	short	thumbnail_buffer[];
void menu_file_Draw(void) {
	int			i,col;

	Bmp_put(0);

	// 現在地
	mh_strncpy(path_tmp,now_path,40);
	mh_print(0,0,path_tmp,rgb2col(255,255,255),0,0);
	mh_print(480-(sizeof(title)*5), 0,title,rgb2col( 55,255,255),0,0);

	mh_print(0,262,"○：決定　×：キャンセル　△：フォルダ戻る",rgb2col( 255,255,255),0,0);

	// ディレクトリリスト
	i = dlist_start;
	while (i<(dlist_start+PATHLIST_H)) {
		if (i<dlist_num) {
			col = rgb2col(255,255,255);
			if (dlist[i].type & TYPE_DIR) {
				col = rgb2col(255,255,0);
			}
			if (i==dlist_curpos) {
				col = rgb2col(255,0,0);
			}
			mh_strncpy(path_tmp,dlist[i].name,40);
			mh_print(32,((i-dlist_start)+2)*10,path_tmp,col,0,0);
		}
		i++;
	}

	if( !file_type ){
			static char	thumbnail_name[1024],*p;
		if( !thumbnail_cf){
			char	*p;
			_strcpy( thumbnail_name, path_main);
			_strcat( thumbnail_name, "SAVE/");
		//	if( pcechk(dlist[dlist_curpos].name) ){
			//	p=_strrchr( dlist[dlist_curpos].name,'/');
				_strcat( thumbnail_name, dlist[dlist_curpos].name);
		//	}
			p=_strrchr( thumbnail_name,'.');
			if(p)_strcpy(p,".SAV");
			
			
			thumbnail_cf=1;
			{
				int fd;
				int	state_ver;
				fd = sceIoOpen( thumbnail_name, O_RDONLY, 0777);
				if(fd>=0){
					sceIoRead(fd, &state_ver, 4);
					if( state_ver >= 4 ){
						int s;
						s=sceIoRead(fd, state_b1, (128*96*2) );
						iyanbakan_d( state_b2, state_b1, s );
						
						thumbnail_cf=2;
					}
					sceIoClose(fd);
				}
			}
		}
		if( thumbnail_cf==2 ){
			mh_print( 480-(_strlen(thumbnail_name)*5), 272-25, thumbnail_name,rgb2col( 155,155,155),0,0);
			{
				int x,y;
				unsigned int *vr;
				unsigned int *v = (void*)state_b2;
				vr = (void *)pgGetVramAddr(320,150);
				for(y=0;y<96;y++){
					for(x=0;x<128/2;x++){
						*vr++=*v++;
					}
					vr+=(LINESIZE-128)/2;
				}
			}
		}

	}

	pgScreenFlipV();
}

int menu_file_Control(void) {
	unsigned long key;
	int i,f;

	// wait key
	while(!psp_exit_flag) {
		key = Read_Key();
		if (key != 0) break;
		pgWaitV();
	}

	f=dlist_curpos;
	if (key & CTRL_UP) {
		if (dlist_curpos > 0) {
			dlist_curpos--;
			if (dlist_curpos < dlist_start) { dlist_start = dlist_curpos; }
		}
	}
	if (key & CTRL_DOWN) {
		if (dlist_curpos < (dlist_num-1)) {
			dlist_curpos++;
			if (dlist_curpos >= (dlist_start+PATHLIST_H)) { dlist_start++; }
		}
	}
	if (key & CTRL_LEFT) {
		dlist_curpos -= PATHLIST_H;
		if (dlist_curpos <  0)          { dlist_curpos = 0;           }
		if (dlist_curpos < dlist_start) { dlist_start = dlist_curpos; }
	}
	if (key & CTRL_RIGHT) {
		dlist_curpos += PATHLIST_H;
		if (dlist_curpos >= dlist_num) { dlist_curpos = dlist_num-1; }
		while (dlist_curpos >= (dlist_start+PATHLIST_H)) { dlist_start++; }
	}
	if( dlist_curpos!=f )
		thumbnail_cf=0;

	if (key & CTRL_CIRCLE) {
		if( dlist[dlist_curpos].name[0]=='.' ){
			goto pathmove;
		}
		if (dlist[dlist_curpos].type & TYPE_DIR) {
			if (now_depth<MAXDEPTH) {
				// path移動
				mh_strncat(now_path,dlist[dlist_curpos].name,MAXPATH);
				mh_strncat(now_path,"/",MAXPATH);
				cbuf_start[now_depth] = dlist_start;
				cbuf_curpos[now_depth] = dlist_curpos;
				dlist_start  = 0;
				dlist_curpos = 0;
				now_depth++;
				return 1;
			}
		} else {
			return 2;
		}
	}
	if (key & CTRL_TRIANGLE) {
		pathmove:
		if (now_depth > 0) {
			// path移動
			for(i=0;i<MAXPATH;i++) {
				if (now_path[i]==0) break;
			}
			i--;
			while(i>4) {
				if (now_path[i-1]=='/') {
					now_path[i]=0;
					break;
				}
				i--;
			}
			now_depth--;
			dlist_start  = cbuf_start[now_depth];
			dlist_curpos = cbuf_curpos[now_depth];
			return 1;
		}
	}
	if( key & CTRL_CROSS )return 3;
	
	return 0;
}
int		menu_file(void)
{
	thumbnail_cf=0;
	while( !psp_exit_flag ){
		menu_file_Draw();
		switch(menu_file_Control()) {
		case 1:
			menu_Get_DirList(now_path);
			break;
		case 2:
			mh_strncpy(target,now_path,MAXPATH);
			mh_strncat(target,dlist[dlist_curpos].name,MAXPATH);
/*
			CpuClockSet(ini_data.CpuClock+3);
			if( pce_main((char *)&target) ){
				state_f=0;
				return 1;
			}
*/
				return 1;
			break;
		case 3:
			return 0;
			break;
		}
	}
	return 0;
}


ini_info	ini_data;
char	ini_filename[]="config.ini";
void	ini_read(void)
{
	char path_ini[MAXPATH];
	int fd;

	wram_read();

	bmp_f[0]=bmp_f[1]=0;
	_strcpy( path_ini, path_main);
	_strcat( path_ini, ini_filename);
	fd = sceIoOpen(path_ini,O_RDONLY, 0777);
	if(fd>=0){
		sceIoRead(fd, &ini_data, sizeof(ini_info));
		sceIoClose(fd);
		if( ini_data.ver == 70 ){
			BmpRead(0,ini_data.path_WallPaper_menu);
			BmpRead(1,ini_data.path_WallPaper_game);
			return;
		}
		Error_mes("コンフィグファイルのバージョンが一致しないので初期化します");
	}
	_memset( &ini_data, 0, sizeof(ini_info));
	ini_data.ver = 70;
	_strcpy( ini_data.path_rom, path_main);
	_strcpy( ini_data.path_cd, path_main);
	ini_data.BaseClock = 7160000;;//5000000;//7160000;
	ini_data.frame_skip = 3;
	ini_data.Debug = 0;
	ini_data.key[0] = CTRL_CIRCLE;	//	I
	ini_data.key[1] = CTRL_CROSS;	//	II
	ini_data.key[2] = CTRL_TRIANGLE;//	RI
	ini_data.key[3] = CTRL_SQUARE;	//	RII
	ini_data.key[4] = CTRL_SELECT;	//	SELECT
	ini_data.key[5] = CTRL_START;	//	START
	ini_data.key[6] = CTRL_LTRIGGER;//	MENU
	ini_data.key[7] = 0;			//	file select
	ini_data.key[8] = 0;			//	quick load
	ini_data.key[9] = 0;			//	quick save
	ini_data.key[10] = 0;			//	file load
	ini_data.key[11] = 0;			//	file save
	ini_data.key[12] = 0;			//	video mode
	ini_data.path_WallPaper_menu[0]=0;
	ini_data.path_WallPaper_game[0]=0;
	ini_data.WallPaper[0]=
	ini_data.WallPaper[1]=10;
	ini_data.sound=1;
	ini_data.pcm=1;
	ini_data.autosave=0;
	ini_data.videomode=0;
	ini_data.CpuClock=0;
	ini_data.rapid1=2;
	ini_data.rapid2=2;
	_memset(ini_data.dummy,0,100*4);
}
void	ini_write(void)
{
	char path_ini[MAXPATH];
	int fd;

	wram_write();

	_strcpy( path_ini, path_main);
	_strcat( path_ini, ini_filename);
	fd = sceIoOpen( path_ini, O_CREAT|O_WRONLY|O_TRUNC, 0777);
	if(fd>=0){
		sceIoWrite(fd, &ini_data, sizeof(ini_info));
		sceIoClose(fd);
	}
}


void	menu_init(void)
{

	ini_read();

	// default value
//	mh_strncpy(now_path,"ms0:/",MAXPATH);
	mh_strncpy( now_path, ini_data.path_rom, MAXPATH);


	//
	//	romパッチ移動
	{
		char *m = ini_data.path_rom;
		char *n = now_path;
		while(1){
			if(!*m)break;
			else
			if(*m++=='/'){
				now_depth++;
			}
		}
		if( now_depth )now_depth--;

	}

	menu_Get_DirList(now_path);
	dlist_start  = 0;
	dlist_curpos = 0;


}

int	ini_cur=14;
int	key_config_cur;
void	menu_key_config_Draw(void)
{
	int			i,col;
	static int	cy[]={
		50,
		60,
		70,
		80,
		90,
		100,
		110,
		120,
		130,
		140,
		150,
		160,
		170,
		200,
		210,
		240,
	};

	Bmp_put(0);

	mh_print(480-(sizeof(title)*5), 0,title,rgb2col( 55,255,255),0,0);

//	mh_print(0,262,"△：戻る",rgb2col( 255,255,255),0,0);
	
	mh_print(60,cy[key_config_cur],"(^ω^)",rgb2col( 255,55,55),0,0);
	{
		int i;
		static char *t[]={
			"Ｉ",
			"II",
			"Ｉrapid",
			"IIrapid",
			"SELECT",
			"START",
			"MENU",
		//	"FILE",
			"",
			"QuickLoad",
			"QuickSave",
			"FileLoad",
			"FileSave",
			"VideoMode",
		};
		for(i=0;i<13;i++){
			int x=250,y=cy[i];
			mh_print( 100, y, t[i],rgb2col( 255,255,255),0,0);
			if( ini_data.key[i]&CTRL_SQUARE   ){mh_print( x, y,"□",rgb2col( 255,255,255),0,0);x+=12;}
			if( ini_data.key[i]&CTRL_TRIANGLE ){mh_print( x, y,"△",rgb2col( 255,255,255),0,0);x+=12;}
			if( ini_data.key[i]&CTRL_CROSS    ){mh_print( x, y,"×",rgb2col( 255,255,255),0,0);x+=12;}
			if( ini_data.key[i]&CTRL_CIRCLE   ){mh_print( x, y,"○",rgb2col( 255,255,255),0,0);x+=12;}
			if( ini_data.key[i]&CTRL_START    ){mh_print( x, y,"ST",rgb2col( 255,255,255),0,0);x+=12;}
			if( ini_data.key[i]&CTRL_SELECT   ){mh_print( x, y,"SE",rgb2col( 255,255,255),0,0);x+=12;}
			if( ini_data.key[i]&CTRL_LTRIGGER ){mh_print( x, y,"Ｌ",rgb2col( 255,255,255),0,0);x+=12;}
			if( ini_data.key[i]&CTRL_RTRIGGER ){mh_print( x, y,"Ｒ",rgb2col( 255,255,255),0,0);x+=12;}
			if( ini_data.key[i]&CTRL_LEFT     ){mh_print( x, y,"←",rgb2col( 255,255,255),0,0);x+=12;}
			if( ini_data.key[i]&CTRL_RIGHT    ){mh_print( x, y,"→",rgb2col( 255,255,255),0,0);x+=12;}
		}
	}
	mh_print( 100, cy[13],"Ｉ rapid speed",rgb2col( 255,255,255),0,0);
	counter(  200, cy[13], ini_data.rapid1, 3, -1);
	mh_print( 100, cy[14],"II rapid speed",rgb2col( 255,255,255),0,0);
	counter(  200, cy[14], ini_data.rapid2, 3, -1);
	mh_print( 100, cy[15],"exit",rgb2col( 255,255,255),0,0);

	{
		static char *t[]={
			"Ｉボタン",
			"ＩＩボタン",
			"Ｉボタン連打",
			"ＩＩボタン連打",
			"セレクト",
			"スタート",
			"このメニュー呼び出し",
		//	"ファイル読み込みメニュー呼び出し",
			"",
			"クイックロード（ファイルから読みません）",
			"クイックセーブ（ファイルに書き込みません）",
			"ファイルロード",
			"ファイルセーブ（クイックサーブもされちゃいます）",
			"画面出力モード",
			"連打速度　Ｉボタン　　数値が低いほど早い",
			"連打速度　ＩＩボタン　数値が低いほど早い",
			"コンフィグに戻る",
		};
		mh_print( 100, 260, t[key_config_cur],rgb2col( 155,255,155),0,0);
	}

	pgScreenFlipV();
}

int		menu_key_config_Control(void)
{
	unsigned long key;
	int i,x=0,y=0;

	// wait key
	while(!psp_exit_flag) {
		key = Read_Key();
		if (key != 0) break;
		pgWaitV();
	}
	if (key & CTRL_UP   ) y=-1;
	if (key & CTRL_DOWN ) y= 1;
	if (key & CTRL_LEFT ) x=-1;
	if (key & CTRL_RIGHT) x= 1;

	key_config_cur += y;
	if( key_config_cur <  0 )key_config_cur = 15;
	if( key_config_cur > 15 )key_config_cur =  0;

	switch( key_config_cur ){
	case	0:
	case	1:
	case	2:
	case	3:
	case	4:
	case	5:
	case	6:
//	case	7:
	case	8:
	case	9:
	case	10:
	case	11:
	case	12:
		ini_data.key[key_config_cur] ^= key&(0xf309+CTRL_LEFT+CTRL_RIGHT);
		break;
	case	13:
		ini_data.rapid1 += x;
		if( ini_data.rapid1 > 30 )ini_data.rapid1 =  1;
		if( ini_data.rapid1 <  1 )ini_data.rapid1 = 30;
		break;
	case	14:
		ini_data.rapid2 += x;
		if( ini_data.rapid2 > 30 )ini_data.rapid2 =  1;
		if( ini_data.rapid2 <  1 )ini_data.rapid2 = 30;
		break;
	case	15:
		if (key & CTRL_CIRCLE) {
			if( ini_data.key[6] ){
				return 1;
			}else{
				Error_mes("メニュー呼び出しに何か割り当てれ");
			}
		}
		break;
	}
	return	0;
}
int		menu_keyconfig_ini(void)
{
	key_config_cur = 15;
	while(!psp_exit_flag) {
		menu_key_config_Draw();
		switch(menu_key_config_Control()) {
		case	1:
			return 1;
		}
	}
	return 1;
}
int	ini_bmp;
void	RefreshScreen(void);
int	state_slot=0;
	char	*stateslot[]={
		".SAV",
		".SA1",
		".SA2",
		".SA3",
		".SA4",
		".SA5",
		".SA6",
		".SA7",
		".SA8",
		".SA9",
	};
extern	char	path_state[];
extern	unsigned char CD_emulation ;
void	state_name_change(void)
{
	if( state_slot<0 || state_slot>9 ){
		Error_mes("state name change nb error");
		return;
	}
	if( CD_emulation ){
		if( ini_data.path_cd[0] ){
			char	*p;
			_strcpy( path_state, path_main);
			_strcat( path_state, "SAVE");
			p=_strrchr( ini_data.path_cd,'/');
			_strcat( path_state, p);
		//	Error_mes(path_state);
		}
	}
	{
		char	*p;
		p=_strrchr( path_state,'.');
		if(p)_strcpy(p,stateslot[state_slot]);
	}
}

void	CpuClockSet(int i)
{
	static int c[]={
		133, 166, 190, 222, 266, 333, 
	};
	if( i< 0 || i>5 ){
		Error_mes("CpuClock setting error");
	}else{
		scePowerSetClockFrequency(c[i],c[i],c[i]/2);
	}
}

			extern	short	thumbnail_buffer[128*96];
void	menu_ini_Draw(void)
{
	int			i,col;
	static int	cy[]={
		20,
		30,
		40,
		50,
		60,
		70,
		90,
		110,
		120,
		140,
		160,
		170,
		180,
		200,
		210,
		225,
		240,
	};
	static char	*onoff[]={
		"OFF",
		"ON",
	};
	static char	*videomode[]={
		"normal",
		"1:1 272x272",
		"1:1 272x272 shading",
		"4:3 360x272",
		"4:3 360x272 shading",
		"wide 480x272",
		"wide 480x272 shading",
	};
	static char	*CpuClock[]={
		"222Mhz",
		"266Mhz",
		"333Mhz",
	};

	if(ini_cur==5){
		Bmp_put(1);
		RefreshScreen();
	}else{
		Bmp_put(ini_bmp);
	}

	if( ini_cur==11 || ini_cur==12 ){
/*
		if( ini_cur==12 ){
			thumbnail_buffer_set();
			thumbnail_cf=2;
		}
		if( ini_cur==11 ){
*/	
		{
			state_name_change();
			if( !thumbnail_cf){
				thumbnail_cf=1;
				{
					int fd;
					int	state_ver;

					fd = sceIoOpen( path_state, O_RDONLY, 0777);
					if(fd>=0){
						sceIoRead(fd, &state_ver, 4);
						if( state_ver >= 4 ){
							int s;
							s=sceIoRead(fd, state_b1, (128*96*2) );
							iyanbakan_d( state_b2, state_b1, s );
							_memcpy( thumbnail_buffer, state_b2, 128*96*2);
							thumbnail_cf=2;
						}
						sceIoClose(fd);
						mh_print( 480-(_strlen(path_state)*5), 272-25, path_state, rgb2col( 155,155,155),0,0);
					}

				}
			}
		}
		if( thumbnail_cf==2 )
		{
			int x,y;
			unsigned int *vr;
			unsigned int *v = (void*)thumbnail_buffer;
			vr = (void *)pgGetVramAddr(320,150);
			for(y=0;y<96;y++){
				for(x=0;x<128/2;x++){
					*vr++=*v++;
				}
				vr+=(LINESIZE-128)/2;
			}
		}
	}else{


	}

			thumbnail_buffer_set();
		{
			int x,y;
			unsigned int *vr;
			unsigned int *v = (void*)thumbnail_buffer;
			vr = (void *)pgGetVramAddr(320, 50);
			for(y=0;y<96;y++){
				for(x=0;x<128/2;x++){
					*vr++=*v++;
				}
				vr+=(LINESIZE-128)/2;
			}
		}

	mh_print(480-(sizeof(title)*5), 0,title,rgb2col( 55,255,255),0,0);

//	mh_print(0,262,"△：戻る",rgb2col( 255,255,255),0,0);
	
	mh_print(60,cy[ini_cur],"(^ω^)",rgb2col( 255,55,55),0,0);
	if( ini_data.autosave ){
		mh_print(100, cy[0],"BaseClock",rgb2col( 155,155,155),0,0);
		counter( 300, cy[0], ini_data.BaseClock, 7, rgb2col( 155,155,155));
	}else{
		mh_print(100, cy[0],"BaseClock",rgb2col( 255,255,255),0,0);
		counter( 300, cy[0], ini_data.BaseClock, 7, -1);
	}
	mh_print(100, cy[1],"AutoFrameSkip",rgb2col( 255,255,255),0,0);
	counter( 300, cy[1], ini_data.frame_skip, 1, -1);
	mh_print(100, cy[2],"Debug",rgb2col( 255,255,255),0,0);
	counter( 300, cy[2], ini_data.Debug, 1, -1);
	mh_print(100, cy[3],"sound",rgb2col( 255,255,255),0,0);
	mh_print(200, cy[3], onoff[ini_data.sound],rgb2col( 255,255,255),0,0);
	mh_print(100, cy[4],"pcm",rgb2col( 255,255,255),0,0);
	mh_print(200, cy[4], onoff[ini_data.pcm],rgb2col( 255,255,255),0,0);
	mh_print(100, cy[5],"video mode",rgb2col( 255,255,255),0,0);
	mh_print(200, cy[5], videomode[ini_data.videomode],rgb2col( 255,255,255),0,0);
	mh_print( 100, cy[6],"key config",rgb2col( 255,255,255),0,0);
	mh_print( 100, cy[7],"WallPaper menu",rgb2col( 255,255,255),0,0);
	counter(  190, cy[7], ini_data.WallPaper[0], 3, -1);
	mh_print( 200, cy[7], ini_data.path_WallPaper_menu,rgb2col( 155,155,155),0,0);
	mh_print( 100, cy[8],"WallPaper game",rgb2col( 255,255,255),0,0);
	counter(  190, cy[8], ini_data.WallPaper[1], 3, -1);
	mh_print( 200, cy[8], ini_data.path_WallPaper_game,rgb2col( 155,155,155),0,0);
//	mh_print( 100, cy[ 9],"config save",rgb2col( 255,255,255),0,0);
	mh_print( 100, cy[ 9],"PSP Clock",rgb2col( 255,255,255),0,0);
	mh_print( 200, cy[ 9], CpuClock[ini_data.CpuClock],rgb2col( 255,255,255),0,0);
	mh_print( 100, cy[10],"auto save load",rgb2col( 255,255,255),0,0);
	mh_print( 200, cy[10], onoff[ini_data.autosave],rgb2col( 255,255,255),0,0);
	mh_print( 100, cy[11],"state load",rgb2col( 255,255,255),0,0);
	mh_print( 100, cy[12],"state save",rgb2col( 255,255,255),0,0);
	mh_print( 200, cy[11]+5, stateslot[state_slot],rgb2col( 255,255,255),0,0);
	mh_print( 100, cy[13],"card change",rgb2col( 255,255,255),0,0);
	mh_print( 100, cy[14],"cd change",rgb2col( 255,255,255),0,0);
	mh_print( 200, cy[14], ini_data.path_cd,rgb2col( 155,155,155),0,0);
	mh_print( 100, cy[15],"continue",rgb2col( 255,255,255),0,0);
	mh_print( 100, cy[16],"exit",rgb2col( 255,255,255),0,0);

	{
		static char *t[]={
			"画面が壊れる場合 MAXで安定するかも。要オートセーブOFFで読込み直し",
			"ガクガク度合い。偶数だと点滅処理が見えない場合も",
			"ひ・み・つ",
			"音全般",
			"ボイスとか",
			"画面出力方法",
			"キーコンフィグ",
			"○で壁紙ファイル選択。480x272x24のBMP。ZIP対応。左右で明るさ調整",
			"○で壁紙ファイル選択。480x272x24のBMP。ZIP対応。左右で明るさ調整",
		//	"コンフィグセーブ。HOMEボタン終了時には保存されないので保険",
			"PSPのゲーム中クロック。デフォルト222　PSPが壊れても知りません",
			"開始時に読み込んだりゲームの切り替えや終了時に自動的に保存したり *.SAVのみ",
			"状態ファイルを読み込み　左右でスロット切り替え",
			"今の状態をファイルに保存します　左右でスロット切り替え",
			"Huカードを変えます。オートセーブの場合、状態セーブもされます",
			"CD ROM^2 の拡張子toc選択。　zip未対応　×で取り外し",
			"ゲームに戻る",
			"終了する",
		};
		mh_print( 100, 260, t[ini_cur],rgb2col( 155,255,155),0,0);
	}


	pgScreenFlipV();
}

extern int		state_flag;
extern	int BaseClock;
int	state_read_chk(void);
int	cd_toc_read(void);
int		qf;
int		menu_ini_Control(void)
{
	unsigned long key;
	int i,x=0,y=0;

	// wait key
	while(!psp_exit_flag) {
		key = Read_Key();
		if( !control_bef_ctl )qf=0;
		if (key != 0) break;
		pgWaitV();
	}
	if (key & CTRL_UP   ) y=-1;
	if (key & CTRL_DOWN ) y= 1;
	if (key & CTRL_LEFT ) x=-1;
	if (key & CTRL_RIGHT) x= 1;

	ini_cur += y;
	if( ini_cur <  0 )ini_cur = 16;
	if( ini_cur > 16 )ini_cur =  0;

			ini_bmp=0;
	switch( ini_cur ){
	case	0:
		if( ini_data.autosave )break;
		ini_data.BaseClock += x*100000;
		if( ini_data.BaseClock > 7160000 )ini_data.BaseClock = 7160000;
		if( ini_data.BaseClock < 2000000 )ini_data.BaseClock = 2000000;
		break;
	case	1:
		ini_data.frame_skip += x;
		if( ini_data.frame_skip > 9 )ini_data.frame_skip = 9;
		if( ini_data.frame_skip < 1 )ini_data.frame_skip = 1;
		break;
	case	2:
		ini_data.Debug = (x+ini_data.Debug)&1;
		break;
	case	3:	//	sound
		ini_data.sound = (x+ini_data.sound)&1;
		break;
	case	4:	//	pcm
		ini_data.pcm = (x+ini_data.pcm)&1;
		break;
	case	5:	
		ini_data.videomode += x;
		if( ini_data.videomode > 6 )ini_data.videomode = 0;
		if( ini_data.videomode < 0 )ini_data.videomode = 6;
		break;
	case	6:	
		if (key & CTRL_CIRCLE) {
			menu_keyconfig_ini();
		}
		break;
	case	7:	//	壁１
		if (key & CTRL_CIRCLE) {
			int f=1;
			file_type=1;
			while(f){
				menu_file_Draw();
				if( control_bef_ctl&CTRL_CROSS )break;
				switch(menu_file_Control()) {
				case 1:
					menu_Get_DirList(now_path);
					break;
				case 2:
					{
						char c[MAXPATH];
						_memset(c,0,MAXPATH);
						mh_strncpy(c,now_path,MAXPATH);
						mh_strncat(c,dlist[dlist_curpos].name,MAXPATH);
						if( BmpRead( 0, c) ){
							Bmp_put(0);
							_strcpy( ini_data.path_WallPaper_menu, c);
							f=0;
						}
					}
					break;
				case 3:
					f=0;
					break;
				}
			}
		}
		if( x ){
			ini_data.WallPaper[0]+=x;
			if( ini_data.WallPaper[0] <   0 )ini_data.WallPaper[0]=  0;
			if( ini_data.WallPaper[0] > 100 )ini_data.WallPaper[0]=100;
			if( bmp_i != 0 )BmpRead( 0, ini_data.path_WallPaper_menu);
			bmp_i=0;
			BmpAkarusa(0);
		}
		break;
	case	8:	//	壁２
			ini_bmp=1;
		if (key & CTRL_CIRCLE) {
			int f=1;
			file_type=1;
			while(f){
				menu_file_Draw();
				if( control_bef_ctl&CTRL_CROSS )break;
				switch(menu_file_Control()) {
				case 1:
					menu_Get_DirList(now_path);
					break;
				case 2:
					{
						char c[MAXPATH];
						_memset(c,0,MAXPATH);
						mh_strncpy(c,now_path,MAXPATH);
						mh_strncat(c,dlist[dlist_curpos].name,MAXPATH);
						if( BmpRead( 1, c) ){
							Bmp_put(1);
							_strcpy( ini_data.path_WallPaper_game, c);
							f=0;
						}
					}
					break;
				case 3:
					f=0;
					break;
				}
			}
		}
		if( x ){
			ini_data.WallPaper[1]+=x;
			if( ini_data.WallPaper[1] <   0 )ini_data.WallPaper[1]=  0;
			if( ini_data.WallPaper[1] > 100 )ini_data.WallPaper[1]=100;
			if( bmp_i != 1 )BmpRead( 1, ini_data.path_WallPaper_game);
			bmp_i=1;
			BmpAkarusa(1);
		}
		break;
	case	9:
		ini_data.CpuClock += x;
		if( ini_data.CpuClock > 2 )ini_data.CpuClock = 0;
		if( ini_data.CpuClock < 0 )ini_data.CpuClock = 2;
/*
		if (key & CTRL_CIRCLE) {
			Bmp_put(0);
			mh_print( 240-20, 130, "セーブ中。", rgb2col( 255,2,2),0,0);
			pgScreenFlipV();

			ini_write();
		}
*/
		break;
	case	10:	//	autosave
		ini_data.autosave = (x+ini_data.autosave)&1;
		if( ini_data.autosave ){
			ini_data.BaseClock = BaseClock;
		}
		break;
	case	11:	//	load
		if (key & CTRL_CIRCLE) {
			thumbnail_cf=0;
			if( state_read_chk() ){
				state_flag=2;
				return 3;
			}
		}else{
			state_slot += x;
			if( state_slot > 9 )state_slot = 0;
			if( state_slot < 0 )state_slot = 9;
			if( x||y )thumbnail_cf=0;
		}
		break;
	case	12:	//	save
		if (key & CTRL_CIRCLE) {
			Bmp_put(0);
			mh_print( 240-20, 130, "セーブ中。", rgb2col( 255,2,2),0,0);
			pgScreenFlipV();

			state_write();
			thumbnail_cf=0;
		}else{
			state_slot += x;
			if( state_slot > 9 )state_slot = 0;
			if( state_slot < 0 )state_slot = 9;
			if( x||y )thumbnail_cf=0;
		}
		break;
	case	13:
		if (key & CTRL_CIRCLE) {
			file_type=0;
			if( menu_file() ){
				state_slot = 0;
				return 3;
			}
		}
		break;
	case	14:
		if (key & CTRL_CIRCLE) {
			file_type=2;
			if( menu_file() ){
				_strcpy( ini_data.path_cd, target);
				cd_toc_read();
			}
		}else
		if (key & CTRL_CROSS) {
			ini_data.path_cd[0]=0;
			return	0;
		}
		break;
	case	15:
		if (key & CTRL_CIRCLE) {
			return 2;
		}
		break;
	case	16:
		if (key & CTRL_CIRCLE) {
			return 1;
		}
		break;
	}
	if (key & CTRL_CROSS) {
		return 2;
	}
	if(!qf)if( control_bef_ctl==ini_data.key[6] && ini_data.key[6] )return 2;
	return	0;
}

int		menu_ini(void)
{
	thumbnail_cf=0;
//	ini_cur = 14;
	CpuClockSet(3);
	ini_bmp = 0;
	qf=1;
	while( !psp_exit_flag ){
		menu_ini_Draw();
		switch(menu_ini_Control()) {
		case	1:
			pgFillvram(0);
			mh_print(100,130,"終了しますよ？　○：終了　Ｘ：やっぱやめ",rgb2col(255,2,2),0,0);
			pgScreenFlipV();
			while( !psp_exit_flag ){
				int key = Read_Key();
				if (key == 0) break;
				pgWaitV();
			}
			pgWaitV();
			while(1) {
				int key = Read_Key();
				if (key & CTRL_CIRCLE){
					pgFillvram(rgb2col(5,100,5));
					mh_print(300,260,"～　終了処理中　お疲れ様でした　～",rgb2col(255,255,255),0,0);
/*
					if( ini_data.autosave ){
						state_write();
					}
*/
					pgScreenFlipV();
		//			ini_write();
					return 1;
				}
				if (key & CTRL_CROSS ) break;
				pgWaitV();
			}
			break;
		case	2:	//	continue
		//	if( ini_data.autosave )
		//		ini_write();
			CpuClockSet(ini_data.CpuClock+3);
			return	0;
		case	3:	//	ゲーム実行
		//	if( ini_data.autosave )
		//		ini_write();


			CpuClockSet(ini_data.CpuClock+3);
			return	2;
		}
	}
	return 1;
}
extern	int	state_f;
void	menu_main(void)
{
	if( !menu_file() )return;
	while(1){
		CpuClockSet(ini_data.CpuClock+3);
		if( pce_main((char *)&target)==1 ){
			state_f=0;
			return;
		}
	}
}


