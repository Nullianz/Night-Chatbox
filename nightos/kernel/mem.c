/* Kernel heap: first-fit allocator with block splitting and coalescing. */
#include "kernel.h"

typedef struct blk { uint32_t size; uint32_t free; struct blk *next; uint32_t magic; } blk;
#define MAGIC 0x4E494748 /* "NIGH" */
static blk *head;
static uint32_t heap_total;

void heap_init(uint32_t start, uint32_t end) {
    start = (start + 15) & ~15u;
    head = (blk *)start;
    head->size = end - start - sizeof(blk);
    head->free = 1; head->next = 0; head->magic = MAGIC;
    heap_total = head->size;
}

void *kmalloc(size_t n) {
    if (!n) n = 1;
    n = (n + 15) & ~15u;
    for (blk *b = head; b; b = b->next) {
        if (!b->free || b->size < n) continue;
        if (b->size >= n + sizeof(blk) + 32) {
            blk *nb = (blk *)((uint8_t *)(b + 1) + n);
            nb->size = b->size - n - sizeof(blk);
            nb->free = 1; nb->next = b->next; nb->magic = MAGIC;
            b->next = nb; b->size = n;
        }
        b->free = 0;
        return b + 1;
    }
    return 0;
}

void *kzalloc(size_t n) { void *p = kmalloc(n); if (p) memset(p, 0, n); return p; }

void kfree(void *p) {
    if (!p) return;
    blk *b = (blk *)p - 1;
    if (b->magic != MAGIC || b->free) return; /* ignore bad/double frees */
    b->free = 1;
    for (blk *x = head; x; x = x->next)
        while (x->free && x->next && x->next->free) {
            x->size += sizeof(blk) + x->next->size;
            x->next = x->next->next;
        }
}

void *krealloc(void *p, size_t n) {
    if (!p) return kmalloc(n);
    blk *b = (blk *)p - 1;
    if (b->size >= n) return p;
    void *q = kmalloc(n);
    if (!q) return 0;
    memcpy(q, p, b->size);
    kfree(p);
    return q;
}

void heap_stats(uint32_t *total, uint32_t *used, uint32_t *blocks) {
    uint32_t u = 0, c = 0;
    for (blk *b = head; b; b = b->next) { c++; if (!b->free) u += b->size; }
    *total = heap_total; *used = u; *blocks = c;
}
