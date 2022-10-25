/*	
 *	clipmp3.h
 *
 *	P/ECE MP3 Driver
 *
 *	CLiP - Common Library for P/ECE
 *	Copyright (C) 2001-2003 Naoyuki Sawa
 *
 *	* Tue Oct 28 08:11:00 JST 2003 Naoyuki Sawa
 *	- 1st �����[�X�B
 *	* Wed Nov 12 05:25:00 JST 2003 Naoyuki Sawa
 *	- CLiP���C�u�������ֈړ��B
 */
#ifndef __CLIP_MP3_H__
#define __CLIP_MP3_H__

#define ASSERT(f)	((void)0)

/****************************************************************************
 *	�萔
 ****************************************************************************/
#define SPEAKER_FREQUENCY 44100

#define MP3_SUBBAND_SIZE	18					/* 1�T�u�o���h����̃T���v���� */
#define MP3_SUBBAND_COUNT	32					/* 1�t���[������̃T�u�o���h�� */
#define MP3_GRANULE_SIZE	(MP3_SUBBAND_SIZE * MP3_SUBBAND_COUNT)	/* 1�O���j���[������̃T���v���� */
#define MP3_GRANULE_COUNT	2					/* 1�t���[������̃O���j���[���� */
#define MP3_FRAME_SIZE		(MP3_GRANULE_SIZE * MP3_GRANULE_COUNT)	/* 1�t���[������̃T���v���� */

#define MP3_SUBBAND_SIZE_2	(MP3_SUBBAND_SIZE * 2)			/* IMDCT�Ŏg�p */
#define MP3_SUBBAND_COUNT_2	(MP3_SUBBAND_COUNT * 2)			/* �T�u�o���h�����Ŏg�p */

#define MP3_SHORT_IMDCT_SIZE	6					/* �V���[�g�u���b�N��IMDCT�T�C�Y */
#define MP3_SUBBLOCK_COUNT	3					/* �V���[�g�u���b�N�̃T�u�u���b�N�� */
/* MP3_SHORT_IMDCT_SIZE(6) * MP3_SUBBLOCK_COUNT(3) = MP3_SUBBAND_SIZE(18) �ƂȂ�܂��B */
#define MP3_SHORT_IMDCT_SIZE_2	(MP3_SHORT_IMDCT_SIZE * 2)		/* �V���[�g�u���b�N��IMDCT�Ŏg�p */

#define MP3_PRE_REQUANTIZE_MAX	(((1 << 4) - 1) + ((1 << 13) - 1))	/* �n�t�}���������o�͂̍ő�l */

/****************************************************************************
 *	ID3 Tag V1
 ****************************************************************************/

typedef struct _MP3ID3TAGV1 {
	char tag[3 + 1];	/* "TAG"�Œ� */
	char title[30 + 1];	/* �^�C�g���� */
	char artist[30 + 1];	/* �A�[�e�B�X�g�� */
	char album[30 + 1];	/* �A���o���� */
	char year[4 + 1];	/* ���J�N */
	char comment[30 + 1];	/* �R�����g */
	short genre;		/* �W�������ԍ� */
} MP3ID3TAGV1;

extern const char* mp3_id3_tag_v1_genre_table[/*MP3ID3TAGV1.genre*/];

int mp3_id3_tag_v1_read(MP3ID3TAGV1* tag, const void* ptr);

/****************************************************************************
 *	�r�b�g�X�g���[��
 ****************************************************************************/

typedef struct _MP3BITSTREAM {
	short read_bits;		/* �ǂݍ��񂾃r�b�g�� */
	short cnt;			/* buf���̗L���r�b�g�� */
	unsigned int buf;		/* bit0�`(cnt-1)�܂ŗL�� */
	const unsigned char* pos;	/* ���̓ǂݍ��݃A�h���X */
} MP3BITSTREAM;

void mp3_bitstream_open(MP3BITSTREAM* bs, const void* pos);
int mp3_bitstream_get(MP3BITSTREAM* bs, int cnt);
void mp3_bitstream_skip(MP3BITSTREAM* bs, int cnt);

/****************************************************************************
 *	�t���[���w�b�_
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
 *	�O���j���[�����
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
 *	�T�C�h���
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
 *	�X�P�[���t�@�N�^
 ****************************************************************************/

typedef struct _MP3SCALEFACTOR {
	short long_block[22];				/* �����O�u���b�N�p */
	short short_block[13][MP3_SUBBLOCK_COUNT];	/* �V���[�g�u���b�N�p */
} MP3SCALEFACTOR;

extern const short mp3_scalefactor_compress_table[16/*MP3GRANLEINFO.scalefac_compress*/][2/*0:slen1/1:slen2*/];
extern const short mp3_scalefactor_band_index_table[3/*0:44.1KHz/1:48KHz/2:32KHz*/][2/*0:LongBlock/1:ShortBlock*/][22/*ScaleFactorBand*/ + 1/*END*/];

