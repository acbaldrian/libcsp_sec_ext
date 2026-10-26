/****************************************************************************
 * **File:** drivers/kiss2/kiss2_usart_linux.c
 *
 * **Description:** USART driver for the kiss2 interface
 *
 * .. note:: This interface implementation only support ONE open UART connection.
 ****************************************************************************/

#include <csp/drivers/kiss2_usart.h>

#include <csp/csp_debug.h> /* for csp_print */
#include <stdlib.h>        /* for exit */
#include <string.h>        /* for strerror */
#include <unistd.h>        /* for read, write, close*/
#include <errno.h>         /* for errno */
#include <sys/ioctl.h>     /* for ioctl*/
#include <asm/termbits.h>  /* for Control character */
#include <fcntl.h>         /* for fcntl  */
#include <assert.h>        /* for asset */

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
 * @param[in] driver_data  driver data from #csp_iface_t
 * @param[in] rx_callback  callback function to use when there is data
 * @param[in] callback_ctx the context that should be passed to the callback
 * @param[in] driver_param additional parameters that the driver may need (can be NULL)
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
static int csp_kiss2_usart_open(void * driver_data, csp_kiss2_rx_callback_t rx_callback, void * callback_ctx, void * driver_param) {
	assert(driver_data != NULL);
	assert(rx_callback != NULL);
	assert(rx_callback != NULL);
	assert(driver_param != NULL);

	const csp_usart_conf_t * conf = (const csp_usart_conf_t *)driver_param;

	int fd = open(conf->device, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0) {
		csp_print("%s: failed to open device: [%s], errno: %s\n", __func__, conf->device, strerror(errno));
		return CSP_ERR_INVAL;
	}

	struct termios2 options;
	if (ioctl(fd, TCGETS2, &options) != 0) {
		close(fd);
		return CSP_ERR_DRIVER;
	}
	options.c_cflag &= ~(CBAUD | CIBAUD);
	options.c_cflag |= BOTHER | (BOTHER << IBSHIFT);
	options.c_ispeed = conf->baudrate;
	options.c_ospeed = conf->baudrate;
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
	if (ioctl(fd, TCSETS2, &options) != 0) {
		csp_print("%s: Failed to set attributes on device: [%s], errno: %s\n", __func__, conf->device, strerror(errno));
		close(fd);
		return CSP_ERR_DRIVER;
	}
	fcntl(fd, F_SETFL, 0);

	/* Flush old transmissions */
	if (ioctl(fd, TCFLSH, TCIOFLUSH) != 0) {
		csp_print("%s: Error flushing device: [%s], errno: %s\n", __func__, conf->device, strerror(errno));
		close(fd);
		return CSP_ERR_DRIVER;
	}

	csp_kiss2_usart_context_t * ctx = (csp_kiss2_usart_context_t *)driver_data;
	ctx->rx_callback = rx_callback;
	ctx->callback_ctx = callback_ctx;
	ctx->fd = fd;
	int ret;
	pthread_attr_t attributes;

	pthread_mutex_init(&ctx->lock, NULL);

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

	return CSP_ERR_NONE;
}

/**
 * @brief Start a driver transmission
 *
 * @note: lock the driver before we use it.
 *
 * @param[in] driver_data driver data from #csp_iface_t
 */
static void csp_kiss2_usart_lock(void * driver_data) {
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
static void csp_kiss2_usart_flush(void * driver_data) {
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
static void csp_kiss2_usart_unlock(void * driver_data) {
	assert(driver_data != NULL);

	csp_kiss2_usart_context_t * ctx = (csp_kiss2_usart_context_t *)driver_data;
	pthread_mutex_unlock(&ctx->lock);
}

/**
 * @brief Write KISS bytes
 *
 * @note: this function is called as the kiss2 interface write byte out.
 *        There is no buffer, the packet is converted on the fly and the
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
	assert(data != NULL);

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
	.finish = csp_kiss2_usart_unlock};

int csp_usart_open_and_add_kiss2_interface(csp_kiss2_usart_context_t * driver_data,
										   const csp_usart_conf_t * conf,
										   const char * ifname, uint16_t addr,
										   csp_iface_t ** return_iface) {
	assert(driver_data != NULL);
	assert(conf != NULL);
	return csp_driver_open_add_kiss2_interface(&driver_data->kiss_ctx, &s_driver,
											   driver_data, (void *)conf,
											   ifname, addr,
											   return_iface);
}
