if [ ! -d "dist" ]; then
    mkdir dist
fi

g++ -g -O2 -o dist/compiled src/main_linux.cpp -lX11 -lm
cp resources/inconsolata.ttf dist/inconsolata.ttf
