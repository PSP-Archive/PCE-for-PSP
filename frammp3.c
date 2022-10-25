/*	
 *	frammp3.c
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
 *	* Wed Apr 14 12:30:00 JST 2004 Naoyuki Sawa
 *	- mp3_bitstream_get()で、24bit以上のビット列を読み込む処理が間違っていたのを修正。
 *	  ただし実際には24bit以上のビット列を読むことはないので、これまでのプログラムでも問題は発現しません。
 */
//#include "clip.h"
#include "clipmp3.h"

/****************************************************************************
 *	ビットストリーム
 ****************************************************************************/

/* ビットストリームからビット列を読み込みます。
 * [in]
 *	bs		ビットストリーム構造体。
 *	cnt		読み込むビット数。(0～32)
 * [out]
 *	戻り値		読み込んだビット列。
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

/* ビットストリームから1ビット読み込みます。
 * [in]
 *	bs		ビットストリーム構造体。
 * [out]
 *	戻り値		読み込んだビット。
 * [note]
 *	* ハフマン復号化処理で1ビット読み込みを多用するので、特別に高速化しました。
 *	  mp3_huffman_decode_bigvalue/count1()の最内周ループでのレジスタ退避・復元が省けるため、
 *	  ハフマン復号化処理が2.0～2.5倍も高速化できます。
 */
#define mp3_bitstream_get1(bs)									\
		((bs)->read_bits++,								\
		((bs)->cnt ? (((bs)->buf               ) >> ((bs)->cnt = (bs)->cnt - 1) & 1)	\
		           : (((bs)->buf = *(bs)->pos++) >> ((bs)->cnt =         8 - 1) & 1)))

/****************************************************************************
 *	ハフマン復号化
 ****************************************************************************/

/* BigValue領域の単一Regionを復号するサブルーチンです。
 * [in]
 *	bs			ビットストリーム構造体。
 *	i_table			復号テーブル番号。
 *	count			復号サンプル数。
 *	out			出力バッファ。
 * [note]
 *	* メモリ節約のために、サブルーチンに分けました。
 *	  もしも、mp3_huffman_decode()の中で3回コード展開すると、約300バイト消費してしまいます。
 *	* この関数は1グラニュール当り3回呼び出されるだけなので、関数呼び出しによる速度低下は少ないと思います。
 */
void
mp3_huffman_decode_bigvalue(MP3BITSTREAM* bs, int i_table, int count, short* out)
{
	int linbits, i_node, x, y;
	const short (*table)[2];

	/* 復号テーブル取得。 */
	table   = mp3_huffman_bigvalue_table[i_table].table;
	linbits = mp3_huffman_bigvalue_table[i_table].linbits;

	/* 復号処理。 */
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
		/* ゼロ固定テーブル。(SCMPXは生成しませんが、LAMEは生成します) */
		while(count) {
			*out++ = 0;
			*out++ = 0;
			count -= 2/*{x,y}*/;
		}
	}
}

/* Count1領域を復号するサブルーチンです。
 * [in]
 *	bs			ビットストリーム構造体。
 *	i_table			復号テーブル番号。
 *	count_limit		最大出力サンプル数。
 *	out			出力バッファ。
 *	part2_3_length		GRANULE.part2_3_lengthを渡してください。
 * [out]
 *	戻り値			出力サンプル数。
 * [note]
 *	* ビットストリーム構造体のread_bitsを、スケールファクタ読み込み直前のタイミングでリセットしておいてください。
 *	* 本当は、BigValue領域とCount1領域を合わせてきちんとMP3_GRANULE_SIZE(576)サンプル以内に収まるはずなのですが、
 *	  デコーダによってはオーバーランしてしまうことがあるようです。
 *	  そのため、最大出力サンプル数によるリミッタが必要です。(既存のMP3デコーダもそうしている)
 *	  SCMPXでエンコードしたMP3ファイルでも、オーバーランしました。もしかしたらこちらのバグかも知れませんが・・・
 */
