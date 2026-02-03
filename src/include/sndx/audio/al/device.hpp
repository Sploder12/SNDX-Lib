#pragma once

#include "./al.hpp"

#include <string>
#include <vector>
#include <algorithm>
#include <utility>

#include "../../mixin/handle.hpp"

namespace sndx::audio {
	namespace detail {
		[[nodiscard]]
		inline std::vector<std::string> getAlStringList(ALCdevice* device, ALCenum enm) {
			auto strings = alcGetString(device, enm);
			if (strings == nullptr || *strings == '\0') return {};

			std::vector<std::string> out{};

			do {
				out.emplace_back(strings);
				strings += out.back().size() + 1;
			} while (*strings != '\0');

			return out;
		}
	}

	[[nodiscard]]
	inline bool isALEnumExtPresent() {
		static bool present =
			alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT") == AL_TRUE;

		return present;
	}

	[[nodiscard]]
	inline bool isALCaptureExtPresent() {
		static bool present =
			alcIsExtensionPresent(nullptr, "ALC_EXT_CAPTURE") == AL_TRUE;

		return present;
	}

	[[nodiscard]]
	inline std::vector<std::string> getAlDevices() {
		if (!isALEnumExtPresent()) {
			return {};
		}

		return detail::getAlStringList(nullptr, ALC_DEVICE_SPECIFIER);
	}

	[[nodiscard]]
	inline std::vector<std::string> getAlCaptureDevices() {
		if (!isALEnumExtPresent() && !isALCaptureExtPresent()) {
			return {};
		}

		return detail::getAlStringList(nullptr, ALC_CAPTURE_DEVICE_SPECIFIER);
	}


	[[nodiscard]]
	inline std::string getDefaultAlDevice() {
		if (!isALEnumExtPresent()) {
			return "";
		}

		return alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
	}
	
	[[nodiscard]]
	inline std::string getDefaultAlCaptureDevice() {
		if (!isALCaptureExtPresent() && !isALCaptureExtPresent()) {
			return "";
		}
		return alcGetString(nullptr, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
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

	using ALdeviceHandle = mixin::Handle<ALdevice>;
}