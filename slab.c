#include "slab.h"

/*
    Creates a cache of objects.

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
        cp->name = name;
        cp->size = size;
        cp->align = align;
        cp->constructor = constructor;
        cp->destructor = destructor;
    }
    
    return cp;
}

void 
kmem_cache_grow(kmem_cache_t cp) {

}
