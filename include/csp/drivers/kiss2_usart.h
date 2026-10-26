/****************************************************************************
* **File:** csp/drivers/kiss2/usart/usart_linux.h
  *
 * **Description:** USART driver for kiss2
 *
 ****************************************************************************/

#pragma once

#include <csp/drivers/usart.h> /* for csp_usart_conf_t */
#include <csp/drivers/kiss2.h> /* for csp_kiss2_rx_callback_t */
#include <pthread.h>           /* for pthread_t, pthread_mutex_t*/

#ifdef __cplusplus
extern "C" {
#endif

#define KISS2_USART_CBUF_SIZE (400) /**< the size of the input buffer */ 

/**
 * @brief the usart context
 * 
 * @note this is is defined in the header so that the caller can use a static context,
 *       or allocate as needed
 */
typedef struct {
	csp_kiss2_driver_context_t kiss_ctx;  /**< additional information the Kiss2 driver and iface need */ 
	csp_kiss2_rx_callback_t rx_callback;  /**< the call back to used then the device has data */ 
	void * callback_ctx;                  /**< the context to be passed to the callback */ 
	int fd;                               /**< File Descriptor for the usart device  */ 
 	uint8_t cbuf[KISS2_USART_CBUF_SIZE];  /**< the input buffer  */
	pthread_t rx_thread;                  /**< thread identifier for the usart listener thread */ 
    pthread_mutex_t lock;                 /**< mutex to lock access to the driver */ 
} csp_kiss2_usart_context_t;

/**
 * @brief Opens UART device and add KISS2 interface.
 *
 * @note This is a convenience function for opening an UART device and adding it as an interface with a given name.
 *
 * @param[out] driver_data the usart context, this will be initialised by the open
 * @param[in]  conf UART configuration.
 * @param[in]  ifname interface name (will be copied), or use NULL for default name.
 * @param[in]  addr CSP address of the interface.
 * @param[out] return_iface the added interface. (can be NULL)
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_usart_open_and_add_kiss2_interface(csp_kiss2_usart_context_t * driver_data, const csp_usart_conf_t * conf, const char * ifname, uint16_t addr, csp_iface_t ** return_iface);

#ifdef __cplusplus
}
#endif
