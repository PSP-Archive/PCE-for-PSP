/*	
 *	frammp3.c
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
 *	* Wed Apr 14 12:30:00 JST 2004 Naoyuki Sawa
 *	- mp3_bitstream_get()�ŁA24bit�ȏ�̃r�b�g���ǂݍ��ޏ������Ԉ���Ă����̂��C���B
 *	  ���������ۂɂ�24bit�ȏ�̃r�b�g���ǂނ��Ƃ͂Ȃ��̂ŁA����܂ł̃v���O�����ł����͔������܂���B
 */
//#include "clip.h"
#include "clipmp3.h"

/****************************************************************************
 *	�r�b�g�X�g���[��
 ****************************************************************************/

/* �r�b�g�X�g���[������r�b�g���ǂݍ��݂܂��B
 * [in]
 *	bs		�r�b�g�X�g���[���\���́B
 *	cnt		�ǂݍ��ރr�b�g���B(0�`32)
 * [out]
 *	�߂�l		�ǂݍ��񂾃r�b�g��B
 */
int
mp3_bitstream_get(MP3BITSTREAM* bs, int cnt)
{
	int bits;

	ASSERT(0 <= cnt && cnt <= 32);

	if(cnt <= 24) {
		bs->read_bits += cnt;
		while(bs->cnt < cnt) {
			bs->buf = bs->buf << 8 | *bs->pos++;
			bs->cnt += 8;
		}
		bs->cnt -= cnt;
		bits = (bs->buf >> bs->cnt) & ((1 << cnt) - 1);
	} else {
		cnt -= 24;
		bits = mp3_bitstream_get(bs,  24) << cnt |
		       mp3_bitstream_get(bs, cnt);
	}

	return bits;
}

/* �r�b�g�X�g���[������1�r�b�g�ǂݍ��݂܂��B
 * [in]
 *	bs		�r�b�g�X�g���[���\���́B
 * [out]
 *	�߂�l		�ǂݍ��񂾃r�b�g�B
 * [note]
 *	* �n�t�}��������������1�r�b�g�ǂݍ��݂𑽗p����̂ŁA���ʂɍ��������܂����B
 *	  mp3_huffman_decode_bigvalue/count1()�̍œ������[�v�ł̃��W�X�^�ޔ��E�������Ȃ��邽�߁A
 *	  �n�t�}��������������2.0�`2.5�{���������ł��܂��B
 */
#define mp3_bitstream_get1(bs)									\
		((bs)->read_bits++,								\
		((bs)->cnt ? (((bs)->buf               ) >> ((bs)->cnt = (bs)->cnt - 1) & 1)	\
		           : (((bs)->buf = *(bs)->pos++) >> ((bs)->cnt =         8 - 1) & 1)))

/****************************************************************************
 *	�n�t�}��������
 ****************************************************************************/

/* BigValue�̈�̒P��Region�𕜍�����T�u���[�`���ł��B
 * [in]
 *	bs			�r�b�g�X�g���[���\���́B
 *	i_table			�����e�[�u���ԍ��B
 *	count			�����T���v�����B
 *	out			�o�̓o�b�t�@�B
 * [note]
 *	* �������ߖ�̂��߂ɁA�T�u���[�`���ɕ����܂����B
 *	  �������Amp3_huffman_decode()�̒���3��R�[�h�W�J����ƁA��300�o�C�g����Ă��܂��܂��B
 *	* ���̊֐���1�O���j���[������3��Ăяo����邾���Ȃ̂ŁA�֐��Ăяo���ɂ�鑬�x�ቺ�͏��Ȃ��Ǝv���܂��B
 */
