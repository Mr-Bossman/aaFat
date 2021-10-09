
#include "aaFat.h"

#define EXAMPLE_
#ifdef EXAMPLE_

#define TABLE_LEN 15
#define BLOCK_SIZE 512
unsigned char store[BLOCK_SIZE*TABLE_LEN] = {0};

int read_blk(size_t offset,unsigned char* mem,size_t len){
    if(offset+len >= sizeof(store)) return -1;
    memcpy(mem,store+offset,len);
    return ERR_OK;
}

int write_blk(size_t offset,unsigned char* mem,size_t len){
    if(offset+len >= sizeof(store)) return -1;
    memcpy(store+offset,mem,len);
    return ERR_OK;
}

#endif
//ADD has init FAT
// fix looping check // https://dev.to/alisabaj/floyd-s-tortoise-and-hare-algorithm-finding-a-cycle-in-a-linked-list-39af
//add more specific err
//double check error checks
// fix read and write offset
// add get size
static enum ERR err = ERR_OK;
#define chk_err() if(err)return;
#define chk_err_e() if(err)return err;

enum ERR FAT_ERRpop(){
    enum ERR tmp = err;
    err = ERR_OK;
    return tmp;
}
// table == oneblock
int write_FAT(){
    // one is the file name table
    unsigned char fat[BLOCK_SIZE];
    memset(fat,0xFF,BLOCK_SIZE);
    ((size_t*)fat)[0] = 1; // size in bytes
    ((size_t*)fat)[1] = 0; // size in bytes
    if(write_blk(0,fat,BLOCK_SIZE)){
        err = READ_BLK_ERR;
        return err;
    }

    // clear block
    memset(fat,0,BLOCK_SIZE);
    if(write_blk(BLOCK_SIZE,fat,BLOCK_SIZE)){
        err = READ_BLK_ERR;
        return err;
    }
    return ERR_OK;
}
int validate_FAT(){
    // one is the file name table
    unsigned char fat[BLOCK_SIZE];
    if(read_blk(0,fat,BLOCK_SIZE)){
        err = READ_BLK_ERR;
        return err;
    }
    //if(((size_t*)fat)[0] != 1)
    //if(((size_t*)fat)[1] != 0)
    //check validity of fat and name file
    return ERR_OK;
}
// zero delims end on chain
static size_t get_nextblock(size_t block_index){
    unsigned char fat[BLOCK_SIZE] = {0};
    if(block_index > TABLE_LEN){
        err = BLK_OOB;
        return err; // -1 err cant have index more than table
    }
    if(read_blk(0,fat,BLOCK_SIZE)){
        err = READ_BLK_ERR;
        return err;
    }
    size_t tmp = ((size_t*)fat)[block_index];
    if(tmp < 0){
        err = BLK_NSP;
        return err; // -1 is a free block 
    }
    return tmp;
}

static int get_block_itter(size_t start,size_t i){
    size_t blocks = 0;
    while(((blocks = get_nextblock(blocks)))){
        chk_err_e();
        if(!i) return blocks;
        i--;
    }
    err = BLK_EOF;
    return err;
}
static int get_block_len(size_t start){
    size_t i = 0;
    size_t blocks = 0;
    while((blocks = get_nextblock(blocks))){
        chk_err_e();
        i++;
    }
    return i;
}
static size_t get_freeblock(){
    unsigned char fat[BLOCK_SIZE] = {0};
    if(read_blk(0,fat,BLOCK_SIZE)){
        err = READ_BLK_ERR;
        return err;
    }
    for(size_t i = 0;i < TABLE_LEN;i++)
        if(((size_t*)fat)[i] == -1) return i; // -1 is a free block
    err = BLK_EOF;
    return err;// -1 err cant have index more than table
}
static int extend_blocks(size_t index){
    unsigned char fat[BLOCK_SIZE] = {0};
    if(read_blk(0,fat,BLOCK_SIZE)){
        err = READ_BLK_ERR;
        return err;
    }
    int bb = 0;
    size_t last;
    while(index){ // we can make loops inside struct
        last = index;
        index = ((size_t*)fat)[index];
        bb++;
        if(bb > TABLE_LEN) return -2; //crap err check
    }
    //last index zero is last
    size_t tmp = get_freeblock();
    chk_err_e();
    ((size_t*)fat)[last] = tmp;
    ((size_t*)fat)[tmp] = 0;
    if(write_blk(0,fat,BLOCK_SIZE)){
        err = READ_BLK_ERR;
        return err;
    }

    // clear block
    memset(fat,0,BLOCK_SIZE);
    if(write_blk(BLOCK_SIZE*tmp,fat,BLOCK_SIZE))err = READ_BLK_ERR;
    return err;
}

