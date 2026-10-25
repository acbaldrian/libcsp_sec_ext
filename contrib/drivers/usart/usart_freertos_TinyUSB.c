/**
 * @file usart_usb_freertos.c
 * @brief USART driver for USB CDC devices using FreeRTOS.
 * 
 * The Arduino USB CDC driver uses TinyUSB stack
 */

#include "csp/drivers/usart.h"  /* for the defintion of the usart driver */

#include <task.h>               /* for xTaskCreate, TaskHandle_t  */
/* define the location of the tusb config file, before including tusb.h */
#define CFG_TUSB_CONFIG_FILE <../tusb_config.h>
#include <tusb.h>
#include <semphr.h> /* for SemaphoreHandle_t*/
#include <FreeRTOSConfig.h>  /* for configMAX_PRIORITIES 32 on the pi pico */

/* 1000 ns = 1 µs → rounds up to 1 tick (1 ms at 1000 Hz tick rate) */
#define CPS_USB_RX_DELAY_TICKS  (pdMS_TO_TICKS(1))
#define CPS_USB_RX_STACK_SIZE_BYTES (1024)
#define CPS_USB_RX_STACK_DEPTH (CPS_USB_RX_STACK_SIZE_BYTES/sizeof(StackType_t))
#define CPS_USB_RX_PRIORITY   (configMAX_PRIORITIES-4)

typedef struct {
	csp_usart_callback_t rx_callback;
	void * user_data;
    SemaphoreHandle_t tud_cdc_driver_lock;
    TaskHandle_t xHandle;
	uint32_t cbuf_len;
	uint8_t cbuf[CFG_TUD_CDC_RX_BUFSIZE];
} usart_context_t;

static usart_context_t s_usart_context = {};

/**
 * @brief The thread to handle the received characters
 * 
 * @param[in] pvParameters, this will be the usart context
 */
static void csp_usart_rx_thread(void * pvParameters) {

	usart_context_t * ctx = (usart_context_t *)pvParameters;

	for( ; ;) {
        if (tud_cdc_available()) {
            ctx->cbuf_len = tud_cdc_read(ctx->cbuf, sizeof(ctx->cbuf));
            if (ctx->cbuf_len > 0) {
                ctx->rx_callback(ctx->user_data, ctx->cbuf, ctx->cbuf_len, NULL);
                ctx->cbuf_len = 0;
            }
        } else {
            vTaskDelay(CPS_USB_RX_DELAY_TICKS);
        }
	}
}

/*
 * usart driver API
 */

/**
 * @brief open an UART device.
 *
 * @note this is using the TinyUSB CDC device directly.
 * 
 * @param[in] conf UART configuration. ** Not used **
 * @param[in] rx_callback receive data callback.
 * @param[in] user_data reference forwarded to the rx_callback function.
 * @param[out] fd the opened file descriptor. ** Not used ** will be zero
 * @return #CSP_ERR_NONE on success, #CSP_ERR_NOMEM if filed to create rx task 
 */
int csp_usart_open(const csp_usart_conf_t * conf, csp_usart_callback_t rx_callback, void * user_data, csp_usart_fd_t * fd) {
    (void)conf;
    if (rx_callback == NULL) {
		return CSP_ERR_INVAL;
	}

    /* check the the tub_cdc device is ready */
    s_usart_context.rx_callback = rx_callback;
    s_usart_context.user_data = user_data;
    s_usart_context.tud_cdc_driver_lock = xSemaphoreCreateMutex();
    if (s_usart_context.tud_cdc_driver_lock == NULL) {
        return CSP_ERR_NOMEM;
    }

    BaseType_t err = xTaskCreate(csp_usart_rx_thread, "csp_usart_rx_thread", CPS_USB_RX_STACK_DEPTH, &s_usart_context, CPS_USB_RX_PRIORITY, &s_usart_context.xHandle);
    if (err != pdPASS) {
        return CSP_ERR_NOMEM;
    }
    *fd = 0;
	return CSP_ERR_NONE;

}

/**
 * @brief Write data on open UART.
 *
 * @note This is using the TinyUSB CDC device directly.
 *       the data is not flushed out
 *
 * @param[in] fd the opened file descriptor. *** not used ***
 * @param[in] data Outbound data to send over the USART interface.
 * @param[in] data_length Outbound data length.
 * @return number of bytes written on success, a negative value on failure.
 */
int csp_usart_write(csp_usart_fd_t fd, const void * data, size_t data_length) {
    (void) fd;
    uint32_t count = tud_cdc_write(data, data_length);
    
    // Flush to ensure data is sent immediately
    tud_cdc_write_flush();
    return (int)count;
}

void csp_usart_lock(void * driver_data) {
    (void)driver_data;
    xSemaphoreTake(s_usart_context.tud_cdc_driver_lock, portMAX_DELAY);
}

void csp_usart_unlock(void * driver_data) {
    (void) driver_data;
    xSemaphoreGive(s_usart_context.tud_cdc_driver_lock);
}
