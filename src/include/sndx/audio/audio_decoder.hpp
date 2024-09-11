#pragma once

#include <cstddef>
#include <chrono>
#include <vector>

#include "./al_audio_data.hpp"

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
		virtual size_t getSampleAlignment() const noexcept = 0;

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
		virtual std::vector<std::byte> readRawBytes(size_t count) = 0;

		[[nodiscard]]
		std::vector<std::byte> readRawSamples(size_t count) {
			auto bytes = (getBitDepth() * getChannels() * count) / 8;
			return readRawBytes(bytes);
		}

		[[nodiscard]]
		std::vector<std::byte> readRawSeconds(std::chrono::duration<float> seconds) {
			return readRawSamples(size_t(seconds.count() * getSampleRate()));
		}

		[[nodiscard]]
		std::vector<std::byte> readAllRaw() {
			return readRawBytes(std::numeric_limits<size_t>::max());
		}

		[[nodiscard]]
		virtual ALaudioData readSamples(size_t count) = 0;

		[[nodiscard]]
		ALaudioData readSeconds(std::chrono::duration<float> seconds) {
			return readSamples(size_t(seconds.count() * getSampleRate()));
		}

		[[nodiscard]]
		ALaudioData readAll() {
			return readSamples(std::numeric_limits<size_t>::max());
		}
	};
}