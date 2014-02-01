#include <stdio.h>
#include "slab.h"

#define rdtscll(val) __asm__ __volatile__("rdtsc" : "=A" (val))

/* test approach based inspired by MinUnit 
   http://www.jera.com/techinfo/jtns/jtn002.html */
#define assertt(message, test) do { if (!(test)) return message; } while (0)
#define run_test(test) do { char *message = test(); tests_run++; \
                                if (message) return (char*)message; } while (0)
int tests_run = 0;

static char *
test_cache_create() {
    kmem_cache_t cp = kmem_cache_create("test", 12, 0, NULL, NULL);
    assertt("cache creation returned null?", cp);

    assertt("effective size miscalculated", cp->effsize == 16);

    kmem_cache_destroy(cp);

    return 0;
}

static char *
test_cache_grow() {
    kmem_cache_t cp = kmem_cache_create("test", 12, 0, NULL, NULL);

    kmem_cache_grow(cp);

    kmem_cache_destroy(cp);

    return 0;
}

static char *
test_cache_alloc() {
    // 12-byte struct
    struct test {
        int a, b, c;
    };
    struct test * obj;

    kmem_cache_t cp = kmem_cache_create("test", sizeof(struct test), 0, NULL, NULL);

    obj = (struct test *)kmem_cache_alloc(cp, KM_NOSLEEP);

    obj->a=1;
    obj->b=1;
    obj->c=1;

    obj = (struct test *)kmem_cache_alloc(cp, KM_NOSLEEP);

    obj->a=1;
    obj->b=1;
    obj->c=1;

    kmem_cache_destroy(cp);

    return 0;
}

static char *
test_perf_cache_alloc() {
    #define ITERATIONS 30000000
    unsigned long long start, end;    
    int i;
    // 12-byte struct
    struct test {
        int a, b, c;
    };
    struct test * obj;

    kmem_cache_t cp = kmem_cache_create("test", sizeof(struct test), 0, NULL, NULL);

    rdtscll(start);
    for (i=0; i<ITERATIONS; i++)
        obj = (struct test *)kmem_cache_alloc(cp, KM_NOSLEEP);
    rdtscll(end);

    printf("# %lld cycles for cache mem alloc\n", (end-start)/ITERATIONS);


    rdtscll(start);
    for (i=0; i<ITERATIONS; i++)
        obj = (struct test *)malloc(sizeof(struct test));
    rdtscll(end);

    printf("# %lld cycles for malloc\n", (end-start)/ITERATIONS);

    kmem_cache_destroy(cp);

    return 0;
}

static char *
test_all () {
    // run_test(test_cache_create);
    // run_test(test_cache_grow);
    // run_test(test_cache_alloc);
    run_test(test_perf_cache_alloc);

    return 0;
}

int 
main(void) {
    char * result = test_all();
    if (result) 
        printf("Test failed: %s\n", result);
    else 
        printf("ALL TESTS PASSED!\n");

    printf("=====================\nTOTAL TESTS:\t%04d\n", tests_run);

    printf("%d\n", (0 || 2014));
    return (result != 0);
}