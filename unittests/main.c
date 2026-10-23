#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "../include/csp/autoconfig.h" // -> CSP_X defines (compile configuration)

#define DEFAULT_PRINT_VERBOSITY (CK_NORMAL)

Suite * queue_suite(void);
Suite * buffer_suite(void);
Suite * route_suite(void);
Suite * wire_suite(void);
#if (CSP_USE_HMAC)
Suite * hmac_suite(void);
#if (CSP_USE_SHA1)
#else
	Suite * sha256_suite(void);
#endif
#endif
#if (CSP_HAVE_LIBZMQ)
Suite * zmqhub_suite(void);
#endif

static struct option long_options[] = {
    {"verbose", no_argument, 0, 'V'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

static void print_help() {
    printf("Usage: csp_tests [options]\n");
	printf("Run libcsp unit tests.\n\n");
	printf("Without any option, it will run all tests.\n\n");
	printf("   --verbose        print verbose message\n"
		   "   -h               print help\n");
}

int main(int argc, char *argv[])
{
	int number_failed;
	SRunner *sr;
	int opt;
	enum print_output print_verbosity = DEFAULT_PRINT_VERBOSITY;

	while ((opt = getopt_long(argc, argv, "h", long_options, NULL)) != -1) {
		switch (opt) {
		case 'V':
			print_verbosity = CK_VERBOSE;
			break;
		case 'h':
			print_help();
			exit(EXIT_SUCCESS);
		case '?':
			/* Invalid option or missing argument */
			print_help();
			exit(EXIT_FAILURE);
		}
	}

	sr = srunner_create(NULL);
	srunner_add_suite(sr, queue_suite());
	srunner_add_suite(sr, buffer_suite());
	srunner_add_suite(sr, route_suite());
	srunner_add_suite(sr, wire_suite());
#if (CSP_USE_HMAC)
	srunner_add_suite(sr, hmac_suite());
#if (CSP_HAVE_LIBZMQ)
	srunner_add_suite(sr, zmqhub_suite());
#endif

#if (CSP_USE_SHA1)
#else
	srunner_add_suite(sr, sha256_suite());
#endif
#endif
	srunner_run_all(sr, print_verbosity);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