void
mp3_huffman_decode_bigvalue(MP3BITSTREAM* bs, int i_table, int count, short* out)
{
	int linbits, i_node, x, y;
	const short (*table)[2];

	/* �����e�[�u���擾�B */
	table   = mp3_huffman_bigvalue_table[i_table].table;
	linbits = mp3_huffman_bigvalue_table[i_table].linbits;

	/* ���������B */
	if(table) {
		while(count) {
			i_node = 0;
			do {
				i_node = table[i_node][mp3_bitstream_get1(bs)];
			} while(i_node > 0);
			i_node = -i_node;
			x = i_node >> 4;
			y = i_node & 15;
			if(x == 15) x += mp3_bitstream_get(bs, linbits);
			if(x && mp3_bitstream_get1(bs)) x = -x;
			if(y == 15) y += mp3_bitstream_get(bs, linbits);
			if(y && mp3_bitstream_get1(bs)) y = -y;
			*out++ = x;
			*out++ = y;
			count -= 2/*{x,y}*/;
		}
	} else {
		/* �[���Œ�e�[�u���B(SCMPX�͐������܂��񂪁ALAME�͐������܂�) */
		while(count) {
			*out++ = 0;
			*out++ = 0;
			count -= 2/*{x,y}*/;
		}
	}
}

/* Count1�̈�𕜍�����T�u���[�`���ł��B
 * [in]
 *	bs			�r�b�g�X�g���[���\���́B
 *	i_table			�����e�[�u���ԍ��B
 *	count_limit		�ő�o�̓T���v�����B
 *	out			�o�̓o�b�t�@�B
 *	part2_3_length		GRANULE.part2_3_length��n���Ă��������B
 * [out]
 *	�߂�l			�o�̓T���v�����B
 * [note]
 *	* �r�b�g�X�g���[���\���̂�read_bits���A�X�P�[���t�@�N�^�ǂݍ��ݒ��O�̃^�C�~���O�Ń��Z�b�g���Ă����Ă��������B
 *	* �{���́ABigValue�̈��Count1�̈�����킹�Ă������MP3_GRANULE_SIZE(576)�T���v���ȓ��Ɏ��܂�͂��Ȃ̂ł����A
 *	  �f�R�[�_�ɂ���Ă̓I�[�o�[�������Ă��܂����Ƃ�����悤�ł��B
 *	  ���̂��߁A�ő�o�̓T���v�����ɂ�郊�~�b�^���K�v�ł��B(������MP3�f�R�[�_���������Ă���)
 *	  SCMPX�ŃG���R�[�h����MP3�t�@�C���ł��A�I�[�o�[�������܂����B�����������炱����̃o�O�����m��܂��񂪁E�E�E
 */
int
mp3_huffman_decode_count1(MP3BITSTREAM* bs, int i_table, int count_limit, short* out, int part2_3_length)
{
	int i_node, v, w, x, y;
	const short (*table)[2];
	short *out0, *out_limit;

	out0 = out;
	out_limit = out + count_limit - 4/*{v,w,x,y}*/;

	/* �����e�[�u���擾�B */
	table = mp3_huffman_count1_table[i_table].table;

	/* ���������B */
	while((out <= out_limit) && (bs->read_bits < part2_3_length)) {
		i_node = 0;
		do {
			i_node = table[i_node][mp3_bitstream_get1(bs)];
		} while(i_node > 0);
		i_node = -i_node;
		v = i_node >> 3 & 1;
		w = i_node >> 2 & 1;
		x = i_node >> 1 & 1;
		y = i_node >> 0 & 1;
		if(v && mp3_bitstream_get1(bs)) v = -v;
		if(w && mp3_bitstream_get1(bs)) w = -w;
		if(x && mp3_bitstream_get1(bs)) x = -x;
		if(y && mp3_bitstream_get1(bs)) y = -y;
		*out++ = v;
		*out++ = w;
		*out++ = x;
		*out++ = y;
	}

	return out - out0;
}

/****************************************************************************
 *	�t�ʎq��
 ****************************************************************************/

