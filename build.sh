if [ ! -d "dist" ]; then
    mkdir dist
fi

clang++ -O1 -g -o dist/compiled src/main_linux.cpp -lX11 -lm -fno-caret-diagnostics
cp resources/inconsolata.ttf     dist/inconsolata.ttf
