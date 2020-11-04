if [ ! -d "dist" ]; then
    mkdir dist
fi

clang -pg -o dist/compiled src/main.c -Wall -lX11 -lm -fno-caret-diagnostics
