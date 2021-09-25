#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"
#include <assert.h>

#define TABLE_LEN 8
#define BLOCK_SIZE 512
unsigned char store[4096] = {0};

int read(size_t offset,unsigned char* mem,size_t len){
    if(offset+len >= sizeof(store)) return -1;
    memcpy(mem,store+offset,len);
    return 0;
}

int write(size_t offset,unsigned char* mem,size_t len){
    if(offset+len >= sizeof(store)) return -1;
    memcpy(store+offset,mem,len);
    return 0;
}




// table == oneblock
int write_FAT(){
    // one is the file name table
    unsigned char fat[BLOCK_SIZE];
    memset(fat,0xFF,BLOCK_SIZE);
    ((size_t*)fat)[0] = 1; // size in bytes
    ((size_t*)fat)[1] = 0; // size in bytes
    int b = write(0,fat,BLOCK_SIZE);
    if(b) return -1;

    // clear block
    memset(fat,0,BLOCK_SIZE);
    return write(BLOCK_SIZE,fat,BLOCK_SIZE);
}

// zero delims end on chain
size_t get_nextblock(size_t block_index){
    unsigned char fat[BLOCK_SIZE] = {0};
    if(block_index > TABLE_LEN) return -1; // -1 err cant have index more than table
    if(read(0,fat,BLOCK_SIZE)) return -4;
    size_t tmp = ((size_t*)fat)[block_index];
    if(tmp == -1) return 0; // -1 is a free block
    return tmp;
}

int get_block_itter(size_t start,size_t i){
    size_t blocks = 0;
    unsigned char name_table[BLOCK_SIZE] = {0};
    while(blocks = get_nextblock(blocks)){
        if(blocks == -1) return -1;
        if(!i) return blocks;
        i--;
    }
    return -2;
}
int get_block_len(size_t start){
    size_t i = 0;
    size_t blocks = 0;
    unsigned char name_table[BLOCK_SIZE] = {0};
    while(blocks = get_nextblock(blocks)){
        if(blocks == -1) return -1;
        i++;
    }
    return i;
}
size_t get_freeblock(){
    unsigned char fat[BLOCK_SIZE] = {0};
    if(read(0,fat,BLOCK_SIZE)) return -4;
    for(size_t i = 0;i < TABLE_LEN;i++)
        if(((size_t*)fat)[i] == -1) return i; // -1 is a free block
    return -1;// -1 err cant have index more than table
}
int extend_blocks(size_t index){
    unsigned char fat[BLOCK_SIZE] = {0};
    if(read(0,fat,BLOCK_SIZE)) return -4;
    int err = 0;
    size_t last;
    while(index){ // we can make loops inside struct
        last = index;
        index = ((size_t*)fat)[index];
        err++;
        if(err > TABLE_LEN) return -1; //crap err check
    }
    //last index zero is last
    size_t tmp = get_freeblock();
    if(tmp == -1)return -2;
    ((size_t*)fat)[last] = tmp;
    ((size_t*)fat)[tmp] = 0;
    if(write(0,fat,BLOCK_SIZE)) return -2;


    // clear block
    memset(fat,0,BLOCK_SIZE);
    return write(BLOCK_SIZE*tmp,fat,BLOCK_SIZE)*2;
}

size_t add_block(){
    unsigned char fat[BLOCK_SIZE] = {0};
    if(read(0,fat,BLOCK_SIZE)) return -4;
    size_t tmp = get_freeblock();
    if(tmp == -1)return -1;
    ((size_t*)fat)[tmp] = 0;
    if(write(0,fat,BLOCK_SIZE))return -2;

    // clear block
    memset(fat,0,BLOCK_SIZE);
    if(write(BLOCK_SIZE*tmp,fat,BLOCK_SIZE))
    return -3;
    return tmp;
}

int del_block(size_t index){
    if(index == 0) return -1;
    // mabey check for file name table too
    unsigned char fat[BLOCK_SIZE] = {0};
    if(read(0,fat,BLOCK_SIZE)) return -3;
    int err = 0;
    size_t last;
    while(index){ // we can make loops inside struct
        last = index;
        index = ((size_t*)fat)[index];
        ((size_t*)fat)[last] = -1;
        err++;
        if(err > TABLE_LEN) return -2; //crap err check
    }
    return write(0,fat,BLOCK_SIZE)*4;
}

typedef struct
{
    char name[16];
    size_t index;
}name_file __attribute__((packed));


int file_count(){
    size_t n = 0;
    size_t blocks = 0;
    unsigned char name_table[BLOCK_SIZE] = {0};
    while(blocks = get_nextblock(blocks)){
        if(blocks == -1) return -1;
        size_t i = 0;
        if(read(blocks*BLOCK_SIZE,name_table,BLOCK_SIZE)) return -1;
        while(1){
            if(i*sizeof(name_file) >= BLOCK_SIZE) 
                break;
            if(((name_file*)name_table)[i].index == 0)
                break;
            if(((name_file*)name_table)[i].index == 1){
                i++;
                if(i*sizeof(name_file) >= BLOCK_SIZE) 
                    break;
                continue;
            }
            i++;
            if(i*sizeof(name_file) >= BLOCK_SIZE) 
                break;
        }
        n += i-1;
    }
    return n;
}

