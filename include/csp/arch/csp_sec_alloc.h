/****************************************************************************
 * **File:** csp/arch/csp_sec_alloc.h
 *
 * **Description:** CSP "safer ish" allocate and free for sensitive data,
 *                  If available of the hardware platform this SHOULD
 *                  us apropdate temper prof and/or encrypted memory
 ****************************************************************************/
#pragma once

#include <stddef.h>    /* for size_t */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Allocate memory and immediately lock it into RAM 
 * 
 * @param [in] size The requested size of the 
 * 
 * @return The pointer the the allocated memory,
 *         NULL, an error occurred 
 */
void * csp_sec_alloc(size_t size) __attribute__((warn_unused_result));

/**
 * @brief  Wipe with random bytes, unlock, then free
 * 
 * @param [in] ptr  The pointer to the memory to free
 * @param [in] size The size of the memory to free
 * 
 */
void csp_sec_free(void * ptr, size_t size);

#ifdef __cplusplus
}
#endif
