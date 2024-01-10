#pragma once

#define NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN

#define WIN32_LEAN_AND_MEAN
#include <AL/al.h>
#include <AL/alc.h>
#undef WIN32_LEAN_AND_MEAN
#else 
#include <AL/al.h>
#include <AL/alc.h>
#endif

#include "audiodata.hpp"
#include "abo.hpp"
#include "alsource.hpp"

#include <stdexcept>
#include <unordered_map>
#include <array>



// note because of lgpl OpenAL itself must be dynamically linked
// I have modified my vcpkg triplet for this, you must too

// https://openal.org/documentation/OpenAL_Programmers_Guide.pdf
// https://openal.org/documentation/openal-1.1-specification.pdf

namespace sndx {

	inline std::vector<std::string> getAlDevices() {
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

		mutable std::string deviceName;

		explicit ALContext(const ALCchar* deviceName = nullptr, const ALCint* attrList = nullptr) :
			device(alcOpenDevice(deviceName)), buffers({}), sources({}) {

			if (device == nullptr) throw std::runtime_error("OpenAL failed to open device.");

			context = alcCreateContext(device, attrList);

			if (context == nullptr) throw std::runtime_error("OpenAL failed to create context.");
		}

		ALContext(ALContext&& other) noexcept:
			device(std::exchange(other.device, nullptr)), context(std::exchange(other.context, nullptr)),
			buffers(std::exchange(other.buffers, {})), sources(std::exchange(other.sources, {})) {}

		ALContext(const ALContext&) = delete;

		ALContext& operator=(ALContext&& other) noexcept {
			std::swap(device, other.device);
			std::swap(context, other.context);
			std::swap(buffers, other.buffers);
			std::swap(sources, other.sources);
			return *this;
		}

		~ALContext() {
			for (auto& src : sources) {
				src.second.destroy();
			}

			sources.clear();

			for (auto& buf : buffers) {
				buf.second.destroy();
			}

			buffers.clear();

			if (alcGetCurrentContext() == context) {
				alcMakeContextCurrent(nullptr);
			}

			if (context != nullptr) alcDestroyContext(context);

			if (device != nullptr) alcCloseDevice(device);
		}

		const auto& bind() const {
			alcMakeContextCurrent(context);
			return *this;
		}

		[[nodiscard]]
		const std::string& currentDevice() const {
			if (deviceName == "" && alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT") == AL_TRUE) {
				deviceName = std::string(alcGetString(device, ALC_DEVICE_SPECIFIER));
			}

			return deviceName;
		}

		template <typename T> [[nodiscard]]
		ABO createBuffer(const IdT& id, const AudioData<T>& data) {
			ABO out{};
			out.setData(ALenum(data.format), std::span(data.buffer), data.freq);
			buffers.emplace(id, out);
			return out;
		}

		// make sure you unbind the buffer from all sources!
		bool deleteBuffer(const IdT& id) {
			if (auto it = buffers.find(id); it != buffers.end()) {
				it->second.destroy();

				buffers.erase(it);
				return true;
			}

			return false;
		}

		bool deleteSource(const IdT& id) {
			if (auto it = sources.find(id); it != sources.end()) {
				it->second.destroy();

				sources.erase(it);
				return true;
			}

			return false;
		}

		[[nodiscard]]
		ALSource createSource(const IdT& id) {
			ALSource out{};
			out.gen();
			sources.emplace(id, out);
			return out;
		}

		const auto& setVolume(float gain) const {
			alListenerf(AL_GAIN, gain);
			return *this;
		}

		const auto& setListenerPos(glm::vec3 pos) const {
			alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
			return *this;
		}

		const auto& setListenerVel(glm::vec3 vel) const {
			alListener3f(AL_POSITION, vel.x, vel.y, vel.z);
			return *this;
		}

		const auto& setListenerOrientation(glm::vec3 at, glm::vec3 up) const {
			std::array<glm::vec3, 2> dat{ at, up };
			alListenerfv(AL_ORIENTATION, (ALfloat*)dat.data());
			return *this;
		}

		[[nodiscard]]
		float getVolume() const {
			float out;
			alGetListenerf(AL_GAIN, &out);
			return out;
		}
	};
}