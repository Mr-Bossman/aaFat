#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"
#include <assert.h>
#include "aaFat.h"

int main(){
    write_FAT();
    printf("%d\n",FAT_ERRpop());
    new_file("br");
    printf("%d\n",FAT_ERRpop());
    print_file_table();

    new_file("3");
    printf("%d\n",FAT_ERRpop());
    print_fat();
    printf("%d\n",get_file_block("3"));
    printf("%d\n",FAT_ERRpop());
    print_file_table();
    del_file("br");
    printf("%d\n",FAT_ERRpop());
    print_file_table();
    print_fat();
    char data[] ="If successful, returns the smaller of two floating point values. The value returned is exact and does not depend on any rounding modes.";
    write_file("3",data,sizeof(data),8);
    printf("%d\n",FAT_ERRpop());
    memset(data,0,sizeof(data));
    read_file("3",data,20,65);
    printf("%d\n",FAT_ERRpop());
    puts(data);
    print_file_table();
    print_fat();
    return 0;
}