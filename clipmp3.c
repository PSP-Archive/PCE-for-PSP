/*	
 *	clipmp3.c
 *
 *	P/ECE MP3 Driver
 *
 *	CLiP - Common Library for P/ECE
 *	Copyright (C) 2001-2003 Naoyuki Sawa
 *
 *	* Tue Oct 28 08:11:00 JST 2003 Naoyuki Sawa
 *	- 1st �����[�X�B
 *	* Tue Nov  4 00:00:00 JST 2003 Naoyuki Sawa
 *	- SCMPX�ŃG���R�[�h����MP3�t�@�C�����Đ��ł���悤�A�������ł��B
 *	  ���̐ݒ�ŃG���R�[�h�����t�@�C�����Đ����āA�n���O�A�b�v�͂��Ȃ��Ȃ�܂����B
 *		���C���[	III
 *		�r�b�g���[�g	64000 �Œ�
 *		���m����
 *		�G���t�@�V�X	�Ȃ�
 *	  ���͂܂��܂��ŁA�ƂĂ��m�C�Y������܂��B
 *	  �����́A�܂��V���[�g�u���b�N�E�~�b�N�X�u���b�N�ɑΉ����Ă��Ȃ�����ł��B
 *	  �����_�ł́A�ǂ���������O�u���b�N�Ƃ݂Ȃ��čĐ����Ă��܂��Ă��܂��B
 *	* Thu Nov  6 23:50:00 JST 2003 Naoyuki Sawa
 *	- �V���[�g�u���b�N�Ή��B
 *	- �X�P�[���t�@�N�^�Ή��B
 *	- mp3_read_granule_info()�̃o�O�C���B
 *	- mp3_read_scalefactor()�̃o�O�C���B
 *	* Mon Nov 10 03:15:00 JST 2003 Naoyuki Sawa
 *	- mp3_bitstream_skip()��read_bits���J�E���g���Ă��Ȃ������o�O���C���B
 *	  ����ɂ͊֌W����܂���(part2_3_length�̈��mp3_bitstream_skip()�͎g��Ȃ�����)���A
 *	  ����̂��߂ɏC�����Ă����܂����B
 *	* Wed Nov 12 05:25:00 JST 2003 Naoyuki Sawa
 *	- CLiP���C�u�������ֈړ��B
 *	* Mon Nov 24 06:00:00 JST 2003 Naoyuki Sawa
 *	- clippce.h��SPEAKER_FREQUENCY���`�����̂ŁA�o�͎��g���萔�ɂ��̃V���{�����g���悤�C���B
 *	  �\�[�X�ゾ���̏C���ł��B���s�o�C�i���ɂ͕ω�����܂���B
 */
//#include "clip.h"
#include "clipmp3.h"
#include "_clib.h"

/****************************************************************************
 *	��p�W�����C�u����
 ****************************************************************************/

void* _memmove(void *buf1, const void *buf2, int n)
{
	while(n-->0) {
		((unsigned char*)buf1)[n] = ((unsigned char*)buf2)[n];
		((unsigned char*)buf2)[n] = 0x00;
	}
	return buf1;
}

int isspace(char c)
{
	if ( (c >= 0x09 && c <= 0x0D) || c == 0x20) return 1;

	return 0;
}

/****************************************************************************
 *	ID3 Tag V1
 ****************************************************************************/

/* �����g�p�E���m�F */

static const void*
mp3_id3_tag_v1_read1(const void* _in, char* out, int len)
{
	const char* in = (const char*)_in;
	int i;
	_memcpy(out, in, len);
	for(i = len - 1; i >= 0; i--) {
		if(!isspace(out[i])) break;
		out[i] = '\0';
	}
	return in + len;
}
	
/* ID3 Tag V1��ǂݍ��݂܂��B
 * [in]
 *	tag		ID3 Tag V1��ǂݍ��ލ\���́B
 *	ptr		MP3�t�@�C���̏I�[����128�o�C�g�ڂ�n���Ă��������B
 *			�t�@�C����128�o�C�g�����̏ꍇ�́A���̊֐����Ă�ł͂����܂���B
 * [out]
 *	* �߂�l	�ǂݍ��ݐ����Ȃ�0��Ԃ��܂��B
 *			�ǂݍ��ݎ��s�Ȃ�0�ȊO�̒l��Ԃ��܂��B
 */
int
mp3_id3_tag_v1_read(MP3ID3TAGV1* tag, const void* ptr)
{
	if(_memcmp(ptr, "TAG", 3) != 0) return -1;

	_memset(tag, 0, sizeof(MP3ID3TAGV1));
	ptr = mp3_id3_tag_v1_read1(ptr, tag->tag,      3);
	ptr = mp3_id3_tag_v1_read1(ptr, tag->title,   30);
	ptr = mp3_id3_tag_v1_read1(ptr, tag->artist,  30);
	ptr = mp3_id3_tag_v1_read1(ptr, tag->album,   30);
	ptr = mp3_id3_tag_v1_read1(ptr, tag->year,     4);
	ptr = mp3_id3_tag_v1_read1(ptr, tag->comment, 30);
	tag->genre = *(unsigned char*)ptr;

	return 0;
}

