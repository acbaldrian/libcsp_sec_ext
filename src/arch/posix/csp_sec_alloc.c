#include <csp/arch/csp_sec_alloc.h>
#include <csp/arch/csp_random.h>

#include <sys/mman.h>  /* for mlock, madvise */
#include <stdlib.h>    /* for malloc, free */
#include <assert.h>    /* for assert */


void *csp_sec_alloc(size_t size) {
    assert(size >0);
    void *buf = malloc(size);
    if (!buf) {
        return NULL;
    }
    /* Lock the pages so they can never be swapped to disk */
    if (mlock(buf, size) == -1) {
        free(buf);
        return NULL;
    }
    /* prevent this region from appearing in core dumps */
    // madvise(buf, size, MADV_DONTDUMP);

    return buf;
}

void __attribute__((noinline)) csp_sec_free(void *ptr, size_t size) {
    assert(ptr != NULL);
    assert(size > 0);

    csp_random(ptr, size);
    munlock(ptr, size);
    free(ptr);
}

