#include "../../csp_semaphore.h"

#include <inttypes.h>
#include <csp/csp.h>
#include <csp/csp_debug.h>

#include <semaphore.h>
#include <time.h>

void csp_bin_sem_init(csp_bin_sem_t * sem) {
#if (__APPLE__)
    *sem = dispatch_semaphore_create(1);
#else
  	sem_init((sem_t *) sem, 0, 1);
#endif
}

int csp_bin_sem_wait(csp_bin_sem_t * sem, unsigned int timeout) {
#if defined(__APPLE__)
    if (timeout == CSP_MAX_TIMEOUT) {
        dispatch_semaphore_wait(*sem, DISPATCH_TIME_FOREVER);
    } else {
        dispatch_time_t t = dispatch_time(DISPATCH_TIME_NOW, (int64_t)timeout * 1000000LL);
        if (dispatch_semaphore_wait(*sem, t) != 0) {
            return CSP_SEMAPHORE_ERROR;
        }
    }
    return CSP_SEMAPHORE_OK;
#else
	int ret;

	if (timeout == CSP_MAX_TIMEOUT) {
		ret = sem_wait((sem_t *) sem);
	} else {
		struct timespec ts;
		if (clock_gettime(CLOCK_REALTIME, &ts)) {
			return CSP_SEMAPHORE_ERROR;
		}

		uint32_t sec = timeout / 1000;
		uint32_t nsec = (timeout - 1000 * sec) * 1000000;

		ts.tv_sec += sec;

		if (ts.tv_nsec + nsec >= 1000000000) {
			ts.tv_sec++;
		}

		ts.tv_nsec = (ts.tv_nsec + nsec) % 1000000000;

		ret = sem_timedwait((sem_t *) sem, &ts);
	}

	if (ret != 0)
		return CSP_SEMAPHORE_ERROR;

	return CSP_SEMAPHORE_OK;
#endif
}

int csp_bin_sem_post(csp_bin_sem_t * sem) {
#if defined(__APPLE__)
    dispatch_semaphore_signal(*sem);
    return CSP_SEMAPHORE_OK;
#else
	int value;
	sem_getvalue((sem_t *) sem, &value);
	if (value > 0) {
		return CSP_SEMAPHORE_OK;
	}

	if (sem_post((sem_t *) sem) == 0) {
		return CSP_SEMAPHORE_OK;
	}

	return CSP_SEMAPHORE_ERROR;
#endif
}
