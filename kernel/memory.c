/* ♥ SPINLOCK ~ Lock atomico! "__sync_lock_test_and_set protege tudo~"
 * mem_lock/mem_unlock com __sync_lock_test_and_set + pause
 * Protege todas as operacoes do buddy allocator e SLUB!
 * Single-core safe ~ se travar, culpe o scheduler! moe~ */









#include "memory.h"
#include <stdint.h>

#define HEAP_PHYS    0x900000u
#define HEAP_SIZE    (64u * 1024u * 1024u)
#define FRAME_SIZE   4096u
#define FRAME_SHIFT  12u
#define FRAME_COUNT  (HEAP_SIZE / FRAME_SIZE)
#define MAX_ORDER    14u

#define FRAME_ALLOC  0x80u
#define FRAME_USER   0x40u
#define FRAME_ORDER  0x3Fu

#define SLAB_MAGIC   0x534C4142u
#define CACHE_COUNT  8u

static uint8_t frames[FRAME_COUNT];

typedef struct free_block {
    struct free_block *next;
    struct free_block *prev;
} free_block_t;

static free_block_t *free_area[MAX_ORDER + 1];

typedef struct kmem_cache kmem_cache_t;

typedef struct slab {
    uint32_t      magic;
    uint32_t      in_use;
    uint16_t      total;
    void         *freelist;
    struct slab  *next;
    struct slab  *prev;
    kmem_cache_t *cache;
} slab_t;

struct kmem_cache {
    size_t  obj_size;
    slab_t *slabs;
    int     obj_per_slab;
};

static kmem_cache_t kmalloc_caches[CACHE_COUNT];

static volatile int memlock = 0;

static inline void mem_lock(void) {
    while (__sync_lock_test_and_set(&memlock, 1))
        __asm__ volatile("pause");
    __asm__ volatile("" ::: "memory");
}

static inline void mem_unlock(void) {
    __asm__ volatile("" ::: "memory");
    __sync_lock_release(&memlock);
}

static inline size_t addr_to_frame(uint64_t addr) {
    return (addr - HEAP_PHYS) >> FRAME_SHIFT;
}

static inline uint64_t frame_to_addr(size_t idx) {
    return HEAP_PHYS + (idx << FRAME_SHIFT);
}

static void list_add(free_block_t **head, free_block_t *blk) {
    blk->next = *head;
    blk->prev = NULL;
    if (*head) (*head)->prev = blk;
    *head = blk;
}

static void list_remove(free_block_t **head, free_block_t *blk) {
    if (blk->prev)
        blk->prev->next = blk->next;
    else
        *head = blk->next;
    if (blk->next)
        blk->next->prev = blk->prev;
}

static void *buddy_alloc(int order, int user) {
    if (order < 0 || order > MAX_ORDER) return NULL;
    mem_lock();
    int found_order = -1;
    for (int o = order; o <= MAX_ORDER; o++) {
        if (free_area[o]) { found_order = o; break; }
    }
    if (found_order < 0) { mem_unlock(); return NULL; }

    free_block_t *block = free_area[found_order];
    list_remove(&free_area[found_order], block);
    size_t block_idx = addr_to_frame((uint64_t)block);

    for (int o = found_order; o > order; o--) {
        size_t buddy_idx = block_idx ^ (1UL << (o - 1));
        free_block_t *buddy = (free_block_t *)frame_to_addr(buddy_idx);
        frames[buddy_idx] = (uint8_t)(o - 1);
        list_add(&free_area[o - 1], buddy);
    }

    frames[block_idx] = (uint8_t)(order | FRAME_ALLOC);
    if (user) frames[block_idx] |= FRAME_USER;
    mem_unlock();
    return (void *)frame_to_addr(block_idx);
}

static void buddy_free(void *addr, int order) {
    if (!addr || order < 0 || order > MAX_ORDER) return;
    uint64_t phys = (uint64_t)addr;
    if (phys < HEAP_PHYS || phys >= HEAP_PHYS + HEAP_SIZE) return;
    if (phys & (FRAME_SIZE - 1)) return;
    mem_lock();
    size_t idx = addr_to_frame(phys);
    frames[idx] = (uint8_t)order;
    while (order < MAX_ORDER) {
        size_t buddy_idx = idx ^ (1UL << order);
        if ((frames[buddy_idx] & (FRAME_ALLOC | FRAME_ORDER)) == (uint8_t)order) {
            free_block_t *buddy = (free_block_t *)frame_to_addr(buddy_idx);
            list_remove(&free_area[order], buddy);
            idx = (idx < buddy_idx) ? idx : buddy_idx;
            order++;
            frames[idx] = (uint8_t)order;
        } else {
            break;
        }
    }
    free_block_t *block = (free_block_t *)frame_to_addr(idx);
    list_add(&free_area[order], block);
    mem_unlock();
}

