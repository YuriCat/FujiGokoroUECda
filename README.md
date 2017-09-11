# コンピュータ大貧民プログラム FujiGokoro (藤心)

UECコンピュータ大貧民大会（UECda）ルール・プロトコルに準拠した大富豪（大貧民）の思考プログラムです。

Daifugo Program for UEC Computer Daihinmin Contest (UECda) 

UECda2014, 2015, 2016 無差別級で優勝しています。

2016/5/19深夜放送の日本テレビ系「変ラボ」にてタレントの手越祐也さんと対決させていただきました。

## 使用方法

make release -j4

でビルドし、

./out/release/server -g 100 &

./out/release/client &  
./out/release/client &  
./out/release/client &  
./out/release/client &  
./out/release/client &  

のように5体の自己対戦を行うことができます。

./out/release/server -g 100 -bc -l tmp.dat

としてサーバーを起動すると試合内容をコンソールで表示し、tmp.datに棋譜を保存することができます。

## 注意

過去に大会出場したバージョンは

http://www.tnlab.inf.uec.ac.jp/daihinmin/2017/downloads.html

からダウンロードできます。

こちらは開発版なので、弱かったり大会の時間制限を満たしていないことがあります。

パラメータファイルは未アップです。

## ルール等 公式のドキュメント

http://www.tnlab.inf.uec.ac.jp/daihinmin/2017/document.html

## 今年度の大会

http://www.tnlab.inf.uec.ac.jp/daihinmin/2017/

今年度の大会は2017/11/25(土)に開催される予定だそうです。

大会では基本的に公開されている過去の出場プログラムを自由に改変して出場可能のはずですが、

特に私の製作プログラムについては、こちらの開発版コードの利用しての出場も許可します。

（出場申し込みの際に、コード利用の由を運営方に伝えたほうがよいかとは思います）

ただし、大会出場以外での利用は要相談でお願い致します。