/****************************************************************************
 *	�r�b�g�X�g���[��
 ****************************************************************************/

/* �r�b�g�X�g���[�����J���܂��B
 * [in]
 *	bs		�r�b�g�X�g���[���\���́B
 *	pos		�ŏ��̓ǂݍ��݃A�h���X�B
 */
void
mp3_bitstream_open(MP3BITSTREAM* bs, const void* pos)
{
	bs->read_bits = 0;
	bs->cnt = 0;
	/*bs->buf = 0;{{�s�v}}*/
	bs->pos = (const unsigned char*)pos;
}

/* �r�b�g�X�g���[������r�b�g���ǂݔ�΂��܂��B
 * [in]
 *	bs		�r�b�g�X�g���[���\���́B
 *	cnt		�ǂݔ�΂��r�b�g���B(0�`�C��)
 */
void
mp3_bitstream_skip(MP3BITSTREAM* bs, int cnt)
{
	bs->read_bits += cnt;
	if(bs->cnt >= cnt) {
		bs->cnt -= cnt;
	} else {
		cnt -= bs->cnt;
		bs->pos += cnt / 8;
		cnt &= 7;
		if(cnt) {
			bs->buf = *bs->pos++;
			bs->cnt = 8 - cnt;
		} else {
			bs->cnt = 0;
		}
	}
}

/****************************************************************************
 *	�t���[���w�b�_
 ****************************************************************************/

/* �t���[���w�b�_��ǂݍ��݂܂��B
 * [in]
 *	bs		�r�b�g�X�g���[���\���́B
 *	frame		�t���[���w�b�_���i�[����\���́B
 * [out]
 *	�߂�l		�����Ȃ�0��Ԃ��܂��B
 *			���s�Ȃ�0�ȊO�̒l��Ԃ��܂��B
 */
int
mp3_read_frame_header(MP3BITSTREAM* bs, MP3FRAMEHEADER* frame)
{
	frame->syncword			= mp3_bitstream_get(bs, 12);
	frame->id			= mp3_bitstream_get(bs,  1);
	frame->layer			= mp3_bitstream_get(bs,  2);
	frame->protection_bit		= mp3_bitstream_get(bs,  1);
	frame->bitrate_index		= mp3_bitstream_get(bs,  4);
	frame->sampling_frequency	= mp3_bitstream_get(bs,  2);
	frame->padding_bit		= mp3_bitstream_get(bs,  1);
	frame->private_bit		= mp3_bitstream_get(bs,  1);
	frame->mode			= mp3_bitstream_get(bs,  2);
	frame->mode_extention		= mp3_bitstream_get(bs,  2);
	frame->copyright		= mp3_bitstream_get(bs,  1);
	frame->original			= mp3_bitstream_get(bs,  1);
	frame->emphasis			= mp3_bitstream_get(bs,  2);

	/* �Œ���̃G���[�`�F�b�N�B */
	if(frame->syncword != 0xfff) return -1;	/* �������[�h */
	if(frame->id       !=     1) return -1;	/* MPEG1      */
	if(frame->layer    !=     1) return -1;	/* LayerIII   */

	/* CRC�ی삠��Ȃ�ACRC�ی�r�b�g��ǂݔ�΂��܂��B
	 * �uprotection_bit=0:CRC�ی삠��/1:�Ȃ��v�ł��B�v����!!
	 */
	if(!frame->protection_bit) mp3_bitstream_skip(bs, 16);

	return 0;
}

/****************************************************************************
 *	�O���j���[�����
 ****************************************************************************/

/* �O���j���[������ǂݍ��݂܂��B
 * [in]
 *	bs		�r�b�g�X�g���[���\���́B
 *	granule		�O���j���[�������i�[����\���́B
 */
void
mp3_read_granule_info(MP3BITSTREAM* bs, MP3GRANULEINFO* granule)
{
	granule->part2_3_length			= mp3_bitstream_get(bs, 12);
	granule->big_values			= mp3_bitstream_get(bs,  9);
	granule->global_gain			= mp3_bitstream_get(bs,  8);
	granule->scalefac_compress		= mp3_bitstream_get(bs,  4);
	granule->window_switching_flag		= mp3_bitstream_get(bs,  1);
	if(!granule->window_switching_flag) {
		granule->block_type		= 0;
		granule->table_select[0]	= mp3_bitstream_get(bs,  5);
		granule->table_select[1]	= mp3_bitstream_get(bs,  5);
		granule->table_select[2]	= mp3_bitstream_get(bs,  5);
		granule->region0_count		= mp3_bitstream_get(bs,  4);
		granule->region1_count		= mp3_bitstream_get(bs,  3);
	} else {
		granule->block_type		= mp3_bitstream_get(bs,  2);
		granule->mixed_block_flag	= mp3_bitstream_get(bs,  1);
		granule->table_select[0]	= mp3_bitstream_get(bs,  5);
		granule->table_select[1]	= mp3_bitstream_get(bs,  5);
		granule->subblock_gain[0]	= mp3_bitstream_get(bs,  3);
		granule->subblock_gain[1]	= mp3_bitstream_get(bs,  3);
		granule->subblock_gain[2]	= mp3_bitstream_get(bs,  3);
		if(granule->block_type == 2 && !granule->mixed_block_flag) {
			granule->region0_count	=       8         ;
			granule->region1_count	= 22 - (8 + 1) - 1;
		} else {
			granule->region0_count	=       7         ;
			granule->region1_count	= 22 - (7 + 1) - 1;
		}
	}
	granule->preflag			= mp3_bitstream_get(bs,  1);
	granule->scalefac_scale			= mp3_bitstream_get(bs,  1);
	granule->count1table_select		= mp3_bitstream_get(bs,  1);
}

