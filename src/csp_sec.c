#include <csp/csp_sec.h>			/* for the sec API */

#include <csp/csp.h>             	/* for csp_sendto_reply */
#include <csp/csp_error.h>       	/* for CSP_ERR_NONE, CSP_ERR_INVAL */
#include <csp/csp_buffer.h>      	/* for csp_buffer_free */
#include <csp/crypto/csp_hmac.h> 	/* for CSP_HMAC_RECORD_LENGTH */
#include <csp/arch/csp_random.h>	/* for csp_random_init, csp_random csp_random_crypto*/

#include <stdbool.h>	/* for bool */
#include <assert.h>     /* for assert*/
#include <endian.h>     /* for be32toh */
#include <string.h>		/* for memcpy */
#include <sys/types.h>  /* for ssize_t*/

#define CSP_SEC_CACHE_CAPACITY 32 		/* Capacity of the sequence number cache, maximum number of client we can handle*/

#define CSP_SEC_SEQ_CHECK_WINDOW 200 	/* The windows size for sequence number validation */

/**
 * @brief The codes for the CPS security extensions
 */
typedef enum {
	CSP_SEC_HELLO = 1,
	CSP_SEC_HELLO_COMPLETE = 2,
} csp_sec_code_t;

/**
 * @brief All CSP Security messages start with a code. The dispatcher validates this
 *        header before casting packet data to one of the concrete packed layouts below.
 */
typedef struct __attribute__((__packed__)) {
	uint8_t code;
} csp_sec_header_t;

/**
 * @brief Packed CSP SEC key exchange init messages.
 */
typedef struct __attribute__((__packed__)) {
	uint8_t code;        /* The message code */
	uint32_t seq_server; /* The sequence number to use for this node */
	uint32_t seq_client; /* The sequence number to use for the client node */
} csp_sec_hello_t;

/**
 * @brief The sequence number cache item
 */
typedef struct {
	uint16_t node;
	uint32_t seq_receive; /* The sequence number received from the node */
	uint32_t seq_send;    /* The sequence number to send to the node */
} seq_item_t;

/**
 * @brief The sequence number cache
 */
typedef struct {
	seq_item_t items[CSP_SEC_CACHE_CAPACITY];
	size_t len;
} seq_cache_t;

/**
 * @brief The sequence number cache for out going packets
 */
static seq_cache_t s_seq_cache;

/**
 * @brief Validate a sequence number received from the node against the received sequence cache
 *
 * @note  Search the sequence cache for the last sequence number received from the node (server).
 *        If the sequence number is greater  than the last sequence number
 *        received from the node, the sequence number is valid.
 *        The sequence number is updated in the cache.
 *
 * @param[in] cache The sequence cache to validate that the sequence number
 * @param[in] node  Node address
 * @param[in] seq   Sequence number
 *
 * @return #CSP_ERR_NONE  - The sequence number is valid
 *         <0 - #CSP_ERR_SEC, The sequence number is invalid
 */
static int csp_sec_seq_cache_validate(seq_cache_t * cache, uint16_t node, uint32_t seq) {

	assert(cache != NULL);

	/* Search for existing entry */
	size_t index = 0;
	bool found = false;
	for (; index < cache->len; index++) {
		if (cache->items[index].node == node) {
			found = true;
			break;
		}
	}

	if (found &&
		/* Check if the sequence number is valid
		 * the new sequence number must be grater then the last sequence number received,
		 * but less than last plus the windows size
		 *
		 * We also need the check for overflow, as the protocol is lossy
		 * the last received may not be MAX, but close, equally the next
		 * may not be one, but a low value.
		 */

		((seq > cache->items[index].seq_receive &&
		  (size_t)seq <= ((size_t)cache->items[index].seq_receive + (size_t)CSP_SEC_SEQ_CHECK_WINDOW)) ||
		 (cache->items[index].seq_receive > (UINT32_MAX - CSP_SEC_SEQ_CHECK_WINDOW) &&
		  seq < CSP_SEC_SEQ_CHECK_WINDOW &&
		  (UINT32_MAX - cache->items[index].seq_receive + seq <= CSP_SEC_SEQ_CHECK_WINDOW)))) {
		cache->items[index].seq_receive = seq;
		return CSP_ERR_NONE;
	}
	return CSP_ERR_SEC;
}

