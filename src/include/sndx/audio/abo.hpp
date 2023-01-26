#pragma once

#include <AL/al.h>
#include <AL/alc.h>

#include <span>

namespace sndx {

	struct ABO {
		ALuint id;

		void gen() {
			alGenBuffers(1, &id);
		}

		template <typename T>
		void setData(ALenum format, std::span<T> data, ALsizei freq) {
			static_assert(std::is_integral_v<T>);

			ALsizei size = data.size() * sizeof(T);
			if (size > 0) {
				if (id == 0) gen();

				alBufferData(id, format, data.data(), size, freq);
			}
		}

		// get rid of the sources first please.
		void destroy() {
			if (id != 0) alDeleteBuffers(1, &id);

			id = 0;
		}

	};
}