int
mp3_huffman_decode_count1(MP3BITSTREAM* bs, int i_table, int count_limit, short* out, int part2_3_length)
{
	int i_node, v, w, x, y;
	const short (*table)[2];
	short *out0, *out_limit;

	out0 = out;
	out_limit = out + count_limit - 4/*{v,w,x,y}*/;

	/* 復号テーブル取得。 */
	table = mp3_huffman_count1_table[i_table].table;

	/* 復号処理。 */
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
 *	逆量子化
 ****************************************************************************/

/* * MP3仕様に定められた、逆量子化処理の計算は次の通りです。
 *
 *	xr ＝ is × |is|^(1/3) × 2^(Gain-Scale)
 *
 *	is: 逆量子化処理への入力データ
 *	xr: 逆量子化処理からの出力結果
 *
 *   ただし、
 *
 *	Gain  ＝ (GlobalGain-210)/4 - SubblockGain
 *	Scale ＝ ((ScalefactorScale+1)×(Scalefactor+Preemphasis)) / 2
 *
 *   SubblockGain、ScalefactorScale、Preemphaisの処理を省略して0と仮定すると、
 *
 *	Gain  ＝ (GlobalGain-210) / 4
 *	Scale ＝ Scalefactor / 2
 *
 *   まとめると、整数演算では次のように処理できます。
 *
 *	xr ＝ is × |is|^(1/3) × 2^((GlobalGain-210)/4 - Scalefactor/2)
 *	   ＝ is × |is|^(1/3) × 2^((GlobalGain-210)/4) ÷ 2^(Scalefactor/2)
 *	   ＝ is × |is|^(1/3) × 2^((GlobalGain-210)/4) >> (Scalefactor/2)
 *	            ~~~~~~~~~~    ~~~~~~~~~~~~~~~~~~~~~~
 *	            テーブル化          テーブル化      
 */

/* 逆量子化処理。(ロングブロック)
 * [in]
 *	frame		フレームヘッダ構造体。
 *	granule		グラニュール情報構造体。
 *	scale		スケールファクタ構造体。
 *	in		入力バッファ。
 *	out		出力バッファ。
 *	count		BigValue領域とCount1領域のデータ数合計。
 * [note]
 *	* スケールファクタスケール(MP3GRANULEINFO.scalefac_scale)は未対応です。
 *	  実際のMP3ファイルではほとんど0になっているようなので、あまり影響はないと思います。
 *	* 高速化のために、BigValue領域とCount1領域のデータのみ処理します。
 *	  rzero領域のデータは処理しません。
 */
void
mp3_requantize_long(const MP3FRAMEHEADER* frame, const MP3GRANULEINFO* granule, const MP3SCALEFACTOR* scale, const short* in/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, short* out/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, int count)
{
	int gain, i, w, index;
	short h;
	const short* sfb_index;
	const short* sfb_scale;

	/* スケールファクタバンドの準備。 */
	sfb_index = &mp3_scalefactor_band_index_table[frame->sampling_frequency][0/*LongBlock*/][1/*次のバンドの開始インデクス*/];
	sfb_scale = scale->long_block;
	index = 0;

	gain = mp3_gain_table[granule->global_gain];	/* 1.13.18 */
	for(i = 0; i < count; i++) {
		/* 周波数軸方向に1サンプル逆量子化。 */
		h = *in++;				/* 1.15. 0 */
		w = h * mp3_pow1_3_table[abs(h)];	/* 1.21.10 (short×short) */
		w = w * gain;				/* 1. 3.28 (int×int) */
		*out++ = w >> (14 + *sfb_scale / 2);	/* 1. 1.14 (オーバーフロー無視) */

		/* 次のスケールファクタバンドの開始インデクスに到達したら、使用スケールを次へ進めます。 */
		index++;
		if(index == *sfb_index) {
			sfb_index++;
			sfb_scale++;
		}
	}
}

/****************************************************************************
 *	エリアシング削減
 ****************************************************************************/

/* * 少し処理をはしょっています。例えば、有意なサブバンドが3つの場合、
 *
 *	○本当はこうしなければいけない
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
 *	+-------------+  |バタフライ演算
 *	|             |<-+
 *	|# 2          |
 *	|             |<-+
 *	+-------------+  |バタフライ演算
 *	|             |<-+
 *	|# 1          |
 *	|             |<-+
 *	+-------------+  |バタフライ演算
 *	|             |<-+
 *	|# 0          |
 *	|             |
 *	+-------------+
 *
 *   本当は、サブバンド#2と#3間のバタフライ演算も行わなければいけません。
 *   が、現在の実装では、最後のバタフライ演算を行っていません。
 *
 *	○現状はこうなっている
 *	+-------------+
 *	|             |
 *	|#31(  ゴミ  )|
 *	|             |
 *	+-------------+
 *	| .           |
 *	| . (  ゴミ  )|
 *	| .           |
 *	+-------------+
 *	|             |
 *	|# 3(  ゴミ  )|
 *	|             |
 *	+-------------+
 *	|             |
 *	|# 2          |
 *	|             |<-+
 *	+-------------+  |バタフライ演算
 *	|             |<-+
 *	|# 1          |
 *	|             |<-+
 *	+-------------+  |バタフライ演算
 *	|             |<-+
 *	|# 0          |
 *	|             |
 *	+-------------+
 *
 *   なぜなら、サブバンド#3～31に対しては逆量子化処理での出力を行っておらず、ゴミが入っているからです。
 *   サブバンド#2と#3間のバタフライ演算のみ、#3をall zeroと見なして特別処理することもできます。
 *
 *	○改良案（未採用）
 *	+-------------+
 *	|             |
 *	|#31(  ゴミ  )|
 *	|             |
 *	+-------------+
 *	| .           |
 *	| . (  ゴミ  )|
 *	| .           |
 *	+-------------+
 *	|             |
 *	|# 3(  ゴミ  )|
 *	|             |
 *	+-------------+  |#3をall zeroと見なして特別処理
 *	|             |<-+
 *	|# 2          |
 *	|             |<-+
 *	+-------------+  |バタフライ演算
 *	|             |<-+
 *	|# 1          |
 *	|             |<-+
 *	+-------------+  |バタフライ演算
 *	|             |<-+
 *	|# 0          |
 *	|             |
 *	+-------------+
 *
 *   が、今回はそうはしませんでした。
 *   有意なデータのうちいちばん高音域の部分なので、少し違っていてもあまり影響はないと判断したからです。
 */

/* エリアシング削減処理。(ロングブロックのみ)
 * [in]
 *	in_out		入出力バッファ
 *	subband_count	
 *	subband_count	BigValue領域またはCount1領域のデータを少なくとも一部含むサブバンドの数。
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
			*pa   = (a * cs - b * ca) >> 14;	/* short×short 1.3.28 => 1.1.14 */
			*pb++ = (b * cs + a * ca) >> 14;	/* short×short 1.3.28 => 1.1.14 */
			pb++;
		}
		table -= 2 * 8;
	}
}


