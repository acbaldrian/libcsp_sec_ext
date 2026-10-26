/****************************************************************************
 * **File:** drivers/kiss2/kiss2.c
 *
 * **Description:** helper for the kiss2 drivers *
 ****************************************************************************/

#include <csp/drivers/kiss2.h>        /* for csp_kiss2_driver_context_t */
#include <csp/drivers/kiss2_driver.h> /* for csp_kiss2_driver_t */

#include <string.h> /* for strncpy */
#include <assert.h> /* for assert */

/**
 * @brief Callback for returning data to interface.
 *
 * @param[in] callback_ctx the callback context
 * @param[in] buf          data received.
 * @param[in] len          data length (number of bytes in \a buf).
 * @param[out] pxTaskWoken Valid reference if called from ISR, otherwise NULL!
 */
static void csp_kiss2_rx_callback(void * callback_ctx, uint8_t * buf, size_t len, void * pxTaskWoken) {
	csp_kiss2_driver_context_t * kiss_ctx = (csp_kiss2_driver_context_t *)callback_ctx;
	csp_kiss2_rx(&kiss_ctx->iface, buf, len, pxTaskWoken);
}

int csp_driver_open_add_kiss2_interface(csp_kiss2_driver_context_t * kiss_ctx,
										csp_kiss2_driver_t * driver,
										void * driver_data,
										void * driver_param,
										const char * ifname, uint16_t addr,
										csp_iface_t ** return_iface) {
	assert(kiss_ctx != NULL);
	assert(driver != NULL);

	if (ifname == NULL) {
		ifname = CSP_IF_KISS2_DEFAULT_NAME;
	}
	(void)driver_data;
	(void)addr;

	/* copy a maximum of buffer size -1 in to name */
	strncpy(kiss_ctx->name, ifname, sizeof(kiss_ctx->name) - 1);
	/* force the termination of the thring if it overflowed */
	kiss_ctx->name[sizeof(kiss_ctx->name)] = 0;
	/* point the iface name to the name that we have copied */
	kiss_ctx->iface.name = kiss_ctx->name;
	/* set the iface address */
	kiss_ctx->iface.addr = addr;
	/* save the driver data in the iface */
	kiss_ctx->iface.driver_data = driver_data;
	/* interface_data is the kiss2_interface data, */
	kiss_ctx->iface.interface_data = &kiss_ctx->ifdata;
	/* The iface now has the driver */
	kiss_ctx->ifdata.driver = driver;

	/* add the iface to the list */
	int res = csp_kiss2_add_interface(&kiss_ctx->iface);
	if (res == CSP_ERR_NONE) {
		res = kiss_ctx->ifdata.driver->open(kiss_ctx->iface.driver_data, csp_kiss2_rx_callback, kiss_ctx, driver_param);
	}

	if (return_iface) {
		*return_iface = &kiss_ctx->iface;
	}

	return res;
}