/**
 * @brief Get the sequence number to send to the node
 *
 * @note  Search the sequence cache for the last sequence number sent to the node.
 *        If the node is found in the cache, the sequence number is incremented and returned.
 *
 * @param[in]  cache The sequence cache
 * @param[in]  node  Node address
 * @param[out] seq   The sequence number
 *
 * @return #CSP_ERR_NONE  - The operation was successful
 *         <0 - #CSP_ERR_SEC - The node not found
 */
static int csp_sec_seq_cache_get(seq_cache_t * cache, uint16_t node, uint32_t * seq) {

	assert(cache != NULL);
	assert(seq != NULL);

	/* Search for existing entry */
	size_t index = 0;
	bool found = false;
	for (; index < cache->len; index++) {
		if (cache->items[index].node == node) {
			found = true;
			break;
		}
	}

	if (found) {
		/* The sequence number of zero is use for the sever hello and hello complete
		 * do not use zero for normal message */
		if (++cache->items[index].seq_send == 0) {
			++cache->items[index].seq_send;
		}
		*seq = cache->items[index].seq_send;
		return CSP_ERR_NONE;
	}
	return CSP_ERR_SEC;
}

/**
 * @brief Initialize the sequence number for a node
 *
 * @note  If the node exists in the cache, the existing sequence numbers are returned.
 * 	      Otherwise  new entry is created with random sequence numbers.
 *
 * @param[in]  cache       The sequence cache
 * @param[in]  node        Node address
 * @param[out] seq_receive The sequence number that will be used to validate the received sequence number from the node
 * @param[out] seq_send    The sequence number that will sent to the node
 *
 * @return #CSP_ERR_NONE  - The operation was successful
 *         <0 - #CSP_ERR_NOMEM - No more space in the cache
 *              #CSP_ERR_SEC - unable to generate a random sequence number
 */
static int csp_sec_seq_cache_init(seq_cache_t * cache, uint16_t node,
								  uint32_t * seq_receive,
								  uint32_t * seq_send) {
	assert(cache != NULL);
	assert(seq_receive != NULL);
	assert(seq_send != NULL);

	/* Search for existing entry */
	size_t index = 0;
	bool found = false;
	for (; index < cache->len; index++) {
		if (cache->items[index].node == node) {
			found = true;
			break;
		}
	}

	if (found) {
		/* Return the existing sequence numbers */
		*seq_receive = cache->items[index].seq_receive;
		*seq_send = cache->items[index].seq_send;
	} else {
		/* Add a new entry to the cache */
		if (cache->len >= CSP_SEC_CACHE_CAPACITY) {
			return CSP_ERR_NOMEM;
		}
		index = cache->len++;
		csp_random_crypto((uint8_t *)seq_receive, sizeof(uint32_t));
		csp_random_crypto((uint8_t *)seq_send, sizeof(uint32_t));
		cache->items[index].node = node;
		cache->items[index].seq_receive = *seq_receive;
		cache->items[index].seq_send = *seq_send;
	}
	return CSP_ERR_NONE;
}

/**
 * @brief Add a node to the sequence number cache
 *
 * @note This operation failed if the node is allready in the cache
 *
 * @param[in] cache       The sequence cache
 * @param[in] node        Node address
 * @param[in] seq_receive The sequence number the will be used to validate the received sequence number from the node
 * @param[in] seq_send    The sequence number that will sent to the node
 *
 * @return #CSP_ERR_NONE  - The operation was successful
 *         <0 - #CSP_ERR_NOMEM - No more space in the cache
 *              #CSP_ERR_SEC   - The node is allready in the cache
 */
static int csp_sec_seq_cache_add(seq_cache_t * cache, uint16_t node,
								 uint32_t seq_receive,
								 uint32_t seq_send) {
	assert(cache != NULL);

	/* Search for existing entry */
	size_t index = 0;
	bool found = false;
	for (; index < cache->len; index++) {
		if (cache->items[index].node == node) {
			found = true;
			break;
		}
	}

	if (found) {
		return CSP_ERR_SEC;
	} else {
		/* Add a new entry to the cache */
		if (cache->len >= CSP_SEC_CACHE_CAPACITY) {
			return CSP_ERR_NOMEM;
		}
		index = cache->len++;
		cache->items[index].node = node;
		cache->items[index].seq_receive = seq_receive;
		cache->items[index].seq_send = seq_send;
	}
	return CSP_ERR_NONE;
}

