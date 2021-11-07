#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "aaFat.h"

int main(){
    write_FAT();
    print_ERR();

    new_file("br");
    print_ERR();


    print_file_table();

    new_file("3");
    print_ERR();

    print_fat();
    get_file_block("3");
    print_ERR();

    print_file_table();

    del_file("br");
    print_ERR();

    print_file_table();
    print_fat();

    new_file("3");
    print_ERR();

    char data[] ="If successful, returns the smaller of two floating point values. The value returned is exact and does not depend on any rounding modes.";
    write_file("3",data,sizeof(data),8);
    print_ERR();

    memset(data,0,sizeof(data));
    read_file("3",data,20,65);
    print_ERR();

    puts(data);
    print_file_table();
    print_fat();

    printf("%d\n",get_file_size("3"));
    print_ERR();
    return 0;
}