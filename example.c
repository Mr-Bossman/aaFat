#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "aaFat.h"

int main() {
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
	printf("%u\n",validate_FAT());
	return 0;
}
