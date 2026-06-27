#pragma once

#include <array>
#include <concepts>
#include <cstdint>

namespace sndx::render {
	enum class Type : uint8_t {
		bytes,
		ubytes,
		shorts,
		ushorts,
		ints,
		uints,
		halfFloats,
		floats,
		doubles,
	};

	template <typename T>
	concept matrix = requires (T m) {
		T::length(); typename T::col_type;
		m[0][0];
	};

	template <typename T>
	concept vector = requires (T v) {
		T::length(); T::x;
		v[0];
	} && !matrix<T>;

	template <class T> [[nodiscard]]
	constexpr Type asLayoutType() noexcept {
		static_assert(!std::is_same_v<T, T>);
		return Type::floats;
	}

	template <> [[nodiscard]]
	constexpr Type asLayoutType<int8_t>() noexcept {
		return Type::bytes;
	}

	template <> [[nodiscard]]
	constexpr Type asLayoutType<uint8_t>() noexcept {
		return Type::ubytes;
	}

	template <> [[nodiscard]]
	constexpr Type asLayoutType<int16_t>() noexcept {
		return Type::shorts;
	}

	template <> [[nodiscard]]
	constexpr Type asLayoutType<uint16_t>() noexcept {
		return Type::ushorts;
	}

	template <> [[nodiscard]]
	constexpr Type asLayoutType<int32_t>() noexcept {
		return Type::ints;
	}

	template <> [[nodiscard]]
	constexpr Type asLayoutType<uint32_t>() noexcept {
		return Type::uints;
	}

	template <> [[nodiscard]]
	constexpr Type asLayoutType<float>() noexcept {
		return Type::floats;
	}

	template <> [[nodiscard]]
		constexpr Type asLayoutType<double>() noexcept {
		return Type::doubles;
	}

	template <matrix T> [[nodiscard]]
	constexpr Type asLayoutType() noexcept {
		return asLayoutType<typename T::value_type>();
	}

	template <vector T> [[nodiscard]]
	constexpr Type asLayoutType() noexcept {
		return asLayoutType<typename T::value_type>();
	}

	template <class T>
	struct Normalized {
		using value_type = T;
		using normalized = std::bool_constant<true>;

		T data;
	};

	template <typename T>
	concept normalized = requires (T) {
		requires std::same_as<typename T::normalized, std::bool_constant<true>>;
	};

	template <normalized T> [[nodiscard]]
	constexpr Type asLayoutType() noexcept {
		return asLayoutType<typename T::value_type>();
	}

	struct LayoutEntry {
		uint32_t offset;
		uint32_t stride;
		Type type;
		uint8_t count;
		bool normalized;
		bool instanced;
	};

	namespace detail {
		template <class T, class... Rest> [[nodiscard]]
		static constexpr uint32_t size() noexcept {
			if constexpr (sizeof...(Rest) > 0) {
				return sizeof(T) + size<Rest...>();
			}
			else {
				return sizeof(T);
			}
		}

		template <class T> [[nodiscard]]
		static constexpr size_t entryCount() noexcept {
			if constexpr (vector<T>) {
				return 1;
			}
			else if constexpr (matrix<T>) {
				return T::length();
			}
			else {
				return 1;
			}
		}
	}

	template <class... Ts>
	class Layout {
		static_assert(sizeof...(Ts) > 0);
	public:
		[[nodiscard]]
		static constexpr size_t entryCount() noexcept {
			return (detail::entryCount<Ts>() + ...);
		}

		[[nodiscard]]
		static constexpr uint32_t stride() {
			return detail::size<Ts...>();
		}

		using Entries = std::array<LayoutEntry, entryCount()>;
	private:
		template <class T> [[nodiscard]]
		static constexpr uint32_t addEntry(Entries& out, size_t i, uint32_t stride, bool instanced, uint32_t curOffset) {
			constexpr Type type = asLayoutType<T>();
			constexpr bool normal = sndx::render::normalized<T>;

			if constexpr (vector<T>) {
				out[i] = LayoutEntry{ curOffset, stride, type, T::length(), normal, instanced };
				return curOffset + sizeof(T);
			}
			else if constexpr (matrix<T>) {
				for (size_t j = 0; j < T::length(); ++j) {
					curOffset = addEntry<typename T::col_type>(out, i + j, stride, instanced, curOffset);
				}
				return curOffset;
			}
			else {
				out[i] = LayoutEntry{ curOffset, stride, type, 1, normal, instanced };
				return curOffset + sizeof(T);
			}
		}

		template <class T, class... Rest>
		static constexpr void addEntries(Entries& out, size_t i, uint32_t stride, bool instanced, uint32_t curOffset = 0) {
			curOffset = addEntry<T>(out, i, stride, instanced, curOffset);

			if constexpr (sizeof...(Rest) > 0) {
				addEntries<Rest...>(out, i + detail::entryCount<T>(), stride, instanced, curOffset);
			}
		}

	public:
		[[nodiscard]]
		static constexpr auto getEntries(bool instanced) {
			Entries out{};
			addEntries<Ts...>(out, 0, stride(), instanced);
			return out;
		}

		template <bool instanced = false>
		static constexpr auto value = getEntries(instanced);
	};

}