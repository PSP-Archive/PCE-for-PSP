PCE for PSP 0.7s

■どんなん？
ソース。糞汚いです。
先日、psptoolchain仕様にしたばかりなので、コンパイル面倒かも。エラーでまくるしな！
圧縮アルゴリズムは仕事で使ったことあったりするのでオブジェクトファイルで＞ｗ＜
ガンヘッド動かせるようにしてくれる神募集


■インストール
/PSP/GAME/の下に適当にフォルダ作ってEBOOT.PBPを入れてください

■CD-ROM^2
CD単位で適当にフォルダ作ってファイル放り込みます
必要なファイル
	半角二文字のドライブ番号.mp3
		曲ファイル。全てのmp3フォーマットは対応されていないと思われ
		128kだとプチプチするかも
		出力はモノラル限定です
		私はEACで取り込み、LAME96kでエンコードしてチェックしています
	半角二文字のドライブ番号.iso
		データートラック
		私はDiscJuggler体験版で取り込みしてみました
	適当な名前.toc
		テキストでディスク情報を入力しておきます
		某TOCサイトのデーターそのままコピペで問題ないのかもしれない
		例：超兄貴

Track 01 Audio 00:02:00 LBA=000000 
Track 02 Data 00:49:65 LBA=003590 
Track 03 Audio 01:34:00 LBA=006900 
Track 04 Audio 02:13:64 LBA=009889 
Track 05 Audio 02:49:39 LBA=012564 
Track 06 Audio 03:31:17 LBA=015692 
Track 07 Audio 04:16:00 LBA=019050 
Track 08 Audio 04:53:05 LBA=021830 
Track 09 Audio 05:30:03 LBA=024603 
Track 10 Audio 06:07:50 LBA=027425 
Track 11 Audio 06:46:72 LBA=030372 
Track 12 Audio 07:49:15 LBA=035040 
Track 13 Audio 08:54:17 LBA=039917 
Track 14 Audio 09:54:66 LBA=044466 
Track 15 Audio 11:07:60 LBA=049935 
Track 16 Audio 12:14:09 LBA=054909 
Track 17 Audio 13:14:65 LBA=059465 
Track 18 Audio 13:55:07 LBA=062482 
Track 19 Audio 14:57:11 LBA=067136 
Track 20 Audio 16:03:14 LBA=072089 
Track 21 Audio 16:59:50 LBA=076325 
Track 22 Audio 19:02:00 LBA=085500 
Track 23 Audio 19:09:61 LBA=086086 
Track 24 Audio 19:21:39 LBA=086964 
Track 25 Audio 21:00:68 LBA=094418 
Track 26 Audio 22:02:23 LBA=099023 
Track 27 Audio 22:32:51 LBA=101301 
Track 28 Audio 24:36:58 LBA=110608 

		こんな感じで

一桁台の番号は0を入れておいてください。例 01.mp3 02.iso 03.mp3
んで、メニューのcd changeでtocファイルを選択して、システムカードを実行してみてください
運がよければ動きます
当方の環境だと。スーパーダライアスはタイトルまで。天外魔境２は起動不可
超兄貴とドラキュラXはある程度動くのを確認しました

■使い方
メニュー
	十字	上下で移動、左右で設定
	○	決定
ファイルセレクト
	十字	移動
	○	決定・フォルダ進む
	△	フォルダ戻る
	×	ファイル選ばないで戻る
ゲーム
	Ｌ	コンフィグメニュー
	×	Ｉ
	○	ＩＩ
	□	Ｉ連打
	△	ＩＩ連打
	SELECT	セレクト
	START	スタート

■補足
ペースはXPCEです、CD部はHuEです。

■履歴
0.1	とりあえず動いた
0.2	軽くなった
0.3	メニュー付いた
0.4	セーブとか
	HOMEでも３０秒ほど待てば戻れる予感もするね
	ZIP対応
0.41	ボタン逆だったかｗｗｗ
	セーブファイルの圧縮。旧ファイルは消さないと小さくならない
	クイックセーブはメモステと別
	*.pce *.zip 以外は読み込まないようにした
0.42	まだ逆だった、疲れてるな
	ZIPで圧縮されてる壁紙読み込み
	画面拡大表示
0.5	細かい調整、バグ取り
	セーブファイル名がバグってたｗｗｗついでにSAVEフォルダ勝手に作ってみたり
	ボカシ重めぇーよ
0.51	セーブファイルまだバグってるし
0.52	無圧縮ゲームファイルでアレか
0.53	細かい修正
0.6	333Mhz。サムネ
0.61	状態セーブが上手くいっていなかったので修正
	嫌がらせアイコン
	ファイルメニュー操作を他のエミュにあわせた
0.62	0.6から動かなくなったソフトがまた動くように
	連打速度調整機能
0.7	CD-ROM^2とりあえず動いた
	スト2とか動くかも
	WRAMファイル保存
	セーブファイルのフォーマット変更

■謝辞
GB File Selectorに付いてきたソースをそのままぱくってます
ファイルセレクトとかそのままです・・・すみません
その他もろもろサンプルソースをあちこちからいただいています
ruka氏のUnzip library forPSP beta使わせていただきました
名高忍さんが公開されてるmp3デコーダーを使わせていただきました

■フォントについて
このアプリには
http://hp.vector.co.jp/authors/VA013391/fonts/
で配布されているビットマップフォントである「ナガ10」が組み込まれています。

■HomePage
http://osakanapsp.hp.infoseek.co.jp/

