/**
 * Hash-based Message Authentication Code(HMAC) - SHA1 based on code from libtom.org
 *
 * The SHA256 based HMAC, is based on the  RFC 6151, https://www.rfc-editor.org/info/rfc6151/
 */

#include <csp/crypto/csp_hmac.h> /* For the hMAC API */

#if (CSP_USE_SHA1)
#include <csp/crypto/csp_sha1.h> /* SHA1 is only used for backword compatibility */
#else
#include <csp/crypto/csp_sha256.h> /* SHA256 is the recommended hash function */
#endif
#if (CSP_USE_SEC)
#include <csp/csp_sec.h> /* for csp_sec_send_cache_get_seq, csp_sec_recv_cache_validate_seq */
#include <csp/csp_id.h>  /* for csp_id_prepend */
#endif
#include <csp/arch/csp_sec_alloc.h> /* for csp_sec_memset */

#include <string.h> /* for memcmp, memcpy, memset */
#include <assert.h> /* for assert */
#include <endian.h> /* for htobe32 */

// #include <csp/csp_debug.h> /* for csp_print */

/**
 * @brief HMAC key
 *
 * @note the is the key that is used to generate the the HMAC for the CSP packet.
 *       It is set by @see csp_hmac_set_key().
 */
static uint8_t csp_hmac_key[HMAC_KEY_LENGTH];

/**
 * @brief HKDF info
 *
 * @note The HMAC KEY is derived from the HKDF using a secrect key and the info string.
 */
#define CSP_HKDF_INFO     "csp_hkdf_info"             /* binding information for HKDF for the HMAC */
#define CSP_HKDF_INFO_LEN (sizeof(CSP_HKDF_INFO) - 1) /* length of the binding information for HKDF for the HMAC */

/**
 * @brief HMAC state structure
 */
#if (CSP_USE_SHA1)
struct csp_hmac_state {
	csp_sha1_state_t md;
	uint8_t key[CSP_SHA1_BLOCKSIZE];
};
#else
struct csp_hmac_state {
	csp_sha256_ctx_t inner_ctx;          /* Tracks ongoing hash state of the message*/
	uint8_t opad[CSP_SHA256_BLOCK_SIZE]; /* Cached outer padding block for the final step */
};
#endif

/**
 * @brief  Init HMAC state
 *
 * @param[in,out] state  HMAC state
 * @param[in]     key    secret key
 * @param[in]     keylen length of key
 */
void csp_hmac_init(csp_hmac_state_t * state, const uint8_t * key, size_t keylen) {
	assert(state != NULL);
	assert(key != NULL);
	assert(keylen > 0);

#if (CSP_USE_SHA1)
	uint32_t i;
	uint8_t buf[CSP_SHA1_BLOCKSIZE];

	/* Make sure we have a large enough key */
	if (keylen > CSP_SHA1_BLOCKSIZE) {
		csp_sha1_memory(key, keylen, state->key);
		if (CSP_SHA1_DIGESTSIZE < CSP_SHA1_BLOCKSIZE)
			memset((state->key) + CSP_SHA1_DIGESTSIZE, 0, (CSP_SHA1_BLOCKSIZE - CSP_SHA1_DIGESTSIZE));
	} else {
		memcpy(state->key, key, keylen);
		if (keylen < CSP_SHA1_BLOCKSIZE)
			memset((state->key) + keylen, 0, (CSP_SHA1_BLOCKSIZE - keylen));
	}

	/* Create the initial vector */
	for (i = 0; i < CSP_SHA1_BLOCKSIZE; i++) {
		buf[i] = state->key[i] ^ 0x36;
	}

	/* Prepend to the hash data */
	csp_sha1_init(&state->md);
	csp_sha1_process(&state->md, buf, CSP_SHA1_BLOCKSIZE);
#else

	csp_sha256_ctx_t sha256_ctx;
	uint8_t k0[CSP_SHA256_BLOCK_SIZE];
	uint8_t ipad[CSP_SHA256_BLOCK_SIZE];

	/* Normalize the secret key length to the block size (64 bytes) */
	memset(k0, 0, CSP_SHA256_BLOCK_SIZE);
	if (keylen > CSP_SHA256_BLOCK_SIZE) {
		csp_sha256_init(&sha256_ctx);
		csp_sha256_update(&sha256_ctx, key, keylen);
		csp_sha256_final(&sha256_ctx, k0);
	} else {
		memcpy(k0, key, keylen);
	}

	/* Compute Inner and Outer Key Paddings */
	for (size_t i = 0; i < CSP_SHA256_BLOCK_SIZE; i++) {
		ipad[i] = k0[i] ^ 0x36;
		state->opad[i] = k0[i] ^ 0x5C;
	}

	/* Seed the inner hashing state with the Inner Pad */
	csp_sha256_init(&state->inner_ctx);
	csp_sha256_update(&state->inner_ctx, ipad, CSP_SHA256_BLOCK_SIZE);

	/* Clean up transient key material from stack */
	csp_sec_memset((void *)k0, 0, CSP_SHA256_BLOCK_SIZE);
	csp_sec_memset((void *)ipad, 0, CSP_SHA256_BLOCK_SIZE);
#endif
}

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
void csp_hmac_process(csp_hmac_state_t * state, const uint8_t * data, size_t datalen) {
	assert(state != NULL);
	assert(data != NULL);

	/* Process data */
#if (CSP_USE_SHA1)
	csp_sha1_process(&state->md, data, datalen);
#else
	csp_sha256_update(&state->inner_ctx, data, datalen);
#endif
}

