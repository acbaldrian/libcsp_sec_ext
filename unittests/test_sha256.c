#include <check.h>
#include "../include/csp/crypto/csp_sha256.h" /* sha256 API */

START_TEST(test_sha256)
{
   	csp_sha256_ctx_t ctx;

    char * data = "abc";
    uint8_t actual_hash[CSP_SHA256_DIGEST_SIZE];

	csp_sha256_init(&ctx);
	csp_sha256_update(&ctx, data, strlen(data));
	csp_sha256_final(&ctx, actual_hash);

    uint8_t expected_hash[] = { 0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
                                0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
                                0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
                                0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad};
								
    ck_assert_mem_eq(actual_hash, expected_hash, sizeof(expected_hash));
}
END_TEST

START_TEST(test_sha256_2)
{
   	csp_sha256_ctx_t ctx;

    char * data = "zarking fardwarks";
    uint8_t actual_hash[CSP_SHA256_DIGEST_SIZE];

	csp_sha256_init(&ctx);
	csp_sha256_update(&ctx, data, strlen(data));
	csp_sha256_final(&ctx, actual_hash);

    uint8_t expected_hash[] = { 0x6d, 0xb3, 0x85, 0x33, 0x3d, 0x4e, 0xb2, 0x8d,
		                        0xa6, 0x97, 0x12, 0x8f, 0x13, 0x11, 0xba, 0x69,
								0x99, 0x3d, 0x53, 0x8f, 0x2b, 0x40, 0xa5, 0xd3,
								0x26, 0xf3, 0xa7, 0xce, 0xbf, 0x7c, 0x8f, 0x18};
    ck_assert_mem_eq(actual_hash, expected_hash, sizeof(expected_hash));
}
END_TEST

START_TEST(test_csp_sha256_memory)
{
    char * data = "this is a test";
    uint8_t actual_hash[CSP_SHA256_DIGEST_SIZE];

	csp_sha256_memory(data, strlen(data), actual_hash);

    uint8_t expected_hash[] = { 0x2e, 0x99, 0x75, 0x85, 0x48, 0x97, 0x2a, 0x8e,
		                        0x88, 0x22, 0xad, 0x47, 0xfa, 0x10, 0x17, 0xff,
								0x72, 0xf0, 0x6f, 0x3f, 0xf6, 0xa0, 0x16, 0x85,
								0x1f, 0x45, 0xc3, 0x98, 0x73, 0x2b, 0xc5, 0x0c};
    ck_assert_mem_eq(actual_hash, expected_hash, sizeof(expected_hash));
}
END_TEST

Suite * sha256_suite(void)
{
	Suite *s;
	TCase *tc_sha256;

	s = suite_create("SHA256");

	tc_sha256 = tcase_create("hash");
	tcase_add_test(tc_sha256, test_sha256);
	tcase_add_test(tc_sha256, test_sha256_2);
	tcase_add_test(tc_sha256, test_csp_sha256_memory);
	suite_add_tcase(s, tc_sha256);

	return s;
}