/* * MP3�d�l�ɒ�߂�ꂽ�A�t�ʎq�������̌v�Z�͎��̒ʂ�ł��B
 *
 *	xr �� is �~ |is|^(1/3) �~ 2^(Gain-Scale)
 *
 *	is: �t�ʎq�������ւ̓��̓f�[�^
 *	xr: �t�ʎq����������̏o�͌���
 *
 *   �������A
 *
 *	Gain  �� (GlobalGain-210)/4 - SubblockGain
 *	Scale �� ((ScalefactorScale+1)�~(Scalefactor+Preemphasis)) / 2
 *
 *   SubblockGain�AScalefactorScale�APreemphais�̏������ȗ�����0�Ɖ��肷��ƁA
 *
 *	Gain  �� (GlobalGain-210) / 4
 *	Scale �� Scalefactor / 2
 *
 *   �܂Ƃ߂�ƁA�������Z�ł͎��̂悤�ɏ����ł��܂��B
 *
 *	xr �� is �~ |is|^(1/3) �~ 2^((GlobalGain-210)/4 - Scalefactor/2)
 *	   �� is �~ |is|^(1/3) �~ 2^((GlobalGain-210)/4) �� 2^(Scalefactor/2)
 *	   �� is �~ |is|^(1/3) �~ 2^((GlobalGain-210)/4) >> (Scalefactor/2)
 *	            ~~~~~~~~~~    ~~~~~~~~~~~~~~~~~~~~~~
 *	            �e�[�u����          �e�[�u����      
 */

/* �t�ʎq�������B(�����O�u���b�N)
 * [in]
 *	frame		�t���[���w�b�_�\���́B
 *	granule		�O���j���[�����\���́B
 *	scale		�X�P�[���t�@�N�^�\���́B
 *	in		���̓o�b�t�@�B
 *	out		�o�̓o�b�t�@�B
 *	count		BigValue�̈��Count1�̈�̃f�[�^�����v�B
 * [note]
 *	* �X�P�[���t�@�N�^�X�P�[��(MP3GRANULEINFO.scalefac_scale)�͖��Ή��ł��B
 *	  ���ۂ�MP3�t�@�C���ł͂قƂ��0�ɂȂ��Ă���悤�Ȃ̂ŁA���܂�e���͂Ȃ��Ǝv���܂��B
 *	* �������̂��߂ɁABigValue�̈��Count1�̈�̃f�[�^�̂ݏ������܂��B
 *	  rzero�̈�̃f�[�^�͏������܂���B
 */
void
mp3_requantize_long(const MP3FRAMEHEADER* frame, const MP3GRANULEINFO* granule, const MP3SCALEFACTOR* scale, const short* in/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, short* out/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, int count)
{
	int gain, i, w, index;
	short h;
	const short* sfb_index;
	const short* sfb_scale;

	/* �X�P�[���t�@�N�^�o���h�̏����B */
	sfb_index = &mp3_scalefactor_band_index_table[frame->sampling_frequency][0/*LongBlock*/][1/*���̃o���h�̊J�n�C���f�N�X*/];
	sfb_scale = scale->long_block;
	index = 0;

	gain = mp3_gain_table[granule->global_gain];	/* 1.13.18 */
	for(i = 0; i < count; i++) {
		/* ���g����������1�T���v���t�ʎq���B */
		h = *in++;				/* 1.15. 0 */
		w = h * mp3_pow1_3_table[abs(h)];	/* 1.21.10 (short�~short) */
		w = w * gain;				/* 1. 3.28 (int�~int) */
		*out++ = w >> (14 + *sfb_scale / 2);	/* 1. 1.14 (�I�[�o�[�t���[����) */

		/* ���̃X�P�[���t�@�N�^�o���h�̊J�n�C���f�N�X�ɓ��B������A�g�p�X�P�[�������֐i�߂܂��B */
		index++;
		if(index == *sfb_index) {
			sfb_index++;
			sfb_scale++;
		}
	}
}

/****************************************************************************
 *	�G���A�V���O�팸
 ****************************************************************************/

