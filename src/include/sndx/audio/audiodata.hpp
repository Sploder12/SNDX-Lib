#pragma once

#include <chrono>
#include <numeric>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include "../math/math.hpp"

namespace sndx::audio {
	template <class SampleT = uint16_t>
	class AudioData {
	private:
		// interleaved 
		std::vector<SampleT> m_buffer{};
		size_t m_channels = 1;
		size_t m_frequency = 1;

	public:
		using sample_type = SampleT;

		[[nodiscard]]
		constexpr size_t frequency() const noexcept {
			return m_frequency;
		}

		[[nodiscard]]
		constexpr size_t channels() const noexcept {
			return m_channels;
		}

		[[nodiscard]]
		constexpr size_t totalSamples() const noexcept {
			return m_buffer.size();
		}

		[[nodiscard]]
		constexpr size_t sampleFrames() const noexcept {
			return totalSamples() / channels();
		}

		[[nodiscard]]
		constexpr auto data() const noexcept {
			return m_buffer.data();
		}

		[[nodiscard]]
		constexpr auto byteSize() const noexcept {
			return m_buffer.size() * sizeof(SampleT);
		}

		[[nodiscard]]
		constexpr std::chrono::duration<double> lengthSeconds() const {
			return std::chrono::duration<double>(double(sampleFrames()) / double(frequency()));
		}

		[[nodiscard]]
		size_t samplePos(size_t sampleFrame, size_t channel) const {
			if (sampleFrame >= sampleFrames())
				throw std::out_of_range("Sample beyond sample count");

			if (channel >= channels())
				throw std::out_of_range("Channel beyond channel count");

			return sampleFrame * channels() + channel;
		}

		[[nodiscard]]
		SampleT getSample(size_t sampleFrame, size_t channel) const {
			return m_buffer[samplePos(sampleFrame, channel)];
		}

		void setSample(size_t sampleFrame, size_t channel, SampleT value) {
			m_buffer[samplePos(sampleFrame, channel)] = value;
		}

		explicit constexpr AudioData() noexcept = default;

		explicit constexpr AudioData(size_t channels, size_t frequency) noexcept :
			m_channels(channels), m_frequency(frequency) {}

		explicit constexpr AudioData(size_t channels, size_t frequency, std::vector<SampleT>&& buf) noexcept :
			m_buffer{ std::move(buf) }, m_channels(channels), m_frequency(frequency) {}
	};

	template <class SampleT> [[nodiscard]]
	constexpr SampleT sampleMinValue() noexcept {
		return std::is_floating_point_v<SampleT> ? SampleT(-1) : std::numeric_limits<SampleT>::min();
	}

	template <class SampleT> [[nodiscard]]
	constexpr SampleT sampleMaxValue() noexcept {
		return std::is_floating_point_v<SampleT> ? SampleT(1) : std::numeric_limits<SampleT>::max();
	}

	template <class SampleT> [[nodiscard]]
	constexpr SampleT sampleCenterValue() noexcept {
		return std::is_unsigned_v<SampleT> ? std::midpoint(sampleMinValue<SampleT>(), sampleMaxValue<SampleT>()) : SampleT(0);
	}

	template <class NewT, class OldT>
	inline AudioData<NewT> convert(const AudioData<OldT>& old) {
		std::vector<NewT> data{};
		data.reserve(old.totalSamples());

		constexpr auto newMin = sampleMinValue<NewT>();
		constexpr auto newMax = sampleMaxValue<NewT>();
		constexpr auto newCenter = sampleCenterValue<NewT>();

		constexpr auto oldMin = sampleMinValue<OldT>();
		constexpr auto oldMax = sampleMaxValue<OldT>();
		constexpr auto oldCenter = sampleCenterValue<OldT>();

		for (size_t sampleFrame = 0; sampleFrame < old.sampleFrames(); ++sampleFrame) {
			for (size_t channel = 0; channel < old.channels(); ++channel) {
				const auto& sample = old.getSample(sampleFrame, channel);

				using T = decltype(oldCenter + newCenter);
				if constexpr (std::is_floating_point_v<T>) {
					T r = math::remapBalanced<T>((T)sample, (T)oldCenter, (T)newCenter, (T)oldMin, (T)oldMax, (T)newMin, (T)newMax);
					data.emplace_back((NewT)r);
				}
				else {
					data.emplace_back(math::remapBalanced<NewT>(sample, oldCenter, newCenter));
				}
			}
		}

		return AudioData<NewT>{ old.channels(), old.frequency(), std::move(data) };
	}

	template <class T, class SameT>
		requires std::is_same_v<T, SameT>
	constexpr const AudioData<T>& convert(const AudioData<SameT>& self) {
		return self;
	}

	template <class T, class SameT>
		requires std::is_same_v<T, SameT>
	constexpr AudioData<T>&& convert(AudioData<SameT>&& self) noexcept {
		return std::move(self);
	}

	template <class SampleT>
	inline AudioData<SampleT> asMono(const AudioData<SampleT>& data) {
		if (data.channels() <= 1) [[unlikely]] return data;

		std::vector<SampleT> out{};
		out.reserve(data.sampleFrames());

		for (size_t sampleFrame = 0; sampleFrame < data.sampleFrames(); ++sampleFrame) {
			auto span = std::span<const SampleT>{data.data() + sampleFrame * data.channels(), data.channels()};
			out.emplace_back(math::average<SampleT>(span.begin(), span.end()));
		}


		return AudioData<SampleT>{ 1, data.frequency(), std::move(out) };
	}
}