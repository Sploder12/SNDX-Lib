#pragma once

#include <cstddef>

#include <bit>
#include <bitset>
#include <cstdint>
#include <stdexcept>
#include <iterator>

#include "../mixin/derived_ops.hpp"

namespace sndx::container {

	template <size_t bits, std::endian endianess = std::endian::native>
	class PackedView {
		static_assert(bits > 0);
		static_assert(bits <= sizeof(unsigned long long) * 8);

		static_assert(endianess == std::endian::little || endianess == std::endian::big);

	protected:
		const std::byte* m_data = nullptr;
		size_t m_count = 0;

		uint8_t m_offset = 0; // value between 0 and 7;
	public:
		static constexpr size_t s_bitsPerEntry = bits;
		static constexpr std::endian s_endianess = endianess;

		using out_type = std::bitset<s_bitsPerEntry>;

		// https://en.cppreference.com/w/cpp/iterator/bidirectional_iterator
		// yes, it's a random_access_iterator.
		class Iterator : 
			public mixin::Comparisons<Iterator>,
			public mixin::AddSub<Iterator, long long, true> {
		public:
			// iterator - iterator overload forces this line to be present
			using mixin::AddSub<Iterator, long long, true>::operator-;

			using iterator_category = std::random_access_iterator_tag;
			using difference_type = long long;
			using value_type = out_type;
			using pointer = void;
			using reference = void;

			using container = PackedView<s_bitsPerEntry, s_endianess>;

			friend container;

		private:
			const container* m_container = nullptr;
			size_t m_pos = 0;

			explicit constexpr Iterator(const container& contain, size_t pos) noexcept:
				m_container(std::addressof(contain)), m_pos(pos) {}

		public:
			explicit constexpr Iterator() noexcept = default;

			constexpr Iterator& operator+=(difference_type n) noexcept {
				m_pos += n;
				return *this;
			}

			constexpr Iterator& operator-=(difference_type n) noexcept {
				m_pos -= n;
				return *this;
			}

			[[nodiscard]]
			value_type operator*() const noexcept {
				return (*m_container)[m_pos];
			}

			[[nodiscard]]
			constexpr value_type operator[](difference_type pos) const noexcept {
				return *((*this) + pos);
			}

			[[nodiscard]]
			constexpr difference_type operator-(const Iterator& other) const noexcept {
				return (difference_type)m_pos - (difference_type)other.m_pos;
			}

			[[nodiscard]]
			constexpr std::strong_ordering operator<=>(const Iterator& other) const {
				if (m_container != other.m_container)
					throw std::logic_error("Compared Iterators of PackedView refer to different PackedViews!");

				if (m_pos < other.m_pos) 
					return std::strong_ordering::less;

				if (m_pos > other.m_pos)
					return std::strong_ordering::greater;

				return std::strong_ordering::equal;
			}
		};

		static_assert(std::random_access_iterator<Iterator>);

	public:
		explicit constexpr PackedView() noexcept = default;

		constexpr PackedView(const std::byte* data, size_t count, uint8_t offset = 0) noexcept:
			m_data(data), m_count(count), m_offset(offset % 8) {}

		constexpr PackedView(std::nullptr_t, size_t, uint8_t = 0) noexcept = delete;

		// can be made constexpr c++23
		[[nodiscard]]
		out_type at(size_t pos) const {
			if (pos >= size())
				throw std::out_of_range("PackedView::at pos out of range");

			return (*this)[pos];
		}

		[[nodiscard]]
		out_type operator[](size_t pos) const noexcept {
			size_t bitPos = (s_bitsPerEntry * pos) + m_offset;

			size_t bytePos = bitPos / 8;
			uint8_t bitOffset = bitPos % 8;

			out_type out{};

			auto curByte = std::bitset<8>((unsigned long long)m_data[bytePos]);

			for (size_t i = 0; i < out.size(); ++i) {
				if constexpr (s_endianess == std::endian::big) {
					out[out.size() - 1 - i] = curByte[8 - 1 - bitOffset];
				}
				else {
					out[i] = curByte[bitOffset];
				}

				++bitOffset;
				if (bitOffset >= 8) {
					bitOffset = 0;

					++bytePos;
					curByte = std::bitset<8>((unsigned long long)m_data[bytePos]);
				}
			}

			return out;
		}

		[[nodiscard]]
		constexpr auto size() const noexcept {
			return m_count;
		}

		[[nodiscard]]
		constexpr auto subview(size_t pos, size_t count = -1) const {
			if (pos > size())
				throw std::out_of_range("subview pos exceeds current size");

			if (pos == size())
				return PackedView(m_data, 0);

			auto bitPos = pos * s_bitsPerEntry + m_offset;

			auto bytePos = bitPos / 8;
			uint8_t bitOffset = bitPos % 8;

			return PackedView(m_data + bytePos, std::min(count, m_count - bytePos), bitOffset);
		}

		[[nodiscard]]
		constexpr auto data() const noexcept {
			return m_data;
		}

		[[nodiscard]]
		constexpr auto begin() const noexcept {
			return Iterator(*this, 0);
		}

		[[nodiscard]]
		constexpr auto end() const noexcept {
			return Iterator(*this, size());
		}

		[[nodiscard]]
		constexpr auto rbegin() const noexcept {
			return std::make_reverse_iterator(end());
		}

		[[nodiscard]]
		constexpr auto rend() const noexcept {
			return std::make_reverse_iterator(begin());
		}
	};
}