static size_t add_block(){
    unsigned char fat[BLOCK_SIZE] = {0};
    if(read_blk(0,fat,BLOCK_SIZE)){
        err = READ_BLK_ERR;
        return err;
    }
    size_t tmp = get_freeblock();
    chk_err_e();
    ((size_t*)fat)[tmp] = 0;
    if(write_blk(0,fat,BLOCK_SIZE)){
        err = READ_BLK_ERR;
        return err;
    }

    // clear block
    memset(fat,0,BLOCK_SIZE);
    if(write_blk(BLOCK_SIZE*tmp,fat,BLOCK_SIZE)){
        err = READ_BLK_ERR;
        return err;
    }

    return tmp;
}

static int del_block(size_t index){
    if(index == 0){
        err = BLK_EOF;
        return err;
    }
    // mabey check for file name table too
    unsigned char fat[BLOCK_SIZE] = {0};
    if(read_blk(0,fat,BLOCK_SIZE)){
        err = READ_BLK_ERR;
        return err;
    }
    int err = 0;
    size_t last;
    while(index){ // we can make loops inside struct
        last = index;
        index = ((size_t*)fat)[index];
        ((size_t*)fat)[last] = -1;
        err++;
        if(err > TABLE_LEN) return -3; //crap err check
    }
    if(write_blk(0,fat,BLOCK_SIZE)){
        err = READ_BLK_ERR;
        return err;
    }
    return ERR_OK;
}