/**
 * @brief handle the client request to sever hello
 *
 * @note The in coming packet is reused as the reply packet
 *
 * @param[in] packet the request packet
 *
 * @return #CSP_ERR_NONE       - The operation was successful
 *         <0 - #CSP_ERR_NOMEM - No more space in the cache
 */
static int csp_sec_hello_handler(csp_packet_t * packet) {
	assert(packet != NULL);
	assert(((csp_sec_header_t *)packet->data)->code == CSP_SEC_HELLO);

	if (packet->length != sizeof(csp_sec_header_t)) {
		return CSP_ERR_SEC;
	}

	/* Use the existing packet for the out going reply */
	csp_sec_hello_t * sec_hello = (csp_sec_hello_t *)packet->data;

	packet->length = sizeof(csp_sec_hello_t);
	/* The csp_sec_hello_t struct must fit in to one packet */
	assert(sizeof(csp_sec_hello_t) < CSP_BUFFER_SIZE - (sizeof(uint32_t) + CSP_HMAC_LENGTH));

	/* Initialize the sequence number cache for this node */
	uint32_t seq_client;
	uint32_t seq_server;

	int err = csp_sec_seq_cache_init(&s_seq_cache, packet->id.src, &seq_client, &seq_server);
	if (err != CSP_ERR_NONE) {
		return err;
	}

	sec_hello->code = CSP_SEC_HELLO_COMPLETE;
	sec_hello->seq_client = htobe32(seq_client);
	sec_hello->seq_server = htobe32(seq_server);

	/* Replay to the incoming address and port 
	 * use the data in the packet as the response, 
	 * the is done in the service handler */
	return CSP_ERR_NONE;
}

void csp_sec_init(void) {

	s_seq_cache.len = 0;
	csp_random_init();
}

/**
 * @brief Validate a sequence number received from the node against the received sequence cache
 *
 * @note  Search the sequence cache for the last sequence number received from the node (server).
 *        If the sequence number is greater  than the last sequence number
 *        received from the node, the sequence number is valid.
 *        The sequence number is updated in the cache.
 *
 * @note Before the Sequence number  is validated, the packet is checked to see if it is a
 *       SEC, server hello, if it is and  sequence numbner is zero then is is valid.
 *       The hello server message is used to initialise the Sequence numbers, so by definition can
 *       not contain a valid sequence number.
 *       The hello server returns the current sequence numbers, replaying this massage will have no
 *       effect, other than having to respond to a packet. (possible DOS...)
 *
 * @param[in] packet The packet with the sequence number to validate
 * @param[in] seq    Sequence number
 *
 * @return #CSP_ERR_NONE  - The sequence number is valid
 *         <0 - #CSP_ERR_SEC, The sequence number is invalid
 */
static int csp_sec_seq_validate(const csp_packet_t * packet, uint32_t seq) {

	assert(packet != NULL);

	/* If we are sending a Sever hello or responding with a hello complete message and have sequence number of zero it is valid */
	/* This requires an understanding the internal implementation of the packet and the sequence number record */
	if (seq == 0 &&
		((packet->id.dport == CSP_SEC_PORT && packet->length == sizeof(csp_sec_header_t) + sizeof(seq) && ((csp_sec_header_t *)packet->data)->code == CSP_SEC_HELLO) ||
		 (packet->id.sport == CSP_SEC_PORT && packet->length == sizeof(csp_sec_hello_t) + sizeof(seq) && ((csp_sec_hello_t *)packet->data)->code == CSP_SEC_HELLO_COMPLETE))) {
		return CSP_ERR_NONE;
	}
	return csp_sec_seq_cache_validate(&s_seq_cache, packet->id.src, seq);
}

