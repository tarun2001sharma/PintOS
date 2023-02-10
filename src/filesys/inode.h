#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "devices/block.h"

#include <list.h>
#define OP_ISDIR 0
#define OP_PARENT 1
#define OP_OPENCNT 2

#define I0 0
#define I1 1
#define I2 2
#define I3 3
#define PAR 4
#define DIR 5
#define LEN 6
#define SIZE 7

#define MEM_SIZE (115 * sizeof(uint32_t))
char zeros[BLOCK_SECTOR_SIZE];

#define I0_BLOCKS 113
#define MAX_FILESIZE 8512000
struct inode_disk{
	block_sector_t start; 
	off_t length; 
	unsigned magic;
#ifndef P4FILESYS
	uint32_t unused[125];
#else
	uint32_t inode_data[10];
	block_sector_t block[115];
#endif
};
struct inode{
	struct list_elem elem;
	block_sector_t sector;
	int open_cnt;
	bool removed;
	int deny_write_cnt;
	struct inode_disk data;

#ifdef P4FILESYS
	uint32_t inode_data[10];
	uint32_t block[115];
#endif
};

struct bitmap;

void inode_init(void);
#ifndef P4FILESYS
bool inode_create(block_sector_t, off_t);
#else
bool inode_create(block_sector_t, off_t, bool);
#endif
struct inode *inode_open(block_sector_t);
struct inode *inode_reopen(struct inode *);
block_sector_t inode_get_inumber(const struct inode *);
void inode_close(struct inode *);
void inode_remove(struct inode *);
void inode_expand(struct inode *inode, off_t length, size_t new_data_sectors);
off_t inode_read_at(struct inode *, void *, off_t size, off_t offset);
off_t inode_write_at(struct inode *, const void *, off_t size, off_t offset);
void inode_deny_write(struct inode *);
void inode_allow_write(struct inode *);
off_t inode_length(struct inode *);

#endif /* filesys/inode.h */
