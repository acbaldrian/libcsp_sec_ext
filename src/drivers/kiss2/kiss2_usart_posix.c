/****************************************************************************
 * **File:** drivers/kiss2/kiss2_usart_posix.c
 *
 * **Description:** USART driver for the kiss2 interface
 *
 * .. note:: This interface implementation only support ONE open UART connection.
 ****************************************************************************/

#include <csp/drivers/kiss2_usart.h>

#include <csp/csp_debug.h>  /* for csp_print */
#include <stdlib.h>  		/* for exit */
#include <string.h>  		/* for strerror*/
#include <unistd.h>     	/* for read, close write */
#include <errno.h>			/* for error */
#include <fcntl.h>      	/* for openfcntl */
#include <termios.h>   		/* for tcgetattr, cfsetispeed*/
#include <assert.h>			/* for asset */

/**
 * @brief the tread to read data from the usart
 * 
 * @param [arg] the usart device context
 */
static void * usart_rx_thread(void * arg) {

	csp_kiss2_usart_context_t * ctx = arg;

	// Receive loop
	while (1) {
		int length = read(ctx->fd, ctx->cbuf, sizeof(ctx->cbuf));
		if (length <= 0) {
			csp_print("%s: read() failed, returned: %d\n", __func__, length);
			exit(1);
		}
		ctx->rx_callback(ctx->callback_ctx, ctx->cbuf, length, NULL);
	}
	return NULL;
}

/**
 * @brief Open a driver 
 * 
 * @param[in] driver_data   driver data from #csp_iface_t
 * @param[in] rx_callback  callback function to use when there is data
 * @param[in] callback_ctx the context that should be passed to the callback
 * @param[in] driver_param additional parameters that the driver may need (can be NULL)
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
static int csp_kiss2_usart_open(void * driver_data, csp_kiss2_rx_callback_t rx_callback, void * callback_ctx, void * driver_param)
{
	assert(driver_data != NULL);
	assert(rx_callback != NULL);
	assert(rx_callback != NULL);
	assert(driver_param != NULL);
	
	const csp_usart_conf_t * conf =  (const csp_usart_conf_t *)driver_param;
	int brate = 0;
	switch (conf->baudrate) {
		case 4800:
			brate = B4800;
			break;
		case 9600:
			brate = B9600;
			break;
		case 19200:
			brate = B19200;
			break;
		case 38400:
			brate = B38400;
			break;
		case 57600:
			brate = B57600;
			break;
		case 115200:
			brate = B115200;
			break;
		case 230400:
			brate = B230400;
			break;
#if !defined(__APPLE__)
		case 460800:
			brate = B460800;
			break;
		case 500000:
			brate = B500000;
			break;
		case 576000:
			brate = B576000;
			break;
		case 921600:
			brate = B921600;
			break;
		case 1000000:
			brate = B1000000;
			break;
		case 1152000:
			brate = B1152000;
			break;
		case 1500000:
			brate = B1500000;
			break;
		case 2000000:
			brate = B2000000;
			break;
		case 2500000:
			brate = B2500000;
			break;
		case 3000000:
			brate = B3000000;
			break;
#ifndef __CYGWIN__
		case 3500000:
			brate = B3500000;
			break;
		case 4000000:
			brate = B4000000;
			break;
#endif
#endif
		default:
			csp_print("%s: Unsupported baudrate: %u\n", __func__, conf->baudrate);
			return CSP_ERR_INVAL;
	}

	int fd = open(conf->device, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0) {
		csp_print("%s: failed to open device: [%s], errno: %s\n", __func__, conf->device, strerror(errno));
		return CSP_ERR_INVAL;
	}

	struct termios options;
	tcgetattr(fd, &options);
	cfsetispeed(&options, brate);
	cfsetospeed(&options, brate);
	options.c_cflag |= (CLOCAL | CREAD);
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
	options.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
	options.c_iflag &= ~(IGNBRK | BRKINT | ICRNL | INLCR | PARMRK | INPCK | ISTRIP | IXON);
	options.c_oflag &= ~(OCRNL | ONLCR | ONLRET | ONOCR | OFILL | OPOST);
	options.c_cc[VTIME] = 0;
	options.c_cc[VMIN] = 1;
	/* tcsetattr() succeeds if just one attribute was changed, should read back attributes and check all has been changed */
	if (tcsetattr(fd, TCSANOW, &options) != 0) {
		csp_print("%s: Failed to set attributes on device: [%s], errno: %s\n", __func__, conf->device, strerror(errno));
		close(fd);
		return CSP_ERR_DRIVER;
	}
	fcntl(fd, F_SETFL, 0);

	/* Flush old transmissions */
	if (tcflush(fd, TCIOFLUSH) != 0) {
		csp_print("%s: Error flushing device: [%s], errno: %s\n", __func__, conf->device, strerror(errno));
		close(fd);
		return CSP_ERR_DRIVER;
	}

	csp_kiss2_usart_context_t * ctx = (csp_kiss2_usart_context_t  *)driver_data;
	ctx->rx_callback = rx_callback;
	ctx->callback_ctx = callback_ctx;
	ctx->fd = fd;
	int ret;
	pthread_attr_t attributes;

	ret = pthread_attr_init(&attributes);
	if (ret != 0) {
		close(fd);
		return CSP_ERR_NOMEM;
	}
	pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
	ret = pthread_create(&ctx->rx_thread, &attributes, usart_rx_thread, ctx);
	if (ret != 0) {
		csp_print("%s: pthread_create() failed to create Rx thread for device: [%s], errno: %s\n", __func__, conf->device, strerror(errno));
		close(fd);
		return CSP_ERR_NOMEM;
	}
	ret = pthread_attr_destroy(&attributes);
	if (ret != 0) {
		csp_print("%s: pthread_attr_destroy() failed: %s, errno: %d\n", __func__, strerror(ret), ret);
	}

	pthread_mutex_init(&ctx->lock, NULL); 

	return CSP_ERR_NONE;

}

