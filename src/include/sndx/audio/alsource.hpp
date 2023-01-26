#pragma once

#include <AL/al.h>
#include <AL/alc.h>

#include <glm/glm.hpp>

#include <span>
#include <chrono>

#include "abo.hpp"

namespace sndx {

	struct ALSource {
		ALuint id;

		void stop() const {
			alSourceStop(id);
		}

		void pause() const {
			alSourcePause(id);
		}

		void play() const {
			alSourcePlay(id);
		}

		void rewind() const {
			alSourceRewind(id);
		}

		template <typename T>
		void setParam(ALenum, T) const;

		template <>
		void setParam(ALenum param, ALfloat val) const {
			alSourcef(id, param, val);
		}

		template <>
		void setParam(ALenum param, glm::vec3 val) const {
			alSource3f(id, param, val.x, val.y, val.z);
		}

		template <>
		void setParam(ALenum param, ALint val) const {
			alSourcei(id, param, val);
		}

		template <typename T> [[nodiscard]]
		T getParam(ALenum) const;

		template <> [[nodiscard]]
		float getParam(ALenum param) const {
			float out;
			alGetSourcef(id, param, &out);
			return out;
		}

		template <> [[nodiscard]]
		glm::vec3 getParam(ALenum param) const {
			glm::vec3 out;
			alGetSource3f(id, param, &out.x, &out.y, &out.z);
			return out;
		}

		template <> [[nodiscard]]
		int getParam(ALenum param) const {
			int out;
			alGetSourcei(id, param, &out);
			return out;
		}

		void setPos(glm::vec3 pos) const {
			setParam(AL_POSITION, pos);
		}

		void setVel(glm::vec3 velocity) const {
			setParam(AL_VELOCITY, velocity);
		}

		void setOrientation(glm::vec3 at, glm::vec3 up) const {
			std::array<glm::vec3, 2> dat{ at, up };
			alSourcefv(id, AL_ORIENTATION, (ALfloat*)dat.data());
		}

		void setGain(float gain) const {
			setParam(AL_GAIN, gain);
		}

		void setSpeed(float speed) const {
			setParam(AL_PITCH, speed);
		}

		void setBuffer(const ABO& buffer) const {
			setParam(AL_BUFFER, int(buffer.id));
		}

		void queueBuffers(std::span<ABO> buffers) const {
			alSourceQueueBuffers(id, (ALsizei)buffers.size(), (ALuint*)buffers.data());
		}

		void dequeueBuffers(std::span<ABO> buffers) const {
			alSourceUnqueueBuffers(id, (ALsizei)buffers.size(), (ALuint*)buffers.data());
		}

		// does account for speed up/slowdown
		void seekSec(std::chrono::duration<float> seconds) const {
			if (seconds.count() >= 0) {
				float pitch = getParam<float>(AL_PITCH);
				setParam(AL_SEC_OFFSET, seconds.count() / pitch);
			}
		}

		// doesn't account for speed up/slowdown
		void seekSecRaw(std::chrono::duration<float> seconds) const {
			if (seconds.count() >= 0) {
				setParam(AL_SEC_OFFSET, seconds.count());
			}
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

		void gen() {
			if (id != 0) destroy();
		
			alGenSources(1, &id);
		}

		void destroy() {
			if (id != 0) alDeleteSources(1, &id);

			id = 0;
		}
	};
}