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

#include <stdexcept>

namespace sndx::audio {
	enum class ALformat : ALenum {
		mono8 = AL_FORMAT_MONO8,
		mono16 = AL_FORMAT_MONO16,
		stereo8 = AL_FORMAT_STEREO8,
		stereo16 = AL_FORMAT_STEREO16,
	};

	[[nodiscard]]
	constexpr bool isMono(ALformat format) noexcept {
		return format == ALformat::mono8 || format == ALformat::mono16;
	}

	[[nodiscard]]
	constexpr bool isStereo(ALformat format) noexcept {
		return format == ALformat::stereo8 || format == ALformat::stereo16;
	}

	[[nodiscard]]
	constexpr bool is8Bit(ALformat format) noexcept {
		return format == ALformat::mono8 || format == ALformat::stereo8;
	}

	[[nodiscard]]
	constexpr bool is16Bit(ALformat format) noexcept {
		return format == ALformat::mono16 || format == ALformat::stereo16;
	}

	[[nodiscard]]
	constexpr ALformat determineALformat(short bitDepth, short channels) {
		using enum ALformat;
		if (channels == 1) {
			if (bitDepth == 8) {
				return mono8;
			}
			else if (bitDepth == 16) {
				return mono16;
			}
		}
		else if (channels == 2) {
			if (bitDepth == 8) {
				return stereo8;
			}
			else if (bitDepth == 16) {
				return stereo16;
			}
		}

		throw std::invalid_argument("No matching ALformat");
	}

	[[nodiscard]]
	constexpr short getBitDepth(ALformat format) noexcept {
		if (is8Bit(format)) return 8;

		return 16;
	}

	[[nodiscard]]
	constexpr short getByteDepth(ALformat format) noexcept {
		if (is8Bit(format)) return 1;

		return 2;
	}

	[[nodiscard]]
	constexpr short getChannels(ALformat format) noexcept {
		if (isMono(format)) return 1;

		return 2;
	}

	[[nodiscard]]
	constexpr unsigned char getBytesPerSample(ALformat format) noexcept {
		return (unsigned char)(getByteDepth(format) * getChannels(format));
	}

	[[nodiscard]]
	constexpr long double getMaxValue(ALformat format) {
		if (is8Bit(format)) return (long double)std::numeric_limits<uint8_t>::max();

		return (long double)std::numeric_limits<int16_t>::max();
	}

	[[nodiscard]]
	constexpr long double getMinValue(ALformat format) {
		if (is8Bit(format)) return (long double)std::numeric_limits<uint8_t>::lowest();

		return (long double)std::numeric_limits<int16_t>::lowest();
	}

	[[nodiscard]]
	constexpr long double getCenterValue(ALformat format) {
		if (is8Bit(format)) return 128.0l;

		return 0.0l;
	}

	[[nodiscard]]
	constexpr ALformat toMono(ALformat format) {
		return determineALformat(getBitDepth(format), 1);
	}

	[[nodiscard]]
	constexpr ALformat toStereo(ALformat format) {
		return determineALformat(getBitDepth(format), 2);
	}

	[[nodiscard]]
	constexpr ALformat to8Bit(ALformat format) {
		return determineALformat(8, getChannels(format));
	}

	[[nodiscard]]
	constexpr ALformat to16Bit(ALformat format) {
		return determineALformat(16, getChannels(format));
	}
}

#ifdef UNDEF_WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#undef UNDEF_WIN32_LEAN_AND_MEAN
#endif

#ifdef UNDEF_NOMINMAX
#undef NOMINMAX
#undef UNDEF_NOMINMAX
#endif