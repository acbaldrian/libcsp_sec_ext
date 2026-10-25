#include <csp/csp_hooks.h>
#include "csp_macro.h"

#include <FreeRTOS.h>
#include <task.h>

/**
 * @brief provide a default implementation of 
 *        xPortGetFreeHeapSize
 * 
 * @note #if (configSUPPORT_DYNAMIC_ALLOCATION == 1) should work
 *       however the PI Pico defines configSUPPORT_DYNAMIC_ALLOCATION,
 *       but links with heep_3.c not heep_4.c
 *       
 *       This should allow linking to xPortGetFreeHeapSize if it
 *       is provided, but stop the error if not.
 */
__csp_weak size_t xPortGetFreeHeapSize(void) {
	return 0;
}

__csp_weak uint32_t csp_memfree_hook(void) {
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
	return (uint32_t)xPortGetFreeHeapSize();
#else
	return 0;
#endif
}

__csp_weak unsigned int csp_ps_hook(csp_packet_t * packet) {
	(void)packet; /* Avoid compiler warnings about unused parameter */
	return 0;
}
