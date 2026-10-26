/****************************************************************************
 * **File:** csp/drivers/kiss2.h
 *
 * **Description:** helper for kiss drivers
 *
 ****************************************************************************/

#pragma once

#include <csp/interfaces/csp_if_kiss2.h> /* for csp_kiss2_interface_data_t, csp_iface_t */
#include <csp/drivers/kiss2_driver.h>    /* for csp_kiss2_driver_t */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief the kiss2 driver context
 *
 * @note this is is defined in the header so that the caller can use a static context,
 *       or allocate is needed
 */
typedef struct {
	char name[CSP_IFLIST_NAME_MAX + 1];
	csp_iface_t iface;                 /**< Interface we are using */
	csp_kiss2_interface_data_t ifdata; /**< kiss2 interface context */
} csp_kiss2_driver_context_t;

/**
 * @brief initialise a kiss driver context
 *
 * @note  This is a helper function to initialise the the kiss driver context
 *        this function is called be the driver before setting up the rest of the context
 *
 * @param[out] kiss_ctx     kiss context to in initialised
 * @param[in]  driver       driver that will be opend
 * @param[in]  driver_ctx   the driver context to be pass to each drive call
 * @param[in]  driver_param additional parameters that the driver may need (can be NULL)
 * @param[in]  ifname       interface name (will be copied), or use NULL for default name.
 * @param[in]  addr         CSP address of the interface.
 * @param[out] return_iface the added interface. (can be NULL)
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_driver_open_add_kiss2_interface(csp_kiss2_driver_context_t * kiss_ctx, csp_kiss2_driver_t * driver, void * driver_ctx, void * driver_param, const char * ifname, uint16_t addr, csp_iface_t ** return_iface);

#ifdef __cplusplus
}
#endif
