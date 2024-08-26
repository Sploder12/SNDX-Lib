#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#define UNDEF_NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define UNDEF_WIN32_LEAN_AND_MEAN
#endif

#include <AL/al.h>
#include <AL/alc.h>

#ifdef UNDEF_WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#undef UNDEF_WIN32_LEAN_AND_MEAN
#endif

#ifdef UNDEF_NOMINMAX
#undef NOMINMAX
#undef UNDEF_NOMINMAX
#endif