#pragma once

#include <assert.h>
#include <stdio.h>

#define helix_assert(cond, reason) \
	assert(cond)

#define helix_unreachable(desc) \
	assert(false)

#define helix_warn(message, ...) \
	printf("[Warning] "); printf(message, __VA_ARGS__); fputc('\n', stdout)