/****************************************************************************
 *	�T�C�h���
 ****************************************************************************/

/* �T�C�h����ǂݍ��݂܂��B
 * [in]
 *	bs		�r�b�g�X�g���[���\���́B
 *	side		�T�C�h�����i�[����\���́B
 *	channels	�`���l�����B
 */
void
mp3_read_side_info(MP3BITSTREAM* bs, MP3SIDEINFO* side, int channels)
{
	int i_granule;
	int i_channel;

	side->main_data_begin		= mp3_bitstream_get(bs, 9);
	switch(channels) {
	case 1:
		side->private_bits	= mp3_bitstream_get(bs, 5);
		side->scfsi[0]		= mp3_bitstream_get(bs, 4);
		break;
	case 2:
		side->private_bits	= mp3_bitstream_get(bs, 3);
		side->scfsi[0]		= mp3_bitstream_get(bs, 4);
		side->scfsi[1]		= mp3_bitstream_get(bs, 4);
		break;
	}
	for(i_granule = 0; i_granule < 2; i_granule++) {
		for(i_channel = 0; i_channel < channels; i_channel++) {
			mp3_read_granule_info(bs, &side->granule_info[i_granule][i_channel]);
		}
	}
}

/****************************************************************************
 *	�X�P�[���t�@�N�^
 ****************************************************************************/

/* �X�P�[���t�@�N�^��ǂݍ��݂܂��B
 * [in]
 *	bs		�r�b�g�X�g���[���\���́B
 *	granule		�O���j���[�����\���́B
 *	scale		�X�P�[���t�@�N�^�[�\���́B(�����ɃX�P�[���t�@�N�^�[��ǂݍ��݂܂�)
 *	scfsi		��1�O���j���[���̏ꍇ�A0��n���Ă��������B
 *			��2�O���j���[���̏ꍇ�AMP3SIDEINFO.scsfi[ch#]�̒l��n���Ă��������B
 * [out]
 *	*scale		�X�P�[���t�@�N�^�[��ǂݍ��݁A�i�[���܂��B
 * [note]
 *	* ��2�O���j���[���̓ǂݍ��݂ł́A�ꕔ�̒l���1�O���j���[���Ƌ��L����ꍇ������܂��B
 *	  ���̏ꍇ�A�ω����镔��������ǂݍ��݁A���L�����͓ǂݍ��݂܂���B
 *	  �]���āA��1�O���j���[���œǂݍ��񂾃X�P�[���t�@�N�^�[�\���̂��A
 *	  ���e��j�󂹂��ɓ���`���l���̑�2�O���j���[���̃X�P�[���t�@�N�^�[�ǂݍ��݂Ɏg���Ă��������B
 */
void
mp3_read_scalefactor(MP3BITSTREAM* bs, const MP3GRANULEINFO* granule, MP3SCALEFACTOR* scale, int scfsi)
{
	int i, slen1, slen2;
	short* out;

	slen1 = mp3_scalefactor_compress_table[granule->scalefac_compress][0];
	slen2 = mp3_scalefactor_compress_table[granule->scalefac_compress][1];

	if(granule->block_type == 2) {
		if(granule->mixed_block_flag) {
			/* �~�b�N�X�u���b�N */
			out = &scale->long_block[(i = 0)];
			do { *out++ = mp3_bitstream_get(bs, slen1); } while(++i <= 7);	/* long_block[0�`7] */
			out = &scale->short_block[(i = 3)][0];
			do { *out++ = mp3_bitstream_get(bs, slen1);
			     *out++ = mp3_bitstream_get(bs, slen1);
			     *out++ = mp3_bitstream_get(bs, slen1); } while(++i <= 5);	/* short_block[3�` 5] */
		} else {
			/* �V���[�g�u���b�N */
			out = &scale->short_block[(i = 0)][0];
			do { *out++ = mp3_bitstream_get(bs, slen1);
			     *out++ = mp3_bitstream_get(bs, slen1);
			     *out++ = mp3_bitstream_get(bs, slen1); } while(++i <= 5);	/* short_block[0�` 5] */
		}
		do { *out++ = mp3_bitstream_get(bs, slen2);
		     *out++ = mp3_bitstream_get(bs, slen2);
		     *out++ = mp3_bitstream_get(bs, slen2); } while(++i <= 11);		/* short_block[6�`11] */
		     *out++ = 0;
		     *out++ = 0;
		     *out++ = 0;							/* short_block[   12] */
	} else {
		/* �����O�u���b�N */
		if(!(scfsi & 1 << 3)) { for(i =  0; i <=  5; i++) { scale->long_block[ i] = mp3_bitstream_get(bs, slen1); } };	/* long_block[ 0�` 5] */
		if(!(scfsi & 1 << 2)) { for(i =  6; i <= 10; i++) { scale->long_block[ i] = mp3_bitstream_get(bs, slen1); } };	/* long_block[ 6�`10] */
		if(!(scfsi & 1 << 1)) { for(i = 11; i <= 15; i++) { scale->long_block[ i] = mp3_bitstream_get(bs, slen2); } };	/* long_block[11�`15] */
		if(!(scfsi & 1 << 0)) { for(i = 16; i <= 20; i++) { scale->long_block[ i] = mp3_bitstream_get(bs, slen2); } };	/* long_block[16�`20] */
								    scale->long_block[21] = 0;					/* long_block[    21] */
	}
}