void mp3_read_scalefactor(MP3BITSTREAM* bs, const MP3GRANULEINFO* granule, MP3SCALEFACTOR* scale, int scfsi);

/****************************************************************************
 *	�n�t�}��������
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
 *	�t�ʎq��
 ****************************************************************************/

extern const short mp3_reorder_read_index_table[3/*MP3FRAMEHEADER.sampling_frequency=0:44.1KHz/1:48KHz/2:32KHz*/][MP3_GRANULE_SIZE];
extern const int mp3_gain_table[256/*MP3GRANULEINFO.global_gain*/];
extern const short mp3_pow1_3_table[MP3_PRE_REQUANTIZE_MAX + 1];

void mp3_requantize_long(const MP3FRAMEHEADER* frame, const MP3GRANULEINFO* granule, const MP3SCALEFACTOR* scale, const short* in/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, short* out/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, int count);
void mp3_requantize_short(const MP3FRAMEHEADER* frame, const MP3GRANULEINFO* granule, const MP3SCALEFACTOR* scale, const short* in/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, short* out/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/);
//��TODO: void mp3_requantize_mixed(const MP3FRAMEHEADER* frame, const MP3GRANULEINFO* granule, const MP3SCALEFACTOR* scale, const short* in/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, short* out/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/);

/****************************************************************************
 *	�G���A�V���O�팸
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
//��TODO: void mp3_imdct_mixed(const short* in/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, short* out/*[MP3_SUBBAND_SIZE][MP3_SUBBAND_COUNT]*/, short* save/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/);

/****************************************************************************
 *	�T�u�o���h����
 ****************************************************************************/

/* ����=MP3{44.1KHz/48KHz/32KHz} => �o��=P/ECE{16KHz} �T���v�����O���[�g�ϊ��p�e�[�u�� */
typedef struct _MP3SAMPLINGFREQUENCYCONVERTTABLE {	/* �����C�A�E�g�ύX�s��!!�� */
	const unsigned char* table;			/* +0,4: ���o�T���v���̃C���f�N�X�e�[�u�� */
	short count;					/* +4,2: ����32(=MP3_SUBBAND_COUNT)�T���v������̒��o�T���v���� */
	unsigned short sampling_frequency;		/* +6,2: �o�̓T���v�����O���g�� */
} MP3SAMPLINGFREQUENCYCONVERTTABLE;			/* =8 */

extern const short mp3_subband_matrix[MP3_SUBBAND_COUNT_2][MP3_SUBBAND_COUNT];
extern const short mp3_polyphase_filter[MP3_SUBBAND_COUNT][16];
extern const MP3SAMPLINGFREQUENCYCONVERTTABLE mp3_sampling_frequency_convert_table[3/*0:44.1KHz/1:48KHz/2:32KHz*/];

int mp3_subband_synthesys(const short* in/*[MP3_SUBBAND_SIZE][MP3_SUBBAND_COUNT]*/, short* out/*[upto MP3_GRANULE_SIZE]*/, short* save/*[16][MP3_SUBBAND_COUNT_2]*/, int offset, const MP3SAMPLINGFREQUENCYCONVERTTABLE* convert);

/****************************************************************************
 *	MP3�h���C�o
 ****************************************************************************/

typedef struct _MP3DRIVER {
	const unsigned char* pos;
	const unsigned char* end;
	/* �T���v�����O���g���ϊ��p�o�b�t�@ */
	int sampling_frequency;					/* fbuff���f�[�^�̃T���v�����O���g�� */
	int c_fbuff;						/* fbuff���f�[�^�� */
	int i_fbuff;						/* �T���v�����O���g���ϊ��pDDA�p�����^ */
	int e_fbuff;						/* �T���v�����O���g���ϊ��pDDA�p�����^ */
	short fbuff[MP3_FRAME_SIZE];
	/* �t���[���Ԃŕۑ�����K�v�̂���o�b�t�@ */
	unsigned char main_save[4096];				/* ���C���f�[�^�̃r�b�g�~�ϗp�B���_���4096+1441�o�C�g�܂ŗL�蓾�܂����A���ۂ�4KB������Ώ[��������͂��ł��B */
	int main_save_size;					/* (����) */
	short imdct_save[MP3_GRANULE_SIZE];			/* IMDCT�p */
	short subband_synthesys_save[16][MP3_SUBBAND_COUNT_2];	/* �T�u�o���h�����p */
	int subband_synthesys_offset;				/* (����) */
} MP3DRIVER;

int mp3_init(MP3DRIVER* mp3, const void* data, int len);
int mp3_stream_callback(short* wbuff, int param);
int mp3_decode_frame(MP3DRIVER* mp3);

/****************************************************************************
 *	�A�v���P�[�V�����p�֐�
 ****************************************************************************/

int mp3_play(const void* data, int len);
void mp3_stop();

#endif /*__CLIP_MP3_H__*/
