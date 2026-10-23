/****************************************************************************
 * **File:** csp/arch/csp_random.h
 *
 * **Description:** CSP random number generator
 ****************************************************************************/
#pragma once

#include <stddef.h>    /* for size_t */
#include <stdint.h>    /* for uint8_t*/
#include <sys/types.h> /* for ssize_t*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function to init the random number generator
 */
void csp_random_init(void);

/**
 * @brief Function to generate a random string of bytes
 *
 * @note This function may use a pseudo-random numbers, not cryptographically secure,
 *
 * @param[out] buf  Output buffer for the generated random data.
 * @param[in]  len  Number of bytes to generate and output.
 *
 * @return >=0 the number of bytes written or
 *         <0  failed to create the random number
 */
ssize_t csp_random(uint8_t * buf, const size_t len);

/**
 * @brief Function to generate a random string of bytes
 *
 * @note This function should use the hardware True Random Number Generator, or other cryptographically sound implementations
 *
 * @param[out] buf  Output buffer for the generated random data.
 * @param[in]  len  Number of bytes to generate and output.
 *
 * @return >=0 the number of bytes written or
 *         <0  failed to create the random number
 */
ssize_t csp_random_crypto(uint8_t * buf, const size_t len) __attribute__((warn_unused_result));

#ifdef __cplusplus
}
#endif
