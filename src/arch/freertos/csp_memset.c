#include <csp/arch/csp_memset.h>

#include <assert.h>    /* for assert */

inline void csp_memset(void * ptr, int ch, size_t size) {
    assert(ptr != NULL);
    assert(size > 0);

    volatile unsigned char *p = (volatile unsigned char *)ptr;
    while (size--)
        *p++ = (unsigned char)ch;
}

