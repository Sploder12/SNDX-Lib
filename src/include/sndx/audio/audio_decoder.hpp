#pragma once

#include <cstddef>
#include <chrono>
#include <vector>

namespace sndx::audio {

	enum class DataFormat : uint8_t {
		error = 0,
		pcm_int,
		iee_float,

		a_law,
		mu_law
	};

	class AudioDecoder {
	public:
		virtual ~AudioDecoder() = default;

		[[nodiscard]]
		virtual size_t getBitDepth() const noexcept = 0;

		[[nodiscard]]
		virtual size_t getChannels() const noexcept = 0;

		[[nodiscard]]
		virtual size_t getSampleRate() const noexcept = 0;

		[[nodiscard]]
		virtual DataFormat getFormat() const noexcept = 0;

		[[nodiscard]]
		virtual bool done() const = 0;

		[[nodiscard]]
		virtual size_t tell() const = 0;

		virtual size_t seek(size_t pos) = 0;

		// returns previous position
		size_t seekSample(size_t sample) {
			auto bytePos = (getBitDepth() * getChannels() * sample) / 8;

			return seek(bytePos);
		}

		// returns previous position
		size_t seekSecond(std::chrono::duration<float> second) {
			return seekSample(size_t(second.count() * getSampleRate()));
		}

		[[nodiscard]]
		virtual std::vector<std::byte> readBytes(size_t count) = 0;

		[[nodiscard]]
		std::vector<std::byte> readSamples(size_t count) {
			auto bytes = (getBitDepth() * getChannels() * count) / 8;
			return readBytes(bytes);
		}

		[[nodiscard]]
		std::vector<std::byte> readSeconds(std::chrono::duration<float> seconds) {
			return readSamples(size_t(seconds.count() * getSampleRate()));
		}
	};
}