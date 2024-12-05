#pragma once

#include <array>
#include <string>
#include <set>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <stdexcept>

#include <glm/glm.hpp>

namespace sndx::math {

	template <class IdT = std::string>
	struct Packing {
		std::unordered_map<IdT, glm::vec<2, size_t>> positions{};
		size_t neededWidth{};
		size_t neededHeight{};

		[[nodiscard]]
		auto empty() const noexcept {
			return positions.empty();
		}

		[[nodiscard]]
		auto find(const IdT& id) const noexcept {
			return positions.find(id);
		}

		[[nodiscard]]
		auto begin() const noexcept {
			return positions.begin();
		}

		[[nodiscard]]
		auto end() const noexcept {
			return positions.end();
		}

		[[nodiscard]]
		auto contains(const IdT& id) const noexcept {
			return positions.contains(id);
		}

		[[nodiscard]]
		auto width() const noexcept {
			return neededWidth;
		}

		[[nodiscard]]
		auto height() const noexcept {
			return neededHeight;
		}
	};

	template <bool horizontal>
	struct BinPackEntry {
		std::array<size_t, 2> dims{};

		constexpr BinPackEntry(size_t width, size_t height) noexcept :
			dims{ width, height } {
		}

		[[nodiscard]]
		constexpr size_t getPrimaryDim() const noexcept {
			return dims[horizontal];
		}

		[[nodiscard]]
		constexpr size_t getSecondaryDim() const noexcept {
			return dims[!horizontal];
		}

		constexpr bool operator>(const BinPackEntry& other) const noexcept {
			if (getPrimaryDim() == other.getPrimaryDim())
				return getSecondaryDim() > other.getSecondaryDim();

			return getPrimaryDim() > other.getPrimaryDim();
		}
	};

	template <bool horizontal = true, class IdT = std::string>
	class BinPacker {
	private:
		using Entry = BinPackEntry<horizontal>;
		
		struct LabeledEntry : public Entry {
			using Entry::operator>;

			IdT id;

			constexpr LabeledEntry(const IdT& ID, size_t width, size_t height) noexcept :
				Entry{ width, height }, id(ID) {}
		};

		struct Shelf {
			Entry dims{};
			size_t occupied{};

			std::vector<std::pair<IdT, const Entry*>> entries{};

			Shelf(size_t primary, size_t secondary) noexcept :
				dims{ primary, secondary } {
			
				if constexpr (horizontal) {
					std::swap(dims.dims[0], dims.dims[1]);
				}
			}

			[[nodiscard]]
			bool canAddEntry(const Entry& entry) const noexcept {
				return occupied + entry.getSecondaryDim() <= dims.getSecondaryDim();
			}

			void addEntry(const LabeledEntry& entry, size_t padding = 0) noexcept {
				occupied += entry.getSecondaryDim() + padding;
				entries.emplace_back(entry.id, std::addressof(entry));
			}

			[[nodiscard]]
			bool tryAddEntry(const LabeledEntry& entry, size_t padding = 0) noexcept {
				return canAddEntry(entry) && (addEntry(entry, padding), true);
			}
		};

		std::multiset<LabeledEntry, std::greater<LabeledEntry>> m_entries{};

	public:
		void add(const IdT& id, size_t width, size_t height) {
			m_entries.emplace(id, width, height);
		}

		// uses a modified Next-Fit Decreasing Height/Width algorithm.
		[[nodiscard]]
		Packing<IdT> pack(size_t dimConstraint, size_t padding = 0) const {
			Packing<IdT> out{};

			if (m_entries.size() == 0) [[unlikely]]
				return out;

			out.positions.reserve(m_entries.size());

			std::vector<Shelf> shelves{};
			for (const auto& entry : m_entries) {

				if (entry.getSecondaryDim() > dimConstraint)
					throw std::invalid_argument("Cannot pack box that exceeds size constraint itself.");

				bool added = false;
				for (auto& prevShelf : shelves) {
					added = prevShelf.tryAddEntry(entry, padding);
					if (added)
						break;
				}

				if (!added) {
					shelves.emplace_back(entry.getPrimaryDim(), dimConstraint);
					shelves.back().addEntry(entry, padding);
				}
			}

			size_t neededSecondary = 0;
			size_t primary = 0;
			for (const auto& shelf : shelves) {
				size_t secondary = 0;

				for (auto& [id, entry] : shelf.entries) {
					if constexpr (horizontal) {
						out.positions.emplace(std::move(id), glm::vec<2, size_t>{secondary, primary});
					}
					else {
						out.positions.emplace(std::move(id), glm::vec<2, size_t>{primary, secondary});
					}

					secondary += entry->getSecondaryDim() + padding;
				}

				primary += shelf.dims.getPrimaryDim() + padding;
				neededSecondary = std::max(neededSecondary, secondary);
			}

			if (primary >= padding)
				primary -= padding;

			if (neededSecondary >= padding)
				neededSecondary -= padding;

			if constexpr (horizontal) {
				out.neededHeight = primary;
				out.neededWidth = neededSecondary;
			}
			else {
				out.neededWidth = primary;
				out.neededHeight = neededSecondary;
			}

			return out;
		}
	};
}