/**
 * @brief  Finalize and output HMAC
 *
 * @param[in,out] state  pointer to HMAC state
 * @param[out]    hmac   output buffer, must be CSP_HMAC_DIGEST_LEN bytes
 */
void csp_hmac_done(csp_hmac_state_t * state, uint8_t hmac[CSP_HMAC_DIGEST_LEN]) {
	assert(state != NULL);
	assert(hmac != NULL);

#if (CSP_USE_SHA1)
	/* Get the hash of the first HMAC vector plus the data */
	uint8_t isha[CSP_SHA1_DIGESTSIZE];
	csp_sha1_done(&state->md, isha);

	/* Create the second HMAC vector vector */
	uint8_t buf[CSP_SHA1_BLOCKSIZE];
	for (unsigned int i = 0; i < sizeof(buf); i++) {
		buf[i] = state->key[i] ^ 0x5C;
	}

	/* Now calculate the outer hash */
	csp_sha1_init(&state->md);
	csp_sha1_process(&state->md, buf, sizeof(buf));
	csp_sha1_process(&state->md, isha, sizeof(isha));
	csp_sha1_done(&state->md, buf);

	/* Copy to output  */
	memcpy(hmac, buf, CSP_HMAC_DIGEST_LEN);
#else
	csp_sha256_ctx_t sha256_ctx;
	uint8_t inner_hash[CSP_SHA256_DIGEST_SIZE];

	/* Complete the inner hash calculation: H((K0 ^ ipad) || message) */
	csp_sha256_final(&state->inner_ctx, inner_hash);

	/* Execute outer hashing stage: H((K0 ^ opad) || Inner Hash) */
	csp_sha256_init(&sha256_ctx);
	csp_sha256_update(&sha256_ctx, state->opad, CSP_SHA256_BLOCK_SIZE);
	csp_sha256_update(&sha256_ctx, inner_hash, CSP_SHA256_DIGEST_SIZE);
	csp_sha256_final(&sha256_ctx, hmac);

	/* Clean up residual key material from context states */
	csp_sec_memset(&sha256_ctx, 0, sizeof(sha256_ctx));
	csp_sec_memset(inner_hash, 0, sizeof(inner_hash));
	csp_sec_memset(state, 0, sizeof(csp_hmac_state_t));
#endif
}

#if !(CSP_USE_SHA1)
/**
 * @brief HKDF Extract
 *
 * @note PRK = HMAC-SHA256(salt, IKM)
 *
 * @param[in]  ikm      Input Keying Material (the secret to derive from), NOT NULL
 * @param[in]  ikm_len  Length of ikm in bytes must be > 0
 * @param[in]  salt     Salt used as the HMAC key (if NULL, treated as CSP_HMAC_DIGEST_LEN zeros)
 * @param[in]  salt_len Length of salt in bytes, ignored if salt is NULL
 * @param[out] prk      Output buffer for the Pseudo-Random Key (must be >= CSP_HMAC_DIGEST_LEN bytes)
 */
static void csp_hkdf_extract(const uint8_t * ikm, size_t ikm_len,
							 const uint8_t * salt, size_t salt_len,
							 uint8_t prk[CSP_HMAC_DIGEST_LEN]) {
	assert(salt == NULL || (salt != NULL && salt_len > 0));
	assert(ikm != NULL && ikm_len > 0);
	assert(prk != NULL);

	csp_hmac_state_t hmac;
	if (salt == NULL) {
		const uint8_t zeros[CSP_HMAC_DIGEST_LEN] = {0};
		csp_hmac_init(&hmac, zeros, CSP_HMAC_DIGEST_LEN);
	} else {
		csp_hmac_init(&hmac, salt, salt_len);
	}
	csp_hmac_process(&hmac, ikm, ikm_len);
	csp_hmac_done(&hmac, prk);
}

