#pragma once

#include <vector>
#include <cstddef>
#include <algorithm>
#include <span>
#include <stdexcept>
#include <iterator>
#include <optional>
#include <filesystem>
#include <execution>

#include <glm/glm.hpp>

#include "../../data/serialize.hpp"

namespace sndx::render {
	template <class T = std::byte>
	class ImageDat {
	private:
		using value_type = T;

		std::vector<value_type> m_data{};
		size_t m_width{}, m_height{};
		uint8_t m_channels{};

		template <glm::length_t n, glm::length_t c, std::floating_point F, class Fn> [[nodiscard]]
		ImageDat transformStub(Fn&& func) const {
			using oldVec = glm::vec<c, value_type>;
			using newVec = glm::vec<n, value_type>;

			size_t pixels = m_width * m_height;
			std::vector<value_type> data(pixels * n);

			static_assert(sizeof(oldVec) == c * sizeof(value_type));
			static_assert(sizeof(newVec) == n * sizeof(value_type));

			auto asVecs = reinterpret_cast<const oldVec*>(m_data.data());
			auto newVecs = reinterpret_cast<newVec*>(data.data());

#ifndef __APPLE__
			std::transform(std::execution::par_unseq, asVecs, asVecs + pixels, newVecs, std::forward<Fn>(func));
#else
			std::transform(asVecs, asVecs + pixels, newVecs, std::forward<Fn>(func));
#endif

			return ImageDat{ m_width, m_height, n, std::move(data) };
		}
	public:
		explicit ImageDat() = default;

		ImageDat(size_t width, size_t height, uint8_t channels, decltype(m_data)&& data) :
			m_data(std::move(data)), m_width(width), m_height(height), m_channels(channels) {}

		ImageDat(size_t width, size_t height, uint8_t channels, std::span<const T> data) :
			m_width(width), m_height(height), m_channels(channels) {

			if (channels <= 0 || channels > 4)
				throw std::invalid_argument("Channels must be between 1 and 4.");

			auto size = width * height * channels;
			if (size != data.size())
				throw std::domain_error("Data size mismatch");

			m_data.resize(size);
			std::copy_n(data.begin(), size, m_data.begin());
		}

		[[nodiscard]] auto width() const noexcept {
			return m_width;
		}

		[[nodiscard]] auto height() const noexcept {
			return m_height;
		}

		[[nodiscard]] auto channels() const noexcept {
			return m_channels;
		}

		[[nodiscard]] auto pixels() const noexcept {
			return width() * height();
		}

		[[nodiscard]] auto bytes() const noexcept {
			return pixels() * channels() * sizeof(T);
		}

		[[nodiscard]] auto data() const noexcept {
			return m_data.data();
		}

		[[nodiscard]]
		auto& at(size_t x, size_t y, size_t channel) {
			if (x >= width() || y >= height() || channel >= channels())
				throw std::domain_error("Out of bounds access detected");

			return m_data[y * width() * channels() + x * channels() + channel];
		}

		[[nodiscard]]
		const auto& at(size_t x, size_t y, size_t channel) const {
			if (x >= width() || y >= height() || channel >= channels())
				throw std::domain_error("Out of bounds access detected");

			return m_data[y * width() * channels() + x * channels() + channel];
		}

		template <glm::length_t n> [[nodiscard]]
		glm::vec<n, value_type> at(size_t x, size_t y) const {
			if (n != channels())
				throw std::invalid_argument("n must be equal to channels");

			if (x >= width() || y >= height())
				throw std::domain_error("Out of bounds access detected");

			using Vec = glm::vec<n, value_type>;
			const Vec* asVecs = reinterpret_cast<const Vec*>(m_data.data());

			return asVecs[y * width() + x];
		}

		template <glm::length_t n, glm::length_t c, std::floating_point F = float> [[nodiscard]]
		ImageDat transform(const glm::mat<n, c, F>& matrix) const {
			if (c != m_channels)
				throw std::invalid_argument("Transform matrix must have 'channels' rows");

			using Vec = glm::vec<c, value_type>;
			using fVec = glm::vec<c, F>;
			using newVec = glm::vec<n, value_type>;

			return transformStub<n, c, F>([&matrix](const Vec& vec) {
				auto out = fVec{ vec } * matrix;
				return newVec(out);
			});
		}

		template <glm::length_t c, std::floating_point F = float> [[nodiscard]]
		ImageDat transform(const glm::vec<c, F>& matrix) const {
			if (c != m_channels)
				throw std::invalid_argument("Transform matrix must have 'channels' rows");

			using Vec = glm::vec<c, value_type>;
			using fVec = glm::vec<c, F>;
			using newVec = glm::vec<1, value_type>;

			return transformStub<1, c, F>([&matrix](const Vec& vec) {
				auto out = glm::dot(matrix, fVec{ vec });
				return newVec(value_type(out));
			});
		}

		[[nodiscard]]
		ImageDat asGrayscale() const {
			auto colors = std::min(uint8_t(3), m_channels);
			float c = 1.0f / colors;

			switch (m_channels) {
			case 2:
				return transform(glm::vec2{ c });
			case 3:
				return transform(glm::vec3{ c });
			case 4:
				return transform(glm::vec4{ c, c, c, 0.0f });
			default:
				return *this;
			}
		}

		template <class SerializeIt>
		void serialize(SerializeIt& it) const {
			serializeToAdjust(it, m_width);
			serializeToAdjust(it, m_height);
			serializeToAdjust(it, m_channels);
			
			for (auto b : m_data) {
				serializeToAdjust(it, b);
			}
		}

		template <class InputIt>
		void deserialize(InputIt& in, InputIt end) {
			deserializeFromAdjust(m_width, in, end);
			deserializeFromAdjust(m_height, in, end);
			deserializeFromAdjust(m_channels, in, end);

			if (m_channels <= 0 || m_channels > 4)
				throw deserialize_error("Tried to deserialize invalid channel count");

			size_t size = m_width * m_height * m_channels;
			m_data.resize(size);

			for (auto& b : m_data) {
				deserializeFromAdjust(b, in, end);
			}
		}
	};

	template <class Loader> [[nodiscard]]
	auto loadImageFile(const std::filesystem::path& path, uint8_t channels, const Loader& loader) {
		if (channels <= 0 || channels > 4)
			throw std::invalid_argument("Channels must be between 1 and 4");

		return loader.loadFromFile(path, channels);
	}

	using ImageData = ImageDat<std::byte>;
	using FloatImageData = ImageDat<float>;

	template <class Saver>
	bool saveImageFile(const std::filesystem::path& path, const ImageData& image, const Saver& saver) {
		return saver.save(path, image);
	}
}


namespace sndx {
	template<class T>
	struct Serializer<render::ImageDat<T>> {
		template <class SerializeIt>
		constexpr void serialize(const render::ImageDat<T>& v, SerializeIt& it) const {
			v.serialize(it);
		}
	};

	template<class T>
	struct Deserializer<render::ImageDat<T>> {
		template <class DeserializeIt>
		constexpr void deserialize(render::ImageDat<T>& to, DeserializeIt& in, DeserializeIt end) const {
			to.deserialize(in, end);
		}
	};
}