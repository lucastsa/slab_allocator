#include "slab.h"
#include <stdio.h>

/*  Creates a cache of objects.

    @name used for reference
    @size size of the objects
    @align align boundary
    @constructor object constructor
    @destructor object destructor

    Returns a cache pointer or NULL if no memory is available.
*/
kmem_cache_t
kmem_cache_create(char *name, size_t size, int align, 
                  void (*constructor)(void *, size_t),
                  void (*destructor)(void *, size_t)) {

    kmem_cache_t cp = malloc(sizeof(struct kmem_cache));

    if (cp != NULL) {
        if (align == 0) align = SLAB_DEFAULT_ALIGN;

        cp->name = name;
        cp->size = size;
        cp->effsize = align * ((size-1)/align + 1);
        cp->slab_maxbuf = (PAGE_SZ - sizeof(struct kmem_slab)) / cp->effsize;
        cp->constructor = constructor;
        cp->destructor = destructor;
        cp->slabs = NULL;
    }
    
    return cp;
}

/* Grow a specified cache. Specifically adds one slab to it.

   @cp cache pointer
*/
void 
kmem_cache_grow(kmem_cache_t cp) {
    void *mem;
    kmem_slab_t slab;
    void * p, * lastbuf;

    // if this is a small object
    if (cp->size <= SLAB_SMALL_OBJ_SZ) {
        // allocating one "page"
        mem = malloc(PAGE_SZ);

        if (mem == NULL) return;

        // positioning slab at the end of the "page"
        slab = mem + PAGE_SZ - sizeof(struct kmem_slab);
        
        // check if there is any slab in the cache
        if (cp->slabs == NULL) {
            slab->prev = NULL;
        }
        else {
            slab->prev = cp->slabs;
            cp->slabs->next = slab;
        }
        slab->next = NULL;
        slab->bufcount = 0;
        slab->free_list = mem;
        
        // creating linkage
        lastbuf = mem + (cp->effsize * (cp->slab_maxbuf-1));
        for (p=mem; p < lastbuf; p+=cp->effsize) 
             *((void **)p) = p + cp->effsize;

        // complete slab at the front...
        cp->slabs = slab;

        // printf("\n%p\n%p\n%#x\n%#x\n", mem, slab, sizeof(struct kmem_slab), sizeof(struct kmem_cache));
    }
    else {

    }
}

/* Requests an allocated object from the cache. 

    @cp cache pointer
    @flags flags KM_SLEEP or KM_NOSLEEP
*/
void *
kmem_cache_alloc(kmem_cache_t cp, int flags) {
    void *buf;

    // grow the cache if necessary...
    if (cp->slabs == NULL) 
        kmem_cache_grow(cp);

    if (cp->slabs->bufcount == cp->slab_maxbuf)
        kmem_cache_grow(cp);

    // if this is a small object
    if (cp->size <= SLAB_SMALL_OBJ_SZ) {
        buf = cp->slabs->free_list;
        cp->slabs->free_list = *((void**)buf);
        cp->slabs->bufcount++;   
    }

    return buf;
}

/* Frees an allocated object from the cache. 

    @cp cache pointer
    @buf object pointer
*/
void 
kmem_cache_free(kmem_cache_t cp, void *buf) {

}

/* Destroys a specified cache.

    @cp cache pointer
*/
void 
kmem_cache_destroy(kmem_cache_t cp) {
    /* TODO: IMPLEMENT recursive free */
    free(cp);
}
