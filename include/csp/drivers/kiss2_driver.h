/****************************************************************************
 * **File:** csp/drivers/kiss2_driver.h
 *
 * **Description:** abstract definition of a driver the kiss2 iface 
 *
 ****************************************************************************/

#pragma once

#include <csp/interfaces/csp_if_kiss2.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Callback for returning data to interface.
 *
 * @param[in] callback_ctx the context that was passed on open
 * @param[in] buf data received.
 * @param[in] len data length (number of bytes in \a buf).
 * @param[out] pxTaskWoken Valid reference if called from ISR, otherwise NULL!
 */
typedef void (*csp_kiss2_rx_callback_t)(void * callback_ctx, uint8_t * buf, size_t len, void * pxTaskWoken);

/**
 * @brief Open a driver 
 * 
 * @note: This function should start the task to read data from the hardware
 *
 * @param[in] driver_data   the driver context from #csp_iface_t
 * @param[in] rx_callback  callback function to use when there is data
 * @param[in] callback_ctx the context that should be passed to the callback
 * @param[in] driver_param additional parameters that the driver may need (can be NULL)
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
typedef int (*csp_kiss2_driver_open_t)(void * driver_data, csp_kiss2_rx_callback_t rx_callback, void * callback_ctx, void * driver_param);

/**
 * @brief Start a driver transmission
 * 
 * @note: This function is call befor the interface start sending data to the tx function
 *
 * @param[in] driver_data the driver context from #csp_iface_t
 */
typedef void (*csp_kiss2_driver_tx_start_t)(void * driver_data);

/**
 * @brief write KISS bytes
 * 
 * @note: this function is called as the kiss2 interface write byte out.
 *        There is no buffer, the packet is convered on the fly and the
 *        converted bytes sent to the driver.
 * 
 * @note: if this returned an error, packet transmission is terminated and 
 *        csp_kiss2_driver_tx_end_t called
 *
 * @param[in] driver_data  the driver context from #csp_iface_t
 * @param[in] data        data to send
 * @param[in] len         length of \a data.
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
typedef int (*csp_kiss2_driver_tx_write_t)(void * driver_data, const uint8_t * data, size_t len);

/**
 * @brief flush the transmitted data
 * 
 * @note: This function is call after all the KISS data has been send
 *
 * @param[in] driver_data the driver context from #csp_iface_t
 */
typedef void (*csp_kiss2_driver_flush_t)(void * driver_data);


/**
 * @brief finish a driver transmission
 * 
 * @note: This function is call when tx is finished, this will be called even if the packet tx failed to send
 *
 * @param[in] driver_data the driver context from #csp_iface_t
 */
typedef void (*csp_kiss2_driver_tx_finish_t)(void * driver_data);

/**
 * @brief the driver 
 */
typedef struct  {
    csp_kiss2_driver_open_t open;
    csp_kiss2_driver_tx_start_t start;
    csp_kiss2_driver_tx_write_t write;
    csp_kiss2_driver_flush_t flush;
    csp_kiss2_driver_tx_finish_t finish;
} csp_kiss2_driver_t;


#ifdef __cplusplus
}
#endif
