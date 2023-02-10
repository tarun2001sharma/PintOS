#ifndef VM_STRUCT_H
#define VM_STRUCT_H

#include <stdio.h>
#include <string.h>
#include <bitmap.h>
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/block.h"
#include "threads/pte.h"

#include "vm/frame.h"
#include "vm/page.h"
#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include "threads/malloc.h"

#include <hash.h>
#include <list.h>
#include <stdbool.h>
#include <stddef.h>

#define NO_OF_LOCKS 7
struct lock l[NO_OF_LOCKS];
#define LOCK_LOAD 0
#define LOCK_UNLOAD 1
#define LOCK_FILE 2

#define OP_LOAD 0
#define OP_UNLOAD 1
#define OP_FIND 2
#define OP_FREE 3

#define LOCK_FRAME 3
#define LOCK_EVICT 4
#define LOCK_SWAP 5
#define LOCK_MMAP 6

#define TYPE_ZERO 0
#define TYPE_FILE 1
#define TYPE_SWAP 2

struct page_struct{
	int type; 
	void *virtual_address; 
	void *physical_address; 
	bool writable; 
	uint32_t *pagedir; 
	struct list_elem frame_elem;
	size_t index; 
	bool loaded;  
	struct file *file; 
	off_t offset;  
	off_t bid; 
	size_t read_bytes; 
	size_t zero_bytes; 
};

struct hash hash_frame;
struct list hash_frame_list;

struct frame_struct{
	void *physical_address; 
	bool persistent; 
	struct list shared_pages; 
	struct list_elem frame_list_elem; 
	struct lock page_list_lock; 
	struct hash_elem hash_elem; 
};


struct block *swap_block;
struct bitmap *swap_bitmap;
size_t swap_size;

struct hash hash_mmap;
struct mmap_struct{
	mapid_t mapid;
	int fid;
	struct hash_elem frame_hash_elem; 
	struct list_elem thread_mmap_list;
	void *start_address;
	void *end_address;
};
#endif
