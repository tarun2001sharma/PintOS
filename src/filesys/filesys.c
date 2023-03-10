#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"

#ifdef P4FILESYS
#include "threads/thread.h"
#include "threads/malloc.h"
#endif

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format(void);

/* Initializes the file system module.
 If FORMAT is true, reformats the file system. */
void filesys_init(bool format)
{
	fs_device = block_get_role(BLOCK_FILESYS);
	if (fs_device == NULL)
		PANIC("No file system device found, can't initialize file system.");

	inode_init();

#ifdef P4FILESYS
	list_init(&list_buffer);
	lock_init(&buffer_lock);
	buffer_size = 0;
#endif

	free_map_init();

	if (format)
		do_format();

	free_map_open();
}

/* Shuts down the file system module, writing any unwritten data
 to disk. */
void filesys_done(void)
{
  #ifdef P4FILESYS

  struct list_elem *e;
	struct buffer_struct *temp_var = NULL;
	
	lock_acquire(&buffer_lock);
	for (e = list_begin(&list_buffer); e != list_end(&list_buffer); e =
			list_next(e)){
    
		temp_var = list_entry(e, struct buffer_struct, buffer_listelem);
		if (temp_var != NULL){
			if (temp_var->dirty) block_write(fs_device, temp_var->sector, &temp_var->content);
		}
		else
			PANIC("NULL value has been detected in buffer");

		list_remove(&temp_var->buffer_listelem);
	}
	free(temp_var);
	lock_release(&buffer_lock);
#endif

	free_map_close();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
 Returns true if successful, false otherwise.
 Fails if a file named NAME already exists,
 or if internal memory allocation fails. */
#ifndef P4FILESYS
bool filesys_create(const char *name, off_t initial_size)
#else
bool filesys_create(const char *name, off_t initial_size, bool isdir)
#endif
{
#ifdef P4FILESYS
    char* file_name = NULL;
	char previous[] = "..";
    struct dir *dir = NULL;
    char current[] = ".";
	filesys_inside_dir(&dir, name);
	filesys_filename(&file_name, name);
	if (strcmp(previous, file_name) == 0){
		free(file_name);
		return false;
	}
    else if(strcmp(current, file_name) == 0){
		free(file_name);
		return false;
	}
#endif

	block_sector_t inode_sector = 0;

#ifndef P4FILESYS
	struct dir *dir = dir_open_root ();
	bool success = (dir != NULL
            && inode_create (inode_sector, initial_size)
			&& free_map_allocate (1, &inode_sector)
			&& dir_add (dir, name, inode_sector));
	if (!success && inode_sector != 0)
	free_map_release (inode_sector, 1);
#else
	bool success = (dir != NULL && free_map_allocate(1, &inode_sector)
            && dir_add(dir, file_name, inode_sector)
			&& inode_create(inode_sector, initial_size, !isdir));
	if (!success && inode_sector != 0)
		free_map_release(inode_sector, 1);
#endif
	dir_close(dir);
#ifdef P4FILESYS
	free(file_name);
#endif
	return success;

}

/* Opens the file with the given NAME.
 Returns the new file if successful or a null pointer
 otherwise.
 Fails if no file named NAME exists,
 or if an internal memory allocation fails. */
struct file *
filesys_open(const char *name)
{
#ifndef P4FILESYS
    struct inode *inode = NULL;
	struct dir *dir = dir_open_root ();

	if (dir != NULL)
	dir_lookup (dir, name, &inode);
	dir_close (dir);

	return file_open (inode);
#else
    struct dir* dir = NULL;
    struct inode *inode = NULL;
	char* file_name = NULL;
	
	if (*name == '\0') return NULL;

	filesys_inside_dir(&dir, name);
	filesys_filename(&file_name, name);
	if (dir == NULL || file_name == NULL) return NULL;

	if (strstr(file_name, ".") == NULL){
		if (file_name[0] == '\0'){
			free(file_name);
			return dir;
		}
		bool success = dir_lookup(dir, file_name, &inode);
		if (success){
			dir_close(dir);
			free(file_name);
			if (inode){
				if (inode_op(OP_ISDIR, inode))
					return dir_open(inode);
				return file_open(inode);
			}
		}
		return NULL;
	}
    else if((strlen(file_name) != 1 && strlen(file_name) != 2)) {
		if (file_name[0] == '\0'){
			free(file_name);
			return dir;
		}
		bool success = dir_lookup(dir, file_name, &inode);
		if (success){
			dir_close(dir);
			free(file_name);
			if (inode){
				if (inode_op(OP_ISDIR, inode))
					return dir_open(inode);
				return file_open(inode);
			}
		}
		return NULL;
	}
	if (strcmp(file_name, ".") == 0){
		free(file_name);
		return dir;
	}
	if (strcmp(file_name, "..") == 0){
		bool success = dir_op(DIROP_GETPAR, dir, &inode, 0);
		if (success){
			free(file_name);
			if (inode){
				if (inode_op(OP_ISDIR, inode))
					return dir_open(inode);
				return file_open(inode);
			}
		}
		return NULL;
	}
#endif
}

/* Deletes the file named NAME.
 Returns true if successful, false on failure.
 Fails if no file named NAME exists,
 or if an internal memory allocation fails. */
bool filesys_remove(const char *name)
{
#ifndef P4FILESYS
	struct dir *dir = dir_open_root ();
	bool success = dir != NULL && dir_remove (dir, name);
	dir_close (dir);

	return success;
#else
    struct dir* dir = NULL;
	char* file_name = NULL;
	filesys_inside_dir(&dir, name);
	filesys_filename(&file_name, name);
	bool success = file_name != NULL && dir != NULL
			&& dir_remove(dir, file_name);
	dir_close(dir);
	free(file_name);
	return success;
#endif
}

/* Formats the file system. */
static void do_format(void)
{
	printf("Please wait. File system is formatting");
	free_map_create();
	if (!dir_create(ROOT_DIR_SECTOR, 16))
		PANIC("Root directory creation failed");
	free_map_close();
	printf("done.\n");
}

#ifdef P4FILESYS
void filesys_inside_dir(struct dir **dir, char* path)
{
    struct inode *inode;
    char prev_dir[] = "..";
	char cur_dir[] = ".";
	*dir = NULL;
	char *curr = NULL;
	char *save_ptr;
    char *next = NULL;
    
	size_t len = strlen(path) + 1;
	char *tmp = (char*) malloc((len) * sizeof(char));
	if (tmp == NULL)
		PANIC("Memory cannot be allocated. This hinders with the path of memory");
	strlcpy(tmp, path, len);

	char *ptr_b = tmp;
	if (*ptr_b == '/')
		*dir = dir_open_root();
	else
	{
		if (thread_current()->working_dir == NULL)
			*dir = dir_open_root();
		else
			*dir = dir_reopen(thread_current()->working_dir);
	}
	if (*dir == NULL)
		PANIC("Directoty is null -- containing dir");

	curr = strtok_r(tmp, "/", &save_ptr);
	if (curr == NULL)
		return;

	for (next = strtok_r(NULL, "/", &save_ptr); next != NULL;
			next = strtok_r(NULL, "/", &save_ptr))
	{
		if (strstr(curr, ".") == NULL)
		{
			bool success = dir_lookup(*dir, curr, &inode);
			if (success){
				switch (inode_op(OP_ISDIR, inode)){
                case false:
					inode_close(inode);
					break;
				case true:
					dir_close(*dir);
					*dir = dir_open(inode);
					break;
				default:
					PANIC("problem in containing dir");
				}
				curr = next;
				continue;
			}
			*dir = NULL;
			return;
		}

		if (strcmp(cur_dir, curr) == 0){
			curr = next;
			continue;
		}

		if (strcmp(prev_dir, curr) == 0){
			bool success = dir_op(DIROP_GETPAR,*dir,&inode, 0);
			if (success){
				switch (inode_op(OP_ISDIR, inode))
				{
                case false:
					inode_close(inode);
					break;
				case true:
					dir_close(*dir);
					*dir = dir_open(inode);
					break;
				default:
					PANIC("problem in containing dir");
				}
				curr = next;
				continue;
			}
			*dir = NULL;
			return;
		}
	}
	return;
}

void filesys_filename(char** fname, char* path)
{
    char *file = path;
	*fname = NULL;
	
	size_t len = strlen(path) + 1;
	for (char *temp = path; *temp != '\0'; temp++){
		if (*temp == '/'){
			file = temp + 1;
			len = strlen(file) + 1;
		}
		else
			continue;
	}
	*fname = (char*) malloc((len) * sizeof(char));
	if (*fname == NULL)
		PANIC("Cannot allocate memory");
	strlcpy(*fname, file, len);
}

struct buffer_struct* buffer_evict(uint32_t sector){
	struct buffer_struct *bc = NULL;

	if (buffer_size > BUFFER_SIZE){
		for (struct list_elem *e = list_begin(&list_buffer);
				e != list_end(&list_buffer); e = list_next(e))
		{
			if (list_entry(e, struct buffer_struct, buffer_listelem)->dirty
					&& !list_entry(e, struct buffer_struct, buffer_listelem)->access)
			{
				bc = list_entry(e, struct buffer_struct, buffer_listelem);
				block_write(fs_device, bc->sector, &bc->content);
				break;
			}
			else if (list_entry(e, struct buffer_struct, buffer_listelem)->access)
			{
				list_entry(e, struct buffer_struct, buffer_listelem)->access =
				false;
			}
		}
		if (bc != NULL){
			bc->sector = sector;
			block_read(fs_device, bc->sector, &bc->content);
			return bc;
		}
		return NULL;
	}
	bc = (struct buffer_struct *) malloc(sizeof(struct buffer_struct));
	if (bc != NULL){
		list_push_back(&list_buffer, &bc->buffer_listelem);
		bc->sector = sector;
		block_read(fs_device, bc->sector, &bc->content);
		buffer_size += 1;
		return bc;
	}
	return NULL;

}
#endif