#pragma once

#include "al.h"

#include "audiodata.hpp"
#include "abo.hpp"
#include "alsource.hpp"
#include "aldevice.hpp"

#include <stdexcept>
#include <unordered_map>
#include <array>

// https://openal.org/documentation/OpenAL_Programmers_Guide.pdf
// https://openal.org/documentation/openal-1.1-specification.pdf

namespace sndx {

	template <typename IdT = std::string>
	struct ALContext {
		ALDevice device;
		ALCcontext* context;

		// buffers and sources are context specific, which sucks
		// also despite being based on OpenGL there is no "bindBuffer" etc.
		std::unordered_map<IdT, ABO> buffers;
		std::unordered_map<IdT, ALSource> sources;

		explicit ALContext(const ALCchar* deviceName = nullptr, const ALCint* attrList = nullptr) :
			device(deviceName), context(nullptr), buffers{}, sources{} {

			if (!device.valid()) return;
	
			context = alcCreateContext(device, attrList);
		}

		ALContext(ALContext&& other) noexcept:
			device(std::exchange(other.device, {})), context(std::exchange(other.context, nullptr)),
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
		}

		[[nodiscard]]
		bool valid() const {
			return device.valid() && context != nullptr;
		}

		const auto& bind() const {
			alcMakeContextCurrent(context);
			return *this;
		}

		[[nodiscard]]
		const std::string& currentDevice() const {
			return device.getName();
		}

		template <typename T> [[nodiscard]]
		ABO createBuffer(const IdT& id, const AudioData<T>& data) {
			ABO out{};

			if (context) [[likely]] {
				out.setData(data);
				buffers.emplace(id, out);
			}

			return out;
		}

		ABO createBuffer(const IdT& id) {
			ABO out{};

			if (context) [[likely]] {
				out.gen();
				buffers.emplace(id, out);
			}

			return out;
		}

		// make sure you unbind the buffer from all sources!
		bool deleteBuffer(const IdT& id) {
			if (!context) [[unlikely]] return;

			if (auto it = buffers.find(id); it != buffers.end()) {
				it->second.destroy();

				buffers.erase(it);
				return true;
			}

			return false;
		}

		bool deleteSource(const IdT& id) {
			if (!context) [[unlikely]] return;

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

			if (context) [[likely]] {
				out.gen();
				sources.emplace(id, out);
			}

			return out;
		}

		const auto& setVolume(float gain) const {
			if (context) [[likely]] {
				alListenerf(AL_GAIN, gain);
			}
			return *this;
		}

		const auto& setListenerPos(glm::vec3 pos) const {
			if (context) [[likely]] {
				alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
			}
			return *this;
		}

		const auto& setListenerVel(glm::vec3 vel) const {
			if (context) [[likely]] {
				alListener3f(AL_POSITION, vel.x, vel.y, vel.z);
			}
			return *this;
		}

		const auto& setListenerOrientation(glm::vec3 at, glm::vec3 up) const {
			if (context) [[likely]] {
				std::array<glm::vec3, 2> dat{ at, up };
				alListenerfv(AL_ORIENTATION, (ALfloat*)dat.data());
			}
			return *this;
		}

		[[nodiscard]]
		float getVolume() const {
			float out = 0.0f;
			if (context) [[likely]] {
				alGetListenerf(AL_GAIN, &out);
			}
			return out;
		}
	};
}