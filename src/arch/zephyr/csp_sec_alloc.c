#include <csp/arch/csp_sec_alloc.h>
#include <csp/arch/csp_random.h>

#include <sys/mman.h>  /* for mlock, madvise */
#include <stdlib.h>    /* for malloc, free */
#include <assert.h>    /* for assert */

inline void csp_sec_memset(void * ptr, int ch, size_t size) {
    volatile unsigned char *p = ptr;
    while (size--) {
        *p++ = (unsigned char)ch;
    }
}

inline void *csp_sec_alloc(size_t size) {
    assert(size > 0);

    /* Just use malloc, but randomise the memory on free*/
    return malloc(size);
}

void __attribute__((noinline)) csp_sec_free(void *ptr, size_t size) {
    assert(ptr != NULL);
    assert(size > 0);

    csp_random(ptr, size);
    free(ptr);
}
