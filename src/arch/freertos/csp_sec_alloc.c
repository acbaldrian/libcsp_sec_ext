#include <csp/arch/csp_sec_alloc.h>
#include <csp/arch/csp_random.h>

#include <stdlib.h>    /* for malloc, free */
#include <assert.h>    /* for assert */

void * csp_sec_alloc(size_t size) {
    assert(size > 0);

    return  malloc(size);
}

void __attribute__((noinline)) csp_sec_free(void *ptr, size_t size) {
    assert(ptr != NULL);
    assert(size > 0);

    csp_random(ptr, size);
    free(ptr);
}

