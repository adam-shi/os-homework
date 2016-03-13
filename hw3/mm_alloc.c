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

    if (head == NULL) {
    	struct metadata* heap_start = sbrk(header_size + size);

    	struct metadata header;
    	header.prev = NULL;
    	header.next = NULL;
    	header.is_free = 0;
    	header.size = size;
    	*heap_start = header;

    	head = heap_start;

    	memset((heap_start + header_size), 0, size);
    	return (void*) (heap_start + header_size);
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
}
