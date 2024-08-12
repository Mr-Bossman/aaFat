#include "aaFat.h"

#ifdef BACKTRACE_ERR
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#endif

/* TODO: double check error checks */

/* Curent error number. */
static ERR err = -ERR_OK;

static fs_config_t config;

#define chk_err()               \
	do {                    \
		if (err)        \
			return; \
	} while (0)

#define chk_err_e()                 \
	do {                        \
		if (err)            \
			return err; \
	} while (0)

#define fatal_check()                       \
	do {                                \
		if (err)                    \
			if (validate_FAT()) \
				return err; \
	} while (0)

#define min(a, b) (((a) > (b)) ? (b) : (a))
#define max(a, b) (((a) < (b)) ? (b) : (a))

/* Clears and returns current error number. */
ERR FAT_ERRpop(void) {
	ERR tmp = err;
	err = -ERR_OK;
	return tmp;
}

/* Names of ERR numbers. */
static const char *ERR_NAME[] = {
	ENUMS(ERR_OK),
	ENUMS(READ_BLK_ERR),
	ENUMS(WRITE_BLK_ERR),
	ENUMS(BLK_OOB),
	ENUMS(BLK_NSP),
	ENUMS(BLK_EOF),
	ENUMS(FS_LOOP),
	ENUMS(FS_FNF),
	ENUMS(FS_BNAME),
	ENUMS(FS_OOB),
	ENUMS(FS_INVALID)
};

const char *get_err_name(void) {
	return ERR_NAME[-err];
}

/* Prints current error number and name. */
int print_ERR(void) {
	ERR tmp = FAT_ERRpop();
	int ret = printf("ERR: %s\n", ERR_NAME[-tmp]);
	if (tmp != ERR_OK) {
#ifdef BACKTRACE_ERR
		void *buffer[BT_SZ];
		int nptrs = backtrace(buffer, BT_SZ);
		printf("backtrace() returned %d addresses\n", nptrs);
		backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO);
#endif
	}
	return ret;
}

#if !defined(BLOCK_SIZE) || !defined(TABLE_LEN)
static int read_blk(size_t offset, unsigned char *mem);
static int write_blk(size_t offset, unsigned char *mem);
#endif

static LIST_TYPE get_nextblock(LIST_TYPE block_index);
static LIST_TYPE get_block_itter(LIST_TYPE start, LIST_TYPE i);
static LIST_TYPE get_block_len(LIST_TYPE start);
static LIST_TYPE get_freeblock(void);
static LIST_TYPE extend_blocks(LIST_TYPE index);
static LIST_TYPE add_block(void);
static int del_block(LIST_TYPE index);
static int new_file_size(const char *name, size_t size, uint8_t shrink);
static int check_block_loop(LIST_TYPE index);
static int end_block(LIST_TYPE index);
static inline int match_name(const char *name_table_name, const char *name,
			     size_t name_len);

#if !defined(BLOCK_SIZE) || !defined(TABLE_LEN)

static int read_blk(size_t offset, unsigned char *mem) {
	config.read_blk(offset, mem);
	return ERR_OK;
}

static int write_blk(size_t offset, unsigned char *mem) {
	config.write_blk(offset, mem);
	return ERR_OK;
}

#undef BLOCK_SIZE
#define BLOCK_SIZE config.block_size

#undef TABLE_LEN
#define TABLE_LEN config.table_len

#endif

