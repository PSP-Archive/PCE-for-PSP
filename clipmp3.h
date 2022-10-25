/*	
 *	clipmp3.h
 *
 *	P/ECE MP3 Driver
 *
 *	CLiP - Common Library for P/ECE
 *	Copyright (C) 2001-2003 Naoyuki Sawa
 *
 *	* Tue Oct 28 08:11:00 JST 2003 Naoyuki Sawa
 *	- 1st リリース。
 *	* Wed Nov 12 05:25:00 JST 2003 Naoyuki Sawa
 *	- CLiPライブラリ内へ移動。
 */
#ifndef __CLIP_MP3_H__
#define __CLIP_MP3_H__

#define ASSERT(f)	((void)0)

/****************************************************************************
 *	定数
 ****************************************************************************/
#define SPEAKER_FREQUENCY 44100

#define MP3_SUBBAND_SIZE	18					/* 1サブバンド当りのサンプル数 */
#define MP3_SUBBAND_COUNT	32					/* 1フレーム当りのサブバンド数 */
#define MP3_GRANULE_SIZE	(MP3_SUBBAND_SIZE * MP3_SUBBAND_COUNT)	/* 1グラニュール当りのサンプル数 */
#define MP3_GRANULE_COUNT	2					/* 1フレーム当りのグラニュール数 */
#define MP3_FRAME_SIZE		(MP3_GRANULE_SIZE * MP3_GRANULE_COUNT)	/* 1フレーム当りのサンプル数 */

#define MP3_SUBBAND_SIZE_2	(MP3_SUBBAND_SIZE * 2)			/* IMDCTで使用 */
#define MP3_SUBBAND_COUNT_2	(MP3_SUBBAND_COUNT * 2)			/* サブバンド合成で使用 */

#define MP3_SHORT_IMDCT_SIZE	6					/* ショートブロックのIMDCTサイズ */
#define MP3_SUBBLOCK_COUNT	3					/* ショートブロックのサブブロック数 */
/* MP3_SHORT_IMDCT_SIZE(6) * MP3_SUBBLOCK_COUNT(3) = MP3_SUBBAND_SIZE(18) となります。 */
#define MP3_SHORT_IMDCT_SIZE_2	(MP3_SHORT_IMDCT_SIZE * 2)		/* ショートブロックのIMDCTで使用 */

#define MP3_PRE_REQUANTIZE_MAX	(((1 << 4) - 1) + ((1 << 13) - 1))	/* ハフマン復号化出力の最大値 */

/****************************************************************************
 *	ID3 Tag V1
 ****************************************************************************/

typedef struct _MP3ID3TAGV1 {
	char tag[3 + 1];	/* "TAG"固定 */
	char title[30 + 1];	/* タイトル名 */
	char artist[30 + 1];	/* アーティスト名 */
	char album[30 + 1];	/* アルバム名 */
	char year[4 + 1];	/* 公開年 */
	char comment[30 + 1];	/* コメント */
	short genre;		/* ジャンル番号 */
} MP3ID3TAGV1;

extern const char* mp3_id3_tag_v1_genre_table[/*MP3ID3TAGV1.genre*/];

int mp3_id3_tag_v1_read(MP3ID3TAGV1* tag, const void* ptr);

/****************************************************************************
 *	ビットストリーム
 ****************************************************************************/

typedef struct _MP3BITSTREAM {
	short read_bits;		/* 読み込んだビット数 */
	short cnt;			/* buf中の有効ビット数 */
	unsigned int buf;		/* bit0～(cnt-1)まで有効 */
	const unsigned char* pos;	/* 次の読み込みアドレス */
} MP3BITSTREAM;

void mp3_bitstream_open(MP3BITSTREAM* bs, const void* pos);
int mp3_bitstream_get(MP3BITSTREAM* bs, int cnt);
void mp3_bitstream_skip(MP3BITSTREAM* bs, int cnt);

/****************************************************************************
 *	フレームヘッダ
 ****************************************************************************/

typedef struct _MP3FRAMEHEADER {
	short syncword;			/* 12 */
	short id;			/*  1 */
	short layer;			/*  2 */
	short protection_bit;		/*  1 */
	short bitrate_index;		/*  4 */
	short sampling_frequency;	/*  2 */
	short padding_bit;		/*  1 */
	short private_bit;		/*  1 */
	short mode;			/*  2 */
	short mode_extention;		/*  2 */
	short copyright;		/*  1 */
	short original;			/*  1 */
	short emphasis;			/*  2 */
} MP3FRAMEHEADER;			/*=32 */

