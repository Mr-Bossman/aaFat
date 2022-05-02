gcc -D__AAFAT_CONF_H -Wall -pedantic -Wno-unused-function -g aaFat.c example.c -o example
gcc -Wall -pedantic -Wno-unused-function -g aaFat.c fuse_example.c `pkg-config fuse3 --cflags --libs` -o aafat
./aafat -f test