#define for_each_block(block_data)                  \
	LIST_TYPE blocks = 0;                        \
	while ((blocks = get_nextblock(blocks))) {  \
		chk_err_e();                        \
		if (read_blk(blocks, block_data)) { \
			err = -READ_BLK_ERR;        \
			return err;                 \
		}

#define get_name_file(name_table, i) ((name_file *)name_table)[i]

static inline int match_name(const char *name_table_name, const char *name,
			     size_t name_len) {
	return ((strnlen(name_table_name, 16) == name_len) &&
		!strncmp(name, name_table_name, 16));
}

int init_fs(fs_config_t *config_) {
	config = *config_;
	return ERR_OK;
}

/* Writes FAT to block, over writes. */
/* Returns error num. */
int write_FAT(void) {
	if (BLOCK_SIZE < TABLE_LEN * sizeof(LIST_TYPE)) {
		printf("Invalid conf.\n");
		err = FS_INVALID;
		return err;
	}
	unsigned char fat[BLOCK_SIZE];
	memset(fat, 0xFF, BLOCK_SIZE);
	((LIST_TYPE *)fat)[0] = 1;
	((LIST_TYPE *)fat)[1] = 0;
	if (write_blk(0, fat)) {
		err = -WRITE_BLK_ERR;
		return err;
	}
	memset(fat, 0, BLOCK_SIZE);
	if (write_blk(1, fat)) {
		err = -WRITE_BLK_ERR;
		return err;
	}
	err = ERR_OK;
	return err;
}

/* Returns the integrity of file system. */
/* Returns error num. */
int validate_FAT(void) {
	err = ERR_OK;
	unsigned char fat[BLOCK_SIZE];
	if (read_blk(0, fat)) {
		err = -READ_BLK_ERR;
		return err;
	}
	if (((LIST_TYPE *)fat)[0] != 1) {
		err = -FS_INVALID;
		return err;
	}
	if (((LIST_TYPE *)fat)[1] != 0) {
		err = -FS_INVALID;
		return err;
	}
	check_block_loop(1);
	chk_err_e();
	size_t b = file_count();
	chk_err_e();
	name_file file;
	while (b--) {
		get_file_index(&file, b);
		chk_err_e();
		if (strnlen(file.name, 16) == 16) {
			err = -FS_BNAME;
			return err;
		}
		check_block_loop(file.index);
		chk_err_e();
	}
	return ERR_OK;
}

/* Gets Next block in linked list. */
/* Returns error num or block. */
static LIST_TYPE get_nextblock(LIST_TYPE block_index) {
	unsigned char fat[BLOCK_SIZE];
	if (block_index > TABLE_LEN) {
		err = -BLK_OOB;
		return err;
	}
	if (read_blk(0, fat)) {
		err = -READ_BLK_ERR;
		return err;
	}
	LIST_TYPE tmp = ((LIST_TYPE *)fat)[block_index];
	if (tmp == LIST_TYPE_MAX) {
		err = -FS_INVALID;
		return err;
	}
	return tmp;
}

/* Get 'i'th block from start. */
/* Returns error num or block. */
static LIST_TYPE get_block_itter(LIST_TYPE start, LIST_TYPE i) {
	if (!i)
		return start;
	LIST_TYPE blocks = start;
	while (((blocks = get_nextblock(blocks)))) {
		chk_err_e();
		i--;
		if (!i)
			return blocks;
	}
	err = -BLK_EOF;
	return err;
}

/* Get total block lenth of linked list staring at start. */
/* Returns error num or number of blocks. */
static LIST_TYPE get_block_len(LIST_TYPE start) {
	LIST_TYPE i = 0;
	LIST_TYPE blocks = start;
	while ((blocks = get_nextblock(blocks))) {
		chk_err_e();
		i++;
	}
	return i;
}

/* Checks for loops in file system linked list. */
/* Returns error num. */
static int check_block_loop(LIST_TYPE index) {
	unsigned char fat[BLOCK_SIZE];
	if (read_blk(0, fat)) {
		err = -READ_BLK_ERR;
		return err;
	}
	if (!index) {
		err = -BLK_OOB;
		return err;
	}
	/*if(index >= TABLE_LEN)
	{
		err = -FS_INVALID;
		return err;
	}
	LIST_TYPE tortoise = index;
	LIST_TYPE hare = ((LIST_TYPE *)fat)[index];
	while (hare && hare  >= TABLE_LEN && ((LIST_TYPE *)fat)[hare] && ((LIST_TYPE *)fat)[hare]  >= TABLE_LEN)
	{
		if (tortoise == hare)
		{
			err = -FS_LOOP;
			return err;
		}
		tortoise = ((LIST_TYPE *)fat)[tortoise];
		if(tortoise >= TABLE_LEN)
		{
			err = -FS_INVALID;
			return err;
		}
		if(tortoise == LIST_TYPE_MAX)
			tortoise = 0;
		LIST_TYPE tmp = ((LIST_TYPE *)fat)[hare];
		if(tmp >= TABLE_LEN)
		{
			err = -FS_INVALID;
			return err;
		}
		if(tmp == LIST_TYPE_MAX)
			hare = 0;
		else
			hare = ((LIST_TYPE *)fat)[tmp];
		if(hare >= TABLE_LEN)
		{
			err = -FS_INVALID;
			return err;
		}
		if(hare == LIST_TYPE_MAX)
			hare = 0;
	}*/
	return ERR_OK;
}

/* Gets next free block. */
/* Returns error num or block. */
static LIST_TYPE get_freeblock(void) {
	unsigned char fat[BLOCK_SIZE];
	if (read_blk(0, fat)) {
		err = -READ_BLK_ERR;
		return err;
	}
	for (size_t i = 0; i < TABLE_LEN; i++)
		if (((LIST_TYPE *)fat)[i] == LIST_TYPE_MAX)
			return i;
	err = -BLK_NSP;
	return err;
}

/* Adds free block to end of link list. */
/* Returns error num. */
static LIST_TYPE extend_blocks(LIST_TYPE index) {
	unsigned char fat[BLOCK_SIZE];
	if (read_blk(0, fat)) {
		err = -READ_BLK_ERR;
		return err;
	}
	check_block_loop(index);
	chk_err_e();
	LIST_TYPE last;
	while (index) {
		last = index;
		index = ((LIST_TYPE *)fat)[index];
	}
	LIST_TYPE tmp = get_freeblock();
	chk_err_e();
	((LIST_TYPE *)fat)[last] = tmp;
	((LIST_TYPE *)fat)[tmp] = 0;
	if (write_blk(0, fat)) {
		err = -WRITE_BLK_ERR;
		return err;
	}

	memset(fat, 0, BLOCK_SIZE);
	if (write_blk(tmp, fat)) {
		err = -WRITE_BLK_ERR;
		return err;
	}
	return tmp;
}

/* Ends a block after deleting. */
/* Returns error num. */
static int end_block(LIST_TYPE index) {
	unsigned char fat[BLOCK_SIZE];
	if (read_blk(0, fat)) {
		err = -READ_BLK_ERR;
		return err;
	}
	((LIST_TYPE *)fat)[index] = 0;
	if (write_blk(0, fat)) {
		err = -WRITE_BLK_ERR;
		return err;
	}
	return ERR_OK;
}

/* Add new linked list to FAT. */
/* Returns error num or block. */
static LIST_TYPE add_block(void) {
	unsigned char fat[BLOCK_SIZE];
	if (read_blk(0, fat)) {
		err = -READ_BLK_ERR;
		return err;
	}
	LIST_TYPE tmp = get_freeblock();
	chk_err_e();
	((LIST_TYPE *)fat)[tmp] = 0;
	if (write_blk(0, fat)) {
		err = -WRITE_BLK_ERR;
		return err;
	}

	memset(fat, 0, BLOCK_SIZE);
	if (write_blk(tmp, fat)) {
		err = -WRITE_BLK_ERR;
		return err;
	}

	return tmp;
}

/* Deletes all blocks in linked list after index. */
/* Returns error num. */
static int del_block(LIST_TYPE index) {
	if (index == 0 || index > TABLE_LEN) {
		err = -BLK_OOB;
		return err;
	}
	unsigned char fat[BLOCK_SIZE];
	if (read_blk(0, fat)) {
		err = -READ_BLK_ERR;
		return err;
	}
	check_block_loop(index);
	chk_err_e();
	LIST_TYPE last;
	while (index) {
		last = index;
		index = ((LIST_TYPE *)fat)[index];
		((LIST_TYPE *)fat)[last] = -1;
	}
	if (write_blk(0, fat)) {
		err = -WRITE_BLK_ERR;
		return err;
	}
	return ERR_OK;
}

/* Gets the total number of files. */
/* Returns error num or number of files. */
size_t file_count(void) {
	fatal_check();
	size_t n = 0;
	unsigned char name_table[BLOCK_SIZE];
	for_each_block(name_table)
		size_t i = 0;
		while (1) {
			if (get_name_file(name_table, i).index == 0)
				break;
			if (get_name_file(name_table, i).index == 1)
				n--;
			i++;
			if (i * sizeof(name_file) >= BLOCK_SIZE)
				break;
		}
		n += i;
	}
	return n;
}

