#include "my_alloc.h"
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define ALIGN 16
#define HEADER_SIZE 32
#define FOOTER_SIZE 8
#define MIN_BLOCK_SIZE (HEADER_SIZE + FOOTER_SIZE + ALIGN)
#define LARGE_ALLOC_THRESHOLD (1024 * 1024)
#define HEAP_INIT_SIZE (64 * 1024 * 1024)

typedef struct block_header {
    size_t size;
    uint8_t is_free;
    uint8_t _pad[7];
    struct block_header *prev;
    struct block_header *next;
} block_header_t;

typedef struct {
    void *heap_start;
    size_t heap_size;
    block_header_t *free_list_head;
    size_t current_in_use;
    size_t peak_usage;
    size_t total_allocated;
    size_t num_free_blocks;
    size_t num_alloc_blocks;
} heap_t;

heap_t g_heap = {0};

static size_t align_up(size_t size) {
    return (size + ALIGN - 1) & ~(ALIGN - 1);
}

static size_t *get_footer(block_header_t *block) {
    return (size_t *)((char *)block + HEADER_SIZE + block->size);
}

static void unlink_block(block_header_t *block) {
    if (block->prev) {
        block->prev->next = block->next;
    } else {
        g_heap.free_list_head = block->next;
    }
    if (block->next) {
        block->next->prev = block->prev;
    }
    block->prev = NULL;
    block->next = NULL;
    g_heap.num_free_blocks--;
}

static void insert_head(block_header_t *block) {
    block->next = g_heap.free_list_head;
    block->prev = NULL;
    if (g_heap.free_list_head) {
        g_heap.free_list_head->prev = block;
    }
    g_heap.free_list_head = block;
    g_heap.num_free_blocks++;
}

static void init_heap() {
    g_heap.heap_start = mmap(NULL, HEAP_INIT_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (g_heap.heap_start == MAP_FAILED) {
        g_heap.heap_start = NULL;
        return;
    }
    g_heap.heap_size = HEAP_INIT_SIZE;

    block_header_t *first_block = (block_header_t *)g_heap.heap_start;
    first_block->size = HEAP_INIT_SIZE - HEADER_SIZE - FOOTER_SIZE;
    first_block->is_free = 1;
    first_block->prev = NULL;
    first_block->next = NULL;

    size_t *footer = get_footer(first_block);
    *footer = first_block->size;

    g_heap.free_list_head = first_block;
    g_heap.num_free_blocks = 1;
}

static void split_block(block_header_t *block, size_t req_size) {
    (void)block;
    (void)req_size;
    /* TODO: To be implemented - Block splitting logic */
}

void *my_malloc(size_t size) {
    if (size == 0) return NULL;
    
    size_t aligned_size = align_up(size);

    if (!g_heap.heap_start) {
        init_heap();
        if (!g_heap.heap_start) return NULL;
    }

    block_header_t *current = g_heap.free_list_head;
    while (current) {
        if (current->size >= aligned_size) {
            unlink_block(current);
            current->is_free = 0;
            
            split_block(current, aligned_size);
            
            g_heap.current_in_use += current->size;
            g_heap.total_allocated += current->size;
            g_heap.num_alloc_blocks++;
            if (g_heap.current_in_use > g_heap.peak_usage) {
                g_heap.peak_usage = g_heap.current_in_use;
            }
            
            return (char *)current + HEADER_SIZE;
        }
        current = current->next;
    }
    
    return NULL; 
}

void my_free(void *ptr) {
    if (!ptr) return;

    block_header_t *block = (block_header_t *)((char *)ptr - HEADER_SIZE);
    if (block->is_free) return;

    block->is_free = 1;
    g_heap.current_in_use -= block->size;
    g_heap.num_alloc_blocks--;

    /* TODO: To be implemented - Eager coalescing (forward and backward merging) */

    insert_head(block);
}

void *my_calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    if (nmemb != 0 && total / nmemb != size) return NULL;
    
    void *ptr = my_malloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void *my_realloc(void *ptr, size_t size) {
    (void)ptr;
    (void)size;
    /* TODO: To be implemented - Realloc logic */
    return NULL;
}