/****************************************************************************
 *	�n�t�}��������
 ****************************************************************************/

/* �n�t�}�������������B
 * [in]
 *	bs		�r�b�g�X�g���[���\���́B
 *	granule		�O���j���[�����\���́B
 *	out		�o�̓o�b�t�@�B
 * [out]
 *	�߂�l		BigValue�̈��Count1�̈�̍��v�f�[�^���B
 * [note]
 *	* �r�b�g�X�g���[���\���̂�read_bits���A�X�P�[���t�@�N�^�ǂݍ��ݒ��O�̃^�C�~���O�Ń��Z�b�g���Ă����Ă��������B
 */
int
mp3_huffman_decode(MP3BITSTREAM* bs, const MP3FRAMEHEADER* frame, const MP3GRANULEINFO* granule, short* out/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/)
{
	int i, count, region1_start, region2_start, big_values_2;

	/* Region0/1/2�̋��E���擾�B */
	if(granule->block_type == 2) {
		/* �V���[�g�u���b�N(���~�b�N�X�u���b�N����������?) */
		region1_start =  36;
		region2_start = 576;
	} else {
		/* �����O�u���b�N */
		region1_start = mp3_scalefactor_band_index_table[frame->sampling_frequency][0/*LongBlock*/][(granule->region0_count + 1)];
		region2_start = mp3_scalefactor_band_index_table[frame->sampling_frequency][0/*LongBlock*/][(granule->region0_count + 1) + (granule->region1_count + 1)];
	}

	/* BigValue�̈�... */
	big_values_2 = granule->big_values * 2/*{x,y}*/;
	if(region1_start > big_values_2) region1_start = big_values_2; /* �K�v */
	if(region2_start > big_values_2) region2_start = big_values_2; /* �K�v */
	i = 0;
	/* Region0 */
	count = region1_start - i;
	mp3_huffman_decode_bigvalue(bs, granule->table_select[0], count, out);
	i   += count;
	out += count;
	/* Region1 */
	count = region2_start - i;
	mp3_huffman_decode_bigvalue(bs, granule->table_select[1], count, out);
	i   += count;
	out += count;
	/* Region2 */
	count = big_values_2  - i;
	mp3_huffman_decode_bigvalue(bs, granule->table_select[2], count, out);
	i   += count;
	out += count;

	/* Count1�̈�... */
	count = mp3_huffman_decode_count1(bs, granule->count1table_select, MP3_GRANULE_SIZE - i, out, granule->part2_3_length);
	i   += count;
	out += count;
	ASSERT(i <= MP3_GRANULE_SIZE);

	/* BigValue�̈��Count1�̈�̍��v�f�[�^�����L�����Ă����܂��B */
	count = i;

	/* rzero�̈�...
	 * * �����O�u���b�N�̏ꍇ�A���S��rzero�̈�Ɋ܂܂��T�u�o���h�͏������Ă��Ȃ��̂ŁA�[���t�B�����Ȃ��Ă������ł��B
	 *   ���A���G��������邽�߂ƁA���ɏ������x���Ԃɍ����Ă���̂ŁA��ɍŌ�܂Ń[���t�B�����邱�Ƃɂ��܂����B
	 *   ��������A�������̕K�v����������A�������Ă݂Ă��������B
	 */
	while(i < MP3_GRANULE_SIZE) {
		*out++ = 0;
		i++;
	}

	/* BigValue�̈��Count1�̈�̍��v�f�[�^����Ԃ��܂��B */
	return count;
}

/****************************************************************************
 *	�t�ʎq��
 ****************************************************************************/

