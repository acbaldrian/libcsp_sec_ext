if(CHECK_FOUND)
  add_executable(csp_tests)
  target_link_libraries(csp_tests PRIVATE csp ${CHECK_LIBRARIES})
  target_sources(csp_tests PRIVATE
    main.c
    queue.c
    buffer.c
    hmac.c
    route.c
    wire.c
  )
  if(LIBZMQ_FOUND)
    target_sources(csp_tests PRIVATE zmqhub.c)
    target_include_directories(csp_tests PRIVATE ${LIBZMQ_INCLUDE_DIRS})
    target_link_libraries(csp_tests PRIVATE ${LIBZMQ_LIBRARIES})
  endif()
  if(CSP_USE_HMAC)
    target_sources(csp_tests PRIVATE
      hmac.c
    )
    if(CSP_USE_SHA1)
    else()
      target_sources(csp_tests PRIVATE
        test_sha256.c
      )
    endif()
  endif()

endif()

