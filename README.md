# Renderer for fun.
ソフトウェアレンダラの仕組みを学ぶ為に各OSに存在するライブラリのみを用いて書かれたレンダラ。
このレンダラはGithubで公開されているコースの[TinyRenderer - software rendering in 500 lines of code](https://github.com/ssloy/tinyrenderer) を元に作られています。
基本的にコードはこのコースに倣って書かれています。


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