/****************************************************************************
 *	IMDCT
 ****************************************************************************/

/* IMDCT処理。(ロングブロック)
 * [in]
 *	in		入力バッファ。
 *	out		出力バッファ。
 *	save		フレーム間で保存する必要のあるバッファ。
 *	subband_count	BigValue領域またはCount1領域のデータを少なくとも一部含むサブバンドの数。
 * [note]
 *	* サブバンド合成処理の一部を先取りしています。
 *	* 高速化のために、BigValue領域とCount1領域のデータのみ処理します。
 *	  rzero領域のデータは処理しません。
 */
void
mp3_imdct_long(const short* in/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, short* out/*[MP3_SUBBAND_SIZE][MP3_SUBBAND_COUNT]*/, short* save/*[MP3_SUBBAND_COUNT][MP3_SUBBAND_SIZE]*/, int subband_count)
{
	int i, j, k, sum;
	const short* m;

	k = 0;

	/* BigValue/Count1領域 */
	while(k < subband_count) {
		m = &mp3_imdct_long_matrix[0][0];

		/* 「前のグラニュールの後半」と「現在のグラニュールの前半」を合成して出力。 */
		for(i = 0; i < MP3_SUBBAND_SIZE; i++) {
			sum = 0;
			for(j = 0; j < MP3_SUBBAND_SIZE; j++) {
				sum += *in++ * *m++;	/* short×short 1.3.28 */
			}
			sum >>= 14;	/* 1.1.14 */
			sum += *save++;	/* 1.1.14 */
			/* サブバンド合成処理の一部を先取り。奇数行かつ奇数列を符号反転して転置しておく。 */
			if(k & i & 1) sum = -sum;
			*out = sum;	/* 1.1.14 */
			out += MP3_SUBBAND_COUNT;
			in  -= MP3_SUBBAND_SIZE;
		}
		out  -= MP3_GRANULE_SIZE - 1;
		save -= MP3_SUBBAND_SIZE;

		/* 「次のグラニュールの前半」に合成するために「現在のグラニュールの後半」を保存。 */
		for(i = 0; i < MP3_SUBBAND_SIZE; i++) {
			sum = 0;
			for(j = 0; j < MP3_SUBBAND_SIZE; j++) {
				sum += *in++ * *m++;	/* short×short 1.3.28 */
			}
			sum >>= 14;	/* 1.1.14 */
			*save++ = sum;	/* 1.1.14 */
			in -= MP3_SUBBAND_SIZE;
		}
		in += MP3_SUBBAND_SIZE;

		k++;
	}

	/* rzero領域 */
	while(k < MP3_SUBBAND_COUNT) {
		/* 「前のグラニュールの後半」と「現在のグラニュールの前半」を合成して出力。 */
		for(i = 0; i < MP3_SUBBAND_SIZE; i++) {
			sum = *save++;	/* 1.1.14 */
			/* サブバンド合成処理の一部を先取り。奇数行かつ奇数列を符号反転して転置しておく。 */
			if(k & i & 1) sum = -sum;
			*out = sum;	/* 1.1.14 */
			out += MP3_SUBBAND_COUNT;
		}
		out  -= MP3_GRANULE_SIZE - 1;
		save -= MP3_SUBBAND_SIZE;

		/* 「次のグラニュールの前半」に合成するために「現在のグラニュールの後半」を保存。 */
		for(i = 0; i < MP3_SUBBAND_SIZE; i++) {
			*save++ = 0;	/* 1.1.14 */
		}

		k++;
	}
}


