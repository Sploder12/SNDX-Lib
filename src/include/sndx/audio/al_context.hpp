#pragma once

#include "./al.hpp"

#include "./al_audio_data.hpp"
#include "./abo.hpp"
#include "./al_source.hpp"
#include "./al_device.hpp"

#include <stdexcept>
#include <unordered_map>
#include <array>

// https://openal.org/documentation/OpenAL_Programmers_Guide.pdf
// https://openal.org/documentation/openal-1.1-specification.pdf

namespace sndx::audio {

	template <typename IdT = std::string>
	struct ALcontext {
		ALdevice device;
		ALCcontext* context;

		// buffers and sources are context specific, which sucks
		// also despite being based on OpenGL there is no "bindBuffer" etc.
		std::unordered_map<IdT, ABO> buffers;
		std::unordered_map<IdT, ALsource> sources;

		explicit ALcontext(const ALCchar* deviceName = nullptr, const ALCint* attrList = nullptr) :
			device(deviceName), context(nullptr), buffers{}, sources{} {

			if (!device.valid()) return;

			context = alcCreateContext(device, attrList);
		}

		ALcontext(ALcontext&& other) noexcept :
			device(std::exchange(other.device, {})), context(std::exchange(other.context, nullptr)),
			buffers(std::exchange(other.buffers, {})), sources(std::exchange(other.sources, {})) {}

		ALcontext(const ALcontext&) = delete;

		ALcontext& operator=(ALcontext&& other) noexcept {
			std::swap(device, other.device);
			std::swap(context, other.context);
			std::swap(buffers, other.buffers);
			std::swap(sources, other.sources);
			return *this;
		}

		~ALcontext() {
			sources.clear();
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

		[[nodiscard]]
		ABOhandle createBuffer(const IdT& id) {
			auto& [it, _] = buffers.emplace(id, {});

			return it->second;
		}

		[[nodiscard]]
		ABOhandle createBuffer(const IdT& id, const ALaudioData& data) {
			auto handle = createBuffer(id);
			handle.setData(data);

			return handle;
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
		ALsourceHandle createSource(const IdT& id) {
			auto& [it, _] = sources.emplace(id, {});

			return it->second;
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
			float out = 0.0f;
			alGetListenerf(AL_GAIN, &out);

			return out;
		}
	};
}