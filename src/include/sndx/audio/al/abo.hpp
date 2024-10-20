#pragma once

#include "./al.hpp"

#include "./audio_data.hpp"

#include "../../mixin/handle.hpp"

namespace sndx::audio {
	class ABO {
	protected:
		ALuint m_id = 0;

		void destroy() {
			if (m_id != 0) {
				alDeleteBuffers(1, &m_id);
				m_id = 0;
			}
		}

		void gen() {
			if (m_id == 0)
				alGenBuffers(1, &m_id);
		}

	public:
		explicit ABO() {
			gen();
		}

		explicit constexpr ABO(ALuint id) noexcept:
			m_id(id) {}

		explicit constexpr ABO(ABO&& other) noexcept :
			m_id(std::exchange(other.m_id, 0)) {}

		ABO& operator=(ABO&& other) noexcept {
			std::swap(m_id, other.m_id);
			return *this;
		}

		ABO& operator=(ALuint id) noexcept {
			destroy();
			m_id = id;
			return *this;
		}


		constexpr operator ALuint() const noexcept {
			return m_id;
		}

		// make sure the sources are disconnected before this
		~ABO() {
			destroy();
		}

		ABO& setData(const ALaudioData& data) {
			if (auto size = ALsizei(data.getByteSize()); size > 0) {
				gen();
				alBufferData(m_id, ALenum(data.getFormat()), data.data(), size, ALsizei(data.getFrequency()));
			}
			return *this;
		}

		[[nodiscard]]
		size_t lengthSamples() const {
			ALint byteSize, channels, bits;

			alGetBufferi(m_id, AL_SIZE, &byteSize);
			alGetBufferi(m_id, AL_CHANNELS, &channels);
			alGetBufferi(m_id, AL_BITS, &bits);

			return byteSize / (channels * bits / 8);
		}

		[[nodiscard]]
		std::chrono::duration<float> lengthSeconds() const {
			ALint frequency;

			alGetBufferi(m_id, AL_FREQUENCY, &frequency);

			return std::chrono::duration<float>(float(lengthSamples()) / float(frequency));
		}
	};

	static_assert(sizeof(ABO) == sizeof(ALuint));

	using ABOhandle = sndx::mixin::Handle<ABO>;
}