#pragma once

#include <assert.h>
#include <stdio.h>

#define helix_assert(cond, reason) \
	assert(cond && reason)

#define helix_unreachable(desc) \
	assert(false && desc)

/*
#define helix_warn(message, ...) \
	printf("[Warning] "); printf(message, __VA_ARGS__); fputc('\n', stdout)
*/

#define helix_warn(message, ...) (void)0


#define HELIX_NO_STEAL(ClassName) \
	ClassName(const ClassName&) = delete; \
	ClassName(ClassName&&) = delete; \
	ClassName& operator=(const ClassName&) = delete; \
	ClassName& operator=(ClassName&&) = delete