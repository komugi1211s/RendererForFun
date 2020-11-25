# Renderer for fun (Tiny Renderer Implementation)
ソフトウェアレンダラーの仕組みの学習、兼遊びの一環として最小限のライブラリを用いて書かれたレンダラー。<br>
このレンダラはGithubで公開されているコースの[TinyRenderer - software rendering in 500 lines of code](https://github.com/ssloy/tinyrenderer) を元に作られています。<br>
基本的にコードはこのコースに倣って書かれています。<br>

### 現状の外部依存:
- `stb_truetype.h`: 文字のレンダリングに使用しています。
- `stb_image.h`:    テクスチャのロードに使用しています。

# Build
*Linuxの場合*
コンパイラは必要に応じて`build.sh`内を書き換えて下さい。
```
git clone https://github.com/komugi1211s/RendererForFun.git
cd RendererForFun
./build.sh
```
*Windowsの場合*
MSVCを使います。Visual Studioの開発者用コマンドプロンプトで実行して下さい。
```
git clone https://github.com/komugi1211s/RendererForFun.git
cd RendererForFun
./build.bat
```
どちらも`/dist`内にコンパイルされたファイルを作ります。
