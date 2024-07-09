#pragma once

#include "al.h"

#include <string>
#include <vector>
#include <algorithm>

namespace sndx {
	
	[[nodiscard]]
	inline bool isALEnumExtPresent() {
		static bool present = 
			alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT") == AL_TRUE &&
			alcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT") == AL_TRUE;

		return present;
	}

	[[nodiscard]]
	inline std::vector<std::string> getAlDevices() {
		if (!isALEnumExtPresent()) {
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

	[[nodiscard]]
	inline std::string getDefaultAlDevice() {
		return alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
	}

	class ALDevice {
	protected:
		ALCdevice* device = nullptr;

		mutable std::string deviceName = "";

	public:
		explicit ALDevice(const ALCchar* deviceName = nullptr) :
			device(alcOpenDevice(deviceName)) {}

		operator ALCdevice* () {
			return device;
		}

		operator const ALCdevice* () const {
			return device;
		}

		ALDevice(ALDevice&& other) noexcept :
			device(std::exchange(other.device, nullptr)),
			deviceName(std::exchange(other.deviceName, "")) {}

		ALDevice(const ALDevice&) = delete;

		ALDevice& operator=(ALDevice&& other) noexcept {
			std::swap(device, other.device);
			std::swap(deviceName, other.deviceName);
			return *this;
		}

		ALDevice& operator=(const ALDevice&) = delete;

		~ALDevice() {
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
}