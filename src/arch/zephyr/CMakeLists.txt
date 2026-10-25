target_sources(csp PRIVATE
  csp_clock.c
  csp_hooks.c
  csp_queue.c
  csp_semaphore.c
  csp_time.c
  csp_zephyr_init.c
  )

# if atomic operations are not defined
if (CONFIG_ARMV6_M_ARMV8_M_BASELINE)
  target_sources(csp PRIVATE
    csp_atomic.c
  )
endif()

if (CSP_USE_HMAC)
  target_sources(csp PRIVATE
    csp_memset.c
  )
endif()

if (CSP_USE_SEC)
  target_sources(csp PRIVATE
    csp_sec_alloc.c
    csp_random.c
  )
endif()