/* * �����������͂�����Ă��܂��B�Ⴆ�΁A�L�ӂȃT�u�o���h��3�̏ꍇ�A
 *
 *	���{���͂������Ȃ���΂����Ȃ�
 *	+-------------+
 *	|             |
 *	|#31(all zero)|
 *	|             |
 *	+-------------+
 *	| .           |
 *	| . (all zero)|
 *	| .           |
 *	+-------------+
 *	|             |
 *	|# 3(all zero)|
 *	|             |<-+
 *	+-------------+  |�o�^�t���C���Z
 *	|             |<-+
 *	|# 2          |
 *	|             |<-+
 *	+-------------+  |�o�^�t���C���Z
 *	|             |<-+
 *	|# 1          |
 *	|             |<-+
 *	+-------------+  |�o�^�t���C���Z
 *	|             |<-+
 *	|# 0          |
 *	|             |
 *	+-------------+
 *
 *   �{���́A�T�u�o���h#2��#3�Ԃ̃o�^�t���C���Z���s��Ȃ���΂����܂���B
 *   ���A���݂̎����ł́A�Ō�̃o�^�t���C���Z���s���Ă��܂���B
 *
 *	������͂����Ȃ��Ă���
 *	+-------------+
 *	|             |
 *	|#31(  �S�~  )|
 *	|             |
 *	+-------------+
 *	| .           |
 *	| . (  �S�~  )|
 *	| .           |
 *	+-------------+
 *	|             |
 *	|# 3(  �S�~  )|
 *	|             |
 *	+-------------+
 *	|             |
 *	|# 2          |
 *	|             |<-+
 *	+-------------+  |�o�^�t���C���Z
 *	|             |<-+
 *	|# 1          |
 *	|             |<-+
 *	+-------------+  |�o�^�t���C���Z
 *	|             |<-+
 *	|# 0          |
 *	|             |
 *	+-------------+
 *
 *   �Ȃ��Ȃ�A�T�u�o���h#3�`31�ɑ΂��Ă͋t�ʎq�������ł̏o�͂��s���Ă��炸�A�S�~�������Ă��邩��ł��B
 *   �T�u�o���h#2��#3�Ԃ̃o�^�t���C���Z�̂݁A#3��all zero�ƌ��Ȃ��ē��ʏ������邱�Ƃ��ł��܂��B
 *
 *	�����ǈāi���̗p�j
 *	+-------------+
 *	|             |
 *	|#31(  �S�~  )|
 *	|             |
 *	+-------------+
 *	| .           |
 *	| . (  �S�~  )|
 *	| .           |
 *	+-------------+
 *	|             |
 *	|# 3(  �S�~  )|
 *	|             |
 *	+-------------+  |#3��all zero�ƌ��Ȃ��ē��ʏ���
 *	|             |<-+
 *	|# 2          |
 *	|             |<-+
 *	+-------------+  |�o�^�t���C���Z
 *	|             |<-+
 *	|# 1          |
 *	|             |<-+
 *	+-------------+  |�o�^�t���C���Z
 *	|             |<-+
 *	|# 0          |
 *	|             |
 *	+-------------+
 *
 *   ���A����͂����͂��܂���ł����B
 *   �L�ӂȃf�[�^�̂��������΂񍂉���̕����Ȃ̂ŁA��������Ă��Ă����܂�e���͂Ȃ��Ɣ��f��������ł��B
 */

/* �G���A�V���O�팸�����B(�����O�u���b�N�̂�)
 * [in]
 *	in_out		���o�̓o�b�t�@
 *	subband_count	
 *	subband_count	BigValue�̈�܂���Count1�̈�̃f�[�^�����Ȃ��Ƃ��ꕔ�܂ރT�u�o���h�̐��B
 */

void
mp3_antialias(short* in_out, int subband_count)
{
	int i;
	short a, b, cs, ca;
	short *pa, *pb;
	const short* table;

	table = &mp3_antialias_table[0][0];
	while(--subband_count > 0) {
		in_out += MP3_SUBBAND_SIZE;
		pa = in_out;
		pb = in_out;
		for(i = 0; i < 8; i++) {
			a = *--pa;				/* 1.1.14 */
			b = *  pb;				/* 1.1.14 */
			cs = *table++;				/* 1.1.14 */
			ca = *table++;				/* 1.1.14 */
			*pa   = (a * cs - b * ca) >> 14;	/* short�~short 1.3.28 => 1.1.14 */
			*pb++ = (b * cs + a * ca) >> 14;	/* short�~short 1.3.28 => 1.1.14 */
			pb++;
		}
		table -= 2 * 8;
	}
}


