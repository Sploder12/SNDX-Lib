#pragma once

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4232)
#pragma warning(disable: 4244)
#pragma warning(disable: 4267) 
#pragma warning(disable: 4305)
#pragma warning(disable: 4456)
#pragma warning(disable: 4457)
#pragma warning(disable: 4459)
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#define OGG_IMPL
#define VORBIS_IMPL
#include "minivorbis.h"

#undef _CRT_SECURE_NO_WARNINGS

#else

#define OGG_IMPL
#define VORBIS_IMPL
#include "minivorbis.h"

#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif
