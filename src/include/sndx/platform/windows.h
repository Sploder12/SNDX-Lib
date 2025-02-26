#pragma once

#ifdef _WIN32

#ifndef NOMINMAX
#define UNDEF_MINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define UNDEF_LEANMEAN
#endif

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#ifdef UNDEF_MINMAX
#undef NOMINMAX
#undef UNDEF_MINMAX
#endif

#ifdef UNDEF_LEANMEAN
#undef WIN32_LEAN_AND_MEAN
#undef UNDEF_LEANMEAN
#endif

#endif