/* �t�ʎq�������B(�V���[�g�u���b�N)
 * [in]
 *	frame		�t���[���w�b�_�\���́B
 *	granule		�O���j���[�����\���́B
 *	scale		�X�P�[���t�@�N�^�\���́B
 *	in		���̓o�b�t�@�B
 *	out		�o�̓o�b�t�@�B
 * [note]
 *	* �X�P�[���t�@�N�^�X�P�[��(MP3GRANULEINFO.scalefac_scale)�͖��Ή��ł��B
 *	  ���ۂ�MP3�t�@�C���ł͂قƂ��0�ɂȂ��Ă���悤�Ȃ̂ŁA���܂�e���͂Ȃ��Ǝv���܂��B
 *	* �T�u�u���b�N�Q�C��(MP3GRANULEINFO.subblock_gain[])�͖��Ή��ł��B
 *	  ���ۂ�MP3�t�@�C���ł͂قƂ��0�ɂȂ��Ă���悤�Ȃ̂ŁA���܂�e���͂Ȃ��Ǝv���܂��B
 *	* �ʏ��MP3�t�@�C���ł̓����O�u���b�N���唼���߁A�V���[�g�u���b�N�͑S�̂�10%���x�ł��B
 *	  �ł�����A�V���[�g�u���b�N��������������֐������������Ă��A���b�͏��Ȃ��ł��B
 *	  �����������RAM������Ȃ��Ȃ����ꍇ�́A���̊֐���SRAM�Ɉڂ��Ă��܂��Ă������Ǝv���܂��B
 *	  ��
 *	  SRAM�ֈڂ��܂����B
 */
void
mp3_requantize_short(const MP3FRAMEHEADER* frame, const MP3GRANULEINFO* granule, const MP3SCALEFACTOR* scale, const short* in/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, short* out/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/)
{
	int gain, i, j, k, w, index;
	short h;
	const short* sfb_index;
	const short (*sfb_scale)[MP3_SUBBLOCK_COUNT];
	const short* reorder;

	/* �X�P�[���t�@�N�^�o���h�̏����B */
	sfb_index = &mp3_scalefactor_band_index_table[frame->sampling_frequency][1/*ShortBlock*/][1/*���̃o���h�̊J�n�C���f�N�X*/];
	sfb_scale = scale->short_block;
	index = 0;

	/* �T���v�����ёւ��p�́A�ǂݍ��ݑ��C���f�N�X�e�[�u�����擾�B */
	reorder = mp3_reorder_read_index_table[frame->sampling_frequency];

	gain = mp3_gain_table[granule->global_gain];	/* 1.13.18 */
	for(i = 0; i < MP3_SUBBAND_COUNT; i++) {
		for(j = 0; j < MP3_SHORT_IMDCT_SIZE; j++) {
			/* ���g����������1�A���Ԏ�������3�T���v��(=3�T�u�u���b�N)�t�ʎq���B
			 * ���̌�A�V���[�g�u���b�N��IMDCT�� (���g��������6�~���Ԏ�����1)�~���Ԏ�����3 ��P�ʂƂ��ď�������̂ŁA
			 * ������ [���g��������32][���Ԏ�����3][���g��������6] �̏��ɕ��ёւ��Ă����܂��B
			 */
			for(k = 0; k < MP3_SUBBLOCK_COUNT; k++) {
				h = in[*reorder++];			/* 1.15. 0 */
				w = h * mp3_pow1_3_table[abs(h)];	/* 1.21.10 (short�~short) */
				w = w * gain;				/* 1. 3.28 (int�~int) */
				*out = w >> (14 + (*sfb_scale)[k] / 2);	/* 1. 1.14 (�I�[�o�[�t���[����) */
				out += MP3_SHORT_IMDCT_SIZE;
			}
			out -= MP3_SUBBAND_SIZE - 1;

			/* ���̃X�P�[���t�@�N�^�o���h�̊J�n�C���f�N�X�ɓ��B������A�g�p�X�P�[�������֐i�߂܂��B */
			index++;
			if(index == *sfb_index) {
				sfb_index++;
				sfb_scale++;
			}
		}
		out += MP3_SUBBAND_SIZE - MP3_SHORT_IMDCT_SIZE;
	}
}

/****************************************************************************
 *	IMDCT
 ****************************************************************************/

/* IMDCT�����B(�V���[�g�u���b�N)
 * [in]
 *	in		���̓o�b�t�@�B
 *	out		�o�̓o�b�t�@�B
 *	save		�t���[���Ԃŕۑ�����K�v�̂���o�b�t�@�B
 * [note]
 *	* �T�u�o���h���������̈ꕔ����肵�Ă��܂��B
 *	* �ʏ��MP3�t�@�C���ł̓����O�u���b�N���唼���߁A�V���[�g�u���b�N�͑S�̂�10%���x�ł��B
 *	  �ł�����A�V���[�g�u���b�N��������������֐������������Ă��A���b�͏��Ȃ��ł��B
 *	  �����������RAM������Ȃ��Ȃ����ꍇ�́A���̊֐���SRAM�Ɉڂ��Ă��܂��Ă������Ǝv���܂��B
 *	  ��
 *	  SRAM�ֈڂ��܂����B
 */
/**************************************************************/

