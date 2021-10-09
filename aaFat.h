


#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"
#include "math.h"

#ifndef __AAFAT_H
#define __AAFAT_H
typedef struct
{
    char name[16];
    size_t index;
}name_file;

int write_FAT();
int validate_FAT();
int file_count();
int get_file_index(name_file* ret,size_t index);
int get_file_block(const char name[16]);
int new_file(const char name[16]);
int del_file(const char name[16]);
size_t read_file(const char * file_name,void *buf,size_t count,size_t offset);
size_t write_file(const char * file_name,void *buf,size_t count,size_t offset);
void print_fat();
void print_file_table();

typedef enum ERR {
    ERR_OK = 0,
    READ_BLK_ERR = -1,
    WRITE_BLK_ERR = -2,
    BLK_OOB = -3, // out of bounds
    BLK_NSP = -4, // no space left
    BLK_EOF = -5
}ERR;
enum ERR FAT_ERRpop();
#endif