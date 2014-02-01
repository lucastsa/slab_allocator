#ifndef SLAB_H
#define SLAB_H 1

#include <stdlib.h>

#define PAGE_SZ 4096
#define CACHE_LINE_SZ 0

#define KM_SLEEP    0x00
#define KM_NOSLEEP  0x01

struct kmem_bufctl;
struct kmem_slab;
struct kmem_cache;

typedef struct kmem_cache * kmem_cache_t;
typedef struct kmem_bufctl * kmem_bufctl_t;
typedef struct kmem_slab * kmem_slab_t;

struct kmem_bufctl {
    void * buf;
    kmem_bufctl_t next;
    kmem_slab_t slab;
};

struct kmem_slab {
    kmem_slab_t next;
    kmem_slab_t prev;
    kmem_bufctl_t free_list;
    int bufcount;
};

struct kmem_cache {
    char * name;
    size_t size;
    int align;
    void (*constructor)(void *, size_t);
    void (*destructor)(void *, size_t);
    kmem_slab_t slabs;
};


kmem_cache_t
kmem_cache_create(char *name, size_t size, int align, 
                  void (*constructor)(void *, size_t),
                  void (*destructor)(void *, size_t));


void *
kmem_cache_alloc(kmem_cache_t cp, int flags);

void 
kmem_cache_free(kmem_cache_t cp, void *buf);

void 
kmem_cache_destroy(kmem_cache_t cp);

// TODO
void 
kmem_cache_grow(kmem_cache_t cp);

void 
kmem_cache_reap(void);

#endif
