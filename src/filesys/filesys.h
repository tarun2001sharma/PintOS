#ifndef FILESYS_FILESYS_H
#define FILESYS_FILESYS_H

#include <stdbool.h>
#include "filesys/off_t.h"
#ifdef P4FILESYS
#include "filesys/directory.h"
#include "threads/synch.h"
#include <list.h>
#endif

/* Sectors of system file inodes. */
#define FREE_MAP_SECTOR 0       /* Free map file inode sector. */
#define ROOT_DIR_SECTOR 1       /* Root directory file inode sector. */

#ifdef P4FILESYS
#define BUFFER_SIZE 64
struct lock buffer_lock;
struct list list_buffer;
uint32_t buffer_size;

struct buffer_struct* buffer_evict(uint32_t sector);

struct buffer_struct{
	uint8_t content[BLOCK_SECTOR_SIZE];
	struct list_elem buffer_listelem;
    bool dirty;
	bool access;
    uint32_t sector;
};

#endif

/* Block device that contains the file system. */
struct block *fs_device;

void filesys_init(bool format);
void filesys_done(void);

#ifndef P4FILESYS
bool filesys_create(const char *name, off_t initial_size);
#else
bool filesys_create(const char *name, off_t initial_size, bool isdir);
#endif

struct file *filesys_open(const char *name);
bool filesys_remove(const char *name);

#ifdef P4FILESYS
void filesys_inside_dir(struct dir **dir, char *path);
void filesys_filename(char **filename, char *path);
#endif

#endif /* filesys/filesys.h */