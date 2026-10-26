/****************************************************************************
 * **File:** csp/interfaces/csp_if_kiss2.h
 *
 * **Description:** KISS interface (serial).
 *                  this implementation is derived from the csp_if_kiss
 *                  implementation. but uses a different drive interface.
 *                  to allow for more that one implementation of the KISS
 *                  interface.
 *
 ****************************************************************************/
#pragma once

#include <csp/csp_interface.h>          /* for csp_iface_t */
#include <csp/drivers/kiss2_driver.h>   /* for csp_kiss2_driver_t */
#include <csp/interfaces/csp_if_kiss.h> /* for csp_kiss_mode_t */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Default name of KISS2 interface.
 */
#define CSP_IF_KISS2_DEFAULT_NAME "KISS2"

/**
 * @brief KISS2 interface data (state information).
 */
typedef struct {
	const csp_kiss2_driver_t * driver; /**< the driver this interface is using */
	csp_kiss_mode_t rx_mode;           /**< Rx mode/state. */
	unsigned int rx_length;            /**< Rx length */
	bool rx_first;                     /**< Rx first - if set, waiting for first character
											(== TNC_DATA) after start */
	csp_packet_t * rx_packet;          /**< CSP packet for storing Rx data. */
} csp_kiss2_interface_data_t;

/**
 * Add interface.
 *
 * @param[in] iface CSP interface, initialized with name and
 * 					inteface_data pointing to a valid #csp_kiss2_interface_data_t.
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_kiss2_add_interface(csp_iface_t * iface);

// /**
//  * Send CSP packet over KISS (nexthop).
//  *
//  * @return #CSP_ERR_NONE on success, otherwise an error code.
//  */
// int csp_kiss2_tx(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me);

/**
 * Process received KISS frame.
 *
 * Called from driver when a chunk of data has been received. Once a complete
 * frame has been received, the CSP packet will be routed on.
 *
 * @param[in] iface incoming interface.
 * @param[in] buf received data.
 * @param[in] len length of \a buf.
 * @param[out] pxTaskWoken Valid reference if called from ISR, otherwise NULL!
 */
void csp_kiss2_rx(csp_iface_t * iface, const uint8_t * buf, size_t len, void * pxTaskWoken);

#ifdef __cplusplus
}
#endif
