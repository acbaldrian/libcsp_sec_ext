/**
 * @file csp_sec.h
 *
 * Security extensions for Cubesat Space Protocol
 *
 * The extension adds support for HMAC including a sequence number to prevent replay attacks.
 *
 * The HMAC is added to end of the packet, and is calculated over the entire packet
 * (optionally including the header). Additional a sequence number is added to the end of packet,
 * and is included in the HMAC calculation.
 *
 * The destination node keeps track of the last sequence number received from each source node,
 * and will reject any packet with a sequence number that is less than or equal to the last received
 * sequence number.
 *
 * The sender node keeps track of the last sequence number sent to each destination node,
 * and will increment the sequence number for each packet sent to that destination node.
 *
 * Sequence number overflow, i.e. what happens when the sequence number is MAX an needs to be
 * incremented.
 *
 * Option 1: Do not allow overflow, once we get to MAX message stop.
 *           Use a vey big sequence number so that will will not happen in the lifetime of
 *           the cubesat. This means adding a large number to the packet, and packets are
 *           of limited size.
 *
 * Option 2: allow overflow
 *
 *   An attacker can can record an packet, wait for overflow, and send the replay |(at least once)
 *
 *   If we checked the the sequence number was exactly one more than the last, the attack would
 *   have to  wait until the right sequence number came back round, this could be a siginitface
 *   time, if the sequence number was sufficiently large.
 *
 *   But we have a lossy protocol, a packet may be sent, not acknowledged (and not revived),
 *   The next packet sent would have the next sequence number.
 *
 * Option 3: allow overflow, with a test window
 *
 *   Any new new sequence number must be larger than the current sequence number but less that the
 *   current sequence number plus a window.
 *   If the Windown is sufficiently small, an attacker would still need to wait s substantial time
 *   before the recoded packet has a sequence number is within the window.
 *
 *   Should the client find that repeated massages timeout, the client can resend the sever
 *   hello message and update the the sequence number. The Attacker dose not receive an additional
 *   information from a sever hello.
 *
 *   In addition to the test window, the sequence number are started at a random number, to limit
 *   predictability.
 *
 *   With the Sequent number is 32 bits, assuming sending a new message every second.
 *   and a Window of 200, the replay window would take 136 years to repeat.
 *
 */
#pragma once

#include "csp/csp_types.h" /* for csp_packet_t*/
#include <stdint.h>        /* for uint8_t, uint16_t, uint32_t*/

#define CSP_SEC_PORT (10) /* The port to use for security extensions communications */

#define CSP_ERR_SEC     -200 /* security extension failed */
#define CSP_ERR_SEC_SEQ -201 /* Sequence number validation failed */

/**
 * @brief init the security extensions
 */
void csp_sec_init(void);

/**
 * @brief Append the sequence number to the end of the packer
 * 
 * @note the packet size is increased to include the sequence number
 * 
 * @param[in]  packet The packet to append the sequence number
 * 
 * @return #CSP_ERR_NONE  - The operation was successful
 *         <0 - #CSP_ERR_SEC_SEQ - The sequence number validation failed
 *              #CSP_ERR_NOMEM   - no space to append the sequence number
 */
int csp_sec_seq_num_append(csp_packet_t * packet) __attribute__((warn_unused_result));

/**
 * @brief validate the sequence number at the end of the packet,
 *        if valid the the sequence number is removed from the packet
 * 
 * @return #CSP_ERR_NONE  - The operation was successful
 *         <0 - #CSP_ERR_SEC_SEQ - The sequence number validation failed
 */
int csp_sec_seq_num_validate(csp_packet_t * packet) __attribute__((warn_unused_result)); 

/**
 * @brief Send a sever hello message to the node, to initiate the sequence numbers for HMAC
 *
 * @param[in] node    The node to exchange the key with
 * @param[in] timeout The timeout for the response
 * @param[in] opts    The connection options to use
 *
 * @return #CSP_ERR_NONE  - The operation was successful
 *         <0 - failed to initiate the sequence numbers
 */
int csp_sec_send_hello(uint16_t node, uint32_t timeout, uint8_t opts) __attribute__((warn_unused_result));

/**
 * @brief the handler for the security extensions
 *
 * @param[in] packet  The node to exchange the key with
 * 
 * @return #CSP_ERR_NONE  - The operation was successful
 *         <0 - failed with error
 */
int csp_sec_handler(csp_packet_t * packet) __attribute__((warn_unused_result));