/* Gets the file struct at index. */
/* Returns error num. */
int get_file_index(name_file *ret, size_t index) {
	fatal_check();
	size_t n = 0;
	unsigned char name_table[BLOCK_SIZE];
	for_each_block(name_table)
		size_t i = 0;
		while (1) {
			if (get_name_file(name_table, i).index == 1)
				index++;
			if (i + n == index) {
				*ret = get_name_file(name_table, i);
				return ERR_OK;
			}
			i++;
			if (i * sizeof(name_file) >= BLOCK_SIZE)
				break;
		}
		n += i;
	}
	err = -FS_FNF;
	return err;
}

/* Gets the index of index. */
/* Returns error num. */
LIST_TYPE get_index_file(const char *name) {
	fatal_check();
	size_t name_len = strnlen(name, 16);
	if (name_len == 16) {
		err = -FS_BNAME;
		return err;
	}
	size_t n = 0;
	unsigned char name_table[BLOCK_SIZE];
	for_each_block(name_table)
		size_t i = 0;
		while (1) {
			if (match_name(get_name_file(name_table, i).name, name, name_len))
				return n + i;
			i++;
			if (i * sizeof(name_file) >= BLOCK_SIZE)
				break;
		}
		n += i;
	}
	err = -FS_FNF;
	return err;
}

