/*	
 *	clipmp3.c
 *
 *	P/ECE MP3 Driver
 *
 *	CLiP - Common Library for P/ECE
 *	Copyright (C) 2001-2003 Naoyuki Sawa
 *
 *	* Tue Oct 28 08:11:00 JST 2003 Naoyuki Sawa
 *	- 1st リリース。
 *	* Tue Nov  4 00:00:00 JST 2003 Naoyuki Sawa
 *	- SCMPXでエンコードしたMP3ファイルも再生できるよう、改造中です。
 *	  次の設定でエンコードしたファイルを再生して、ハングアップはしなくなりました。
 *		レイヤー	III
 *		ビットレート	64000 固定
 *		モノラル
 *		エンファシス	なし
 *	  音はまだまだで、とてもノイズが入ります。
 *	  原因は、まだショートブロック・ミックスブロックに対応していないからです。
 *	  現時点では、どちらもロングブロックとみなして再生してしまっています。
 *	* Thu Nov  6 23:50:00 JST 2003 Naoyuki Sawa
 *	- ショートブロック対応。
 *	- スケールファクタ対応。
 *	- mp3_read_granule_info()のバグ修正。
 *	- mp3_read_scalefactor()のバグ修正。
 *	* Mon Nov 10 03:15:00 JST 2003 Naoyuki Sawa
 *	- mp3_bitstream_skip()でread_bitsをカウントしていなかったバグを修正。
 *	  動作には関係ありません(part2_3_length領域でmp3_bitstream_skip()は使わないから)が、
 *	  今後のために修正しておきました。
 *	* Wed Nov 12 05:25:00 JST 2003 Naoyuki Sawa
 *	- CLiPライブラリ内へ移動。
 *	* Mon Nov 24 06:00:00 JST 2003 Naoyuki Sawa
 *	- clippce.hでSPEAKER_FREQUENCYを定義したので、出力周波数定数にこのシンボルを使うよう修正。
 *	  ソース上だけの修正です。実行バイナリには変化ありません。
 */
//#include "clip.h"
#include "clipmp3.h"
#include "_clib.h"

/****************************************************************************
 *	代用標準ライブラリ
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

/* ※未使用・未確認 */

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
	
/* ID3 Tag V1を読み込みます。
 * [in]
 *	tag		ID3 Tag V1を読み込む構造体。
 *	ptr		MP3ファイルの終端から128バイト目を渡してください。
 *			ファイルが128バイト未満の場合は、この関数を呼んではいけません。
 * [out]
 *	* 戻り値	読み込み成功なら0を返します。
 *			読み込み失敗なら0以外の値を返します。
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
 *	ビットストリーム
 ****************************************************************************/

/* ビットストリームを開きます。
 * [in]
 *	bs		ビットストリーム構造体。
 *	pos		最初の読み込みアドレス。
 */
void
mp3_bitstream_open(MP3BITSTREAM* bs, const void* pos)
{
	bs->read_bits = 0;
	bs->cnt = 0;
	/*bs->buf = 0;{{不要}}*/
	bs->pos = (const unsigned char*)pos;
}

/* ビットストリームからビット列を読み飛ばします。
 * [in]
 *	bs		ビットストリーム構造体。
 *	cnt		読み飛ばすビット数。(0～任意)
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
 *	フレームヘッダ
 ****************************************************************************/

/* フレームヘッダを読み込みます。
 * [in]
 *	bs		ビットストリーム構造体。
 *	frame		フレームヘッダを格納する構造体。
 * [out]
 *	戻り値		成功なら0を返します。
 *			失敗なら0以外の値を返します。
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

	/* 最低限のエラーチェック。 */
	if(frame->syncword != 0xfff) return -1;	/* 同期ワード */
	if(frame->id       !=     1) return -1;	/* MPEG1      */
	if(frame->layer    !=     1) return -1;	/* LayerIII   */

	/* CRC保護ありなら、CRC保護ビットを読み飛ばします。
	 * 「protection_bit=0:CRC保護あり/1:なし」です。要注意!!
	 */
	if(!frame->protection_bit) mp3_bitstream_skip(bs, 16);

	return 0;
}

/****************************************************************************
 *	グラニュール情報
 ****************************************************************************/

/* グラニュール情報を読み込みます。
 * [in]
 *	bs		ビットストリーム構造体。
 *	granule		グラニュール情報を格納する構造体。
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
 *	サイド情報
 ****************************************************************************/