/**
 * @brief Start a driver transmission
 * 
 * @note: lock the driver before we use it.
 *
 * @param[in] driver_data driver data from #csp_iface_t
 */
static void csp_kiss2_usart_lock(void * driver_data){
	assert(driver_data != NULL);

	csp_kiss2_usart_context_t * ctx = (csp_kiss2_usart_context_t *)driver_data;
	pthread_mutex_lock(&ctx->lock);
}


/**
 * @brief flush the transmitted data
 * 
 * @note: This function is call after all the KISS data has been send
 *
 * @param[in] driver_data the driver context from #csp_iface_t
 */
static void csp_kiss2_usart_flush(void * driver_data){
	assert(driver_data != NULL);
	(void)driver_data;
}

/**
 * @brief finish a driver transmission
 * 
 * @note: unlock the driver now we are done
 * 
 * @param[in] driver_data driver data from #csp_iface_t
 */
static void csp_kiss2_usart_unlock(void * driver_data){
	assert(driver_data != NULL);
	
	csp_kiss2_usart_context_t * ctx = (csp_kiss2_usart_context_t *)driver_data;
	pthread_mutex_unlock(&ctx->lock);
}

/**
 * @brief Write KISS bytes
 * 
 * @note: this function is called as the kiss2 interface write byte out.
 *        There is no buffer, the packet is convered on the fly and the
 *        converted bytes sent to the driver.
 * 
 * @note: if this returned an error, packet transmission is terminated and 
 *        csp_kiss2_driver_tx_end_t called
 *
 * @param[in] driver_data driver data from #csp_iface_t
 * @param[in] data data to send
 * @param[in] len length of \a data.
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
static int csp_kiss2_usart_write(void * driver_data, const uint8_t * data, size_t data_length) {
	assert(driver_data != NULL);
	
	csp_kiss2_usart_context_t * ctx = (csp_kiss2_usart_context_t *)driver_data;

	if (ctx->fd >= 0) {
		int res = write(ctx->fd, data, data_length);
		if (res >= (int)data_length) {
			return CSP_ERR_NONE;
		}
	}
	return CSP_ERR_TX;  // best matching CSP error code.
}

static const csp_kiss2_driver_t s_driver = {
	.open = csp_kiss2_usart_open,
	.start = csp_kiss2_usart_lock,
	.write = csp_kiss2_usart_write,
	.flush = csp_kiss2_usart_flush,
	.finish = csp_kiss2_usart_unlock
};


int csp_usart_open_and_add_kiss2_interface(csp_kiss2_usart_context_t * driver_data, 
	                                       const csp_usart_conf_t * conf,
										   const char * ifname, uint16_t addr,
										   csp_iface_t ** return_iface) {

	return csp_driver_open_add_kiss2_interface(&driver_data->kiss_ctx,
		                                       &s_driver, driver_data, (void *)conf,
											   ifname, addr, return_iface);
}
