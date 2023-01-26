#pragma once

#include <AL/al.h>
#include <AL/alc.h>

#include <stdexcept>
#include <unordered_map>
#include <array>

#include "alsource.hpp"
#include "abo.hpp"
#include "audiodata.hpp"

// note because of lgpl OpenAL itself must be dynamically linked
// I have modified my vcpkg triplet for this, you must too

// https://openal.org/documentation/OpenAL_Programmers_Guide.pdf
// https://openal.org/documentation/openal-1.1-specification.pdf

namespace sndx {

	static std::vector<std::string> getAlDevices() {
		if (alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT") != AL_TRUE) {
			return {};
		}

		auto devices = alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
		if (devices == nullptr || *devices == '\0') return {};

		std::vector<std::string> out{};

		do {
			out.emplace_back(devices);
			devices += out.back().size() + 1;
		} while (*devices != '\0');

		return out;
	}

	template <typename IdT = std::string>
	struct ALContext {
		ALCdevice* device;
		ALCcontext* context;

		// buffers and sources are context specific, which sucks
		// also despite being based on OpenGL there is no "bindBuffer" etc.
		std::unordered_map<IdT, ABO> buffers;
		std::unordered_map<IdT, ALSource> sources;

		explicit ALContext(const ALCchar* deviceName = nullptr, const ALCint* attrList = nullptr) :
			device(alcOpenDevice(deviceName)), buffers({}), sources({}) {

			if (device == nullptr) throw std::runtime_error("OpenAL failed to open device.");

			context = alcCreateContext(device, attrList);

			if (context == nullptr) throw std::runtime_error("OpenAL failed to create context.");
		}

		ALContext(ALContext&& other) noexcept:
			device(std::move(other.device)), context(std::move(other.context)),
			buffers(std::move(other.buffers)), sources(std::move(other.sources)) {

			other.device = nullptr;
			other.context = nullptr;
			other.buffers = {};
			other.sources = {};
		}

		ALContext(const ALContext&) = delete;

		~ALContext() {
			sources.clear();
			buffers.clear();

			if (alcGetCurrentContext() == context) {
				alcMakeContextCurrent(nullptr);
			}

			if (context != nullptr) alcDestroyContext(context);

			if (device != nullptr) alcCloseDevice(device);
		}

		void bind() const {
			alcMakeContextCurrent(context);
		}

		[[nodiscard]]
		std::string currentDevice() {
			if (alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT") != AL_TRUE) {
				return "";
			}

			return std::string(alcGetString(device, ALC_DEVICE_SPECIFIER));
		}

		template <typename T>
		ABO createBuffer(const IdT& id, const AudioData<T>& data) {
			ABO out{};
			out.setData(ALenum(data.format), std::span(data.buffer), data.freq);
			buffers.emplace(id, out);
			return out;
		}

		ALSource createSource(const IdT& id) {
			ALSource out{};
			out.gen();
			sources.emplace(id, out);
			return out;
		}

		void setVolume(float gain) const {
			alListenerf(AL_GAIN, gain);
		}

		void setListenerPos(glm::vec3 pos) const {
			alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
		}

		void setListenerVel(glm::vec3 vel) const {
			alListener3f(AL_POSITION, vel.x, vel.y, vel.z);
		}

		void setListenerOrientation(glm::vec3 at, glm::vec3 up) const {
			std::array<glm::vec3, 2> dat{ at, up };
			alListenerfv(AL_ORIENTATION, (ALfloat*)dat.data());
		}

		[[nodiscard]]
		float getVolume() const {
			float out;
			alGetListenerf(AL_GAIN, &out);
			return out;
		}
	};
}