/* サイド情報を読み込みます。
 * [in]
 *	bs		ビットストリーム構造体。
 *	side		サイド情報を格納する構造体。
 *	channels	チャネル数。
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
 *	スケールファクタ
 ****************************************************************************/

/* スケールファクタを読み込みます。
 * [in]
 *	bs		ビットストリーム構造体。
 *	granule		グラニュール情報構造体。
 *	scale		スケールファクター構造体。(ここにスケールファクターを読み込みます)
 *	scfsi		第1グラニュールの場合、0を渡してください。
 *			第2グラニュールの場合、MP3SIDEINFO.scsfi[ch#]の値を渡してください。
 * [out]
 *	*scale		スケールファクターを読み込み、格納します。
 * [note]
 *	* 第2グラニュールの読み込みでは、一部の値を第1グラニュールと共有する場合があります。
 *	  その場合、変化する部分だけを読み込み、共有部分は読み込みません。
 *	  従って、第1グラニュールで読み込んだスケールファクター構造体を、
 *	  内容を破壊せずに同一チャネルの第2グラニュールのスケールファクター読み込みに使ってください。
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
			/* ミックスブロック */
			out = &scale->long_block[(i = 0)];
			do { *out++ = mp3_bitstream_get(bs, slen1); } while(++i <= 7);	/* long_block[0～7] */
			out = &scale->short_block[(i = 3)][0];
			do { *out++ = mp3_bitstream_get(bs, slen1);
			     *out++ = mp3_bitstream_get(bs, slen1);
			     *out++ = mp3_bitstream_get(bs, slen1); } while(++i <= 5);	/* short_block[3～ 5] */
		} else {
			/* ショートブロック */
			out = &scale->short_block[(i = 0)][0];
			do { *out++ = mp3_bitstream_get(bs, slen1);
			     *out++ = mp3_bitstream_get(bs, slen1);
			     *out++ = mp3_bitstream_get(bs, slen1); } while(++i <= 5);	/* short_block[0～ 5] */
		}
		do { *out++ = mp3_bitstream_get(bs, slen2);
		     *out++ = mp3_bitstream_get(bs, slen2);
		     *out++ = mp3_bitstream_get(bs, slen2); } while(++i <= 11);		/* short_block[6～11] */
		     *out++ = 0;
		     *out++ = 0;
		     *out++ = 0;							/* short_block[   12] */
	} else {
		/* ロングブロック */
		if(!(scfsi & 1 << 3)) { for(i =  0; i <=  5; i++) { scale->long_block[ i] = mp3_bitstream_get(bs, slen1); } };	/* long_block[ 0～ 5] */
		if(!(scfsi & 1 << 2)) { for(i =  6; i <= 10; i++) { scale->long_block[ i] = mp3_bitstream_get(bs, slen1); } };	/* long_block[ 6～10] */
		if(!(scfsi & 1 << 1)) { for(i = 11; i <= 15; i++) { scale->long_block[ i] = mp3_bitstream_get(bs, slen2); } };	/* long_block[11～15] */
		if(!(scfsi & 1 << 0)) { for(i = 16; i <= 20; i++) { scale->long_block[ i] = mp3_bitstream_get(bs, slen2); } };	/* long_block[16～20] */
								    scale->long_block[21] = 0;					/* long_block[    21] */
	}
}

/****************************************************************************
 *	ハフマン復号化
 ****************************************************************************/

/* ハフマン復号化処理。
 * [in]
 *	bs		ビットストリーム構造体。
 *	granule		グラニュール情報構造体。
 *	out		出力バッファ。
 * [out]
 *	戻り値		BigValue領域とCount1領域の合計データ数。
 * [note]
 *	* ビットストリーム構造体のread_bitsを、スケールファクタ読み込み直前のタイミングでリセットしておいてください。
 */
