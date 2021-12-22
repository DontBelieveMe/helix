add_library(LibC
	libc.c
	
	include/stddef.h
)

set_target_properties(LibC PROPERTIES OUTPUT_NAME libc)
