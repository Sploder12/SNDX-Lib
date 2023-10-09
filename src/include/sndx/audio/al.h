#pragma once

#define NOMINMAX

#ifndef WIN32_LEAN_AND_MEAN

#define WIN32_LEAN_AND_MEAN
#include <AL/al.h>
#include <AL/alc.h>
#undef WIN32_LEAN_AND_MEAN

#else 
#include <AL/al.h>
#include <AL/alc.h>
#endif