/* Gets first block of file. */
/* Returns error num or block. */
LIST_TYPE get_file_block(const char *name) {
	fatal_check();
	size_t name_len = strnlen(name, 16);
	if (name_len == 16) {
		err = -FS_BNAME;
		return err;
	}
	unsigned char name_table[BLOCK_SIZE];
	for_each_block(name_table)
		size_t i = 0;
		while (1) {
			if (match_name(get_name_file(name_table, i).name, name, name_len))
				return get_name_file(name_table, i).index;
			i++;
			if (i * sizeof(name_file) >= BLOCK_SIZE)
				break;
		}
	}
	err = -FS_FNF;
	return err;
}

/* Gets if file exists */
/* Returns error. */
int get_file_exists(const char *name) {
	fatal_check();
	size_t name_len = strnlen(name, 16);
	if (name_len == 16) {
		err = -FS_BNAME;
		return err;
	}
	unsigned char name_table[BLOCK_SIZE];
	for_each_block(name_table)
		size_t i = 0;
		while (1) {
			if (match_name(get_name_file(name_table, i).name, name, name_len))
				return ERR_OK;
			i++;
			if (i * sizeof(name_file) >= BLOCK_SIZE)
				break;
		}
	}
	err = -FS_FNF;
	return err;
}

/* Gets file size. */
/* Returns error num or size of file. */
size_t get_file_size(const char *name) {
	fatal_check();
	size_t name_len = strnlen(name, 16);
	if (name_len == 16) {
		err = -FS_BNAME;
		return err;
	}
	unsigned char name_table[BLOCK_SIZE];
	for_each_block(name_table)
		size_t i = 0;
		while (1) {
			if (match_name(get_name_file(name_table, i).name, name, name_len))
				return get_name_file(name_table, i).size_b;
			i++;
			if (i * sizeof(name_file) >= BLOCK_SIZE)
				break;
		}
	}
	err = -FS_FNF;
	return err;
}

/* Updates size not block */
/* Returns error num. */
static int new_file_size(const char *name, size_t size, uint8_t shrink) {
	size_t name_len = strnlen(name, 16);
	if (name_len == 16) {
		err = -FS_BNAME;
		return err;
	}
	unsigned char name_table[BLOCK_SIZE];
	for_each_block(name_table)
		size_t i = 0;
		while (1) {
			if (match_name(get_name_file(name_table, i).name, name, name_len)) {
				size_t tmp = get_name_file(name_table, i).size_b;
				if (!shrink)
					size = max(size, tmp);
				get_name_file(name_table, i).size_b = size;
				if (write_blk(blocks, name_table)) {
					err = -WRITE_BLK_ERR;
					return err;
				}
				return ERR_OK;
			}
			i++;
			if (i * sizeof(name_file) >= BLOCK_SIZE)
				break;
		}
	}
	err = -FS_FNF;
	return err;
}

