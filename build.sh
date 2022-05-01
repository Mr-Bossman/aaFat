gcc -Wall -pedantic -Wno-unused-function -g aaFat.c example.c -lm -o example
gcc -Wall -pedantic -Wno-unused-function -g aaFat.c fuse_example.c `pkg-config fuse3 --cflags --libs` -lm -o aafat
./aafat -f test