static void *slab_alloc(kmem_cache_t *cache) {
    if (!cache) return NULL;
    slab_t *s = cache->slabs;
    while (s) {
        if (s->freelist) {
            void *obj = s->freelist;
            s->freelist = *(void **)obj;
            s->in_use++;
            return obj;
        }
        s = s->next;
    }
    void *page = buddy_alloc(0, 0);
    if (!page) return NULL;
    s = (slab_t *)page;
    s->magic   = SLAB_MAGIC;
    s->in_use  = 0;
    s->total   = cache->obj_per_slab;
    s->cache   = cache;
    s->next    = cache->slabs;
    s->prev    = NULL;
    if (cache->slabs) cache->slabs->prev = s;
    cache->slabs = s;
    char *data = (char *)s + sizeof(slab_t);
    char *end  = (char *)s + FRAME_SIZE;
    void *head = NULL;
    int count = 0;
    while (data + cache->obj_size <= end) {
        *(void **)data = head;
        head = data;
        data += cache->obj_size;
        count++;
    }
    s->freelist = head;
    s->total = count;
    void *obj = s->freelist;
    s->freelist = *(void **)obj;
    s->in_use = 1;
    return obj;
}

static void slab_free(void *ptr) {
    if (!ptr) return;
    slab_t *s = (slab_t *)((uint64_t)ptr & ~(FRAME_SIZE - 1));
    if (s->magic != SLAB_MAGIC) return;
    *(void **)ptr = s->freelist;
    s->freelist = ptr;
    s->in_use--;
    if (s->in_use == 0) {
        kmem_cache_t *cache = s->cache;
        if (s->next) s->next->prev = s->prev;
        if (s->prev) s->prev->next = s->next;
        else if (cache) cache->slabs = s->next;
        buddy_free(s, 0);
    }
}

void memory_init(void) {
    mem_lock();
    for (int i = 0; i <= MAX_ORDER; i++)
        free_area[i] = NULL;
    for (size_t i = 0; i < FRAME_COUNT; i++)
        frames[i] = 0;
    free_block_t *block = (free_block_t *)(uint64_t)HEAP_PHYS;
    block->next = NULL;
    block->prev = NULL;
    free_area[MAX_ORDER] = block;
    frames[0] = MAX_ORDER;
    static const size_t sizes[CACHE_COUNT] = {
        16, 32, 64, 128, 256, 512, 1024, 2048
    };
    for (int i = 0; i < CACHE_COUNT; i++) {
        kmalloc_caches[i].obj_size    = sizes[i];
        kmalloc_caches[i].slabs       = NULL;
        kmalloc_caches[i].obj_per_slab =
            (int)((FRAME_SIZE - sizeof(slab_t)) / sizes[i]);
    }
    mem_unlock();
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    if (size < 16) size = 16;
    if (size >= 2048) {
        size_t pages = (size + FRAME_SIZE - 1) / FRAME_SIZE;
        int order = 0;
        while ((1UL << order) < pages) order++;
        return buddy_alloc(order, 0);
    }
    for (int i = 0; i < CACHE_COUNT; i++) {
        if (kmalloc_caches[i].obj_size >= size)
            return slab_alloc(&kmalloc_caches[i]);
    }
    return NULL;
}

void kfree(void *ptr) {
    if (!ptr) return;
    uint64_t addr = (uint64_t)ptr;
    if (addr < HEAP_PHYS || addr >= HEAP_PHYS + HEAP_SIZE) return;
    slab_t *s = (slab_t *)(addr & ~(FRAME_SIZE - 1));
    if (s->magic == SLAB_MAGIC) { slab_free(ptr); return; }
    if (addr & (FRAME_SIZE - 1)) return;
    size_t idx = addr_to_frame(addr);
    if (idx >= FRAME_COUNT) return;
    if (!(frames[idx] & FRAME_ALLOC)) return;
    int order = frames[idx] & FRAME_ORDER;
    buddy_free(ptr, order);
}

void *kcalloc(size_t count, size_t size) {
    size_t total = count * size;
    void *p = kmalloc(total);
    if (p) {
        size_t alloc_size = total;
        if (total < 2048) {
            for (int i = 0; i < CACHE_COUNT; i++) {
                if (kmalloc_caches[i].obj_size >= total) {
                    alloc_size = kmalloc_caches[i].obj_size;
                    break;
                }
            }
        } else {
            size_t pages = (total + FRAME_SIZE - 1) / FRAME_SIZE;
            int order = 0;
            while ((1UL << order) < pages) order++;
            alloc_size = (1UL << order) * FRAME_SIZE;
        }
        for (size_t i = 0; i < alloc_size; i++)
            ((uint8_t *)p)[i] = 0;
    }
    return p;
}

