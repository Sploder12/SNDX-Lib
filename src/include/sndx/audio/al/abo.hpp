#pragma once

#include "./al.hpp"

#include "../audiodata.hpp"

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

		template <class SampleT>
		ABO& setData(const AudioData<SampleT>& data) {
			if (auto size = ALsizei(data.byteSize()); size <= 0) {
				return *this;
			}
			gen();

			
			if constexpr (std::is_same_v<SampleT, uint8_t> || std::is_same_v<SampleT, int16_t>) {
				ALformat format = determineALformat(sizeof(SampleT) * 8, (short)data.channels());
				alBufferData(m_id, (ALenum)format, data.data(), (ALsizei)data.totalSamples(), ALsizei(data.frequency()));
			}
			else {
				if (data.channels() > 2)
					throw std::runtime_error("OpenAL does not support > 2 channels");

				ALenum format = data.channels() == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
				auto converted = convert<int16_t>(data);
				alBufferData(m_id, format, converted.data(), (ALsizei)converted.totalSamples(), ALsizei(converted.frequency()));
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

	using ABOhandle = mixin::Handle<ABO>;
}