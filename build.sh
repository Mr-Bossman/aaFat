dpkg -l libfuse3-dev &> /dev/null || apt install -y libfuse3-dev fuse3
dpkg -l fuse3 &> /dev/null || apt install -y libfuse3-dev fuse3
gcc -D__AAFAT_CONF_H -Wall -pedantic -Wno-unused-function -g aaFat.c example.c -o example
gcc -Wall -pedantic -Wno-unused-function -g aaFat.c fuse_example.c `pkg-config fuse3 --cflags --libs` -o aafat
mkdir test &> /dev/null
./aafat -f test
