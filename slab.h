#define PAGE_SZ 4096
#define CACHE_LINE_SZ 0

typedef struct kmem_bufctl * kmem_bufctl_t;

struct kmem_bufctl {
    struct kmem_bufctl * next;
    void * buf;
};

typedef struct kmem_cache * kmem_cache_t;

struct kmem_slab {
    struct kmem_slab * next;
    struct kmem_slab * previous;
    kmem_bufctl_t free_list;
};

typedef struct kmem_slab * kmem_slab_t;


struct kmem_cache {

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
kmem_cache_grow(kmem_cache_t c);

void 
kmem_cache_reap(void);