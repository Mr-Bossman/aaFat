#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "example2.h"
#include "aaFat.h"

unsigned char *store;

int read_blk(size_t offset, unsigned char *mem) {
	if (offset >= TABLE_LEN)
		return -1;
	memcpy(mem, store + (offset * BLOCK_SIZE), BLOCK_SIZE);
	return ERR_OK;
}

int write_blk(size_t offset, unsigned char *mem) {
	if (offset >= TABLE_LEN)
		return -1;
	memcpy(store + (offset * BLOCK_SIZE), mem, BLOCK_SIZE);
	return ERR_OK;
}

int main() {
	store = malloc(BLOCK_SIZE * TABLE_LEN);
	if (!store) {
		puts("malloc failed.\n");
		return 1;
	}

	write_FAT();
	print_ERR();

	new_file("br");

	new_file("3");
	new_file("4");

	get_file_block("3");

	del_file("br");
	del_file("3");

	new_file("3");
	new_file("00");

	print_file_table();
	file_count();
	printf("%u\n", validate_FAT());
	return 0;
}
