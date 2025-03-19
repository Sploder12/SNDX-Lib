#pragma once

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#define UNDEF_MINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define UNDEF_LEANMEAN
#endif

#include <windows.h>

#ifdef UNDEF_MINMAX
#undef NOMINMAX
#undef UNDEF_MINMAX
#endif

#ifdef UNDEF_LEANMEAN
#undef WIN32_LEAN_AND_MEAN
#undef UNDEF_LEANMEAN
#endif

#endif