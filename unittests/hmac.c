#include <check.h>
#include "../include/csp/csp.h"
#include "../include/csp/csp_id.h"
#include "../include/csp/crypto/csp_hmac.h"
#include "../src/csp_buffer_private.h"
#if (CSP_USE_SHA1)
#include "../include/csp/crypto/csp_sha1.h"
#else
#include "../include/csp/crypto/csp_sha256.h"
#endif

#if (CSP_USE_SHA1)
#define CSP_HMAC_DIGEST_LEN CSP_SHA1_DIGESTSIZE
#else
#define CSP_HMAC_DIGEST_LEN CSP_SHA256_DIGEST_SIZE
#endif

START_TEST(test_csp_hmac_memory)
{
	const char * key = "testing";
	const char * data ="abc";
	uint8_t actual_hmac[CSP_HMAC_DIGEST_LEN];
	
	csp_hmac_memory(key, strlen(key), data, strlen(data), actual_hmac);
#if (CSP_USE_SHA1)
	uint8_t expected_hmac[] = { 0x5b, 0xdb, 0x32, 0x65, 0x21, 0xb7, 0x12, 0xdc, 
		                        0x10, 0xad, 0x5e, 0x71, 0x63, 0xec, 0x6c, 0xeb,
								0x43, 0x45, 0x46, 0x72 };
#else
	uint8_t expected_hmac[] = { 0x95, 0x14, 0x87, 0xab, 0x29, 0x34, 0x0c, 0xab,
		                        0x43, 0x96, 0xc2, 0xc5, 0x6d, 0xf2, 0x57, 0xb1,
								0xaa, 0x66, 0x36, 0xc7, 0xee, 0x89, 0x1e, 0x29,
								0x69, 0x6e, 0xf8, 0xd2, 0x67, 0x58, 0xaa, 0x96 };
#endif

	ck_assert_mem_eq(actual_hmac, expected_hmac, sizeof(expected_hmac));
}

START_TEST(test_csp_hmac_memory2)
{
	const char * key = "testing";
	const char * data ="the quick brown fox jumps over the lazy dog";
	uint8_t actual_hmac[CSP_HMAC_DIGEST_LEN];
	
	csp_hmac_memory(key, strlen(key), data, strlen(data), actual_hmac);

#if (CSP_USE_SHA1)
	uint8_t expected_hmac[] = { 0xb9, 0x0e, 0xfa, 0xb8, 0x82, 0x04, 0x9d, 0x42,
		                        0x0e, 0x28, 0xdb, 0x35, 0xa1, 0x5e, 0xea, 0x38,
								0x40, 0x8f, 0xbf, 0xa4 };
#else
	uint8_t expected_hmac[] = { 0x25, 0x07, 0xe5, 0x2d, 0x27, 0x39, 0x14, 0x75,
		                        0xa1, 0xfb, 0x15, 0x21, 0x40, 0xff, 0xec, 0x48,
								0x9e, 0x48, 0xfa, 0x30, 0x05, 0x37, 0x5b, 0xef,
								0x3a, 0x53, 0x4a, 0x99, 0xff, 0xbc, 0x41, 0x36 };
#endif
	ck_assert_mem_eq(actual_hmac, expected_hmac, sizeof(expected_hmac));
}


START_TEST(test_hmac_append_no_header)
{
	uint8_t test_data[] = {0x61, 0x62, 0x63}; /* abc */
#if (CSP_USE_SHA1)
	uint8_t expected[] = {0x61, 0x62, 0x63, 0x9b, 0x4a, 0x91, 0x8f};
#else
	uint8_t expected[] = {0x61, 0x62, 0x63, 0x36, 0x58, 0x49, 0xab};
#endif
	csp_packet_t * packet;

	csp_init();

	packet = csp_buffer_get_always();
	ck_assert_ptr_nonnull(packet);
	memcpy(packet->data, test_data, sizeof(test_data));
	packet->length = sizeof(test_data);

	int err = csp_hmac_append(packet, false);
	ck_assert_int_eq(err, CSP_ERR_NONE);
	ck_assert_mem_eq(packet->data, expected, sizeof(expected));

	err = csp_hmac_verify(packet, false);
	ck_assert_int_eq(err, CSP_ERR_NONE);
	ck_assert_mem_eq(packet->data, test_data, sizeof(test_data));
}
END_TEST

START_TEST(test_hmac_append_include_header)
{
	uint8_t test_data[] = {0x61, 0x62, 0x63}; /* abc */
#if (CSP_USE_SHA1)
	uint8_t expected[] = {0x61, 0x62, 0x63, 0x3c, 0xc7, 0x49, 0x8b};
#else
	uint8_t expected[] = {0x61, 0x62, 0x63, 0x36, 0x58, 0x49, 0xab};
#endif
	csp_packet_t * packet;

	csp_init();

	packet = csp_buffer_get_always();
	ck_assert_ptr_nonnull(packet);

	csp_id_prepend(packet);

	memcpy(packet->data, test_data, sizeof(test_data));
	packet->length += sizeof(test_data);
	packet->frame_length += sizeof(test_data);

	int err = csp_hmac_append(packet, true);
	ck_assert_int_eq(err, CSP_ERR_NONE);
	ck_assert_mem_eq(packet->data, expected, sizeof(expected));

	err = csp_hmac_verify(packet, true);
	ck_assert_int_eq(err, CSP_ERR_NONE);
	ck_assert_mem_eq(packet->data, test_data, sizeof(test_data));
}
END_TEST

Suite * hmac_suite(void)
{
	Suite *s;
	TCase *tc_hmac;

	s = suite_create("HMAC");

	tc_hmac = tcase_create("append");
	tcase_add_test(tc_hmac, test_csp_hmac_memory);
	tcase_add_test(tc_hmac, test_csp_hmac_memory2);
	tcase_add_test(tc_hmac, test_hmac_append_no_header);
	tcase_add_test(tc_hmac, test_hmac_append_include_header);
	suite_add_tcase(s, tc_hmac);

	return s;
}
