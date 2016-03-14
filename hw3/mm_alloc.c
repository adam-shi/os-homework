/*
 * mm_alloc.c
 *
 * Stub implementations of the mm_* routines.
 */

#include "mm_alloc.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

struct metadata* head = NULL;

struct metadata
{
	struct metadata* prev;
	struct metadata* next;
	int is_free;
	size_t size;
};

const size_t header_size = sizeof(struct metadata);

void *mm_malloc(size_t size) {
    if (size == 0) {
    	return NULL;
    }

    struct metadata header;

    if (head == NULL) {
    	struct metadata* heap_start = sbrk(header_size + size);

    	header.prev = NULL;
    	header.next = NULL;
    	header.is_free = 0;
    	header.size = size;
    	*heap_start = header;

    	head = heap_start;

    	memset((heap_start + header_size), 0, size);
    	return (void*) (heap_start + header_size);
    }

	struct metadata* new_block;
    struct metadata* block = head;

    while (block != NULL) {
    	if (block->is_free == 1 && block->size > size) {
    		if (block->size > (size + header_size)) {
    			// make a new new_block
    			struct metadata* old_next = block->next;

    			header.prev = block;
    			header.next = old_next;
    			header.is_free = 1;
    			header.size = size - block->size - header_size;

    			*(block->next) = header;
    			block->size = size;
    			block->is_free = 0;

    			memset((block + header_size), 0, size);
    			return (void*) (block + header_size);
    		} else {
    			block->is_free = 0;
    			memset((block + header_size), 0, size);
    			return (void*) (block + header_size);
    		}
    	} else if (block->next != NULL) {
    		block = block->next;
    	} else {
			new_block = sbrk(header_size + size);

    		if (new_block == (void*) -1) {
    			break;
    		}

	    	header.prev = block;
	    	header.next = NULL;
	    	header.is_free = 0;
	    	header.size = size;
	    	*new_block = header;

	    	memset((new_block + header_size), 0, size);
	    	return (void*) (new_block + header_size);   		
    	}
    }

    /* YOUR CODE HERE */
    return NULL;
}

void *mm_realloc(void *ptr, size_t size) {
    /* YOUR CODE HERE */

    return NULL;
}

void mm_free(void *ptr) {
    /* YOUR CODE HERE */

	if (ptr == NULL) {
		return;
	}

	struct metadata* cur_header = ptr - header_size;
	cur_header->is_free = 1;

	// coalesce
	if (cur_header->prev->is_free == 1) {
		struct metadata* cur_next = cur_header->next;
		size_t cur_size = cur_header->size;

		cur_header = cur_header->prev;
		cur_header->size = cur_header->size + header_size + cur_size;
		cur_header->next = cur_next;
	}
	if (cur_header->next->is_free == 1) {
		cur_header->size = cur_header->size + header_size + cur_header->next->size;
		cur_header->next = cur_header->next->next;
	}
}