/**
 * @brief HKDF expand step
 *
 * @param[in]  prk      Pseudorandom key (must be >= CSP_HKDF_DIGEST_LEN bytes)
 * @param[in]  prk_len  Length of the pseudorandom key
 * @param[in]  info     Optional application specific information
 * @param[in]  info_len Length of the application specific information
 * @param[out] okm      Output keying material
 * @param[in]  okmLen   Length of the output keying material
 */
static void csp_hkdf_expand(const uint8_t * prk,
							const uint8_t * info, size_t info_len,
							uint8_t * okm, size_t okm_len) {

	assert(prk != NULL);
	assert(info == NULL || (info != NULL && info_len > 0));
	assert(okm != NULL);
	assert(okm_len > 0);

	size_t t_len = 0;
	uint8_t t[CSP_HMAC_DIGEST_LEN];
	csp_hmac_state_t hmac;

	for (uint8_t i = 1; okm_len > 0; i++) {

		/* Compute T(i) = HMAC-Hash(PRK, T(i-1) | info | i) */
		csp_hmac_init(&hmac, prk, CSP_HMAC_DIGEST_LEN);
		csp_hmac_process(&hmac, t, t_len);
		csp_hmac_process(&hmac, info, info_len);
		csp_hmac_process(&hmac, &i, sizeof(i));
		csp_hmac_done(&hmac, t);

		/* Number of octets in the current block */
		t_len = (okm_len < CSP_HMAC_DIGEST_LEN) ? okm_len : CSP_HMAC_DIGEST_LEN;

		memcpy(okm, t, t_len);

		/* Point to the next block */
		okm += t_len;
		okm_len -= t_len;
	}
}

void csp_hkdf(const uint8_t * ikm, size_t ikm_len,
			  const uint8_t * salt, size_t salt_len,
			  const uint8_t * info, size_t info_len,
			  uint8_t * okm, size_t okm_len) {

	assert(ikm != NULL && ikm_len > 0); /* Input keying material is not optional */
	assert((salt != NULL && salt_len > 0) || (salt == NULL));
	assert((info != NULL && info_len > 0) || (info == NULL));
	assert(okm != NULL && ikm_len > 0); /* Output keying material is not optional */
	assert(ikm_len <= (255 * CSP_HMAC_DIGEST_LEN));

	uint8_t prk[CSP_HMAC_DIGEST_LEN];
	csp_hkdf_extract(ikm, ikm_len, salt, salt_len, prk);
	csp_hkdf_expand(prk, info, info_len, okm, okm_len);
}
#endif

void csp_hmac_memory(const void * key, uint32_t keylen, const void * data, size_t datalen, uint8_t hmac[CSP_HMAC_DIGEST_LEN]) {

	assert(key != NULL && keylen > 0);
	assert(data != NULL && datalen > 0);
	assert(hmac != NULL);

	csp_hmac_state_t state;

	/* Init HMAC state */
	csp_hmac_init(&state, key, keylen);

	/* Process data */
	csp_hmac_process(&state, data, datalen);

	/* Output HMAC */
	csp_hmac_done(&state, hmac);
}

void csp_hmac_set_key(const void * key, size_t keylen) {
#if (CSP_USE_SHA1)
	/* Use SHA1 as KDF */
	uint8_t hash[CSP_SHA1_DIGESTSIZE];
	csp_sha1_memory(key, keylen, hash);

	/* Copy key */
	memcpy(csp_hmac_key, hash, sizeof(csp_hmac_key));
#else
	csp_hkdf((const uint8_t *)key, keylen,
			 (const uint8_t *)NULL, 0, /* salt is NULL, so it will be treated as CSP_HKDF_DIGEST_LEN zero */
			 (const uint8_t *)CSP_HKDF_INFO, CSP_HKDF_INFO_LEN,
			 csp_hmac_key, sizeof(csp_hmac_key));
#endif
}

