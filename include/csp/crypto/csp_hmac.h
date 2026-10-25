/****************************************************************************
 * **File:** csp/crypto/csp_hmac.h
 *
 * **Description:** HMAC support.
 *
 * Hash-based Message Authentication Code - based on code from libtom.org.
 *
 * The SHA256 based HMAC, is based on the  RFC 6151, https://www.rfc-editor.org/info/rfc6151/
 *
 ****************************************************************************/
#pragma once

#include <csp/csp_types.h> /* csp_packet_t, bool, uint32_t, uint8_t */

#if (CSP_USE_SHA1)
#include <csp/crypto/csp_sha1.h> /* for SHA1 CSP_SHA1_DIGESTSIZE */
#else
#include <csp/crypto/csp_sha256.h> /* for CSP_SHA256_DIGEST_SIZE */
#endif

#define HMAC_KEY_LENGTH 16

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Number of bytes from the HMAC calculation, that is appended to the CSP message.
 */
#define CSP_HMAC_LENGTH 4

#if (CSP_USE_SHA1)
#define CSP_HMAC_DIGEST_LEN CSP_SHA1_DIGESTSIZE
#else
#define CSP_HMAC_DIGEST_LEN CSP_SHA256_DIGEST_SIZE
#endif

struct csp_hmac_state;
typedef struct csp_hmac_state csp_hmac_state_t;

/****************************************************************************
 * HMAC functions
 ****************************************************************************/

/**
 * @brief  Init HMAC state
 *
 * @param[in,out] state  HMAC state
 * @param[in]     key    secret key
 * @param[in]     keylen length of key
 */
void csp_hmac_init(csp_hmac_state_t * state, const uint8_t * key, size_t keylen);

/**
 * @brief  Process data (call repeatedly as needed)
 *
 * @note  Streams arbitrary chunks of the payload into the inner hash context.
 *        Can be called repeatedly as data streams in.
 *
 * @param[in,out] state   pointer to HMAC state
 * @param[in]     data    data to process
 * @param[in]     datalen length of data
 */
void csp_hmac_process(csp_hmac_state_t * state, const uint8_t * data, size_t datalen);

/**
 * @brief  Finalize and output HMAC
 *
 * @param[in,out] state  pointer to HMAC state
 * @param[out]    hmac   output buffer, must be CSP_HMAC_DIGEST_LEN bytes
 */
void csp_hmac_done(csp_hmac_state_t * state, uint8_t hmac[CSP_HMAC_DIGEST_LEN]);

#if !(CSP_USE_SHA1)
/**
 * @brief HKDF key derivation function
 *
 * @note HMAC-based key derivation function see RFC5869 for more details
 *
 * @param[in]  ikm      Input keying material
 * @param[in]  ikm_len  Length in the input keying material
 * @param[in]  salt     Optional salt value (a non-secret random value)
 * @param[in]  salt_len Length of the salt
 * @param[in]  info     Optional application specific information
 * @param[in]  info_len Length of the application specific information
 * @param[out] okm      Output keying material
 * @param[in]  okm_len  Length of the output keying material
 */
void csp_hkdf(const uint8_t * ikm, size_t ikm_len,
			  const uint8_t * salt, size_t salt_len,
			  const uint8_t * info, size_t info_len,
			  uint8_t * okm, size_t okm_len);
#endif

/****************************************************************************
 * CSP functions that use HMAC
 ****************************************************************************/

/**
 * @brief  HMAC on buffer
 * @note This function is used by append/verify but can also be called separately.
 *
 * @note For backward compatibility datalen can be zero, this will happen when the header is included,
 *       and we are sending a request such as uptime, that has no data in the packet.
 *       when CSP_USE_SEC is ON, datalen must be greaterthan zero, and the packet header is always included
 *
 * @param[in] key HMAC key
 * @param[in] keylen HMAC key length
 * @param[in] data pointer to data
 * @param[in] datalen length of data
 * @param[out] hmac calculated HMAC hash, minimum #CSP_HMAC_DIGEST_LEN bytes.
 */
void csp_hmac_memory(const void * key, uint32_t keylen, const void * data, size_t datalen, uint8_t hmac[CSP_HMAC_DIGEST_LEN]);

/**
 * Save a copy of the key string for use by the append/verify functions
 *
 * @note The key string is used to derive a key for the all HMAC calculations.
 *
 * @param[in] key HMAC key
 * @param[in] keylen HMAC key length
 */
void csp_hmac_set_key(const void * key, size_t keylen);

/**
 * @brief Append HMAC to packet
 *
 * @note If header is included, csp_id_prepend() must be called beforehand
 *
 * @param[in] packet         CSP packet, must be valid.
 * @param[in] include_header use header in hmac calculation (this will not modify the flags field)
 * 							 if CSP_USE_HMAC is defined this flag is ignored and the header is always
 *                           included in the HMAC calculation.
 *
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_hmac_append(csp_packet_t * packet, bool include_header) __attribute__((warn_unused_result));

/**
 * Verify HMAC of packet
 *
 * @param[in] packet         CSP packet, must be valid.
 * @param[in] include_header use header in hmac calculation (this will not modify the flags field)
 * 							 if CSP_USE_HMAC is defined this flag is ignored and the header is always
 *                           included in the HMAC calculation.
 *
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_hmac_verify(csp_packet_t * packet, bool include_header) __attribute__((warn_unused_result));

#ifdef __cplusplus
}
#endif
