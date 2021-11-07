
#include "aaFat.h"

#define EXAMPLE_
#ifdef EXAMPLE_

#define BACKTRACE_ERR
#define BT_SZ 100
#define TABLE_LEN 15
#define BLOCK_SIZE 128

unsigned char store[BLOCK_SIZE * TABLE_LEN] = {0};

int read_blk(size_t offset, unsigned char *mem)
{
    if ((offset * BLOCK_SIZE) >= sizeof(store))
        return -1;
    memcpy(mem, store + (offset * BLOCK_SIZE), BLOCK_SIZE);
    return ERR_OK;
}

int write_blk(size_t offset, unsigned char *mem)
{
    if ((offset * BLOCK_SIZE) >= sizeof(store))
        return -1;
    memcpy(store + (offset * BLOCK_SIZE), mem, BLOCK_SIZE);
    return ERR_OK;
}

#endif

#ifdef BACKTRACE_ERR
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#endif

/* TODO: clear error on extern funcs / check*/
/* TODO: fix looping check https://dev.to/alisabaj/floyd-s-tortoise-and-hare-algorithm-finding-a-cycle-in-a-linked-list-39af */
/* TODO: double check error checks */
/* TODO: fix int/size_t casts */

static ERR err = -ERR_OK;

#define chk_err() \
    if (err)      \
        return;
#define chk_err_e() \
    if (err)        \
        return err;

ERR FAT_ERRpop()
{
    ERR tmp = err;
    err = -ERR_OK;
    return tmp;
}

static char *ERR_NAME[] =
    {ENUMS(ERR_OK), ENUMS(READ_BLK_ERR), ENUMS(WRITE_BLK_ERR), ENUMS(BLK_OOB),
     ENUMS(BLK_NSP), ENUMS(BLK_EOF), ENUMS(FS_LOOP), ENUMS(FS_FNF), ENUMS(FS_BNAME),
     ENUMS(FS_OOB), ENUMS(FS_INVALID)};

int print_ERR()
{
    ERR tmp = FAT_ERRpop();
    int ret = printf("ERR: %s\n", ERR_NAME[-tmp]);
    if (tmp != ERR_OK)
    {
#ifdef BACKTRACE_ERR
        void *buffer[BT_SZ];
        int nptrs = backtrace(buffer, BT_SZ);
        printf("backtrace() returned %d addresses\n", nptrs);
        backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO);
#endif
    }
    return ret;
}

int write_FAT()
{
    if (BLOCK_SIZE < TABLE_LEN * sizeof(size_t))
    {
        err = FS_INVALID;
        return err;
    }
    unsigned char fat[BLOCK_SIZE];
    memset(fat, 0xFF, BLOCK_SIZE);
    ((size_t *)fat)[0] = 1;
    ((size_t *)fat)[1] = 0;
    if (write_blk(0, fat))
    {
        err = -WRITE_BLK_ERR;
        return err;
    }
    memset(fat, 0, BLOCK_SIZE);
    if (write_blk(1, fat))
    {
        err = -WRITE_BLK_ERR;
        return err;
    }
    return ERR_OK;
}

int validate_FAT()
{
    unsigned char fat[BLOCK_SIZE];
    if (read_blk(0, fat))
    {
        err = -READ_BLK_ERR;
        return err;
    }
    if (((size_t *)fat)[0] != 1)
    {
        return -FS_INVALID;
    }
    if (((size_t *)fat)[1] != 0)
    {
        return -FS_INVALID;
    }
    /* TODO: check validity of fat and name file */
    return ERR_OK;
}

static size_t get_nextblock(size_t block_index)
{
    unsigned char fat[BLOCK_SIZE] = {0};
    if (block_index > TABLE_LEN)
    {
        err = -BLK_OOB;
        return err;
    }
    if (read_blk(0, fat))
    {
        err = -READ_BLK_ERR;
        return err;
    }
    size_t tmp = ((size_t *)fat)[block_index];
    if ((int)tmp < 0)
    {
        err = -FS_INVALID;
        return err;
    }
    return tmp;
}

static int get_block_itter(size_t start, size_t i)
{
    size_t blocks = 0;
    while (((blocks = get_nextblock(blocks))))
    {
        chk_err_e();
        if (!i)
            return blocks;
        i--;
    }
    err = -BLK_EOF;
    return err;
}