int file_count(){
    size_t n = 0;
    size_t blocks = 0;
    unsigned char name_table[BLOCK_SIZE] = {0};
    while((blocks = get_nextblock(blocks))){
        chk_err_e();
        size_t i = 0;
        if(read_blk(blocks*BLOCK_SIZE,name_table,BLOCK_SIZE)){
            err = READ_BLK_ERR;
            return err;
        }
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
    while((blocks = get_nextblock(blocks))){
        chk_err_e();
        if(read_blk(blocks*BLOCK_SIZE,name_table,BLOCK_SIZE)){
            err = READ_BLK_ERR;
            return err;
        }
        size_t i = 0;
        while(1){
            if(((name_file*)name_table)[i].index == 1)
                index++;
            if(i == index){
                *ret = ((name_file*)name_table)[i];
                return ERR_OK;
            }
            i++;
            if(i*sizeof(name_file) >= BLOCK_SIZE) 
                break;
        }
    }
    err = BLK_EOF;
    return err;
}

int get_file_block(const char name[16]){
    size_t blocks = 0;
    unsigned char name_table[BLOCK_SIZE] = {0};
    while((blocks = get_nextblock(blocks))){
        chk_err_e();
        if(read_blk(blocks*BLOCK_SIZE,name_table,BLOCK_SIZE)){
            err = READ_BLK_ERR;
            return err;
        }
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
    err = BLK_EOF;
    return err;
}

int new_file(const char name[16]){
    size_t b = strnlen(name,16);
    char name_padded[16] = {0};
    memcpy(name_padded,name,b);
    size_t blocks = 0;
    unsigned char name_table[BLOCK_SIZE] = {0};
    while((blocks = get_nextblock(blocks))){
        chk_err_e();
        if(read_blk(blocks*BLOCK_SIZE,name_table,BLOCK_SIZE)){
            err = READ_BLK_ERR;
            return err;
        }
        size_t i = 0;
        while(1){
            size_t b = strnlen(((name_file*)name_table)[i].name,16); // check for dups
            if(!b){
                name_file  tmp;
                memcpy(tmp.name,name_padded,16);
                tmp.index = add_block();
                chk_err_e();
                ((name_file*)name_table)[i] = tmp;
                if(write_blk(blocks*BLOCK_SIZE,name_table,BLOCK_SIZE)){
                    err = READ_BLK_ERR;
                    return err;
                }
                break;
            }
            i++;
            if(i*sizeof(name_file) >= BLOCK_SIZE) 
                break;
        }
    }
    return ERR_OK;
}

int del_file(const char name[16]){
    size_t b = strnlen(name,16);
    char name_padded[16] = {0};
    memcpy(name_padded,name,b);
    size_t blocks = 0;
    unsigned char name_table[BLOCK_SIZE] = {0};
    while((blocks = get_nextblock(blocks))){
        chk_err_e();
        if(read_blk(blocks*BLOCK_SIZE,name_table,BLOCK_SIZE)){
            err = READ_BLK_ERR;
            return err;
        }
        size_t i = 0;
        while(1){
            size_t b = strnlen(((name_file*)name_table)[i].name,16); // check for dups
            if(b == strnlen(name,16))
                if(!strncmp(name,((name_file*)name_table)[i].name,16)){
                    del_block(((name_file*)name_table)[i].index);
                    chk_err_e();
                    name_file  tmp;
                    memset(tmp.name,0,16);
                    tmp.index = 1;
                    ((name_file*)name_table)[i] = tmp;
                    if(write_blk(blocks*BLOCK_SIZE,name_table,BLOCK_SIZE)){
                        err = READ_BLK_ERR;
                        return err;
                    }
                    break;
                }
            i++;
            if(i*sizeof(name_file) >= BLOCK_SIZE)
                break;
        }
    }
    return ERR_OK;
}
size_t read_file(const char * file_name,void *buf,size_t count,size_t offset){
    int err = 0;
    size_t blk = get_file_block(file_name);
    chk_err_e();
    unsigned char BLOCKS[BLOCK_SIZE] = {0};
    while((blk = get_nextblock(blk))){
        chk_err_e();
        if(read_blk(blk*BLOCK_SIZE,BLOCKS,BLOCK_SIZE)){
            err = READ_BLK_ERR;
            return err;
        }
        int cpy_len = fmin(count, BLOCK_SIZE);
        count -= cpy_len;
        memcpy(buf,BLOCKS,cpy_len);
        buf+=cpy_len;
        if(!count)break;
    }
    return ERR_OK;
}
size_t write_file(const char * file_name,void *buf,size_t count,size_t offset){
    size_t blk = get_file_block(file_name);
    chk_err_e();
    unsigned char BLOCKS[BLOCK_SIZE] = {0};
    while(count){
        size_t tmp;
        do{
            tmp = get_nextblock(blk);
            chk_err_e();
            if(!tmp) extend_blocks(blk);
            chk_err_e();
        } while(!tmp);
        blk=tmp;
        if(read_blk(blk*BLOCK_SIZE,BLOCKS,BLOCK_SIZE)){
            err = READ_BLK_ERR;
            return err;
        }
        int cpy_len = fmin(count, BLOCK_SIZE);
        count -= cpy_len;
        memcpy(BLOCKS,buf,cpy_len);
        buf+=cpy_len;
        if(write_blk(blk*BLOCK_SIZE,BLOCKS,BLOCK_SIZE)){
            err = READ_BLK_ERR;
            return err;
        }
    }
    return ERR_OK;
}
void print_fat(){
    unsigned char fat[BLOCK_SIZE] = {0};
    read_blk(0,fat,BLOCK_SIZE);
    for(size_t i = 0;i < TABLE_LEN;i++)
        printf("%ld,",((size_t*)fat)[i]);
    puts("\n");
    for(size_t i = 0;i < TABLE_LEN;i++)
        printf("%ld,",i);
    puts("\n");
}

void print_file_table(){
    size_t blocks = 0;
    unsigned char name_table[BLOCK_SIZE] = {0};
    while((blocks = get_nextblock(blocks))){
        read_blk(blocks*BLOCK_SIZE,name_table,BLOCK_SIZE); //err check
        size_t i = 0;
        while(1){
            if(((name_file*)name_table)[i].index == 0)
                break;
            if(((name_file*)name_table)[i].index == 1){
                i++;
                if(i*sizeof(name_file) >= BLOCK_SIZE)
                    break;
                continue;
            }
            printf("%s,%ld",((name_file*)name_table)[i].name,((name_file*)name_table)[i].index);
            {
                size_t blk = ((name_file*)name_table)[i].index;
                while((blk = get_nextblock(blk)))
                    printf(",%ld",blk);
                puts("\n");
            }
            i++;
            if(i*sizeof(name_file) >= BLOCK_SIZE)
                break;
        }
    }
}
