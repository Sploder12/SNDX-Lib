#pragma once

#define NOMINMAX
#include <AL/al.h>
#include <AL/alc.h>

#include <span>
#include <chrono>

namespace sndx {

	struct ABO {
		ALuint id;

		void gen() {
			alGenBuffers(1, &id);
		}

		template <typename T>
		void setData(ALenum format, std::span<T> data, ALsizei freq) {
			static_assert(std::is_integral_v<T>);
			static_assert(decltype(data)::extent != 0);

			ALsizei size = ALsizei(data.size() * sizeof(T));
			if (size > 0) [[likely]] {
				if (id == 0) gen();

				alBufferData(id, format, data.data(), size, freq);
			}
		}

		// get rid of the sources first please.
		void destroy() {
			if (id != 0) alDeleteBuffers(1, &id);

			id = 0;
		}

		[[nodiscard]]
		size_t lengthSamples() const {
			ALint byteSize, channels, bits;

			alGetBufferi(id, AL_SIZE, &byteSize);
			alGetBufferi(id, AL_CHANNELS, &channels);
			alGetBufferi(id, AL_BITS, &bits);

			return byteSize * 8 / (channels * bits);
		}

		[[nodiscard]]
		std::chrono::duration<float> lengthSeconds() const {
			ALint frequency;

			alGetBufferi(id, AL_FREQUENCY, &frequency);

			return std::chrono::duration<float>(float(lengthSamples()) / float(frequency));
		}
	};
}