int
mp3_huffman_decode(MP3BITSTREAM* bs, const MP3FRAMEHEADER* frame, const MP3GRANULEINFO* granule, short* out/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/)
{
	int i, count, region1_start, region2_start, big_values_2;

	/* Region0/1/2の境界を取得。 */
	if(granule->block_type == 2) {
		/* ショートブロック(※ミックスブロックも同じ扱い?) */
		region1_start =  36;
		region2_start = 576;
	} else {
		/* ロングブロック */
		region1_start = mp3_scalefactor_band_index_table[frame->sampling_frequency][0/*LongBlock*/][(granule->region0_count + 1)];
		region2_start = mp3_scalefactor_band_index_table[frame->sampling_frequency][0/*LongBlock*/][(granule->region0_count + 1) + (granule->region1_count + 1)];
	}

	/* BigValue領域... */
	big_values_2 = granule->big_values * 2/*{x,y}*/;
	if(region1_start > big_values_2) region1_start = big_values_2; /* 必要 */
	if(region2_start > big_values_2) region2_start = big_values_2; /* 必要 */
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

	/* Count1領域... */
	count = mp3_huffman_decode_count1(bs, granule->count1table_select, MP3_GRANULE_SIZE - i, out, granule->part2_3_length);
	i   += count;
	out += count;
	ASSERT(i <= MP3_GRANULE_SIZE);

	/* BigValue領域とCount1領域の合計データ数を記憶しておきます。 */
	count = i;

	/* rzero領域...
	 * * ロングブロックの場合、完全にrzero領域に含まれるサブバンドは処理していないので、ゼロフィルしなくてもいいです。
	 *   が、複雑さを避けるためと、既に処理速度が間に合っているので、常に最後までゼロフィルすることにしました。
	 *   もし今後、高速化の必要が生じたら、検討してみてください。
	 */
	while(i < MP3_GRANULE_SIZE) {
		*out++ = 0;
		i++;
	}

	/* BigValue領域とCount1領域の合計データ数を返します。 */
	return count;
}

/****************************************************************************
 *	逆量子化
 ****************************************************************************/

/* 逆量子化処理。(ショートブロック)
 * [in]
 *	frame		フレームヘッダ構造体。
 *	granule		グラニュール情報構造体。
 *	scale		スケールファクタ構造体。
 *	in		入力バッファ。
 *	out		出力バッファ。
 * [note]
 *	* スケールファクタスケール(MP3GRANULEINFO.scalefac_scale)は未対応です。
 *	  実際のMP3ファイルではほとんど0になっているようなので、あまり影響はないと思います。
 *	* サブブロックゲイン(MP3GRANULEINFO.subblock_gain[])は未対応です。
 *	  実際のMP3ファイルではほとんど0になっているようなので、あまり影響はないと思います。
 *	* 通常のMP3ファイルではロングブロックが大半を占め、ショートブロックは全体の10%程度です。
 *	  ですから、ショートブロックだけを処理する関数を高速化しても、恩恵は少ないです。
 *	  今後もし高速RAMが足りなくなった場合は、この関数はSRAMに移してしまってもいいと思います。
 *	  ↓
 *	  SRAMへ移しました。
 */