void
mp3_imdct_short(const short* in/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, short* out/*[MP3_SUBBAND_SIZE][MP3_SUBBAND_COUNT]*/, short* save/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/)
{
	int i, j, k, sum;
	const short* m;

	for(k = 0; k < MP3_SUBBAND_COUNT; k++) {
		/* |=�`=| |=�a=| |=�b=| |=�c=| |=�d=| |=�e=| */
		/* 000000 ------ ------ ------ ------ 000000 */
		/* ------ ?????? ?????? ------ ------ ------ */
		/* ------ ------ ?????? ?????? ------ ------ */
		/* ------ ------ ------ ?????? ?????? ------ */
		/* |=save�Ƒ����ďo��=| |====save�ɕۑ�====| �~ MP3_SUBBAND_COUNT */

		/* |=�`=| */
		for(i = 0; i < MP3_SHORT_IMDCT_SIZE; i++) {
			sum = *save++;	/* 1.1.14 */
			/* �T�u�o���h���������̈ꕔ�����B��s�����𕄍����]���ē]�u���Ă����B */
			if(k & i & 1) sum = -sum;
			*out = sum;	/* 1.1.14 */
			out += MP3_SUBBAND_COUNT;
		}

		/* |=�a=| */
		m = &mp3_imdct_short_matrix[0][0];
		for(i = 0; i < MP3_SHORT_IMDCT_SIZE; i++) {
			sum = 0;
			for(j = 0; j < MP3_SHORT_IMDCT_SIZE; j++) {
				sum += *in++ * *m++;	/* short�~short 1.3.28 */
			}
			sum >>= 14;	/* 1.1.14 */
			sum += *save++;	/* 1.1.14 */
			/* �T�u�o���h���������̈ꕔ�����B��s�����𕄍����]���ē]�u���Ă����B */
			if(k & i & 1) sum = -sum;
			*out = sum;	/* 1.1.14 */
			out += MP3_SUBBAND_COUNT;
			in  -= MP3_SHORT_IMDCT_SIZE;
		}

		/* |=�b=| */
		for(i = 0; i < MP3_SHORT_IMDCT_SIZE; i++) {
			sum = 0;
			for(j = 0; j < MP3_SHORT_IMDCT_SIZE; j++) {
				sum += *in++ * *m++;	/* short�~short 1.3.28 */
			}
			m -= MP3_SHORT_IMDCT_SIZE * MP3_SHORT_IMDCT_SIZE + MP3_SHORT_IMDCT_SIZE;
			for(j = 0; j < MP3_SHORT_IMDCT_SIZE; j++) {
				sum += *in++ * *m++;	/* short�~short 1.3.28 */
			}
			sum >>= 14;	/* 1.1.14 */
			sum += *save++;	/* 1.1.14 */
			/* �T�u�o���h���������̈ꕔ�����B��s�����𕄍����]���ē]�u���Ă����B */
			if(k & i & 1) sum = -sum;
			*out = sum;	/* 1.1.14 */
			out += MP3_SUBBAND_COUNT;
			m  += MP3_SHORT_IMDCT_SIZE * MP3_SHORT_IMDCT_SIZE;
			in -= MP3_SHORT_IMDCT_SIZE + MP3_SHORT_IMDCT_SIZE;
		}
		m    -= MP3_SHORT_IMDCT_SIZE * MP3_SHORT_IMDCT_SIZE;
		in   += MP3_SHORT_IMDCT_SIZE;
		out  -= MP3_GRANULE_SIZE - 1;
		save -= MP3_SUBBAND_SIZE;

		/* |=�c=| */
		for(i = 0; i < MP3_SHORT_IMDCT_SIZE; i++) {
			sum = 0;
			for(j = 0; j < MP3_SHORT_IMDCT_SIZE; j++) {
				sum += *in++ * *m++;	/* short�~short 1.3.28 */
			}
			m -= MP3_SHORT_IMDCT_SIZE * MP3_SHORT_IMDCT_SIZE + MP3_SHORT_IMDCT_SIZE;
			for(j = 0; j < MP3_SHORT_IMDCT_SIZE; j++) {
				sum += *in++ * *m++;	/* short�~short 1.3.28 */
			}
			sum >>= 14;	/* 1.1.14 */
			*save++ = sum;	/* 1.1.14 */
			m  += MP3_SHORT_IMDCT_SIZE * MP3_SHORT_IMDCT_SIZE;
			in -= MP3_SHORT_IMDCT_SIZE + MP3_SHORT_IMDCT_SIZE;
		}
		m  -= MP3_SHORT_IMDCT_SIZE * MP3_SHORT_IMDCT_SIZE;
		in += MP3_SHORT_IMDCT_SIZE;

		/* |=�d=| */
		for(i = 0; i < MP3_SHORT_IMDCT_SIZE; i++) {
			sum = 0;
			for(j = 0; j < MP3_SHORT_IMDCT_SIZE; j++) {
				sum += *in++ * *m++;	/* short�~short 1.3.28 */
			}
			sum >>= 14;	/* 1.1.14 */
			*save++ = sum;	/* 1.1.14 */
			in -= MP3_SHORT_IMDCT_SIZE;
		}
		in += MP3_SHORT_IMDCT_SIZE;

		/* |=�e=| */
		for(i = 0; i < MP3_SHORT_IMDCT_SIZE; i++) {
			*save++ = 0;	/* 1.1.14 */
		}
	}
}

/********************************************************************/


/****************************************************************************
 *	MP3�h���C�o
 ****************************************************************************/

#define MP3BUFLEN	(64 * 5)