int set_file_size(const char *name, size_t size) {
	fatal_check();
	LIST_TYPE blk = get_file_block(name);
	chk_err_e();
	size_t block_num = (size / BLOCK_SIZE);
	LIST_TYPE total = get_block_len(blk);
	chk_err_e();
	if (block_num < total && block_num) {
		LIST_TYPE last = get_block_itter(blk, block_num);
		chk_err_e();
		LIST_TYPE del = get_nextblock(last);
		chk_err_e();
		del_block(del);
		chk_err_e();
		end_block(last);
		chk_err_e();
	} else if (block_num > total) {
		LIST_TYPE last = get_block_itter(blk, total);
		chk_err_e();
		for (LIST_TYPE i = total; i < block_num; i++) {
			extend_blocks(last);
			chk_err_e();
		}
	}
	total = get_block_len(blk);
	chk_err_e();
	new_file_size(name, size, 1);
	chk_err_e();
	return ERR_OK;
}
/* Adds new file. */
/* Returns error num. */
int new_file(const char *name) {
	fatal_check();
	size_t name_len = strnlen(name, 16);
	if (name_len == 16) {
		err = -FS_BNAME;
		return err;
	}
	get_file_exists(name);
	if (err == -ERR_OK) {
		err = -FS_BNAME;
		return err;
	} else if (err != -FS_FNF) {
		chk_err_e();
	} else
		err = -ERR_OK;
	char name_padded[16] = { 0 };
	memcpy(name_padded, name, name_len);
	LIST_TYPE blocks = 0;
	unsigned char name_table[BLOCK_SIZE];
	while (1) {
		LIST_TYPE tmp = get_nextblock(blocks);
		chk_err_e();
		if (!tmp)
			extend_blocks(blocks);
		chk_err_e();
		blocks = tmp;
		if (read_blk(blocks, name_table)) {
			err = -READ_BLK_ERR;
			return err;
		}
		size_t i = 0;
		while (1) {
			if (get_name_file(name_table, i).index == 0 ||
			    get_name_file(name_table, i).index == 1) {
				name_file tmp;
				memcpy(tmp.name, name_padded, 16);
				tmp.index = add_block();
				tmp.size_b = 0;
				chk_err_e();
				get_name_file(name_table, i) = tmp;
				if (write_blk(blocks, name_table)) {
					err = -WRITE_BLK_ERR;
					return err;
				}
				return ERR_OK;
			}
			i++;
			if (i * sizeof(name_file) >= BLOCK_SIZE)
				break;
		}
	}
}

/* Deletes file. */
/* Returns error num. */
int del_file(const char *name) {
	fatal_check();
	size_t name_len = strnlen(name, 16);
	if (name_len == 16) {
		err = -FS_BNAME;
		return err;
	}
	ERR ret = -FS_FNF;
	char name_padded[16] = { 0 };
	memcpy(name_padded, name, name_len);
	unsigned char name_table[BLOCK_SIZE];
	for_each_block(name_table)
		size_t i = 0;
		while (1) {
			if (match_name(get_name_file(name_table, i).name, name, name_len)) {
				ret = -ERR_OK;
				del_block(get_name_file(name_table, i).index);
				chk_err_e();
				name_file tmp;
				memset(tmp.name, 0, 16);
				tmp.index = 1;
				get_name_file(name_table, i) = tmp;
				if (write_blk(blocks, name_table)) {
					err = -WRITE_BLK_ERR;
					return err;
				}
				break;
			}
			i++;
			if (i * sizeof(name_file) >= BLOCK_SIZE)
				break;
		}
	}
	return ret;
}

