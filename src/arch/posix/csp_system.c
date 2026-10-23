#include <csp/csp_hooks.h>

#if defined(__APPLE__)
    #include <sys/sysctl.h>
    #include <sys/reboot.h>
#elif defined(__CYGWIN__)
    #include <csp/csp_debug.h>
#else
    #include <sys/sysinfo.h>
    #include <sys/reboot.h>
    #include <linux/reboot.h>
#endif

uint32_t csp_memfree_hook(void) {
#if defined(__APPLE__)
    int mib[2] = {CTL_HW, HW_USERMEM};
    uint64_t total = 0;
    size_t len = sizeof(total);
    if (sysctl(mib, 2, &total, &len, NULL, 0) == 0) {
        return (uint32_t)total;
    }
    return 0;
#else
	uint32_t total = 0;
	struct sysinfo info;
	sysinfo(&info);
	total = info.freeram * info.mem_unit;
	return total;
#endif
}

unsigned int csp_ps_hook(csp_packet_t * packet) {
	(void)packet; /* Avoid compiler warnings about unused parameter */
	return 0;
}

void csp_reboot_hook(void) {
#if defined(__APPLE__)
    reboot(RB_AUTOBOOT);
#elif defined(__CYGWIN__)
    csp_print("HALTED - Please reboot\n");
    while (true)
        sleep(1);
#else
	sync();
	reboot(LINUX_REBOOT_CMD_RESTART);
#endif
}

void csp_shutdown_hook(void) {
#if defined(__APPLE__)
    reboot(RB_HALT);
#elif defined(__CYGWIN__)
    csp_print("HALTED - Please power off\n");
    while (true)
        sleep(1);
#else
	sync();
	reboot(LINUX_REBOOT_CMD_HALT);
#endif
}
