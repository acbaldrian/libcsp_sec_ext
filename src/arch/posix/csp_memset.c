#include <csp/arch/csp_memset.h>

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

inline void csp_memset(void * ptr, int ch, size_t size) {
    memset_explicit(ptr, ch, size);
}
