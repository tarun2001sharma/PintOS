#include "vm/struct.h"

void *VM_get_frame(void *frame, uint32_t *pagedir, enum palloc_flags flags)
{
	if (frame == NULL)
	{
		struct frame_struct *virtual_frame = NULL;
		void *address = palloc_get_page(flags);

		if (address == NULL)
		{
			evict();
			return VM_get_frame(NULL, NULL, flags);
		}

		virtual_frame = (struct frame_struct *) malloc(sizeof(struct frame_struct));
		if (virtual_frame != NULL)
		{
			virtual_frame->physical_address = address;
			virtual_frame->persistent = true;
			list_init(&virtual_frame->shared_pages);
			lock_init(&virtual_frame->page_list_lock);

			lock_acquire(&l[LOCK_FRAME]);
			list_push_front(&hash_frame_list, &virtual_frame->frame_list_elem);
			hash_insert(&hash_frame, &virtual_frame->hash_elem);
			lock_release(&l[LOCK_FRAME]);
		}
		return address;
	}
	else{
		struct page_struct *page = NULL;
		struct frame_struct *virtual_frame = NULL;
		struct list_elem *element_;
		
		virtual_frame = address_to_frame(frame);
		if (virtual_frame != NULL)
		{
			lock_acquire(&virtual_frame->page_list_lock);
			element_ = list_begin(&virtual_frame->shared_pages);
			while (element_ != list_end(&virtual_frame->shared_pages))
			{
				page = list_entry(element_, struct page_struct, frame_elem);
				if (page->pagedir != pagedir)
				{
					element_ = list_next(element_);
					continue;
				}
				break;
			}
			lock_release(&virtual_frame->page_list_lock);
		}
		return page;
	}

}

void VM_free_frame(void *address, uint32_t *pagedir)
{
	lock_acquire(&l[LOCK_EVICT]);
	
	struct list_elem *e;
	struct page_struct *page = NULL;
	struct frame_struct *virtual_frame = NULL;

	virtual_frame = address_to_frame(address);
	if (virtual_frame == NULL){
		lock_release(&l[LOCK_EVICT]);
		return;
	}

	if (pagedir == NULL){

		lock_acquire(&virtual_frame->page_list_lock);
		while (true){
			if (list_empty(&virtual_frame->shared_pages))
				break;
			e = list_begin(&virtual_frame->shared_pages);
			page = list_entry(e, struct page_struct, frame_elem);
			list_remove(&page->frame_elem);
			VM_operation_page(OP_UNLOAD, page, virtual_frame->physical_address, false);
		}
		lock_release(&virtual_frame->page_list_lock);
	}
	else{

		page = VM_get_frame(address, pagedir, PAL_USER);

		if (page != NULL)
		{
			lock_acquire(&virtual_frame->page_list_lock);
			list_remove(&page->frame_elem);
			lock_release(&virtual_frame->page_list_lock);
			VM_operation_page(OP_UNLOAD, page, virtual_frame->physical_address, false);
		}
	}

	if (!list_empty(&virtual_frame->shared_pages)){
		lock_release(&l[LOCK_EVICT]);
		return;
	}

	lock_acquire(&l[LOCK_FRAME]);
	hash_delete(&hash_frame, &virtual_frame->hash_elem);
	list_remove(&virtual_frame->frame_list_elem);
	free(virtual_frame);
	lock_release(&l[LOCK_FRAME]);
	palloc_free_page(address);

	lock_release(&l[LOCK_EVICT]);
}

bool eviction_clock(struct frame_struct *f){
	struct list_elem *ele = list_begin(&f->shared_pages);
	while (ele != list_end(&f->shared_pages))
	{
		struct page_struct *page = list_entry(ele, struct page_struct,
				frame_elem);
		if (page != NULL)
			if (pagedir_is_accessed(page->pagedir, page->virtual_address))
			{
				pagedir_set_accessed(page->pagedir, page->virtual_address,
				false);
				return false;
			}
		ele = list_next(ele);
	}
	return true;
}

void evict()
{
	lock_acquire(&l[LOCK_EVICT]);
	lock_acquire(&l[LOCK_FRAME]);
	struct frame_struct *frame_to_evict = NULL;
	struct list_elem *e = list_end(&hash_frame_list);
	struct frame_struct *cur_frame = list_entry(e, struct frame_struct,
			frame_list_elem);

	while (true)
	{
		cur_frame = list_entry(e, struct frame_struct, frame_list_elem);
		if (cur_frame == NULL)
			PANIC("Wrong eviction");

		if (cur_frame->persistent == true)
		{
			if (e == NULL || e == list_begin(&hash_frame_list))
				e = list_end(&hash_frame_list);
			else
				e = list_prev(e);
			continue;
		}
		else if (!eviction_clock(cur_frame)){
			if (e == NULL || e == list_begin(&hash_frame_list))
				e = list_end(&hash_frame_list);
			else
				e = list_prev(e);
			continue;
		}

		frame_to_evict = cur_frame;
		break;
	}

	lock_release(&l[LOCK_FRAME]);
	lock_release(&l[LOCK_EVICT]);
	VM_free_frame(frame_to_evict->physical_address, NULL);
}

struct frame_struct *address_to_frame(void *address)
{
	struct frame_struct f;
	f.physical_address = address;

	if (f.physical_address != NULL)
	{
		lock_acquire(&l[LOCK_FRAME]);
		struct hash_elem *e;
		e = hash_find(&hash_frame, &f.hash_elem);
		lock_release(&l[LOCK_FRAME]);
		if (e == NULL)
			return NULL;
		else
		{
			e = hash_entry(e, struct frame_struct, hash_elem);
			return e;
		}
	}
	return NULL;
}