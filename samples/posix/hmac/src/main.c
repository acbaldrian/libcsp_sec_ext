#include <string.h>

#include <csp/csp.h>
#include <csp/csp_debug.h>
#include <csp/csp_id.h>
#include <csp/crypto/csp_hmac.h>

int main(int argc, char * argv[])
{
	csp_packet_t * packet;

	csp_init();

	packet = csp_buffer_get(0);
	if (packet == NULL) {
		return 1;
	}

	csp_id_prepend(packet);

	memcpy(packet->data, "abc", 3);
	packet->length += 3;
	packet->frame_length += 3;

	int err = csp_hmac_append(packet, true);
	if (err != CSP_ERR_NONE) {
		return 1;
	}

	err = csp_hmac_verify(packet, true);
	if (err != CSP_ERR_NONE) {
		return 1;
	}

	return 0;
}