int get_file_index(name_file* ret,size_t index){
    size_t blocks = 0;
    unsigned char name_table[BLOCK_SIZE] = {0};
    while(blocks = get_nextblock(blocks)){
        if(blocks == -1) return -1;
        if(read(blocks*BLOCK_SIZE,name_table,BLOCK_SIZE)) return -1;
        size_t i = 0;
        while(1){
            if(((name_file*)name_table)[i].index == 1)
                index++;
            if(i == index){
                *ret = ((name_file*)name_table)[i];
                return 0;
            }
            i++;
            if(i*sizeof(name_file) >= BLOCK_SIZE) 
                break;
        }
    }
    return -1;
}

int get_file_block(char name[16]){
    size_t blocks = 0;
    unsigned char name_table[BLOCK_SIZE] = {0};
    while(blocks = get_nextblock(blocks)){
        if(blocks == -1) return -1;
        if(read(blocks*BLOCK_SIZE,name_table,BLOCK_SIZE)) return -1;
        size_t i = 0;
        while(1){
            size_t b = strnlen(((name_file*)name_table)[i].name,16); // check for dups
            if(b == strnlen(name,16))
                if(!strncmp(name,((name_file*)name_table)[i].name,16))
                    return ((name_file*)name_table)[i].index;
            i++;
            if(i*sizeof(name_file) >= BLOCK_SIZE) 
                break;
        }
    }
    return 0;
}

int new_file(char name[16]){
    size_t b = strnlen(name,16);
    char name_padded[16] = {0};
    memcpy(name_padded,name,b);
    size_t blocks = 0;
    unsigned char name_table[BLOCK_SIZE] = {0};
    while(blocks = get_nextblock(blocks)){
        if(blocks == -1) return -1;
        if(read(blocks*BLOCK_SIZE,name_table,BLOCK_SIZE)) return -2;
        size_t i = 0;
        while(1){
            size_t b = strnlen(((name_file*)name_table)[i].name,16); // check for dups
            if(!b){
                name_file  tmp;
                memcpy(tmp.name,name_padded,16);
                tmp.index = add_block();
                ((name_file*)name_table)[i] = tmp;
                if(write(blocks*BLOCK_SIZE,name_table,BLOCK_SIZE))return -4;
                    break;
            }
            i++;
            if(i*sizeof(name_file) >= BLOCK_SIZE) 
                break;
        }
    }
    return 0;
}

int del_file(char name[16]){
    size_t b = strnlen(name,16);
    char name_padded[16] = {0};
    memcpy(name_padded,name,b);
    size_t blocks = 0;
    unsigned char name_table[BLOCK_SIZE] = {0};
    while(blocks = get_nextblock(blocks)){
        if(blocks == -1) return -1;
        if(read(blocks*BLOCK_SIZE,name_table,BLOCK_SIZE)) return -2;
        size_t i = 0;
        while(1){
            size_t b = strnlen(((name_file*)name_table)[i].name,16); // check for dups
            if(b == strnlen(name,16))
                if(!strncmp(name,((name_file*)name_table)[i].name,16)){
                    del_block(((name_file*)name_table)[i].index);
                    name_file  tmp;
                    memset(tmp.name,0,16);
                    tmp.index = 1;
                    ((name_file*)name_table)[i] = tmp;
                    if(write(blocks*BLOCK_SIZE,name_table,BLOCK_SIZE))return -4;
                        break;
                }
            i++;
            if(i*sizeof(name_file) >= BLOCK_SIZE)
                break;
        }
    }
    return 1;
}

void print_fat(){
    unsigned char fat[BLOCK_SIZE] = {0};
    read(0,fat,BLOCK_SIZE);
    for(size_t i = 0;i < TABLE_LEN;i++)
        printf("%d,",((size_t*)fat)[i]);
    puts("\n");
}

void print_file_table(){
size_t blocks = 0;
    unsigned char name_table[BLOCK_SIZE] = {0};
    while(blocks = get_nextblock(blocks)){
        read(blocks*BLOCK_SIZE,name_table,BLOCK_SIZE); //err check
        size_t i = 0;
        while(1){
            size_t b = strnlen(((name_file*)name_table)[i].name,16); // check for dups
            if(((name_file*)name_table)[i].index == 0)
                break;
            if(((name_file*)name_table)[i].index == 1){
                i++;
                if(i*sizeof(name_file) >= BLOCK_SIZE)
                    break;
                continue;
            }
            printf("%s,%d\n",((name_file*)name_table)[i].name,((name_file*)name_table)[i].index);
            i++;
            if(i*sizeof(name_file) >= BLOCK_SIZE)
                break;
        }
    }
}



int main(){
    static_assert(sizeof(store) >= BLOCK_SIZE*TABLE_LEN, "err");
    static_assert(BLOCK_SIZE > TABLE_LEN*sizeof(size_t), "err");
    write_FAT();
    new_file("br");
    new_file("3");
    print_fat();
    printf("%u\n",get_file_block("3"));
    //print_file_table();
    del_file("br");
    print_file_table();
    return 0;
}