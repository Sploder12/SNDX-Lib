#pragma once

#include <atomic>
#include <compare>

namespace sndx::utility {
	namespace rid::category {
		constexpr size_t Generic = 0;
	}

	// Runtime IDs are stable and unique for the duration of the program.
	template <size_t category = rid::category::Generic>
	class RID {
		using underlying = uint64_t;

		static inline std::atomic<underlying> counter{1};
		underlying m_id;

		explicit RID() noexcept: m_id(counter.fetch_add(1, std::memory_order_relaxed)) {}

	public:
		constexpr explicit RID(std::nullptr_t) noexcept: m_id(0) {}

		[[nodiscard]]
		static RID generate() noexcept {
			return RID();
		}

		[[nodiscard]]
		auto id() const noexcept {
			return m_id;
		}

		auto operator<=>(const RID&) const = default;
	};

	template <size_t category = rid::category::Generic> [[nodiscard]]
	RID<category> generateRID() noexcept {
		return RID<category>::generate();
	}

	template <size_t category = rid::category::Generic> [[nodiscard]]
	constexpr RID<category> nullRID() noexcept {
		return RID<category>{nullptr};
	}
}

template<size_t category>
struct std::hash<sndx::utility::RID<category>> {
	constexpr size_t operator()(const sndx::utility::RID<category>& rid) const noexcept {
		auto v = rid.id();
		return std::hash<decltype(v)>{}(v);
	}
};