/**
 * @brief Get the sequence number to send to the node
 *
 * @note  Search the sequence cache for the last sequence number sent to the node.
 *        If the node is found in the cache, the sequence number is incremented and returned.
 *
 * @note Before the Sequence number is retrieved from the cache,  the packet is checked to see if it is a
 *       SEC, server hello, if it is zero is returned as the sequence numbner.
 *
 * @param[in]  packet The packet to get the sequence number for
 * @param[out] seq    The sequence number
 *
 * @return #CSP_ERR_NONE  - The operation was successful
 *         <0 - #CSP_ERR_SEC - The node not found
 *              #CSP_ERR_INVAL - The seq pointer is NULL
 */
static int csp_sec_seq_get(const csp_packet_t * packet, uint32_t * seq) {

	assert(packet != NULL);
	assert(seq != NULL);

	/* If we are sending a Sever hello or responding with a hello complete message the sequence number will be zero */
	if ((packet->id.dport == CSP_SEC_PORT && packet->length == sizeof(csp_sec_header_t) && ((csp_sec_header_t *)packet->data)->code == CSP_SEC_HELLO) ||
		(packet->id.sport == CSP_SEC_PORT && packet->length == sizeof(csp_sec_hello_t) && ((csp_sec_hello_t *)packet->data)->code == CSP_SEC_HELLO_COMPLETE)) {
		*seq = 0;
		return CSP_ERR_NONE;
	}
	return csp_sec_seq_cache_get(&s_seq_cache, packet->id.dst, seq);
}

int csp_sec_seq_num_append(csp_packet_t * packet) {

	assert(packet != NULL);

	uint32_t seq;

	if (((size_t)packet->length + sizeof(seq)) > sizeof(packet->data)) {
		/* no space in the packet to append the sequence number */
		return CSP_ERR_NOMEM;
	}

	int err = csp_sec_seq_get(packet, &seq);
	if (err != CSP_ERR_NONE) {
		return err;
	}
	seq = htobe32(seq);
	memcpy(&packet->data[packet->length], &seq, sizeof(seq));
	packet->length += sizeof(seq);
	return CSP_ERR_NONE;
}

int csp_sec_seq_num_validate(csp_packet_t * packet) {

	assert(packet != NULL);

	uint32_t seq;
	/* The packet must be atleast 1 byte longer than the size of the sequence number */
	if (packet->length <= sizeof(seq)) {
		return CSP_ERR_SEC_SEQ;
	}
	memcpy(&seq, &packet->data[packet->length - sizeof(seq)], sizeof(seq));
	seq = be32toh(seq);
	if (csp_sec_seq_validate(packet, seq) != CSP_ERR_NONE) {
		/* Sequence number check failed */
		return CSP_ERR_SEC_SEQ;
	}
	/* Remove the sequence number from the packet */
	packet->length -= sizeof(seq);
	return CSP_ERR_NONE;
}

int csp_sec_send_hello(uint16_t node, uint32_t timeout, uint8_t opts) {

	csp_sec_header_t sec_hdr = {.code = CSP_SEC_HELLO};
	csp_sec_hello_t sec_hello_complete = {0};

	/* Send a server hello, and wait for response */
	int result = csp_transaction_w_opts(CSP_PRIO_NORM, node, CSP_SEC_PORT, timeout,
										&sec_hdr, sizeof(csp_sec_header_t),
										&sec_hello_complete, sizeof(csp_sec_hello_t), opts);
	if (result == 0) {
		return CSP_ERR_TIMEDOUT;
	} else if (result < 0) {
		return result;
	}

	if (sec_hello_complete.code != CSP_SEC_HELLO_COMPLETE) {
		return CSP_ERR_SEC;
	}

	sec_hello_complete.seq_client = be32toh(sec_hello_complete.seq_client);
	sec_hello_complete.seq_server = be32toh(sec_hello_complete.seq_server);
	return csp_sec_seq_cache_add(&s_seq_cache, node, sec_hello_complete.seq_server, sec_hello_complete.seq_client);
}

int csp_sec_handler(csp_packet_t * packet) {
	assert(packet != NULL);
	assert(packet->id.dport == CSP_SEC_PORT);
	
	csp_sec_header_t * sec_header = (csp_sec_header_t *)packet->data;

	int err;
	switch (sec_header->code) {
		case CSP_SEC_HELLO:
			err = csp_sec_hello_handler(packet);
			break;

		default:
			err = CSP_ERR_INVAL;
	}

	return err;
}
