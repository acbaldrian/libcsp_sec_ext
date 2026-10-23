#include <csp/arch/csp_random.h>   /* randdom header file for API defintion*/
#include "csp_macro.h"             /* for __csp_weak  */

#include <zephyr/random/random.h>  /* for sys_rand_get, sys_csrand_get */

#ifdef CONFIG_SOC_SERIES_RP2040
#include <hardware/regs/rosc.h>        /* for ROSC_RANDOMBIT_OFFSET */
#include <hardware/regs/addressmap.h>  /* for ROSC_BASE */

/*
 * sys_csrand_get() requires a registered entropy device driver.
 *
 * The RP2040 has no hardware TRNG peripheral. 
 * the RP2040 only has a Ring Oscillator RANDOMBIT register (section 2.17.5 of the datasheet).
 * This is a weak, non-certified entropy source,  it's essentially a glitchy oscillator bit,
 * not a proper TRNG. 
 * 
 * Zephyr does not ship an entropy driver that uses the RP2040's ROSC as a CSPRNG backend
 */

/**
 * @brief get a random number
 * 
 * @note This uses the ROSC RANDOMBIT to generate random bits
 *       The RP2040 datasheet explicitly states the ROSC RANDOMBIT "does not meet the requirements
 *       of randomness for security systems because it can be compromised." If you need
 *       cryptographic-grade randomness, you'll need a Pico W (use the CYW43439's RF noise) or an
 *       RP2350 (which has a proper hardware TRNG). 
 *       However the only hardware we have is the RP2040, so this is the best (at the moment) we can do
 * 
 * @return the 8 bit random number
 */
static uint8_t get_random(void)
{
    uint8_t random = 0;
    volatile uint8_t *rnd_reg = (uint8_t *)(ROSC_BASE + ROSC_RANDOMBIT_OFFSET);

    for (int i = 0; i < 8; i++) {
        uint8_t bit;
        do {
             bit = *rnd_reg & 1;
        } while (bit != (*rnd_reg & 1));
        random = (random << 1) | bit;
    }
    return random;
}   
#endif

__csp_weak void csp_random_init(void) {
}

__csp_weak ssize_t csp_random(uint8_t * buf, const size_t len) {
    sys_rand_get((void *)buf, len);
    return len;
}

__csp_weak ssize_t csp_random_crypto(uint8_t * buf, const size_t len) {
#ifdef CONFIG_SOC_SERIES_RP2040
    size_t i = 0;
    for (; i < len; i++) {
        buf[i] = get_random();
    }
    return i;
#else
    if (sys_csrand_get(buf, len) == 0) {
        return len;
    } 
    return EIO;
#endif
}
