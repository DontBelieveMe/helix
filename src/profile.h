#pragma once

#if defined(HAS_TRACY)
	#include <Tracy.hpp>

	#define HELIX_PROFILE_END FrameMark
	#define HELIX_PROFILE_ZONE ZoneScoped
	#define HELIX_PROFILE_ZONE_TEXT ZoneText
#else
	#define HELIX_PROFILE_ZONE
	#define HELIX_PROFILE_END
	#define HELIX_PROFILE_ZONE_TEXT
#endif

