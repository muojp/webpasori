# webpasori

## これは何なのか

ソニー製FeliCaリーダ・ライタのPaSoRiをLinuxやmacOSなどの環境から利用できるライブラリであるlibpasori(実際にはその拡張版であるlibpafe)をAndroidやmacOS上のChromeブラウザ内で動作するように移植したものです。

これにより、libpasori(libpafe)を基盤としてFeliCaを読み書きするプログラムを簡単にChromeブラウザ上で動作させられます。

実際のソースコード互換移植例として、

 * libpafeに含まれるPaSoRiテストツール(pasori_test)
 * 同、FeliCaデータのダンプツール(felica_dump)
 * libpafeベースのSuica履歴ダンプツールであるsuica-dump

をWeb用に変換したものを用意しました。

### 動作例

 * Lenovo Phab 2 Pro(Android 6.0) + RC-S330でfelica_dumpサンプルを利用してEdyの公開領域を読み取ったところ

![phab-2-pro-edy](https://muojp.github.io/webpasori/webpasori-demo.jpg)

## 動作環境

 * PaSoRi RC-S330
   * 今のところ、RC-S310/320はサポートしていません
 * AndroidまたはmacOS版のGoogle Chrome 63+
   * Windows環境ではデバイスを認識しないようです

## 簡単な動作確認方法

 * MacもしくはAndroid端末(6.0以上を推奨)を用意する
 * PaSoRi RC-S330を上記環境に接続する
 * Chrome 63以上でデモページを開く
   * PaSoRiの認識とバージョン確認([元リポジトリ](https://github.com/rfujita/libpafe/) by @rfujita and @htrb)
     * https://muojp.github.io/webpasori/pasori_test.html
   * FeliCaのデータダンプ([元リポジトリ](https://github.com/rfujita/libpafe/) by @rfujita and @htrb)
     * https://muojp.github.io/webpasori/felica_dump.html
   * Suicaの履歴ダンプ([元リポジトリ](https://github.com/temmings/suica-dump/) by @temmings)
     * https://muojp.github.io/webpasori/suica_dump.html
 * ※注意: 上記デモページ群では、端末に接続されたPaSoRiやそれを経由して読み取ったFeliCaに関する情報を一切外部に送信していませんが、原理上は容易に送信可能であることを認識した上で読み取り確認をおこなってください

## 移植の目的

 * 個人的に余らせていたPaSoRiを手軽な履歴取り込み手段として復活させること
 * libusbを基盤とするライブラリは、libusbとUSBの仕様を一定理解していれば小規模なコード変更でWebへ移植できると示す(確認する)こと

WebUSBは、USBデバイスの新たな活用場面として大きな可能性を持つものです。
現状のChrome実装(64時点)にはWindows上でのデバイス認識や利用上の問題がある([Chromium issue #637404](https://bugs.chromium.org/p/chromium/issues/detail?id=637404)参照)ものの、既にmacOS(10.10+程度)とAndroid(6.0+程度)では同一のJavaScriptコードでデバイスを制御できる状態にあります。

個人的な感覚ですが、特にAndroid端末とUSB-OTGケーブルを組み合わせることで

 * ターゲットデバイスの現役時代にはAndroidとの組み合わせが存在しなかったもの(過去資産活用)
 * 新規デバイスドライバの開発時にWeb上での動作サポートを選択肢に入れる価値があるもの(クロスプラットフォームドライバとしての活用)

という2つのエリアで活用の可能性を感じます。

libusbを利用してクロスプラットフォームのソースコード互換を実現しているドライバの中には、Windows環境サポートのためにlibusb+WinUSBバックエンド体制を取っているものもあります(まさにChromium/Chromeがこう変わりつつあります)。
これらは、デバイス検出まわりで一定の改変が必要な可能性があるものの比較的容易にWebへと持ち出せるのではないか、と期待しています。

## ビルドについて

### ビルド要件

 * Emscripten(1.37.28で確認済)
 * CMake(3.5.1で確認済)
 * GNU MakeやClang一式

Ubuntu16.04にbuild-essentialとclang、cmakeパッケージを入れた状態にEmscripten環境を構築したものに類似するWSL上の環境でビルド確認をおこないました。

### ビルド手順

```
$ git clone --recursive https://github.com/muojp/webpasori.git
$ cd webpasori
$ mkdir build
$ cd build
$ emcmake cmake ..
$ make
```

### 得られる成果物

```
.
├── felica_dump.html
├── felica_dump.js
├── libpafe-emscripten
│   └── libpafe.a
├── pasori_test.html
├── pasori_test.js
├── suica_dump.html
└── suica_dump.js
```

## トラブルシューティング

### ビルドに失敗する

submoduleをcloneしていない場合(--recursiveをつけずにcloneした場合など)にはビルドが失敗します。

```
$ git submodule init
$ git submodule update
```

を実行してください。

### ローカルホストでは実行できるがLAN内環境で実行できない

WebUSBはそのパワフルさと潜在的な危険性のため、利用にさまざまな制限が課せられています。
代表的なものが

 * ローカルホスト以外でのHTTPS必須
 * ユーザアクションをきっかけとしたデバイス接続必須

です。

LAN内の環境で実行する場合、手元環境にてHTTPS待ち受けが必要です。
このとき、443番ポートの利用は必須ではありません。
また、証明書エラーが出る状態でも強制的に続行すれば動作します。

詳しくは[Access USB Devices on the Web](https://developers.google.com/web/updates/2016/03/access-usb-devices-on-the-web)を参照してください。

開発が落ち着いてきたらGitHub PagesやAmazon S3、Azure Storage (BLOB)などへhtml/jsファイルをアップロードして動作確認するのが楽です。

## 移植方針/移植状況

 * なるべく外部リポジトリのコードに手を加えずにEmscriptenビルドを通したいところでしたが、libpafeの構造上本家へのパッチ無しにこれを実現するのは難しいと判断しました
 * 手持ちのPaSoRiがRC-S330のみであったため、S310/320はサポートしていません。具体的には、該当するデバイス依存コードを移植していないので、現状ではS310/320で動作しません