extern const int mp3_bitrate_table[16/*bitrate_index*/];
extern const int mp3_sampling_frequency_table[4/*sampling_frequency*/];

int mp3_read_frame_header(MP3BITSTREAM* bs, MP3FRAMEHEADER* frame);

/****************************************************************************
 *	グラニュール情報
 ****************************************************************************/

typedef struct _MP3GRANULEINFO {
	short part2_3_length;			/* 12 */
	short big_values;			/*  9 */
	short global_gain;			/*  8 */
	short scalefac_compress;		/*  4 */
	short window_switching_flag;		/*  1 */
	/* switch(window_switching_flag)	  {0:  | 1: }*/
	short block_type;			/*     | 2   */
	short mixed_block_flag;			/*     | 1   */
	short table_select[3];			/* 5x3 | 5x2 */
	short subblock_gain[3];			/*     | 3x3 */
	short region0_count;			/* 4   |     */
	short region1_count;			/* 3   |     */
	short preflag;				/*  1 */
	short scalefac_scale;			/*  1 */
	short count1table_select;		/*  1 */
} MP3GRANULEINFO;				/*=59 */

void mp3_read_granule_info(MP3BITSTREAM* bs, MP3GRANULEINFO* granule);

/****************************************************************************
 *	サイド情報
 ****************************************************************************/

typedef struct _MP3SIDEINFO {
	short main_data_begin;			/* 9 */
	/* switch(channels)			  {1:     | 2:    }*/
	short private_bits;			/*  5     |  3     */
	short scfsi[2];				/*  4     |  4x2   */
	MP3GRANULEINFO granule_info[2][2];	/* 59x2x1 | 59x2x2 */
} MP3SIDEINFO;					/*=136      256    */

void mp3_read_side_info(MP3BITSTREAM* bs, MP3SIDEINFO* side, int channels);

/****************************************************************************
 *	スケールファクタ
 ****************************************************************************/

typedef struct _MP3SCALEFACTOR {
	short long_block[22];				/* ロングブロック用 */
	short short_block[13][MP3_SUBBLOCK_COUNT];	/* ショートブロック用 */
} MP3SCALEFACTOR;

extern const short mp3_scalefactor_compress_table[16/*MP3GRANLEINFO.scalefac_compress*/][2/*0:slen1/1:slen2*/];
extern const short mp3_scalefactor_band_index_table[3/*0:44.1KHz/1:48KHz/2:32KHz*/][2/*0:LongBlock/1:ShortBlock*/][22/*ScaleFactorBand*/ + 1/*END*/];

void mp3_read_scalefactor(MP3BITSTREAM* bs, const MP3GRANULEINFO* granule, MP3SCALEFACTOR* scale, int scfsi);

/****************************************************************************
 *	ハフマン復号化
 ****************************************************************************/

typedef struct _MP3HUFFMANBIGVALUETABLE {
	const short (*table)[2];
	short linbits;
} MP3HUFFMANBIGVALUETABLE;
extern const MP3HUFFMANBIGVALUETABLE mp3_huffman_bigvalue_table[32];

typedef struct _MP3HUFFMANCOUNT1TABLE {
	const short (*table)[2];
} MP3HUFFMANCOUNT1TABLE;
extern const MP3HUFFMANCOUNT1TABLE mp3_huffman_count1_table[2];

int mp3_huffman_decode(MP3BITSTREAM* bs, const MP3FRAMEHEADER* frame, const MP3GRANULEINFO* granule, short* out/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/);
void mp3_huffman_decode_bigvalue(MP3BITSTREAM* bs, int i_table, int count, short* out);
int mp3_huffman_decode_count1(MP3BITSTREAM* bs, int i_table, int count_limit, short* out, int part2_3_length);

/****************************************************************************
 *	逆量子化
 ****************************************************************************/

extern const short mp3_reorder_read_index_table[3/*MP3FRAMEHEADER.sampling_frequency=0:44.1KHz/1:48KHz/2:32KHz*/][MP3_GRANULE_SIZE];
extern const int mp3_gain_table[256/*MP3GRANULEINFO.global_gain*/];
extern const short mp3_pow1_3_table[MP3_PRE_REQUANTIZE_MAX + 1];