void
mp3_requantize_short(const MP3FRAMEHEADER* frame, const MP3GRANULEINFO* granule, const MP3SCALEFACTOR* scale, const short* in/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, short* out/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/)
{
	int gain, i, j, k, w, index;
	short h;
	const short* sfb_index;
	const short (*sfb_scale)[MP3_SUBBLOCK_COUNT];
	const short* reorder;

	/* スケールファクタバンドの準備。 */
	sfb_index = &mp3_scalefactor_band_index_table[frame->sampling_frequency][1/*ShortBlock*/][1/*次のバンドの開始インデクス*/];
	sfb_scale = scale->short_block;
	index = 0;

	/* サンプル並び替え用の、読み込み側インデクステーブルを取得。 */
	reorder = mp3_reorder_read_index_table[frame->sampling_frequency];

	gain = mp3_gain_table[granule->global_gain];	/* 1.13.18 */
	for(i = 0; i < MP3_SUBBAND_COUNT; i++) {
		for(j = 0; j < MP3_SHORT_IMDCT_SIZE; j++) {
			/* 周波数軸方向に1、時間軸方向に3サンプル(=3サブブロック)逆量子化。
			 * この後、ショートブロックのIMDCTは (周波数軸方向6×時間軸方向1)×時間軸方向3 を単位として処理するので、
			 * ここで [周波数軸方向32][時間軸方向3][周波数軸方向6] の順に並び替えておきます。
			 */
			for(k = 0; k < MP3_SUBBLOCK_COUNT; k++) {
				h = in[*reorder++];			/* 1.15. 0 */
				w = h * mp3_pow1_3_table[abs(h)];	/* 1.21.10 (short×short) */
				w = w * gain;				/* 1. 3.28 (int×int) */
				*out = w >> (14 + (*sfb_scale)[k] / 2);	/* 1. 1.14 (オーバーフロー無視) */
				out += MP3_SHORT_IMDCT_SIZE;
			}
			out -= MP3_SUBBAND_SIZE - 1;

			/* 次のスケールファクタバンドの開始インデクスに到達したら、使用スケールを次へ進めます。 */
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

/* IMDCT処理。(ショートブロック)
 * [in]
 *	in		入力バッファ。
 *	out		出力バッファ。
 *	save		フレーム間で保存する必要のあるバッファ。
 * [note]
 *	* サブバンド合成処理の一部を先取りしています。
 *	* 通常のMP3ファイルではロングブロックが大半を占め、ショートブロックは全体の10%程度です。
 *	  ですから、ショートブロックだけを処理する関数を高速化しても、恩恵は少ないです。
 *	  今後もし高速RAMが足りなくなった場合は、この関数はSRAMに移してしまってもいいと思います。
 *	  ↓
 *	  SRAMへ移しました。
 */
/**************************************************************/

void
mp3_imdct_short(const short* in/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, short* out/*[MP3_SUBBAND_SIZE][MP3_SUBBAND_COUNT]*/, short* save/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/)
{
	int i, j, k, sum;
	const short* m;

	for(k = 0; k < MP3_SUBBAND_COUNT; k++) {
		/* |=Ａ=| |=Ｂ=| |=Ｃ=| |=Ｄ=| |=Ｅ=| |=Ｆ=| */
		/* 000000 ------ ------ ------ ------ 000000 */
		/* ------ ?????? ?????? ------ ------ ------ */
		/* ------ ------ ?????? ?????? ------ ------ */
		/* ------ ------ ------ ?????? ?????? ------ */
		/* |=saveと足して出力=| |====saveに保存====| × MP3_SUBBAND_COUNT */

		/* |=Ａ=| */
		for(i = 0; i < MP3_SHORT_IMDCT_SIZE; i++) {
			sum = *save++;	/* 1.1.14 */
			/* サブバンド合成処理の一部を先取り。奇数行かつ奇数列を符号反転して転置しておく。 */
			if(k & i & 1) sum = -sum;
			*out = sum;	/* 1.1.14 */
			out += MP3_SUBBAND_COUNT;
		}

		/* |=Ｂ=| */
		m = &mp3_imdct_short_matrix[0][0];
		for(i = 0; i < MP3_SHORT_IMDCT_SIZE; i++) {
			sum = 0;
			for(j = 0; j < MP3_SHORT_IMDCT_SIZE; j++) {
				sum += *in++ * *m++;	/* short×short 1.3.28 */
			}
			sum >>= 14;	/* 1.1.14 */
			sum += *save++;	/* 1.1.14 */
			/* サブバンド合成処理の一部を先取り。奇数行かつ奇数列を符号反転して転置しておく。 */
			if(k & i & 1) sum = -sum;
			*out = sum;	/* 1.1.14 */
			out += MP3_SUBBAND_COUNT;
			in  -= MP3_SHORT_IMDCT_SIZE;
		}

		/* |=Ｃ=| */
		for(i = 0; i < MP3_SHORT_IMDCT_SIZE; i++) {
			sum = 0;
			for(j = 0; j < MP3_SHORT_IMDCT_SIZE; j++) {
				sum += *in++ * *m++;	/* short×short 1.3.28 */
			}
			m -= MP3_SHORT_IMDCT_SIZE * MP3_SHORT_IMDCT_SIZE + MP3_SHORT_IMDCT_SIZE;
			for(j = 0; j < MP3_SHORT_IMDCT_SIZE; j++) {
				sum += *in++ * *m++;	/* short×short 1.3.28 */
			}
			sum >>= 14;	/* 1.1.14 */
			sum += *save++;	/* 1.1.14 */
			/* サブバンド合成処理の一部を先取り。奇数行かつ奇数列を符号反転して転置しておく。 */
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

		/* |=Ｄ=| */
		for(i = 0; i < MP3_SHORT_IMDCT_SIZE; i++) {
			sum = 0;
			for(j = 0; j < MP3_SHORT_IMDCT_SIZE; j++) {
				sum += *in++ * *m++;	/* short×short 1.3.28 */
			}
			m -= MP3_SHORT_IMDCT_SIZE * MP3_SHORT_IMDCT_SIZE + MP3_SHORT_IMDCT_SIZE;
			for(j = 0; j < MP3_SHORT_IMDCT_SIZE; j++) {
				sum += *in++ * *m++;	/* short×short 1.3.28 */
			}
			sum >>= 14;	/* 1.1.14 */
			*save++ = sum;	/* 1.1.14 */
			m  += MP3_SHORT_IMDCT_SIZE * MP3_SHORT_IMDCT_SIZE;
			in -= MP3_SHORT_IMDCT_SIZE + MP3_SHORT_IMDCT_SIZE;
		}
		m  -= MP3_SHORT_IMDCT_SIZE * MP3_SHORT_IMDCT_SIZE;
		in += MP3_SHORT_IMDCT_SIZE;

		/* |=Ｅ=| */
		for(i = 0; i < MP3_SHORT_IMDCT_SIZE; i++) {
			sum = 0;
			for(j = 0; j < MP3_SHORT_IMDCT_SIZE; j++) {
				sum += *in++ * *m++;	/* short×short 1.3.28 */
			}
			sum >>= 14;	/* 1.1.14 */
			*save++ = sum;	/* 1.1.14 */
			in -= MP3_SHORT_IMDCT_SIZE;
		}
		in += MP3_SHORT_IMDCT_SIZE;

		/* |=Ｆ=| */
		for(i = 0; i < MP3_SHORT_IMDCT_SIZE; i++) {
			*save++ = 0;	/* 1.1.14 */
		}
	}
}

/********************************************************************/


/****************************************************************************
 *	MP3ドライバ
 ****************************************************************************/

#define MP3BUFLEN	(64 * 5)

int
mp3_init(MP3DRIVER* mp3, const void* data, int len)
{
	/* まずゼロクリア。 */
	_memset(mp3, 0, sizeof(MP3DRIVER));

	/* 読み込み位置と終端位置を格納します。 */
	mp3->pos = data;
	mp3->end = mp3->pos + len;

	/* サンプリング周波数変換DDAの準備。 */
	mp3->e_fbuff = -SPEAKER_FREQUENCY;

	/* 最初のフレームをデコードし、バッファを満たします。 */
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

/* 1フレームデコードします。
 * [in]
 *	mp3			MP3ドライバ構造体。
 * [out]
 *	戻り値			成功なら0を返します。
 *				失敗なら0以外の値を返します。(終端に達した場合など)
 *	mp3->fbuff		デコードしたフレームデータを格納します。
 *	mp3->sampling_frequency	デコードしたフレームのサンプリング周波数を格納します。
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

	/* フレーム間で保存する必要のないバッファ。 */
	/* SRAMに取る場合。 */
	static short buf1[MP3_GRANULE_SIZE];
	static short buf2[MP3_GRANULE_SIZE];

	/* フレーム開始位置を検索します。 */
	for(;;) {
		/* データ終端? */
		if(mp3->pos >= mp3->end - 1) return -1;
		/* 同期ワード? */
		if(mp3->pos[0] == 0xff &&
		  (mp3->pos[1] &  0xf0) == 0xf0) break;
		mp3->pos++;
	}

	/* ビットストリームを開きます。 */
	mp3_bitstream_open(&bs, mp3->pos);

	/* フレームヘッダを読み込みます。 */
	if(mp3_read_frame_header(&bs, &frame) != 0) return -1/*フレームヘッダが不正*/;
	bitrate = mp3_bitrate_table[frame.bitrate_index];
	sampling_frequency = mp3_sampling_frequency_table[frame.sampling_frequency];
	channels = frame.mode == 3 ? 1 : 2;
	frame_size = 144 * bitrate / sampling_frequency + frame.padding_bit; /* フレームサイズ計算 */

	/* サイド情報を読み込みます。 */
	mp3_read_side_info(&bs, &side, channels);

	/* メインデータのビット蓄積。 */
	ASSERT(bs.cnt == 0); /* サイド情報の末尾でバイト境界に整列しているはず */
	main_data_size = frame_size - (bs.pos - mp3->pos)/*フレームヘッダとサイド情報を引いた残りがメインデータ*/;
	if(mp3->main_save_size + main_data_size > sizeof mp3->main_save) {
		_memmove(&mp3->main_save[0], &mp3->main_save[mp3->main_save_size - side.main_data_begin], side.main_data_begin);
		mp3->main_save_size = side.main_data_begin;
	}
	_memcpy(&mp3->main_save[mp3->main_save_size], bs.pos, main_data_size);
	mp3_bitstream_open(&bs, &mp3->main_save[mp3->main_save_size - side.main_data_begin]);
	mp3->main_save_size += main_data_size;

	/* サンプリング周波数変換テーブルを取得します。 */
	convert = &mp3_sampling_frequency_convert_table[frame.sampling_frequency];
//		static MP3SAMPLINGFREQUENCYCONVERTTABLE _convert;
//		unsigned char _table[32];
//		int i;
	for(i = 0; i < 32; i++) table[i] = i;
	_convert.table = table;
	_convert.count = 32;
	_convert.sampling_frequency = mp3_sampling_frequency_table[frame.sampling_frequency];
	convert = &_convert;

	/* 各グラニュールをデコードします。 */
	out = mp3->fbuff;
	for(i_granule = 0; i_granule < MP3_GRANULE_COUNT; i_granule++) {
		for(i_channel = 0; i_channel < channels; i_channel++) {
			granule = &side.granule_info[i_granule][i_channel];
			if(i_channel == 0) { /* 左チャネルのみ使用 */
				bs_tmp = bs;
				/* ハフマン復号化のCount1領域終端検出のため、読み込んだビット数をリセットしておきます。
				 * スケールファクタ(Part2)+ハフマン符号(Part3)の合計ビット数がGRANULE.part2_3_lengthとなります。
				 */
				bs_tmp.read_bits = 0;
				/* Part2: スケールファクタ読み込み。 */
				mp3_read_scalefactor(&bs_tmp, granule, &scale, !i_granule ? 0 : side.scfsi[i_channel]);
				/* Part3: ハフマン復号化。 */
				count = mp3_huffman_decode(&bs_tmp, &frame, granule, buf1);
				if(granule->block_type == 2) {
					if(granule->mixed_block_flag) {
						/* ミックスブロック */
						/* ※TODO: ミックスブロックを生成するエンコーダがまだ存在しないらしいので、とりあえず必要ないようです。 */
						_memset(buf1, 0, sizeof buf1);
					} else {
						/* ショートブロック */
						/* 逆量子化。 */
						mp3_requantize_short(&frame, granule, &scale, buf1, buf2);
						/* ショートブロックにはもともとエリアシング削減処理はありません。 */
						/* IMDCT。 */
						mp3_imdct_short(buf2, buf1, &mp3->imdct_save[0]);
					}
				} else {
					/* BigValue/Count1領域のデータを含むサブバンド数を求めます。 */
					subband_count = (count + (MP3_SUBBAND_COUNT - 1)) / MP3_SUBBAND_COUNT;
					/* ロングブロック */
					/* 逆量子化。 */
					mp3_requantize_long(&frame, granule, &scale, buf1, buf2, count);
					/* エリアシング削減。 */
					mp3_antialias(buf2, subband_count);
					/* IMDCT。 */
					mp3_imdct_long(buf2, buf1, &mp3->imdct_save[0], subband_count);
				}
				/* サブバンド合成。 */
				mp3->subband_synthesys_offset = mp3_subband_synthesys(buf1, out, &mp3->subband_synthesys_save[0][0], mp3->subband_synthesys_offset, convert);
				out += convert->count * MP3_SUBBAND_SIZE; /* サンプリング周波数変換後のグラニュール当りサンプル数 */
			}
			mp3_bitstream_skip(&bs, granule->part2_3_length);
		}
	}

	/* サンプリング周波数とサンプル数を格納します。 */
	mp3->sampling_frequency = convert->sampling_frequency;
	mp3->c_fbuff = convert->count * MP3_SUBBAND_SIZE * MP3_GRANULE_COUNT; /* サンプリング周波数変換後のフレーム当りサンプル数 */

	/* フレーム位置を進めます。 */
	mp3->pos += frame_size;

	return 0;
}

/****************************************************************************
 *	アプリケーション用関数
 ****************************************************************************/

int
mp3_play(const void* data, int len)
{
	static MP3DRIVER mp3; /* STATICです! */

	/* まず確実に停止します。 */
	mp3_stop();

	/* MP3ドライバを初期化します。 */
	if(mp3_init(&mp3, data, len) != 0) return -1;

	/* ストリーム再生を開始します。 */
//	stream_play(MP3BUFLEN, mp3_stream_callback, (int)&mp3, 0);

	return 0;
}

void
mp3_stop()
{
	/* ストリーム再生を停止します。 */
//	stream_stop();

	/* MP3ドライバのクリーンアップは不要です。 */
}

