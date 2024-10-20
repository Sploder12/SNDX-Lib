#pragma once

#include "./al.hpp"

#include "./audio_data.hpp"
#include "./abo.hpp"
#include "./source.hpp"
#include "./device.hpp"

#include <stdexcept>
#include <unordered_map>
#include <array>

// https://openal.org/documentation/OpenAL_Programmers_Guide.pdf
// https://openal.org/documentation/openal-1.1-specification.pdf

namespace sndx::audio {

	template <typename IdT = std::string>
	class ALcontext {
	private:
		ALdevice m_device;
		ALCcontext* m_context{nullptr};

		// buffers and sources are context specific
		// also despite being based on OpenGL there is no "bindBuffer" etc.
		std::unordered_map<IdT, ABO> m_buffers{};
		std::unordered_map<IdT, ALsource> m_sources{};

	public:
		explicit ALcontext(const ALCchar* deviceName = nullptr, const ALCint* attrList = nullptr) :
			m_device(deviceName) {

			if (!m_device.valid()) return;

			m_context = alcCreateContext(m_device, attrList);
		}

		explicit ALcontext(ALdevice&& device, const ALCint* attrList = nullptr) :
			m_device(std::move(device)) {

			if (!m_device.valid()) return;

			m_context = alcCreateContext(m_device, attrList);
		}

		ALcontext(ALcontext&& other) noexcept :
			m_device(std::exchange(other.m_device, ALdevice{})), m_context(std::exchange(other.m_context, nullptr)),
			m_buffers(std::exchange(other.m_buffers, {})), m_sources(std::exchange(other.m_sources, {})) {}

		ALcontext(const ALcontext&) = delete;

		ALcontext& operator=(ALcontext&& other) noexcept {
			std::swap(m_device, other.m_device);
			std::swap(m_context, other.m_context);
			std::swap(m_buffers, other.m_buffers);
			std::swap(m_sources, other.m_sources);
			return *this;
		}

		~ALcontext() {
			m_sources.clear();
			m_buffers.clear();

			if (alcGetCurrentContext() == m_context) {
				alcMakeContextCurrent(nullptr);
			}

			if (m_context != nullptr) alcDestroyContext(m_context);
			m_context = nullptr;
		}

		[[nodiscard]]
		bool valid() const {
			return m_device.valid() && m_context != nullptr;
		}

		const auto& bind() const {
			alcMakeContextCurrent(m_context);
			return *this;
		}

		[[nodiscard]]
		ALCcontext* getContext() const {
			return m_context;
		}

		[[nodiscard]]
		ALdeviceHandle getDevice() {
			return m_device;
		}

		[[nodiscard]]
		const std::string& getDeviceName() const {
			return m_device.getName();
		}

		[[nodiscard]]
		ABOhandle createBuffer(const IdT& id) {
			auto [it, _] = m_buffers.emplace(id, ABO{});

			return it->second;
		}

		[[nodiscard]]
		ABOhandle createBuffer(const IdT& id, const ALaudioData& data) {
			auto handle = createBuffer(id);
			handle->setData(data);

			return handle;
		}

		[[nodiscard]]
		ABOhandle getBuffer(const IdT& id) {
			return m_buffers.at(id);
		}

		// make sure you unbind the buffer from all sources!
		bool deleteBuffer(const IdT& id) {
			return bool(m_buffers.erase(id));
		}

		[[nodiscard]]
		ALsourceHandle createSource(const IdT& id) {
			auto [it, _] = m_sources.emplace(id, ALsource{});

			return it->second;
		}

		[[nodiscard]]
		ALsourceHandle getSource(const IdT& id) {
			return m_sources.at(id);
		}

		bool deleteSource(const IdT& id) {
			return bool(m_sources.erase(id));
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