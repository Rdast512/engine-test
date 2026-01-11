#pragma once

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#else
// Define dummy macros if Tracy is disabled
#define ZoneScoped
#define ZoneScopedN(x)
#define FrameMark
#define FrameMarkNamed(x)
#endif
