#pragma once

#include <cstddef>
#include <vector>
#include <numeric>
#include <stdexcept>
#include <chrono>

#include "../../math/math.hpp"

#include "./al.hpp"

namespace sndx::audio {
	struct ALaudioMeta {
		size_t m_frequency = 1;
		ALformat m_format = ALformat::mono8;
	};

	class ALaudioData {
	private:
		ALaudioMeta m_meta{};
		std::vector<std::byte> m_buffer{};

	public:
		explicit ALaudioData() noexcept = default;

		explicit ALaudioData(const ALaudioMeta& meta) noexcept :
			m_meta(meta) {}

		explicit ALaudioData(const ALaudioMeta& meta, std::vector<std::byte>&& buf) noexcept :
			m_meta(meta), m_buffer{ std::move(buf) } {}

		template <std::floating_point F>
		explicit ALaudioData(const ALaudioMeta& meta, const std::vector<F>& buf) :
			m_meta(meta) {

			m_buffer.resize(buf.size() * getChannels() * getByteDepth(meta.m_format));

			auto newMin = getMinValue(m_meta.m_format);
			auto newMax = getMaxValue(m_meta.m_format);
			auto newCenter = getCenterValue(m_meta.m_format);

			for (size_t i = 0; i < buf.size(); ++i) {
				long double val = buf[i];

				auto newVal = math::remapBalanced(val,
					0.0l, newCenter,
					-1.0l, 1.0l,
					newMin, newMax);

				for (size_t c = 0; c < getChannels(); ++c) {
					setSample(i, c, newVal);
				}
			}
		}

		[[nodiscard]]
		const ALaudioMeta& getMeta() const noexcept {
			return m_meta;
		}

		[[nodiscard]]
		size_t getFrequency() const noexcept {
			return m_meta.m_frequency;
		}

		[[nodiscard]]
		ALformat getFormat() const noexcept {
			return m_meta.m_format;
		}

		[[nodiscard]]
		auto getChannels() const noexcept {
			return audio::getChannels(getFormat());
		}

		[[nodiscard]]
		size_t getSampleCount() const noexcept {
			return m_buffer.size() / getBytesPerSample(m_meta.m_format);
		}

		[[nodiscard]]
		auto data() const noexcept {
			return m_buffer.data();
		}

		[[nodiscard]]
		auto getByteSize() const noexcept {
			return m_buffer.size();
		}

		[[nodiscard]]
		std::chrono::duration<double> getLengthSeconds() const {
			return std::chrono::duration<double>(double(getSampleCount()) / double(getFrequency()));
		}

		[[nodiscard]]
		size_t getBytePos(size_t sample, size_t channel) const {
			if (sample >= getSampleCount())
				throw std::out_of_range("Sample beyond sample count");

			if (channel >= getChannels())
				throw std::out_of_range("Channel beyond channel count");

			return sample * getBytesPerSample(getFormat()) + channel * getByteDepth(getFormat());
		}

		[[nodiscard]]
		long double getSample(size_t sample, size_t channel) const {
			// special mono support
			if (getChannels() == 1)
				channel = 0;

			auto bytePos = getBytePos(sample, channel);

			if (is8Bit(m_meta.m_format)) {
				auto ptr = reinterpret_cast<const uint8_t*>(m_buffer.data() + bytePos);
				return (long double)(*ptr);
			}
			else {
				auto ptr = reinterpret_cast<const int16_t*>(m_buffer.data() + bytePos);
				return (long double)(*ptr);
			}
		}

		// note: value is the value as if it were the correct type.
		// will throw a domain error is the value is not in the bounds of the correct type
		void setSample(size_t sample, size_t channel, long double value) {
			auto bytePos = getBytePos(sample, channel);

			if (is8Bit(getFormat())) {
				if (value < std::numeric_limits<uint8_t>::lowest() || value > std::numeric_limits<uint8_t>::max())
					throw std::domain_error("Sample value exceeds uint8_t");

				auto ptr = (uint8_t*)(m_buffer.data() + bytePos);
				*ptr = uint8_t(value);
			}
			else {
				if (value < std::numeric_limits<int16_t>::lowest() || value > std::numeric_limits<int16_t>::max())
					throw std::domain_error("Sample value exceeds int16_t");

				auto ptr = (int16_t*)(m_buffer.data() + bytePos);
				*ptr = int16_t(value);
			}
		}

		[[nodiscard]]
		ALaudioData asFormat(ALformat format) const {
			if (format == m_meta.m_format)
				return *this;

			ALaudioData out{};
			out.m_meta.m_frequency = m_meta.m_frequency;
			out.m_meta.m_format = format;

			out.m_buffer.resize(getSampleCount() * out.getChannels() * getByteDepth(format));

			auto oldMin = getMinValue(m_meta.m_format);
			auto oldMax = getMaxValue(m_meta.m_format);
			auto oldCenter = getCenterValue(m_meta.m_format);

			auto newMin = getMinValue(format);
			auto newMax = getMaxValue(format);
			auto newCenter = getCenterValue(format);

			for (size_t i = 0; i < getSampleCount(); ++i) {
				std::array<long double, 2> val{};

				if (isStereo(m_meta.m_format) && !isStereo(format)) {
					val[0] = std::midpoint(getSample(i, 0), getSample(i, 1));
					val[1] = val[0];
				}
				else {
					val[0] = getSample(i, 0);
					val[1] = getSample(i, 1);
				}

				for (size_t c = 0; c < out.getChannels(); ++c) {
					auto newValue = math::remapBalanced(val[c],
						oldCenter, newCenter,
						oldMin, oldMax,
						newMin, newMax);

					out.setSample(i, c, newValue);
				}
			}

			return out;
		}
	};
}