static int get_block_len(size_t start)
{
    size_t i = 0;
    size_t blocks = 0;
    while ((blocks = get_nextblock(blocks)))
    {
        chk_err_e();
        i++;
    }
    return i;
}

static size_t get_freeblock()
{
    unsigned char fat[BLOCK_SIZE] = {0};
    if (read_blk(0, fat))
    {
        err = -READ_BLK_ERR;
        return err;
    }
    for (size_t i = 0; i < TABLE_LEN; i++)
        if (((size_t *)fat)[i] == -1)
            return i;
    err = -BLK_NSP;
    return err;
}

static int extend_blocks(size_t index)
{
    unsigned char fat[BLOCK_SIZE] = {0};
    if (read_blk(0, fat))
    {
        err = -READ_BLK_ERR;
        return err;
    }
    int bb = 0;
    size_t last;
    /* TODO: loop check */
    while (index)
    { // we can make loops inside struct
        last = index;
        index = ((size_t *)fat)[index];
        bb++;
        if (bb > TABLE_LEN)
        {
            err = -FS_LOOP;
            return err; //crap err check
        }
    }
    size_t tmp = get_freeblock();
    chk_err_e();
    ((size_t *)fat)[last] = tmp;
    ((size_t *)fat)[tmp] = 0;
    if (write_blk(0, fat))
    {
        err = -WRITE_BLK_ERR;
        return err;
    }

    memset(fat, 0, BLOCK_SIZE);
    if (write_blk(tmp, fat))
        err = -WRITE_BLK_ERR;
    return err;
}

static size_t add_block()
{
    unsigned char fat[BLOCK_SIZE] = {0};
    if (read_blk(0, fat))
    {
        err = -READ_BLK_ERR;
        return err;
    }
    size_t tmp = get_freeblock();
    chk_err_e();
    ((size_t *)fat)[tmp] = 0;
    if (write_blk(0, fat))
    {
        err = -WRITE_BLK_ERR;
        return err;
    }

    memset(fat, 0, BLOCK_SIZE);
    if (write_blk(tmp, fat))
    {
        err = -WRITE_BLK_ERR;
        return err;
    }

    return tmp;
}

static int del_block(size_t index)
{
    if (index == 0 || index > TABLE_LEN)
    {
        err = -BLK_OOB;
        return err;
    }
    unsigned char fat[BLOCK_SIZE] = {0};
    if (read_blk(0, fat))
    {
        err = -READ_BLK_ERR;
        return err;
    }
    int errb = 0;
    size_t last;
    /* TODO: loop check */
    while (index)
    { // we can make loops inside struct
        last = index;
        index = ((size_t *)fat)[index];
        ((size_t *)fat)[last] = -1;
        errb++;
        if (errb > TABLE_LEN)
        {
            err = -FS_LOOP;
            return err; //crap err check
        }
    }
    if (write_blk(0, fat))
    {
        err = -WRITE_BLK_ERR;
        return err;
    }
    return ERR_OK;
}

int file_count()
{
    size_t n = 0;
    size_t blocks = 0;
    unsigned char name_table[BLOCK_SIZE] = {0};
    while ((blocks = get_nextblock(blocks)))
    {
        chk_err_e();
        size_t i = 0;
        if (read_blk(blocks, name_table))
        {
            err = -READ_BLK_ERR;
            return err;
        }
        while (1)
        {
            if (i * sizeof(name_file) >= BLOCK_SIZE)
                break;
            if (((name_file *)name_table)[i].index == 0)
                break;
            if (((name_file *)name_table)[i].index == 1)
            {
                i++;
                if (i * sizeof(name_file) >= BLOCK_SIZE)
                    break;
                continue;
            }
            i++;
            if (i * sizeof(name_file) >= BLOCK_SIZE)
                break;
        }
        n += i - 1;
    }
    return n;
}

