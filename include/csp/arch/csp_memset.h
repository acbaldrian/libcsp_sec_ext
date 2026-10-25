/****************************************************************************
 * **File:** csp/arch/csp_memset.h
 *
 * **Description:** memset, but the compiler "should" not optimism out the call 
 * 
 ****************************************************************************/
#pragma once

#include <stddef.h>    /* for size_t */

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @brief memset, but the compiler "should" not optimism out the call 
 * 
 * @param [in] ptr  The pointer to the memory to fill
 * @param [in] ch   The number to use to fill memory with 
 * @param [in] size The size of the block to fill with ch
 */
void csp_memset(void * ptr, int ch, size_t size);

#ifdef __cplusplus
}
#endif