/****************************************************************************
 *	サブバンド合成
 ****************************************************************************/

/* サブバンド合成処理。
 * [in]
 *	in		入力バッファ。
 *	out		出力バッファ。
 *	save		フレーム間で保存する必要のあるバッファ。
 *	offset		saveオフセット。
 *			前回のmp3_subband_synthesys()の戻り値を渡してください。
 *	convert		サンプリング周波数変換テーブル。
 * [out]
 *	戻り値		次回のsaveオフセット。
 * [note]
 *	* サブバンド合成処理の一部はmp3_imdct()で先に行っています。
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
		/* 畳み込み演算。(32データを時間領域に変換)
		 * 入力行列の奇数行かつ奇数列を符号反転して転置する処理は、IMDCTの中で行っています。
		 */
		for(i = 0; i < count; i++) {
			/* 前半 */
			m = mp3_subband_matrix[*table++];
			sum = 0;
			for(j = 0; j < MP3_SUBBAND_COUNT; j++) {
				sum += *in++ * *m++;	/* 1.3.28 */
			}
			save[offset] = sum >> 14;	/* 1.1.14 */
			offset += MP3_SUBBAND_COUNT;
			in -= MP3_SUBBAND_COUNT;
			/* 後半 */
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

		/* 16組のデータの重ね合わせ。
		 * ポリフェーズフィルターバンク係数表は、あらかじめ転置してあります。
		 */
		for(i = 0; i < count; i++) {
			m = mp3_polyphase_filter[*table++];
			sum = 0;
			for(j = 0; j < 16 / 2/*2サンプルづつ処理*/; j++) {
				/* 前半 */
				sum += save[offset] * *m++;	/* 1.3.28 */
				offset += MP3_SUBBAND_COUNT_2 + MP3_SUBBAND_COUNT;
				offset &= OFFSET_MASK;
				/* 後半 */
				sum += save[offset] * *m++;	/* 1.3.28 */
				offset += MP3_SUBBAND_COUNT_2 - MP3_SUBBAND_COUNT;
				offset &= OFFSET_MASK;
			}
			offset++;
			/* -0x7fff～0x7fffにスケーリング。 */
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