int get_file_index(name_file *ret, size_t index)
{
    size_t blocks = 0;
    unsigned char name_table[BLOCK_SIZE] = {0};
    while ((blocks = get_nextblock(blocks)))
    {
        chk_err_e();
        if (read_blk(blocks, name_table))
        {
            err = -READ_BLK_ERR;
            return err;
        }
        size_t i = 0;
        while (1)
        {
            if (((name_file *)name_table)[i].index == 1)
                index++;
            if (i == index)
            {
                *ret = ((name_file *)name_table)[i];
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

int get_file_block(const char *name)
{
    size_t b = strnlen(name, 16);
    if (b == 16)
    {
        err = -FS_BNAME;
        return err;
    }
    size_t blocks = 0;
    unsigned char name_table[BLOCK_SIZE] = {0};
    while ((blocks = get_nextblock(blocks)))
    {
        chk_err_e();
        if (read_blk(blocks, name_table))
        {
            err = -READ_BLK_ERR;
            return err;
        }
        size_t i = 0;
        while (1)
        {
            size_t b = strnlen(((name_file *)name_table)[i].name, 16);
            if (b == strnlen(name, 16))
                if (!strncmp(name, ((name_file *)name_table)[i].name, 16))
                    return ((name_file *)name_table)[i].index;
            i++;
            if (i * sizeof(name_file) >= BLOCK_SIZE)
                break;
        }
    }
    err = -FS_FNF;
    return err;
}

int get_file_size(const char *name)
{
    size_t b = strnlen(name, 16);
    if (b == 16)
    {
        err = -FS_BNAME;
        return err;
    }
    size_t blocks = 0;
    unsigned char name_table[BLOCK_SIZE] = {0};
    while ((blocks = get_nextblock(blocks)))
    {
        chk_err_e();
        if (read_blk(blocks, name_table))
        {
            err = -READ_BLK_ERR;
            return err;
        }
        size_t i = 0;
        while (1)
        {
            size_t b = strnlen(((name_file *)name_table)[i].name, 16);
            if (b == strnlen(name, 16))
                if (!strncmp(name, ((name_file *)name_table)[i].name, 16))
                    return ((name_file *)name_table)[i].size_b;
            i++;
            if (i * sizeof(name_file) >= BLOCK_SIZE)
                break;
        }
    }
    err = -FS_FNF;
    return err;
}

static int new_file_size(const char *name, size_t size)
{
    size_t b = strnlen(name, 16);
    if (b == 16)
    {
        err = -FS_BNAME;
        return err;
    }
    size_t blocks = 0;
    unsigned char name_table[BLOCK_SIZE] = {0};
    while ((blocks = get_nextblock(blocks)))
    {
        chk_err_e();
        if (read_blk(blocks, name_table))
        {
            err = -READ_BLK_ERR;
            return err;
        }
        size_t i = 0;
        while (1)
        {
            size_t b = strnlen(((name_file *)name_table)[i].name, 16);
            if (b == strnlen(name, 16))
                if (!strncmp(name, ((name_file *)name_table)[i].name, 16))
                {
                    size_t tmp = ((name_file *)name_table)[i].size_b;
                    ((name_file *)name_table)[i].size_b = fmax(size, tmp);
                    if (write_blk(blocks, name_table))
                    {
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

int new_file(const char *name)
{
    size_t b = strnlen(name, 16);
    if (b == 16)
    {
        err = -FS_BNAME;
        return err;
    }
    int tmp = get_file_block(name);
    if (tmp > 0)
    {
        err = -FS_BNAME;
        return err;
    }
    else if (tmp != -FS_FNF)
    {
        chk_err_e();
    }
    else
        err = -ERR_OK;
    char name_padded[16] = {0};
    memcpy(name_padded, name, b);
    size_t blocks = 0;
    unsigned char name_table[BLOCK_SIZE] = {0};
    while ((blocks = get_nextblock(blocks)))
    {
        chk_err_e();
        if (read_blk(blocks, name_table))
        {
            err = -READ_BLK_ERR;
            return err;
        }
        size_t i = 0;
        while (1)
        {
            size_t b = strnlen(((name_file *)name_table)[i].name, 16);
            if (!b)
            {
                name_file tmp;
                memcpy(tmp.name, name_padded, 16);
                tmp.index = add_block();
                tmp.size_b = 0;
                chk_err_e();
                ((name_file *)name_table)[i] = tmp;
                if (write_blk(blocks, name_table))
                {
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
    return ERR_OK;
}

int del_file(const char *name)
{
    size_t b = strnlen(name, 16);
    if (b == 16)
    {
        err = -FS_BNAME;
        return err;
    }
    ERR ret = -FS_FNF;
    char name_padded[16] = {0};
    memcpy(name_padded, name, b);
    size_t blocks = 0;
    unsigned char name_table[BLOCK_SIZE] = {0};
    while ((blocks = get_nextblock(blocks)))
    {
        chk_err_e();
        if (read_blk(blocks, name_table))
        {
            err = -READ_BLK_ERR;
            return err;
        }
        size_t i = 0;
        while (1)
        {
            size_t b = strnlen(((name_file *)name_table)[i].name, 16);
            if (b == strnlen(name, 16))
                if (!strncmp(name, ((name_file *)name_table)[i].name, 16))
                {
                    ret = -ERR_OK;
                    del_block(((name_file *)name_table)[i].index);
                    chk_err_e();
                    name_file tmp;
                    memset(tmp.name, 0, 16);
                    tmp.index = 1;
                    ((name_file *)name_table)[i] = tmp;
                    if (write_blk(blocks, name_table))
                    {
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

size_t read_file(const char *file_name, void *buffer, size_t count, size_t offset)
{
    char *buf = buffer;
    if (count + offset > get_file_size(file_name))
    {
        err = -FS_OOB;
        return err;
    }
    chk_err_e();
    size_t blk = get_file_block(file_name);
    chk_err_e();
    unsigned char BLOCKS[BLOCK_SIZE] = {0};
    while ((blk = get_nextblock(blk)))
    {
        chk_err_e();
        if (offset >= BLOCK_SIZE)
        {
            offset -= BLOCK_SIZE;
            continue;
        }
        if (read_blk(blk, BLOCKS))
        {
            err = -READ_BLK_ERR;
            return err;
        }
        int cpy_len = fmin(count, BLOCK_SIZE);
        if (cpy_len + offset > BLOCK_SIZE)
            cpy_len -= offset;
        count -= cpy_len;
        memcpy(buf, BLOCKS + offset, cpy_len);
        buf += cpy_len;
        offset = 0;
        if (!count)
            break;
    }
    return ERR_OK;
}

size_t write_file(const char *file_name, void *buffer, size_t count, size_t offset)
{
    char *buf = buffer;
    size_t blk = get_file_block(file_name);
    chk_err_e();
    unsigned char BLOCKS[BLOCK_SIZE] = {0};
    size_t sz = offset + count;
    while (offset + count)
    {
        size_t tmp;
        do
        {
            tmp = get_nextblock(blk);
            chk_err_e();
            if (!tmp)
                extend_blocks(blk);
            chk_err_e();
        } while (!tmp);
        blk = tmp;
        if (offset > BLOCK_SIZE)
        {
            offset -= BLOCK_SIZE;
            continue;
        }
        if (read_blk(blk, BLOCKS))
        {
            err = -READ_BLK_ERR;
            return err;
        }
        int cpy_len = fmin(count, BLOCK_SIZE);
        if (cpy_len + offset > BLOCK_SIZE)
            cpy_len -= offset;
        count -= cpy_len;
        memcpy(BLOCKS + offset, buf, cpy_len);
        buf += cpy_len;
        if (write_blk(blk, BLOCKS))
        {
            err = -WRITE_BLK_ERR;
            return err;
        }
        offset = 0;
    }
    new_file_size(file_name, sz);
    chk_err_e();
    return ERR_OK;
}

void print_fat()
{
    puts("FAT, linked list:");
    unsigned char fat[BLOCK_SIZE] = {0};
    read_blk(0, fat);
    printf("%3ld", ((size_t *)fat)[0]);
    for (size_t i = 1; i < TABLE_LEN; i++)
        printf(",%3ld", ((size_t *)fat)[i]);
    puts("");
    printf("  0");
    for (size_t i = 1; i < TABLE_LEN; i++)
        printf(",%3lu", i);
    puts("");
}

void print_file_table()
{
    size_t blocks = 0;
    unsigned char name_table[BLOCK_SIZE] = {0};
    while ((blocks = get_nextblock(blocks)))
    {
        read_blk(blocks, name_table);
        size_t i = 0;
        while (1)
        {
            if (((name_file *)name_table)[i].index == 0)
                break;
            if (((name_file *)name_table)[i].index == 1)
            {
                i++;
                if (i * sizeof(name_file) >= BLOCK_SIZE)
                    break;
                continue;
            }
            printf("%s, BLKS: %ld", ((name_file *)name_table)[i].name, ((name_file *)name_table)[i].index);
            {
                size_t blk = ((name_file *)name_table)[i].index;
                while ((blk = get_nextblock(blk)))
                    printf(",%ld", blk);
                puts("");
            }
            i++;
            if (i * sizeof(name_file) >= BLOCK_SIZE)
                break;
        }
    }
}
