#define FUSE_USE_VERSION 34

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include "aaFat.h"
#define min(a,b) (((a)>(b))?(b):(a))


static int aafat_getattr(const char *name, struct stat *stbuf, struct fuse_file_info *fi)
{
	(void) fi;
	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(name, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		return 0;
	}
	if(get_file_exists(name+1)){// starts with /
		return -ENOENT;
	}

	stbuf->st_ino = get_index_file(name+1);
	int ret = FAT_ERRpop();
	if (ret)
		return ret;
	stbuf->st_mode = S_IFREG | 0644;
	stbuf->st_nlink = 1;
	stbuf->st_size = get_file_size(name+1);
	ret = FAT_ERRpop();
	if (ret)
		return ret;
	stbuf->st_uid = getuid();
	stbuf->st_gid = getgid();
	stbuf->st_blocks = 0;
	stbuf->st_atime = stbuf->st_mtime = stbuf->st_ctime = 0;
	return 0;
}

static int aafat_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{
	(void) offset;
	(void) fi;
	(void) flags;

	if (strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0, 0);
	filler(buf, "..", NULL, 0, 0);
	size_t fcount = file_count();
	int ret = FAT_ERRpop();
	if (ret)
		return ret;
	for(size_t i = 0;i < fcount;i++){
		name_file tmp;
		get_file_index(&tmp,i);
		int ret = FAT_ERRpop();
		if (ret)
			return ret;
		filler(buf, tmp.name, NULL, 0, 0);
	}
	return 0;
}

static int aafat_read(const char *name, char *buf, size_t size, off_t offset,struct fuse_file_info *fi)
{
	(void) fi;
	size_t sz = get_file_size(name+1);
	if(read_file(name+1,buf,min(sz-offset,size),offset) != 0)
		return -1;
	return min(sz-offset,size);
}

static int aafat_write(const char *name,const char *data, size_t size, off_t off, struct fuse_file_info *fi)
{	(void) fi;
	if(set_file_size(name+1,size+off) != 0)
		return -1;
	if(write_file(name+1,(void*)data,size,off) != 0)
		return -1;
	return size;
}

static int aafat_create(const char * name, mode_t mode, struct fuse_file_info * fi)
{
	int ret = new_file(name+1);
	if(ret)
		print_ERR();
	return ret;
}

static int aafat_unlink(const char *name)
{
	int ret = del_file(name+1);
	if(ret)
		print_ERR();
	return ret;
}

static int aafat_open(const char *path, struct fuse_file_info *fi)
{
	return 0;
}

static const struct fuse_operations aafat_oper = {
	.getattr	= aafat_getattr,
	.readdir	= aafat_readdir,
	.open		= aafat_open,
	.read		= aafat_read,
	.write		= aafat_write,
	.create		= aafat_create,
	.unlink		= aafat_unlink,
};

FILE *fp;

int main(int argc, char *argv[])
{
	fp = fopen("fs.dat","rb+");
	if (!fp){
		fp = fopen("fs.dat","wb+");
		if (!fp){
			printf("Cant open fs.dat\n");
			return 1;
		}
	}
	validate_FAT();
	int ret = FAT_ERRpop();
	if(ret){
		write_FAT();
		ret = FAT_ERRpop();
		if (ret)
		{
			printf("Exit with %d.\n",ret);
			return ret;
		}
	}
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	ret = fuse_main(args.argc, args.argv, &aafat_oper, NULL);
	fuse_opt_free_args(&args);
	return ret;
}

int write_blk(size_t offset, unsigned char *mem){
	fseek(fp, offset*BLOCK_SIZE, SEEK_SET);
	fwrite(mem, 1, BLOCK_SIZE, fp);
	return 0;
}

int read_blk(size_t offset, unsigned char *mem){
	fseek(fp, offset*BLOCK_SIZE, SEEK_SET);
	fread(mem, 1, BLOCK_SIZE, fp);
	return 0;
}