void mp3_requantize_long(const MP3FRAMEHEADER* frame, const MP3GRANULEINFO* granule, const MP3SCALEFACTOR* scale, const short* in/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, short* out/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, int count);
void mp3_requantize_short(const MP3FRAMEHEADER* frame, const MP3GRANULEINFO* granule, const MP3SCALEFACTOR* scale, const short* in/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, short* out/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/);
//※TODO: void mp3_requantize_mixed(const MP3FRAMEHEADER* frame, const MP3GRANULEINFO* granule, const MP3SCALEFACTOR* scale, const short* in/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, short* out/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/);

/****************************************************************************
 *	エリアシング削減
 ****************************************************************************/

extern const short mp3_antialias_table[8][2/*0:cs/1:ca*/];

void mp3_antialias(short* in_out, int subband_count);

/****************************************************************************
 *	IMDCT
 ****************************************************************************/

extern const short mp3_imdct_long_matrix[MP3_SUBBAND_SIZE_2][MP3_SUBBAND_SIZE];
extern const short mp3_imdct_short_matrix[MP3_SHORT_IMDCT_SIZE_2][MP3_SHORT_IMDCT_SIZE];

void mp3_imdct_long(const short* in/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, short* out/*[MP3_SUBBAND_SIZE][MP3_SUBBAND_COUNT]*/, short* save/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, int subband_count);
void mp3_imdct_short(const short* in/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, short* out/*[MP3_SUBBAND_SIZE][MP3_SUBBAND_COUNT]*/, short* save/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/);
//※TODO: void mp3_imdct_mixed(const short* in/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, short* out/*[MP3_SUBBAND_SIZE][MP3_SUBBAND_COUNT]*/, short* save/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/);

/****************************************************************************
 *	サブバンド合成
 ****************************************************************************/

/* 入力=MP3{44.1KHz/48KHz/32KHz} => 出力=P/ECE{16KHz} サンプリングレート変換用テーブル */
typedef struct _MP3SAMPLINGFREQUENCYCONVERTTABLE {	/* ★レイアウト変更不可!!★ */
	const unsigned char* table;			/* +0,4: 抽出サンプルのインデクステーブル */
	short count;					/* +4,2: 入力32(=MP3_SUBBAND_COUNT)サンプル当りの抽出サンプル数 */
	unsigned short sampling_frequency;		/* +6,2: 出力サンプリング周波数 */
} MP3SAMPLINGFREQUENCYCONVERTTABLE;			/* =8 */

extern const short mp3_subband_matrix[MP3_SUBBAND_COUNT_2][MP3_SUBBAND_COUNT];
extern const short mp3_polyphase_filter[MP3_SUBBAND_COUNT][16];
extern const MP3SAMPLINGFREQUENCYCONVERTTABLE mp3_sampling_frequency_convert_table[3/*0:44.1KHz/1:48KHz/2:32KHz*/];

int mp3_subband_synthesys(const short* in/*[MP3_SUBBAND_SIZE][MP3_SUBBAND_COUNT]*/, short* out/*[upto MP3_GRANULE_SIZE]*/, short* save/*[16][MP3_SUBBAND_COUNT_2]*/, int offset, const MP3SAMPLINGFREQUENCYCONVERTTABLE* convert);

/****************************************************************************
 *	MP3ドライバ
 ****************************************************************************/

typedef struct _MP3DRIVER {
	const unsigned char* pos;
	const unsigned char* end;
	/* サンプリング周波数変換用バッファ */
	int sampling_frequency;					/* fbuff内データのサンプリング周波数 */
	int c_fbuff;						/* fbuff内データ数 */
	int i_fbuff;						/* サンプリング周波数変換用DDAパラメタ */
	int e_fbuff;						/* サンプリング周波数変換用DDAパラメタ */
	short fbuff[MP3_FRAME_SIZE];
	/* フレーム間で保存する必要のあるバッファ */
	unsigned char main_save[4096];				/* メインデータのビット蓄積用。理論上は4096+1441バイトまで有り得ますが、実際は4KBもあれば充分すぎるはずです。 */
	int main_save_size;					/* (同上) */
	short imdct_save[MP3_GRANULE_SIZE];			/* IMDCT用 */
	short subband_synthesys_save[16][MP3_SUBBAND_COUNT_2];	/* サブバンド合成用 */
	int subband_synthesys_offset;				/* (同上) */
} MP3DRIVER;

int mp3_init(MP3DRIVER* mp3, const void* data, int len);
int mp3_stream_callback(short* wbuff, int param);
int mp3_decode_frame(MP3DRIVER* mp3);

/****************************************************************************
 *	アプリケーション用関数
 ****************************************************************************/

int mp3_play(const void* data, int len);
void mp3_stop();

#endif /*__CLIP_MP3_H__*/
