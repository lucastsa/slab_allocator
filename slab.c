#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "slab.h"

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
        cp->constructor = constructor;
        cp->destructor = destructor;
        cp->slabs = NULL;
        cp->slabs_back = NULL;

        // if this is for small object
        if (cp->size <= SLAB_SMALL_OBJ_SZ) {
            cp->slab_maxbuf = (PAGE_SZ - sizeof(struct kmem_slab)) / cp->effsize;
        }
        else {
            // TODO: compute number of objects programmatically
            cp->slab_maxbuf = 8;

            // create hash table...
            // hcreate(cp->slab_maxbuf * 100);
        }
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
    void *p, *lastbuf;
    int i;
    kmem_bufctl_t bufctl;

    // if this is a small object
    if (cp->size <= SLAB_SMALL_OBJ_SZ) {
        // allocating one page
        if (0 != posix_memalign(&mem, PAGE_SZ, PAGE_SZ))
            return;

        // positioning slab at the end of the page
        slab = mem + PAGE_SZ - sizeof(struct kmem_slab);

        slab->next = slab->prev = slab;        
        slab->bufcount = 0;
        slab->free_list = mem;
        
        // creating linkage
        lastbuf = mem + (cp->effsize * (cp->slab_maxbuf-1));
        for (p=mem; p < lastbuf; p+=cp->effsize) 
             *((void **)p) = p + cp->effsize;

        // complete slab at the front...
        __slab_move_to_front(cp, slab);
        assert(cp->slabs == slab);

        // printf("\n%p\n%p\n%#x\n%#x\n", mem, slab, sizeof(struct kmem_slab), sizeof(struct kmem_cache));
    }
    // if this is a large object
    else {
        // allocating pages
        if (0 != posix_memalign(&mem, PAGE_SZ, (cp->slab_maxbuf * cp->effsize)/PAGE_SZ))
            return;

        // allocating slab
        slab = (kmem_slab_t)malloc(sizeof(struct kmem_slab));
        
        // initializing slab
        slab->next = slab->prev = slab;        
        slab->bufcount = 0;

        bufctl = (kmem_bufctl_t)malloc(sizeof(struct kmem_bufctl) * cp->slab_maxbuf);
        bufctl[0].next = NULL;
        bufctl[0].buf = mem;
        bufctl[0].slab = slab;
        slab->start = &bufctl[0];
        slab->free_list = &bufctl[0];
        // creating addtl bufctls
        for (i=1; i < cp->slab_maxbuf; i++) {
            bufctl[i].next = slab->free_list;
            bufctl[i].buf = mem + (i*cp->effsize + (PAGE_SZ%cp->effsize * (((i+1)*cp->effsize)/PAGE_SZ)));
            bufctl[i].slab = slab;
            slab->free_list = &bufctl[i];
        }

        // complete slab at the front...
        __slab_move_to_front(cp, slab);        

        // printf("\n%p\n%p\n%#x\n%#x\n", mem, slab, sizeof(struct kmem_slab), sizeof(struct kmem_cache));
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
    else {
        kmem_bufctl_t bufctl = cp->slabs->free_list;
        cp->slabs->free_list = bufctl->next;
        buf = bufctl->buf;
        cp->slabs->bufcount++;
    }

    // if slab is empty
    if (cp->slabs->bufcount == cp->slab_maxbuf)
        __slab_move_to_back(cp, cp->slabs);

    return buf;
}