int csp_hmac_append(csp_packet_t * packet, bool include_header) {

	assert(packet != NULL);

	if (((size_t)packet->length + CSP_HMAC_LENGTH) > sizeof(packet->data)) {
		return CSP_ERR_NOMEM;
	}
	uint8_t hmac[CSP_HMAC_DIGEST_LEN];

#if (CSP_USE_HMAC)
	(void)include_header; /* unused paramiter */

	csp_id_prepend(packet);
	csp_hmac_memory(csp_hmac_key, sizeof(csp_hmac_key), packet->frame_begin, packet->frame_length, hmac);

	// csp_print("%s: HMAC: 0x%02x%02x%02x%02x\n", __func__, hmac[0], hmac[1], hmac[2], hmac[3]);
	/* We are only using the first CSP_HMAC_LENGTH bytes of the HMAC as there is limited space in the packet */
	memcpy(&packet->frame_begin[packet->frame_length], hmac, CSP_HMAC_LENGTH);
	packet->frame_length += CSP_HMAC_LENGTH;
	packet->length += CSP_HMAC_LENGTH;
#else
	if (include_header) {

		/* If header is included, csp_id_prepend() must be called beforehand */
		csp_hmac_memory(csp_hmac_key, sizeof(csp_hmac_key), packet->frame_begin, packet->frame_length, hmac);

		memcpy(&packet->frame_begin[packet->frame_length], hmac, CSP_HMAC_LENGTH);
		packet->frame_length += CSP_HMAC_LENGTH;
		packet->length += CSP_HMAC_LENGTH;

	} else {
		csp_hmac_memory(csp_hmac_key, sizeof(csp_hmac_key), packet->data, packet->length, hmac);
		memcpy(&packet->data[packet->length], hmac, CSP_HMAC_LENGTH);
		packet->length += CSP_HMAC_LENGTH;
	}
#endif
	return CSP_ERR_NONE;
}

int csp_hmac_verify(csp_packet_t * packet, bool include_header) {
	assert(packet != NULL);

	if (packet->length < CSP_HMAC_LENGTH) {
		return CSP_ERR_HMAC;
	}
	uint8_t hmac[CSP_HMAC_DIGEST_LEN];

#if (CSP_USE_HMAC)
	(void)include_header; /* unused paramiter */

	csp_hmac_memory(csp_hmac_key, sizeof(csp_hmac_key), packet->frame_begin, packet->frame_length - CSP_HMAC_LENGTH, hmac);
	// csp_print("%s: calculated HMAC: 0x%02x%02x%02x%02x\n", __func__, hmac[0], hmac[1], hmac[2], hmac[3]);
	// csp_print("%s: packet HMAC:     0x%02x%02x%02x%02x\n", __func__, packet->frame_begin[packet->frame_length - CSP_HMAC_LENGTH], packet->frame_begin[packet->frame_length - CSP_HMAC_LENGTH + 1], packet->frame_begin[packet->frame_length - CSP_HMAC_LENGTH + 2], packet->frame_begin[packet->frame_length - CSP_HMAC_LENGTH + 3]);
	/* Compare calculated HMAC with packet header */
	if (memcmp(&packet->frame_begin[packet->frame_length - CSP_HMAC_LENGTH], hmac, CSP_HMAC_LENGTH) != 0) {
		/* HMAC failed */
		return CSP_ERR_HMAC;
	}

	/* Strip HMAC */
	packet->frame_length -= CSP_HMAC_LENGTH;
	packet->length -= CSP_HMAC_LENGTH;
#else
	if (include_header) {

		csp_hmac_memory(csp_hmac_key, sizeof(csp_hmac_key), packet->frame_begin, packet->frame_length - CSP_HMAC_LENGTH, hmac);

		/* Compare calculated HMAC with packet header */
		if (memcmp(&packet->frame_begin[packet->frame_length - CSP_HMAC_LENGTH], hmac, CSP_HMAC_LENGTH) != 0) {
			/* HMAC failed */
			return CSP_ERR_HMAC;
		}

		/* Strip HMAC */
		packet->frame_length -= CSP_HMAC_LENGTH;
		packet->length -= CSP_HMAC_LENGTH;

	} else {

		csp_hmac_memory(csp_hmac_key, sizeof(csp_hmac_key), packet->data, packet->length - CSP_HMAC_LENGTH, hmac);
		/* Compare calculated HMAC with packet header */
		if (memcmp(&packet->data[packet->length - CSP_HMAC_LENGTH], hmac, CSP_HMAC_LENGTH) != 0) {
			/* HMAC failed */
			return CSP_ERR_HMAC;
		}

		/* Strip HMAC */
		packet->length -= CSP_HMAC_LENGTH;
	}
#endif
	return CSP_ERR_NONE;
}
