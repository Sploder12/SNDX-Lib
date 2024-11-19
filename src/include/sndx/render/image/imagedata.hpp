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
	class ImageData {
	private:
		std::vector<std::byte> m_data{};
		size_t m_width{}, m_height{};
		uint8_t m_channels{};

		template <glm::length_t n, glm::length_t c, std::floating_point T, class Fn> [[nodiscard]]
		ImageData transformStub(Fn&& func) const {
			using oldVec = glm::vec<c, std::byte>;
			using newVec = glm::vec<n, std::byte>;

			size_t pixels = m_width * m_height;
			std::vector<std::byte> data(pixels * n);

			static_assert(sizeof(oldVec) == c * sizeof(std::byte));
			static_assert(sizeof(newVec) == n * sizeof(std::byte));

			auto asVecs = reinterpret_cast<const oldVec*>(m_data.data());
			auto newVecs = reinterpret_cast<newVec*>(data.data());

			std::transform(std::execution::par_unseq, asVecs, asVecs + pixels, newVecs, std::forward<Fn>(func));

			return ImageData{ m_width, m_height, n, std::move(data) };
		}
	public:
		ImageData(size_t width, size_t height, uint8_t channels, decltype(m_data) && data) :
			m_data(std::move(data)), m_width(width), m_height(height), m_channels(channels) {}

		ImageData(size_t width, size_t height, uint8_t channels, std::span<const std::byte> data) :
			m_width(width), m_height(height), m_channels(channels) {

			if (channels <= 0 || channels > 4)
				throw std::invalid_argument("Channels must be between 1 and 4.");

			auto size = width * height * channels;
			if (size != data.size())
				throw std::domain_error("Data size mismatch");

			m_data.reserve(size);
			std::copy_n(data.begin(), size, std::back_inserter(m_data));
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
			return pixels() * channels();
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
		glm::vec<n, std::byte> at(size_t x, size_t y) const {
			if (n != channels())
				throw std::invalid_argument("n must be equal to channels");

			if (x >= width() || y >= height())
				throw std::domain_error("Out of bounds access detected");

			using Vec = glm::vec<n, std::byte>;
			const Vec* asVecs = reinterpret_cast<const Vec*>(m_data.data());

			return asVecs[y * width() + x];
		}

		template <glm::length_t n, glm::length_t c, std::floating_point T = float> [[nodiscard]]
		ImageData transform(const glm::mat<n, c, T>& matrix) const {
			if (c != m_channels)
				throw std::invalid_argument("Transform matrix must have 'channels' rows");

			using Vec = glm::vec<c, std::byte>;
			using fVec = glm::vec<c, T>;
			using newVec = glm::vec<n, std::byte>;

			return transformStub<n, c, T>([&matrix](const Vec& vec) {
				auto out = fVec{ vec } * matrix;
				return newVec(out);
			});
		}

		template <glm::length_t c, std::floating_point T = float> [[nodiscard]]
		ImageData transform(const glm::vec<c, T>& matrix) const {
			if (c != m_channels)
				throw std::invalid_argument("Transform matrix must have 'channels' rows");

			using Vec = glm::vec<c, std::byte>;
			using fVec = glm::vec<c, T>;
			using newVec = glm::vec<1, std::byte>;

			return transformStub<1, c, T>([&matrix](const Vec& vec) {
				auto out = glm::dot(matrix, fVec{ vec });
				return newVec(std::byte(out));
			});
		}

		[[nodiscard]]
		ImageData asGrayscale() const {
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

		void serialize(serialize::Serializer& serializer) const {
			serializer.serialize<std::endian::little>(m_width);
			serializer.serialize<std::endian::little>(m_height);
			serializer.serialize(m_channels);
			serializer.serialize(reinterpret_cast<const uint8_t*>(m_data.data()), m_data.size());
		}

		void deserialize(serialize::Deserializer& deserializer) {
			deserializer.deserialize<std::endian::little>(m_width);
			deserializer.deserialize<std::endian::little>(m_height);
			deserializer.deserialize(m_channels);

			if (m_channels <= 0 || m_channels > 4)
				throw deserialize_error("Tried to deserialize invalid channel count");

			size_t size = m_width * m_height * m_channels;
			m_data.resize(size);
			deserializer.deserialize(reinterpret_cast<uint8_t*>(m_data.data()), size);
		}
	};

	template <class Loader> [[nodiscard]]
	auto loadImageFile(const std::filesystem::path& path, uint8_t channels, const Loader& loader) {
		if (channels <= 0 || channels > 4)
			throw std::invalid_argument("Channels must be between 1 and 4");

		return loader.loadFromFile(path, channels);
	}

	template <class Saver> [[nodiscard]]
	bool saveImageFile(const std::filesystem::path& path, const ImageData& image, const Saver& saver) {
		return saver.save(path, image);
	}
}