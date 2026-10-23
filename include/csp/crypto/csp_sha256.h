/*********************************************************************
* Filename:   sha256.h
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Defines the API for the corresponding SHA1 implementation.
*********************************************************************/

#pragma once


/*************************** HEADER FILES ***************************/
#include <stdint.h> /* for uint8_t, uint32_t */
#include <stddef.h> /* for size_t */

/****************************** MACROS ******************************/
#define CSP_SHA256_DIGEST_SIZE 32            // SHA256 outputs a 32 byte digest
#define CSP_SHA256_BLOCK_SIZE 64             // SHA256 has a 64 byte block size

/**************************** DATA TYPES ****************************/
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint8_t data[64];
	uint32_t datalen;
	unsigned long long bitlen;
	uint32_t state[8];
} csp_sha256_ctx_t;

/*********************** FUNCTION DECLARATIONS **********************/

/**
 * @brief  initializes the SHA256 context
 * 
 * @param[in] ctx pointer to csp_sha256_ctx_t structure
 */
void csp_sha256_init(csp_sha256_ctx_t *ctx);

/**
 * @brief  updates the SHA256 context with new data
 * 
 * @param[in] ctx pointer to csp_sha256_ctx_t structure
 * @param[in] data pointer to the data to be hashed
 * @param[in] len length of the data to be hashed
 */
void csp_sha256_update(csp_sha256_ctx_t *ctx, const uint8_t data[], size_t len);

/**
 * @brief  finalizes the SHA256 hash
 * 
 * @param[in] ctx pointer to csp_sha256_ctx_t structure
 * @param[out] hash pointer to the buffer to store the hash,
 *                  this must be at least CSP_SHA256_DIGEST_SIZE bytes long
 * 					and writable
 */
void csp_sha256_final(csp_sha256_ctx_t *ctx, uint8_t hash[CSP_SHA256_DIGEST_SIZE]);

/**
 * @brief Calculate SHA256 hash of block of memory.
 * 
 * @note: This is a helper function and not part of the core SHA256 API.
 *        It is provided for convenience.
 *
 * @param[in]  data data.
 * @param[in]  len length of data.
 * @param[out] hash user supplied buffer of minimum #CSP_SHA256_DIGEST_SIZE bytes.
 */
void csp_sha256_memory(const uint8_t *data, size_t len, uint8_t hash[CSP_SHA256_DIGEST_SIZE]);

#ifdef __cplusplus
}
#endif