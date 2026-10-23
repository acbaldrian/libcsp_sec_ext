csp_sources += files([
	'csp_queue.c',
	'csp_semaphore.c',
	'csp_system.c',
	'csp_time.c',
	'csp_clock.c',
	'pthread_queue.c'
])

csp_deps += dependency('threads')

if get_option('use_sec')
	csp_sources += files('csp_random.c', 'csp_sec_alloc.c')
endif
