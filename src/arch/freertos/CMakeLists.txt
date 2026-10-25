target_sources(csp PRIVATE
  csp_clock.c
  csp_queue.c
  csp_semaphore.c
  csp_system.c
  csp_time.c
  csp_hooks.c
)

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


target_link_libraries(csp PRIVATE freertos_kernel)
