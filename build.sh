if [ ! -d "dist" ]; then
    mkdir dist
fi

clang -O2 -g -msse2 -o dist/compiled src/main_linux.c -lX11 -lm -fno-caret-diagnostics
cp resources/inconsolata.ttf     dist/inconsolata.ttf
