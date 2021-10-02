#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"
#include <assert.h>
#include "aaFat.h"

int main(){
    write_FAT();

    new_file("br");
    print_file_table();

    new_file("3");
    print_fat();
    printf("%d\n",get_file_block("3"));
    print_file_table();
    del_file("br");
    print_file_table();
    return 0;
}