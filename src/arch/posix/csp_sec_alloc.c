#include <csp/arch/csp_sec_alloc.h>
#include <csp/arch/csp_random.h>

#include <sys/mman.h>  /* for mlock, madvise */
#include <stdlib.h>    /* for malloc, free */
#include <assert.h>    /* for assert */



#if defined(__APPLE__)
  #define __STDC_WANT_LIB_EXT1__ 1
  #include <string.h>
  #define memset_explicit(ptr, ch, size) memset_s((ptr), (size), (ch), (size))
#elif defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 37))
  #include <string.h>  /* memset_explicit is native in glibc >= 2.37 */
#else
  #include <string.h>
  /* Fallback: volatile prevents the compiler from optimizing it away */
  static inline void memset_explicit(void *ptr, int ch, size_t size) {
    volatile unsigned char *p = (volatile unsigned char *)ptr;
    while (size--) *p++ = (unsigned char)ch;
  }
#endif   

inline void csp_sec_memset(void * ptr, int ch, size_t size) {
    memset_explicit(ptr, ch, size);
}

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

