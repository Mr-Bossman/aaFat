#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "aaFat.h"

#define BLOCK_SIZE 1024
#define TABLE_LEN 50

FILE* fp;

static int write_blk(size_t offset, unsigned char *mem){
	fseek(fp, offset*BLOCK_SIZE, SEEK_SET);
	fwrite(mem, 1, BLOCK_SIZE, fp);
	return 0;
}

static int read_blk(size_t offset, unsigned char *mem){
	fseek(fp, offset*BLOCK_SIZE, SEEK_SET);
	fread(mem, 1, BLOCK_SIZE, fp);
	return 0;
}

int main(int argc, char *argv[])
{
	int ret;
	fs_config_t config = {
		.block_size = BLOCK_SIZE,
		.table_len = TABLE_LEN,
		.read_blk = read_blk,
		.write_blk = write_blk
	};

	init_fs(&config);

	if (argc != 2) {
		printf("Usage: %s <file>\n", argv[0]);
		return 1;
	}

	fp = fopen(argv[1],"rb");
	if (!fp) {
		printf("Cant open fs.dat\n");
		return 1;
	}

	validate_FAT();
	ret = FAT_ERRpop();
	puts("print_fat");
	print_fat();
	puts("print_file_table");
	print_file_table();
	printf("validate_FAT: ret = %d\n", ret);
	return 0;
}