int
mp3_init(MP3DRIVER* mp3, const void* data, int len)
{
	/* �܂��[���N���A�B */
	_memset(mp3, 0, sizeof(MP3DRIVER));

	/* �ǂݍ��݈ʒu�ƏI�[�ʒu���i�[���܂��B */
	mp3->pos = data;
	mp3->end = mp3->pos + len;

	/* �T���v�����O���g���ϊ�DDA�̏����B */
	mp3->e_fbuff = -SPEAKER_FREQUENCY;

	/* �ŏ��̃t���[�����f�R�[�h���A�o�b�t�@�𖞂����܂��B */
	if(mp3_decode_frame(mp3) != 0) return -1;

	return 0;
}

int
mp3_stream_callback(short* wbuff, int param)
{
	MP3DRIVER* mp3 = (MP3DRIVER*)param;
	short* wbuff_end = wbuff + MP3BUFLEN;
	int sampling_frequency = mp3->sampling_frequency;
	int c_fbuff = mp3->c_fbuff;
	int i_fbuff = mp3->i_fbuff;
	int e_fbuff = mp3->e_fbuff;
	short* fbuff = mp3->fbuff;

	while(wbuff < wbuff_end) {
		*wbuff++ = fbuff[i_fbuff];
		e_fbuff += sampling_frequency;
		while(e_fbuff >= 0) {
			e_fbuff -= SPEAKER_FREQUENCY;
			i_fbuff++;
		}
		while(i_fbuff >= c_fbuff) {
			i_fbuff -= c_fbuff;
			if(mp3_decode_frame(mp3) != 0) return -1;
			sampling_frequency = mp3->sampling_frequency;
			c_fbuff = mp3->c_fbuff;
		}
	}

	mp3->i_fbuff = i_fbuff;
	mp3->e_fbuff = e_fbuff;

	return 0;
}

/* 1�t���[���f�R�[�h���܂��B
 * [in]
 *	mp3			MP3�h���C�o�\���́B
 * [out]
 *	�߂�l			�����Ȃ�0��Ԃ��܂��B
 *				���s�Ȃ�0�ȊO�̒l��Ԃ��܂��B(�I�[�ɒB�����ꍇ�Ȃ�)
 *	mp3->fbuff		�f�R�[�h�����t���[���f�[�^���i�[���܂��B
 *	mp3->sampling_frequency	�f�R�[�h�����t���[���̃T���v�����O���g�����i�[���܂��B
 */
