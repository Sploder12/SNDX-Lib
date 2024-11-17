#pragma once

#include <vector>
#include <cstddef>
#include <algorithm>
#include <span>
#include <stdexcept>
#include <iterator>
#include <optional>
#include <filesystem>
#include <string>
#include <execution>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace sndx::render {
	class ImageData {
	private:
		std::vector<std::byte> m_data{};
		size_t m_width{}, m_height{};
		uint8_t m_channels{};

		template <size_t n, size_t c, std::floating_point T, class Fn> [[nodiscard]]
		ImageData transformStub(Fn&& func) const {
			using oldVec = glm::vec<c, std::byte>;
			using newVec = glm::vec<n, std::byte>;

			size_t pixels = m_width * m_height;
			std::vector<std::byte> data(pixels * n);

			const oldVec* asVecs = reinterpret_cast<const oldVec*>(m_data.data());
			newVec* newVecs = reinterpret_cast<newVec*>(data.data());

			std::transform(std::execution::par_unseq, asVecs, asVecs + pixels, newVecs, func);

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

		template <size_t n> [[nodiscard]]
		glm::vec<n, std::byte> at(size_t x, size_t y) const {
			if (n != channels())
				throw std::invalid_argument("n must be equal to channels");

			if (x >= width() || y >= height())
				throw std::domain_error("Out of bounds access detected");

			using Vec = glm::vec<n, std::byte>;
			const Vec* asVecs = reinterpret_cast<const Vec*>(m_data.data());

			return asVecs[y * width() + x];
		}

		template <size_t n, size_t c, std::floating_point T = float> [[nodiscard]]
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

		template <size_t c, std::floating_point T = float> [[nodiscard]]
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
	};

	template <class Loader> [[nodiscard]]
	static auto loadImageFile(const std::filesystem::path& path, uint8_t channels, const Loader& loader) {
		if (channels <= 0 || channels > 4)
			throw std::invalid_argument("Channels must be between 1 and 4");

		return loader.loadFromFile(path, channels);
	}
}