void *mmap_user(void *addr, size_t length, int prot, int flags) {
    (void)addr; (void)prot; (void)flags;
    size_t pages = (length + FRAME_SIZE - 1) / FRAME_SIZE;
    if (pages == 0) pages = 1;
    int order = 0;
    while ((1UL << order) < pages) order++;
    return buddy_alloc(order, 1);
}

int munmap_user(void *addr, size_t length) {
    if (!addr) return 0;
    uint64_t a = (uint64_t)addr;
    if (a & (FRAME_SIZE - 1)) return -1;
    size_t idx = addr_to_frame(a);
    if (idx >= FRAME_COUNT) return -1;
    if (!(frames[idx] & FRAME_ALLOC)) return 0;
    int order = frames[idx] & FRAME_ORDER;
    buddy_free(addr, order);
    return 0;
}

void munmap_all_user(void) {
    mem_lock();
    for (size_t i = 0; i < FRAME_COUNT; ) {
        uint8_t f = frames[i];
        if ((f & (FRAME_ALLOC | FRAME_USER)) == (FRAME_ALLOC | FRAME_USER)) {
            int order = f & FRAME_ORDER;
            frames[i] = (uint8_t)order;
            size_t idx = i;
            int ord = order;
            while (ord < MAX_ORDER) {
                size_t buddy_idx = idx ^ (1UL << ord);
                if ((frames[buddy_idx] & (FRAME_ALLOC | FRAME_ORDER)) == (uint8_t)ord) {
                    free_block_t *buddy = (free_block_t *)frame_to_addr(buddy_idx);
                    list_remove(&free_area[ord], buddy);
                    idx = (idx < buddy_idx) ? idx : buddy_idx;
                    ord++;
                    frames[idx] = (uint8_t)ord;
                } else {
                    break;
                }
            }
            free_block_t *blk = (free_block_t *)frame_to_addr(idx);
            list_add(&free_area[ord], blk);
            i += (1UL << order);
        } else {
            i++;
        }
    }
    mem_unlock();
}

static void *page_alloc(void) { return buddy_alloc(0, 0); }
static void page_free(void *p) { buddy_free(p, 0); }

uint64_t pml4_get_current(void) {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

uint64_t pml4_create(void) {
    uint64_t *current = (uint64_t *)pml4_get_current();
    uint64_t *new_pml4 = page_alloc();
    if (!new_pml4) return 0;
    for (int i = 0; i < 512; i++)
        new_pml4[i] = current[i];
    return (uint64_t)(uintptr_t)new_pml4;
}

void pml4_load(uint64_t pml4_pa) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(pml4_pa) : "memory");
}

void pml4_restore(uint64_t pml4_pa) { pml4_load(pml4_pa); }

void pml4_destroy(uint64_t pml4_pa) {
    page_free((void *)(uintptr_t)pml4_pa);
}

int pml4_map_phys(uint64_t pml4_pa, uint64_t virt_addr, uint64_t phys_addr,
                  size_t size, int writable) {
    uint64_t entry_flags = 0x87;  /* 0x83 | 0x04 (User) */
    if (writable) entry_flags |= 0x04;
    uint64_t *pml4 = (uint64_t *)(uintptr_t)pml4_pa;
    for (uint64_t offset = 0; offset < size; offset += 0x200000) {
        uint64_t va = virt_addr + offset;
        uint64_t pa = phys_addr + offset;
        int pml4_idx = (va >> 39) & 0x1FF;
        int pdpt_idx = (va >> 30) & 0x1FF;
        int pd_idx   = (va >> 21) & 0x1FF;
        if (!(pml4[pml4_idx] & 1)) {
            uint64_t *pdpt = mmap_user(0, 4096, 3, 0);
            if (!pdpt) return -1;
            for (int i = 0; i < 512; i++) pdpt[i] = 0;
            pml4[pml4_idx] = (uint64_t)(uintptr_t)pdpt | 0x07;  /* User+RW+Present */
        }
        uint64_t *pdpt = (uint64_t *)(uintptr_t)(pml4[pml4_idx] & ~0xFFF);
        if (!(pdpt[pdpt_idx] & 1)) {
            uint64_t *pd = mmap_user(0, 4096, 3, 0);
            if (!pd) return -1;
            for (int i = 0; i < 512; i++) pd[i] = 0;
            pdpt[pdpt_idx] = (uint64_t)(uintptr_t)pd | 0x07;  /* User+RW+Present */
        }
        uint64_t *pd = (uint64_t *)(uintptr_t)(pdpt[pdpt_idx] & ~0xFFF);
        pd[pd_idx] = pa | entry_flags;  /* entry_flags já tem User=1 */
    }
    __asm__ volatile("mov %0, %%cr3" : : "r"(pml4_pa) : "memory");
    return 0;
}
