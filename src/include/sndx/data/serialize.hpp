#pragma once

#include <array>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <type_traits>
#include <concepts>
#include <stdexcept>
#include <vector>

#include "../utility/endian.hpp"

namespace sndx {
	template <class T>
	struct Serializer {
		template <class SerializeIt>
		constexpr void serialize(const T& value, SerializeIt& it) const;
	};

	template <class OutputIt, class T>
	constexpr OutputIt serializeTo(OutputIt out, const T& value) {
		Serializer<T> serializer{};
		serializer.serialize(value, out);
		return out;
	}

	template <class OutputIt, class T>
	constexpr void serializeToAdjust(OutputIt& out, const T& value) {
		out = serializeTo(out, value);
	}

	template <class T> [[nodiscard]]
	constexpr std::vector<uint8_t> serialize(const T& value) {
		std::vector<uint8_t> out{};

		using OutItT = decltype(std::back_inserter(out));

		serializeTo<OutItT, T>(std::back_inserter(out), value);
		return out;
	}

	template<class T>
		requires
			std::is_same_v<T, int8_t> ||
			std::is_same_v<T, uint8_t> ||
			std::is_same_v<T, std::byte> ||
		    std::is_same_v<T, char>
	struct Serializer<T> {
		template <class SerializeIt>
		constexpr void serialize(const T& value, SerializeIt& it) const {
			*it = std::bit_cast<uint8_t>(value);
			++it;
		}
	};

	template<class T, size_t N>
	struct Serializer<std::array<T, N>> {
		template <class SerializeIt>
		constexpr void serialize(const std::array<T, N>& arr, SerializeIt& it) const {
			for (const auto& v : arr) {
				serializeToAdjust(it, v);
			}
		}
	};

	template<class T>
		requires 
			std::is_same_v<T, int16_t> ||
			std::is_same_v<T, int32_t> ||
			std::is_same_v<T, int64_t> ||
			std::is_same_v<T, uint16_t> ||
			std::is_same_v<T, uint32_t> ||
			std::is_same_v<T, uint64_t>
	struct Serializer<T> {
		template <class SerializeIt>
		constexpr void serialize(T value, SerializeIt& it) const {
			value = utility::fromEndianess<std::endian::little>(value);

			auto bytes = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
			serializeToAdjust(it, bytes);
		}
	};
}

namespace sndx {
	struct deserialize_error : std::runtime_error {
		using runtime_error::runtime_error;
	};

	struct out_of_data_error : deserialize_error {
		using deserialize_error::deserialize_error;
	};

	struct bad_field_error : deserialize_error {
		using deserialize_error::deserialize_error;
	};

	template <class T>
	struct Deserializer {
		template <class DeserializeIt>
		constexpr void deserialize(T& to, DeserializeIt& it, DeserializeIt end) const;
	};

	template <class T, class InputIt>
	constexpr void deserializeFromAdjust(T& to, InputIt& in, InputIt end) {
		Deserializer<T> deserializer{};
		deserializer.deserialize(to, in, end);
	}

	template <class T, class InputIt>
	constexpr void deserializeFrom(T& to, InputIt in, InputIt end) {
		deserializeFromAdjust(to, in, end);
	}


	template <class T> [[nodiscard]]
	constexpr void deserialize(T& to, const std::vector<uint8_t>& bytes) {
		deserializeFrom(to, bytes.begin(), bytes.end());
	}

	template<class T>
		requires
			std::is_same_v<T, int8_t> ||
			std::is_same_v<T, uint8_t> ||
			std::is_same_v<T, std::byte> ||
			std::is_same_v<T, char>
	struct Deserializer<T> {
		template <class DeserializeIt>
		constexpr void deserialize(T& to, DeserializeIt& in, DeserializeIt end) const {
			if (in == end)
				throw out_of_data_error{"Ran out of data while deserializing"};

			uint8_t out = *in;
			++in;
			to = std::bit_cast<T>(out);
		}
	};

	template<class T, size_t N>
	struct Deserializer<std::array<T, N>> {
		template <class DeserializeIt>
		constexpr void deserialize(std::array<T, N>& to, DeserializeIt& in, DeserializeIt end) const {
			for (auto& v : to) {
				deserializeFromAdjust(v, in, end);
			}
		}
	};

	template<class T>
		requires
			std::is_same_v<T, int16_t> ||
			std::is_same_v<T, int32_t> ||
			std::is_same_v<T, int64_t> ||
			std::is_same_v<T, uint16_t> ||
			std::is_same_v<T, uint32_t> ||
			std::is_same_v<T, uint64_t>
	struct Deserializer<T> {
		template <class DeserializeIt>
		constexpr void deserialize(T& to, DeserializeIt& in, DeserializeIt end) const {
			using BufferT = std::array<uint8_t, sizeof(T)>;

			BufferT buffer{};
			deserializeFromAdjust(buffer, in, end);
			auto value = std::bit_cast<T>(buffer);

			to = utility::fromEndianess<std::endian::little>(value);
		}
	};
}
