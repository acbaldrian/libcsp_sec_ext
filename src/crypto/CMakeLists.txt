if(CSP_USE_HMAC)
  target_sources(csp PRIVATE
    csp_hmac.c
  )

  if(CSP_USE_SHA1)
    message(WARNING "building SHA1 instead of SHA256, this is not recommended, please use SHA256 if possible")

    target_sources(csp PRIVATE
      csp_sha1.c
    )
  else()
    target_sources(csp PRIVATE
      csp_sha256.c
    )
  endif()
endif()