/* Frees an allocated object from the cache. 

    @cp cache pointer
    @buf object pointer
*/
void 
kmem_cache_free(kmem_cache_t cp, void *buf) {
    void * mem;
    kmem_slab_t slab;
    // kmem_bufctl_t bufctl;

    // if this is a small object
    if (cp->size <= SLAB_SMALL_OBJ_SZ) {
        // compute slab position
        // TODO: DO IT GENERIC (PAGE_SZ != 0x1000)
        mem = (void*)((long)buf >> 12 << 12); 
        slab = mem + PAGE_SZ - sizeof(struct kmem_slab);

        // put buffer back in the slab free list
        *((void **)buf) = slab->free_list;
        slab->free_list = buf;

        slab->bufcount--;
    
        // if slab is now complete, discard whole page
        if (slab->bufcount == 0) {
            __slab_remove(cp, slab);
            free(mem);
        }

        // if slab WAS empty, re-add to non-empty slabs
        if (slab->bufcount == cp->slab_maxbuf-1) 
            __slab_move_to_front(cp, slab);

    }
    // if this is a large object
    else {
        // use hash table to get to bufctl

        // ...
        // bufctl = (kmem_cache_t)0x4000;
        // slab = bufctl->slab;
        // put bufctl back in the slab free list
        // bufctl->next = slab->free_list;
        // slab->free_list = bufctl;

        // if slab is now complete, discard whole page
        // if (slab->bufcount == 0) {
        //     __slab_remove(cp, slab);
        //     free(slab->start->buf); // free objects
        //     free(slab->start); // free bufctls
        //     free(slab); // free slab
        // }   
    }
}

/* Destroys a specified cache.

    @cp cache pointer
*/
void 
kmem_cache_destroy(kmem_cache_t cp) {
    kmem_slab_t slab;
    void * mem;

    if (cp->size <= SLAB_SMALL_OBJ_SZ) {
        // freeing all allocated memory
        while (cp->slabs) {
            slab = cp->slabs;
            __slab_remove(cp, slab);
            mem = (void*)slab - PAGE_SZ + sizeof(struct kmem_slab);
            free(mem);
        }
    }
    else {
        while (cp->slabs) {
            slab = cp->slabs;
            __slab_remove(cp, slab);
            free(slab->start->buf); // free objects
            free(slab->start); // free bufctls
            free(slab); // free slab
        }
    }

    free(cp);
}


/* Internal auxiliary to remove slab of freelist

    @cp cache pointer
    @slab slab pointer
*/
inline void
__slab_remove(kmem_cache_t cp, kmem_slab_t slab) {
    slab->next->prev = slab->prev;
    slab->prev->next = slab->next;

    // if front slab...
    if (cp->slabs == slab) {
        // if last slab
        if (slab->prev == slab) 
            cp->slabs = NULL;
        else
            cp->slabs = slab->prev;
    }

    // if back slab
    if (cp->slabs_back == slab) { 
        // if last slab
        if (slab->next == slab) 
            cp->slabs_back = NULL;
        else
            cp->slabs_back = slab->next;
    }
}

/* Internal auxiliary to move slab to the front of freelist

    @cp cache pointer
    @slab slab pointer
*/
inline void
__slab_move_to_front(kmem_cache_t cp, kmem_slab_t slab) {
    if (cp->slabs == slab) return;

    __slab_remove(cp, slab);
    
    // check if there is any slab in the cache
    if (cp->slabs == NULL) { 
        slab->prev = slab;
        slab->next = slab;

        cp->slabs_back = slab;
    }
    else {
        slab->prev = cp->slabs;
        cp->slabs->next = slab;

        slab->next = cp->slabs_back;
        cp->slabs_back->prev = slab;
    }
    cp->slabs = slab;
}

/* Internal auxiliary to move slab to the front of freelist

    @cp cache pointer
    @slab slab pointer
*/
inline void
__slab_move_to_back(kmem_cache_t cp, kmem_slab_t slab) {
    if (cp->slabs_back == slab) return;
    
    __slab_remove(cp, slab);

    // check if there is any slab in the cache
    if (cp->slabs == NULL) {
        slab->prev = slab;
        slab->next = slab;

        cp->slabs = slab;
    }
    else {
        slab->prev = cp->slabs;
        cp->slabs->next = slab;

        slab->next = cp->slabs_back;
        cp->slabs_back->prev = slab;
    }
    cp->slabs_back = slab;
}
