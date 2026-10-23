#include <csp/arch/csp_random.h>
#include "csp_macro.h" /* for __csp_weak  */

#include <stdlib.h>     /* For srand rand and arc4random_buf */
#include <time.h>       /* For time */

__csp_weak void csp_random_init(void) {
	// Seed the generator with the current time
	srand((unsigned int)time(NULL));
}

__csp_weak void csp_random(uint8_t * buf, const size_t len) {
	for (size_t i = 0; i < len; i++) {
		buf[i] = (uint8_t)(rand() % 0xff);
	}
}

__csp_weak void csp_random_crypto(uint8_t * buf, const size_t len) {
	arc4random_buf(buf, len);
}