/****************************************************************************
 *	IMDCT
 ****************************************************************************/

/* IMDCT�����B(�����O�u���b�N)
 * [in]
 *	in		���̓o�b�t�@�B
 *	out		�o�̓o�b�t�@�B
 *	save		�t���[���Ԃŕۑ�����K�v�̂���o�b�t�@�B
 *	subband_count	BigValue�̈�܂���Count1�̈�̃f�[�^�����Ȃ��Ƃ��ꕔ�܂ރT�u�o���h�̐��B
 * [note]
 *	* �T�u�o���h���������̈ꕔ����肵�Ă��܂��B
 *	* �������̂��߂ɁABigValue�̈��Count1�̈�̃f�[�^�̂ݏ������܂��B
 *	  rzero�̈�̃f�[�^�͏������܂���B
 */
void
mp3_imdct_long(const short* in/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, short* out/*[MP3_SUBBAND_SIZE][MP3_SUBBAND_COUNT]*/, short* save/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, int subband_count)
{
	int i, j, k, sum;
	const short* m;

	k = 0;

	/* BigValue/Count1�̈� */
	while(k < subband_count) {
		m = &mp3_imdct_long_matrix[0][0];

		/* �u�O�̃O���j���[���̌㔼�v�Ɓu���݂̃O���j���[���̑O���v���������ďo�́B */
		for(i = 0; i < MP3_SUBBAND_SIZE; i++) {
			sum = 0;
			for(j = 0; j < MP3_SUBBAND_SIZE; j++) {
				sum += *in++ * *m++;	/* short�~short 1.3.28 */
			}
			sum >>= 14;	/* 1.1.14 */
			sum += *save++;	/* 1.1.14 */
			/* �T�u�o���h���������̈ꕔ�����B��s�����𕄍����]���ē]�u���Ă����B */
			if(k & i & 1) sum = -sum;
			*out = sum;	/* 1.1.14 */
			out += MP3_SUBBAND_COUNT;
			in  -= MP3_SUBBAND_SIZE;
		}
		out  -= MP3_GRANULE_SIZE - 1;
		save -= MP3_SUBBAND_SIZE;

		/* �u���̃O���j���[���̑O���v�ɍ������邽�߂Ɂu���݂̃O���j���[���̌㔼�v��ۑ��B */
		for(i = 0; i < MP3_SUBBAND_SIZE; i++) {
			sum = 0;
			for(j = 0; j < MP3_SUBBAND_SIZE; j++) {
				sum += *in++ * *m++;	/* short�~short 1.3.28 */
			}
			sum >>= 14;	/* 1.1.14 */
			*save++ = sum;	/* 1.1.14 */
			in -= MP3_SUBBAND_SIZE;
		}
		in += MP3_SUBBAND_SIZE;

		k++;
	}

	/* rzero�̈� */
	while(k < MP3_SUBBAND_COUNT) {
		/* �u�O�̃O���j���[���̌㔼�v�Ɓu���݂̃O���j���[���̑O���v���������ďo�́B */
		for(i = 0; i < MP3_SUBBAND_SIZE; i++) {
			sum = *save++;	/* 1.1.14 */
			/* �T�u�o���h���������̈ꕔ�����B��s�����𕄍����]���ē]�u���Ă����B */
			if(k & i & 1) sum = -sum;
			*out = sum;	/* 1.1.14 */
			out += MP3_SUBBAND_COUNT;
		}
		out  -= MP3_GRANULE_SIZE - 1;
		save -= MP3_SUBBAND_SIZE;

		/* �u���̃O���j���[���̑O���v�ɍ������邽�߂Ɂu���݂̃O���j���[���̌㔼�v��ۑ��B */
		for(i = 0; i < MP3_SUBBAND_SIZE; i++) {
			*save++ = 0;	/* 1.1.14 */
		}

		k++;
	}
}


/****************************************************************************
 *	�T�u�o���h����
 ****************************************************************************/

