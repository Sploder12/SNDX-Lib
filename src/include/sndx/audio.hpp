#pragma once

#include "./audio/audio_decoder.hpp"

#ifndef SNDX_NO_MP3
#include "./audio/mp3.hpp"
#endif

#ifndef SNDX_NO_OGG
#include "./audio/vorbis.hpp"
#endif

#ifndef SNDX_NO_WAV
#include "./audio/wav.hpp"
#endif

#ifndef SNDX_NO_AL
#include "./audio/al.hpp"
#endif