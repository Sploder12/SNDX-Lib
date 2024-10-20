#pragma once

#include "./al.hpp"

#include <string>
#include <vector>
#include <algorithm>
#include <utility>

#include "../../mixin/handle.hpp"

namespace sndx::audio {

	[[nodiscard]]
	inline bool isALEnumExtPresent() {
		static bool present =
			alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT") == AL_TRUE &&
			alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT") == AL_TRUE;

		return present;
	}

	[[nodiscard]]
	inline std::vector<std::string> getAlDevices() {
		if (!isALEnumExtPresent()) {
			return {};
		}

		auto devices = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
		if (devices == nullptr || *devices == '\0') return {};

		std::vector<std::string> out{};

		do {
			out.emplace_back(devices);
			devices += out.back().size() + 1;
		} while (*devices != '\0');

		return out;
	}

	[[nodiscard]]
	inline std::string getDefaultAlDevice() {
		return alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
	}

	class ALdevice {
	protected:
		ALCdevice* device = nullptr;

		mutable std::string deviceName = "";

	public:
		explicit ALdevice(const ALCchar* deviceName = nullptr) :
			device(alcOpenDevice(deviceName)) {}

		operator ALCdevice* () {
			return device;
		}

		operator ALCdevice const* () const {
			return device;
		}

		ALdevice(ALdevice&& other) noexcept :
			device(std::exchange(other.device, nullptr)),
			deviceName(std::exchange(other.deviceName, "")) {}

		ALdevice(const ALdevice&) = delete;

		ALdevice& operator=(ALdevice&& other) noexcept {
			std::swap(device, other.device);
			std::swap(deviceName, other.deviceName);
			return *this;
		}

		ALdevice& operator=(const ALdevice&) = delete;

		~ALdevice() {
			if (device) {
				alcCloseDevice(device);
				device = nullptr;
			}
		}

	public:
		[[nodiscard]]
		bool valid() const {
			return device != nullptr;
		}

		[[nodiscard]]
		const std::string& getName() const {
			if (device != nullptr && isALEnumExtPresent()) [[likely]] {
				if (deviceName == "") {
					deviceName = std::string(alcGetString(device, ALC_DEVICE_SPECIFIER));
				}
			}

			return deviceName;
		}
	};

	using ALdeviceHandle = sndx::mixin::Handle<ALdevice>;
}