/* Reads from file. */
/* Returns error num. */
int read_file(const char *file_name, void *buffer, size_t count,
	      size_t offset) {
	fatal_check();
	unsigned char *buf = buffer;
	if (count + offset > get_file_size(file_name)) {
		err = -FS_OOB;
		return err;
	}
	chk_err_e();
	LIST_TYPE blk = get_file_block(file_name);
	chk_err_e();
	unsigned char BLOCKS[BLOCK_SIZE];
	while (blk) {
		chk_err_e();
		if (offset >= BLOCK_SIZE) {
			offset -= BLOCK_SIZE;
			blk = get_nextblock(blk);
			continue;
		}
		if (!offset && count >= BLOCK_SIZE) {
			if (read_blk(blk, buf)) {
				err = -READ_BLK_ERR;
				return err;
			}
			count -= BLOCK_SIZE;
			buf += BLOCK_SIZE;
			if (!count)
				break;
			blk = get_nextblock(blk);
			continue;
		}

		if (read_blk(blk, BLOCKS)) {
			err = -READ_BLK_ERR;
			return err;
		}
		size_t cpy_len = min(count + offset, BLOCK_SIZE);
		cpy_len -= offset;
		count -= cpy_len;
		memcpy(buf, BLOCKS + offset, cpy_len);
		buf += cpy_len;
		offset = 0;
		if (!count)
			break;
		blk = get_nextblock(blk);
	}
	return ERR_OK;
}

/* Write to file. */
/* Returns error num*/
int write_file(const char *file_name, void *buffer, size_t count,
	       size_t offset) {
	fatal_check();
	unsigned char *buf = buffer;
	LIST_TYPE blk = get_file_block(file_name);
	chk_err_e();
	unsigned char BLOCKS[BLOCK_SIZE];
	size_t sz = offset + count;
	while (1) {
		LIST_TYPE tmp;
		if (offset >= BLOCK_SIZE) {
			offset -= BLOCK_SIZE;
			goto end;
		}

		if (!offset && count >= BLOCK_SIZE) {
			if (write_blk(blk, buf)) {
				err = -WRITE_BLK_ERR;
				return err;
			}
			count -= BLOCK_SIZE;
			buf += BLOCK_SIZE;
			if (!count)
				break;
			goto end;
		}

		if (read_blk(blk, BLOCKS)) {
			err = -READ_BLK_ERR;
			return err;
		}
		size_t cpy_len = min(count + offset, BLOCK_SIZE);
		cpy_len -= offset;
		count -= cpy_len;
		memcpy(BLOCKS + offset, buf, cpy_len);
		buf += cpy_len;
		if (write_blk(blk, BLOCKS)) {
			err = -WRITE_BLK_ERR;
			return err;
		}
		offset = 0;
		if (!count)
			break;
end:
		do {
			tmp = get_nextblock(blk);
			chk_err_e();
			if (!tmp)
				extend_blocks(blk);
			chk_err_e();
		} while (!tmp);
		blk = tmp;
	}
	new_file_size(file_name, sz, 0);
	chk_err_e();
	return ERR_OK;
}

static size_t print_n(LIST_TYPE *mem, size_t len) {
	size_t wrote = 0;
	wrote += printf("%3d", (mem[0] == LIST_TYPE_MAX)? -1 :(int)mem[0]);
	for (size_t i = 1; i < len; i++)
		wrote += printf(",%3d", (mem[i] == LIST_TYPE_MAX)? -1 :(int)mem[i]);
	return wrote;
}

int print_fat(size_t skip) {
	puts("FAT, linked list:");
	unsigned char fat[BLOCK_SIZE];
	if (read_blk(0, fat)) {
		err = -READ_BLK_ERR;
		return err;
	}
	for (size_t i = 0; i < TABLE_LEN; i += skip) {
		printf("%-3ld: ", i);
		print_n((LIST_TYPE *)fat + i, skip);
		puts("");
	}
	return ERR_OK;
}

int print_file_table(void) {
	unsigned char name_table[BLOCK_SIZE];
	for_each_block(name_table)
		size_t i = 0;
		while (1) {
			if (get_name_file(name_table, i).index == 0)
				break;
			if (get_name_file(name_table, i).index == 1) {
				i++;
				if (i * sizeof(name_file) >= BLOCK_SIZE)
					break;
				continue;
			}
			if (strnlen(get_name_file(name_table, i).name, 16) >= 16) {
				err = -FS_BNAME;
				return err;
			}
			printf("%s, BLKS: %u",
			       get_name_file(name_table, i).name,
			       get_name_file(name_table, i).index);
			{
				LIST_TYPE blk =
					get_name_file(name_table, i).index;
				while ((blk = get_nextblock(blk))) {
					chk_err_e();
					printf(",%u", blk);
				}
				puts("");
			}
			i++;
			if (i * sizeof(name_file) >= BLOCK_SIZE)
				break;
		}
	}
	return ERR_OK;
}