int
mp3_decode_frame(MP3DRIVER* mp3)
{
	MP3BITSTREAM bs, bs_tmp;
	MP3FRAMEHEADER frame;
	MP3SIDEINFO side;
	MP3SCALEFACTOR scale;
	MP3GRANULEINFO* granule;
	const MP3SAMPLINGFREQUENCYCONVERTTABLE* convert;
	int bitrate, sampling_frequency, channels, frame_size, main_data_size;
	int i_granule, i_channel, count, subband_count;
	static MP3SAMPLINGFREQUENCYCONVERTTABLE _convert;
	short* out;
	unsigned char table[32];
	int i;

	/* �t���[���Ԃŕۑ�����K�v�̂Ȃ��o�b�t�@�B */
	/* SRAM�Ɏ��ꍇ�B */
	static short buf1[MP3_GRANULE_SIZE];
	static short buf2[MP3_GRANULE_SIZE];

	/* �t���[���J�n�ʒu���������܂��B */
	for(;;) {
		/* �f�[�^�I�[? */
		if(mp3->pos >= mp3->end - 1) return -1;
		/* �������[�h? */
		if(mp3->pos[0] == 0xff &&
		  (mp3->pos[1] &  0xf0) == 0xf0) break;
		mp3->pos++;
	}

	/* �r�b�g�X�g���[�����J���܂��B */
	mp3_bitstream_open(&bs, mp3->pos);

	/* �t���[���w�b�_��ǂݍ��݂܂��B */
	if(mp3_read_frame_header(&bs, &frame) != 0) return -1/*�t���[���w�b�_���s��*/;
	bitrate = mp3_bitrate_table[frame.bitrate_index];
	sampling_frequency = mp3_sampling_frequency_table[frame.sampling_frequency];
	channels = frame.mode == 3 ? 1 : 2;
	frame_size = 144 * bitrate / sampling_frequency + frame.padding_bit; /* �t���[���T�C�Y�v�Z */

	/* �T�C�h����ǂݍ��݂܂��B */
	mp3_read_side_info(&bs, &side, channels);

	/* ���C���f�[�^�̃r�b�g�~�ρB */
	ASSERT(bs.cnt == 0); /* �T�C�h���̖����Ńo�C�g���E�ɐ��񂵂Ă���͂� */
	main_data_size = frame_size - (bs.pos - mp3->pos)/*�t���[���w�b�_�ƃT�C�h�����������c�肪���C���f�[�^*/;
	if(mp3->main_save_size + main_data_size > sizeof mp3->main_save) {
		_memmove(&mp3->main_save[0], &mp3->main_save[mp3->main_save_size - side.main_data_begin], side.main_data_begin);
		mp3->main_save_size = side.main_data_begin;
	}
	_memcpy(&mp3->main_save[mp3->main_save_size], bs.pos, main_data_size);
	mp3_bitstream_open(&bs, &mp3->main_save[mp3->main_save_size - side.main_data_begin]);
	mp3->main_save_size += main_data_size;

	/* �T���v�����O���g���ϊ��e�[�u�����擾���܂��B */
	convert = &mp3_sampling_frequency_convert_table[frame.sampling_frequency];
//		static MP3SAMPLINGFREQUENCYCONVERTTABLE _convert;
//		unsigned char _table[32];
//		int i;
	for(i = 0; i < 32; i++) table[i] = i;
	_convert.table = table;
	_convert.count = 32;
	_convert.sampling_frequency = mp3_sampling_frequency_table[frame.sampling_frequency];
	convert = &_convert;

	/* �e�O���j���[�����f�R�[�h���܂��B */
	out = mp3->fbuff;
	for(i_granule = 0; i_granule < MP3_GRANULE_COUNT; i_granule++) {
		for(i_channel = 0; i_channel < channels; i_channel++) {
			granule = &side.granule_info[i_granule][i_channel];
			if(i_channel == 0) { /* ���`���l���̂ݎg�p */
				bs_tmp = bs;
				/* �n�t�}����������Count1�̈�I�[���o�̂��߁A�ǂݍ��񂾃r�b�g�������Z�b�g���Ă����܂��B
				 * �X�P�[���t�@�N�^(Part2)+�n�t�}������(Part3)�̍��v�r�b�g����GRANULE.part2_3_length�ƂȂ�܂��B
				 */
				bs_tmp.read_bits = 0;
				/* Part2: �X�P�[���t�@�N�^�ǂݍ��݁B */
				mp3_read_scalefactor(&bs_tmp, granule, &scale, !i_granule ? 0 : side.scfsi[i_channel]);
				/* Part3: �n�t�}���������B */
				count = mp3_huffman_decode(&bs_tmp, &frame, granule, buf1);
				if(granule->block_type == 2) {
					if(granule->mixed_block_flag) {
						/* �~�b�N�X�u���b�N */
						/* ��TODO: �~�b�N�X�u���b�N�𐶐�����G���R�[�_���܂����݂��Ȃ��炵���̂ŁA�Ƃ肠�����K�v�Ȃ��悤�ł��B */
						_memset(buf1, 0, sizeof buf1);
					} else {
						/* �V���[�g�u���b�N */
						/* �t�ʎq���B */
						mp3_requantize_short(&frame, granule, &scale, buf1, buf2);
						/* �V���[�g�u���b�N�ɂ͂��Ƃ��ƃG���A�V���O�팸�����͂���܂���B */
						/* IMDCT�B */
						mp3_imdct_short(buf2, buf1, &mp3->imdct_save[0]);
					}
				} else {
					/* BigValue/Count1�̈�̃f�[�^���܂ރT�u�o���h�������߂܂��B */
					subband_count = (count + (MP3_SUBBAND_COUNT - 1)) / MP3_SUBBAND_COUNT;
					/* �����O�u���b�N */
					/* �t�ʎq���B */
					mp3_requantize_long(&frame, granule, &scale, buf1, buf2, count);
					/* �G���A�V���O�팸�B */
					mp3_antialias(buf2, subband_count);
					/* IMDCT�B */
					mp3_imdct_long(buf2, buf1, &mp3->imdct_save[0], subband_count);
				}
				/* �T�u�o���h�����B */
				mp3->subband_synthesys_offset = mp3_subband_synthesys(buf1, out, &mp3->subband_synthesys_save[0][0], mp3->subband_synthesys_offset, convert);
				out += convert->count * MP3_SUBBAND_SIZE; /* �T���v�����O���g���ϊ���̃O���j���[������T���v���� */
			}
			mp3_bitstream_skip(&bs, granule->part2_3_length);
		}
	}

	/* �T���v�����O���g���ƃT���v�������i�[���܂��B */
	mp3->sampling_frequency = convert->sampling_frequency;
	mp3->c_fbuff = convert->count * MP3_SUBBAND_SIZE * MP3_GRANULE_COUNT; /* �T���v�����O���g���ϊ���̃t���[������T���v���� */

	/* �t���[���ʒu��i�߂܂��B */
	mp3->pos += frame_size;

	return 0;
}

/****************************************************************************
 *	�A�v���P�[�V�����p�֐�
 ****************************************************************************/

int
mp3_play(const void* data, int len)
{
	static MP3DRIVER mp3; /* STATIC�ł�! */

	/* �܂��m���ɒ�~���܂��B */
	mp3_stop();

	/* MP3�h���C�o�����������܂��B */
	if(mp3_init(&mp3, data, len) != 0) return -1;

	/* �X�g���[���Đ����J�n���܂��B */
//	stream_play(MP3BUFLEN, mp3_stream_callback, (int)&mp3, 0);

	return 0;
}

void
mp3_stop()
{
	/* �X�g���[���Đ����~���܂��B */
//	stream_stop();

	/* MP3�h���C�o�̃N���[���A�b�v�͕s�v�ł��B */
}