/* �T�u�o���h���������B
 * [in]
 *	in		���̓o�b�t�@�B
 *	out		�o�̓o�b�t�@�B
 *	save		�t���[���Ԃŕۑ�����K�v�̂���o�b�t�@�B
 *	offset		save�I�t�Z�b�g�B
 *			�O���mp3_subband_synthesys()�̖߂�l��n���Ă��������B
 *	convert		�T���v�����O���g���ϊ��e�[�u���B
 * [out]
 *	�߂�l		�����save�I�t�Z�b�g�B
 * [note]
 *	* �T�u�o���h���������̈ꕔ��mp3_imdct()�Ő�ɍs���Ă��܂��B
 */
int
mp3_subband_synthesys(const short* in/*[MP3_SUBBAND_SIZE][MP3_SUBBAND_COUNT]*/, short* out/*[upto MP3_GRANULE_SIZE]*/, short* save/*[16][MP3_SUBBAND_COUNT_2]*/, int offset, const MP3SAMPLINGFREQUENCYCONVERTTABLE* convert)
{
	int i, j, k, sum, count;
	const short* m;
	const unsigned char* table;

#define OFFSET_MASK	(MP3_SUBBAND_COUNT_2 * 16 - 1)

	table = convert->table;
	count = convert->count;

	for(k = 0; k < MP3_SUBBAND_SIZE; k++) {
		/* ��ݍ��݉��Z�B(32�f�[�^�����ԗ̈�ɕϊ�)
		 * ���͍s��̊�s�����𕄍����]���ē]�u���鏈���́AIMDCT�̒��ōs���Ă��܂��B
		 */
		for(i = 0; i < count; i++) {
			/* �O�� */
			m = mp3_subband_matrix[*table++];
			sum = 0;
			for(j = 0; j < MP3_SUBBAND_COUNT; j++) {
				sum += *in++ * *m++;	/* 1.3.28 */
			}
			save[offset] = sum >> 14;	/* 1.1.14 */
			offset += MP3_SUBBAND_COUNT;
			in -= MP3_SUBBAND_COUNT;
			/* �㔼 */
			m += MP3_SUBBAND_COUNT * MP3_SUBBAND_COUNT - MP3_SUBBAND_COUNT;
			sum = 0;
			for(j = 0; j < MP3_SUBBAND_COUNT; j++) {
				sum += *in++ * *m++;	/* 1.3.28 */
			}
			save[offset] = sum >> 14;	/* 1.1.14 */
			offset -= MP3_SUBBAND_COUNT - 1;
			in -= MP3_SUBBAND_COUNT;
		}
		table -= count;
		offset -= count;
		in += MP3_SUBBAND_COUNT;

		/* 16�g�̃f�[�^�̏d�ˍ��킹�B
		 * �|���t�F�[�Y�t�B���^�[�o���N�W���\�́A���炩���ߓ]�u���Ă���܂��B
		 */
		for(i = 0; i < count; i++) {
			m = mp3_polyphase_filter[*table++];
			sum = 0;
			for(j = 0; j < 16 / 2/*2�T���v���Â���*/; j++) {
				/* �O�� */
				sum += save[offset] * *m++;	/* 1.3.28 */
				offset += MP3_SUBBAND_COUNT_2 + MP3_SUBBAND_COUNT;
				offset &= OFFSET_MASK;
				/* �㔼 */
				sum += save[offset] * *m++;	/* 1.3.28 */
				offset += MP3_SUBBAND_COUNT_2 - MP3_SUBBAND_COUNT;
				offset &= OFFSET_MASK;
			}
			offset++;
			/* -0x7fff�`0x7fff�ɃX�P�[�����O�B */
			sum >>= 13;	/* 1.(16).15 */
			if(sum < -0x7fff) sum = -0x7fff;
			if(sum >  0x7fff) sum =  0x7fff;
			*out++ = sum;	/* 1.15 */
		}
		table -= count;
		offset -= MP3_SUBBAND_COUNT_2 + count;
		offset &= OFFSET_MASK;
	}

#undef OFFSET_MASK

	return offset;
}

