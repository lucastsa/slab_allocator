#include <stdio.h>
#include "slab.h"

/* test approach based inspired by MinUnit 
   http://www.jera.com/techinfo/jtns/jtn002.html */
#define assertt(message, test) do { if (!(test)) return message; } while (0)
#define run_test(test) do { char *message = test(); tests_run++; \
                                if (message) return (char*)message; } while (0)
int tests_run = 0;


static char *
test_cache_create() {
    assertt("cache creation returned null?", 
            kmem_cache_create("test", 8, 0, NULL, NULL));

    return 0;
}

static char *
test_all () {
    run_test(test_cache_create);

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

    return (result != 0);
}