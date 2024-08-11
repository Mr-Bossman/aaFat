#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "config.h"

#ifndef __AAFAT_H
#define __AAFAT_H
typedef struct {
	char name[16];
	uint32_t index;
	size_t size_b;
} name_file;

typedef int (*blk_ops_t)(size_t offset, unsigned char *mem);

typedef struct {
	uint32_t block_size;
	uint32_t table_len;
	blk_ops_t read_blk;
	blk_ops_t write_blk;
} fs_config_t;

int init_fs(fs_config_t *config);

int write_FAT();
int validate_FAT();
size_t file_count();
int get_file_index(name_file *ret, size_t index);
/* name max len 16 */
uint32_t get_index_file(const char *name);
/* name max len 16 */
size_t get_file_size(const char *name);
/* name max len 16 */
int set_file_size(const char *name, size_t size);
/* name max len 16 */
uint32_t get_file_block(const char *name);
/* name max len 16 */
int get_file_exists(const char *name);
/* name max len 16 */
int new_file(const char *name);
/* name max len 16 */
int del_file(const char *name);
/* name max len 16 */
int read_file(const char *file_name, void *buf, size_t count, size_t offset);
int write_file(const char *file_name, void *buf, size_t count, size_t offset);
void print_fat();
void print_file_table();
int print_ERR();

#define ENUMS(x) [x] = #x
typedef enum ERR {
	ERR_OK,
	READ_BLK_ERR,
	WRITE_BLK_ERR,
	BLK_OOB, // out of bounds
	BLK_NSP, // no space left
	BLK_EOF, // end of linked list
	FS_LOOP, //linked list refers itself
	FS_FNF, // file not found
	FS_BNAME, // bad name
	FS_OOB, // cant read more than f size
	FS_INVALID, // bad Ftable
} ERR;
ERR FAT_ERRpop();

#if defined(BLOCK_SIZE) && defined(TABLE_LEN)
int read_blk(size_t offset, unsigned char *mem);
int write_blk(size_t offset, unsigned char *mem);
#endif

#endif
