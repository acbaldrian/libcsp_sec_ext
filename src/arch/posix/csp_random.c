#include <csp/arch/csp_random.h>
#include "csp_macro.h" /* for __weak  */

#include <sys/random.h> /* for getrandom */
#include <stdlib.h>     /* For srand rand */
#include <time.h>       /* For time */

__weak void csp_random_init(void) {
	// Seed the generator with the current time
	srand((unsigned int)time(NULL));
}

__weak ssize_t csp_random(uint8_t * buf, const size_t len) {
	for (size_t i = 0; i < len; i++) {
		buf[i] = (uint8_t)(rand() % 0xff);
	}
	return len;
}

__weak ssize_t csp_random_crypto(uint8_t * buf, const size_t len) {
	/* Flags = 0 draws from the urandom source (CSPRNG),
	 * which should be cryptographically secure for all purposes,
	 * including key generation. */
	return getrandom(buf, len, 0);
}
