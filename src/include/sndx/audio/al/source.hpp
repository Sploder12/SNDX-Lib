#pragma once

#include "./al.hpp"

#include "../../mixin/handle.hpp"

#include "./abo.hpp"

#include <glm/glm.hpp>

#include <span>
#include <chrono>
#include <array>


namespace sndx::audio {

	class ALsource {
	protected:
		ALuint m_id = 0;

		const ALsource& gen() {
			if (m_id != 0) destroy();

			alGenSources(1, &m_id);
			return *this;
		}

		void destroy() {
			if (m_id != 0) {
				alDeleteSources(1, &m_id);
			}

			m_id = 0;
		}

	public:
		explicit ALsource() {
			gen();
		}

		explicit constexpr ALsource(ALuint id) noexcept :
			m_id(id) {}

		explicit constexpr ALsource(ALsource&& other) noexcept :
			m_id(std::exchange(other.m_id, 0)) {}

		ALsource& operator=(ALsource&& other) noexcept {
			std::swap(m_id, other.m_id);
			return *this;
		}

		ALsource& operator=(ALuint id) noexcept {
			destroy();
			m_id = id;
			return *this;
		}

		constexpr operator ALuint() const noexcept {
			return m_id;
		}

		// make sure the sources are disconnected before this
		~ALsource() {
			destroy();
		}
	

		const ALsource& stop() const {
			alSourceStop(m_id);
			return *this;
		}

		const ALsource& pause() const {
			alSourcePause(m_id);
			return *this;
		}

		const ALsource& play() const {
			alSourcePlay(m_id);
			return *this;
		}

		const ALsource& rewind() const {
			alSourceRewind(m_id);
			return *this;
		}

		const ALsource& setParam(ALenum param, ALfloat val) const {
			alSourcef(m_id, param, val);
			return *this;
		}

		const ALsource& setParam(ALenum param, glm::vec3 val) const {
			alSource3f(m_id, param, val.x, val.y, val.z);
			return *this;
		}

		const ALsource& setParam(ALenum param, ALint val) const {
			alSourcei(m_id, param, val);
			return *this;
		}

		template <typename T> [[nodiscard]]
		T getParam(ALenum) const = delete;

		template <> [[nodiscard]]
		float getParam(ALenum param) const {
			float out;
			alGetSourcef(m_id, param, &out);
			return out;
		}

		template <> [[nodiscard]]
		glm::vec3 getParam(ALenum param) const {
			glm::vec3 out;
			alGetSource3f(m_id, param, &out.x, &out.y, &out.z);
			return out;
		}

		template <> [[nodiscard]]
		int getParam(ALenum param) const {
			int out;
			alGetSourcei(m_id, param, &out);
			return out;
		}

		const ALsource& setPos(glm::vec3 pos) const {
			setParam(AL_POSITION, pos);
			return *this;
		}

		const ALsource& setVel(glm::vec3 velocity) const {
			setParam(AL_VELOCITY, velocity);
			return *this;
		}

		const ALsource& setOrientation(glm::vec3 at, glm::vec3 up) const {
			std::array<glm::vec3, 2> dat{ at, up };
			alSourcefv(m_id, AL_ORIENTATION, (ALfloat*)dat.data());
			return *this;
		}

		const ALsource& setGain(float gain) const {
			setParam(AL_GAIN, gain);
			return *this;
		}

		const ALsource& setSpeed(float speed) const {
			setParam(AL_PITCH, speed);
			return *this;
		}

		const ALsource& setBuffer(const ABO& buffer) const {
			setParam(AL_BUFFER, ALint(ALuint(buffer)));
			return *this;
		}

		const ALsource& detachBuffer() const {
			setParam(AL_BUFFER, 0);
			return *this;
		}

		const ALsource& queueBuffers(std::span<ABO> buffers) const {
			alSourceQueueBuffers(m_id, (ALsizei)buffers.size(), (ALuint*)buffers.data());
			return *this;
		}

		const ALsource& dequeueBuffers(std::span<ABO> buffers) const {
			alSourceUnqueueBuffers(m_id, (ALsizei)buffers.size(), (ALuint*)buffers.data());
			return *this;
		}

		// does account for speed up/slowdown
		const ALsource& seekSec(std::chrono::duration<float> seconds) const {
			if (seconds.count() >= 0) {
				float pitch = getParam<float>(AL_PITCH);
				setParam(AL_SEC_OFFSET, seconds.count() / pitch);
			}
			return *this;
		}

		// doesn't account for speed up/slowdown
		const ALsource& seekSecRaw(std::chrono::duration<float> seconds) const {
			if (seconds.count() >= 0) {
				setParam(AL_SEC_OFFSET, seconds.count());
			}
			return *this;
		}

		[[nodiscard]]
		std::chrono::duration<float> tell() const {
			float pitch = getParam<float>(AL_PITCH);
			return std::chrono::duration<float>(getParam<float>(AL_SEC_OFFSET) * pitch);
		}

		[[nodiscard]]
		std::chrono::duration<float> tellRaw() const {
			return std::chrono::duration<float>(getParam<float>(AL_SEC_OFFSET));
		}

		[[nodiscard]]
		bool playing() const {
			return getParam<ALenum>(AL_SOURCE_STATE) == AL_PLAYING;
		}
	};

	static_assert(sizeof(ALsource) == sizeof(ALuint));

	using ALsourceHandle = mixin::Handle<ALsource>;
}