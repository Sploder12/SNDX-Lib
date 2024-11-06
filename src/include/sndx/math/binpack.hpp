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


	template <bool horizontal = true, class IdT = std::string>
	class BinPacker {
	private:
		struct Entry {
			std::array<size_t, 2> dims{};

			constexpr Entry(size_t width, size_t height) noexcept:
				dims{ width, height } {}

			[[nodiscard]]
			constexpr size_t getPrimaryDim() const noexcept {
				return dims[horizontal];
			}

			[[nodiscard]]
			constexpr size_t getSecondaryDim() const noexcept {
				return dims[!horizontal];
			}

			constexpr bool operator>(const Entry& other) const noexcept {
				if (getPrimaryDim() == other.getPrimaryDim())
					return getSecondaryDim() > other.getSecondaryDim();

				return getPrimaryDim() > other.getPrimaryDim();
			}
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

			void addEntry(const IdT& id, const Entry& entry, size_t padding = 0) noexcept {
				occupied += entry.getSecondaryDim() + padding;
				entries.emplace_back(id, std::addressof(entry));
			}
		};

		std::multiset<Entry, std::greater<Entry>> m_entries{};
		std::unordered_map<IdT, typename decltype(m_entries)::iterator> m_ids{};

	public:
		bool add(const IdT& id, size_t width, size_t height) {
			return m_ids.emplace(id, m_entries.emplace(width, height)).second;
		}

		bool remove(const IdT& id) {
			if (auto itit = m_ids.find(id); itit != m_ids.end()) {
				m_entries.erase(itit->second);
				m_ids.erase(itit);
				return true;
			}

			return false;
		}

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
			auto contains(const IdT& id) const noexcept {
				return positions.contains(id);
			}
		};

		// uses a modified Next-Fit Decreasing Height/Width algorithm.
		[[nodiscard]]
		Packing pack(size_t dimConstraint, size_t padding = 0) const {
			Packing out{};

			if (m_entries.size() == 0) [[unlikely]]
				return out;

			out.positions.reserve(m_entries.size());

			size_t shelfSecondary = dimConstraint;
			size_t shelfPrimary = m_entries.cbegin()->getPrimaryDim();

			size_t neededPrimary = 0;
			size_t neededSecondary = 0;

			std::vector<Shelf> shelves{};
			Shelf currentShelf{ shelfPrimary, shelfSecondary };

			for (const auto& [id, entry_it] : m_ids) {
				const auto& entry = *entry_it;

				if (entry.getSecondaryDim() > dimConstraint)
					throw std::invalid_argument("Cannot pack box that exceeds size constraint itself.");

				bool added = false;
				for (auto& prevShelf : shelves) {
					if (prevShelf.canAddEntry(entry)) {
						prevShelf.addEntry(id, entry, padding);
						added = true;
					}
				}

				if (!added) {
					if (currentShelf.canAddEntry(entry))
						currentShelf.addEntry(id, entry, padding);
					else {
						neededSecondary = std::max(neededSecondary, currentShelf.occupied);
						neededPrimary += currentShelf.dims.getPrimaryDim() + padding;
						shelves.emplace_back(std::move(currentShelf));
						shelfPrimary = entry.getPrimaryDim();
						currentShelf = Shelf{ shelfPrimary, shelfSecondary };
						currentShelf.addEntry(id, entry, padding);
					}
				}
			}

			if (!currentShelf.entries.empty()) {
				neededSecondary = std::max(neededSecondary, currentShelf.occupied);
				neededPrimary += currentShelf.dims.getPrimaryDim() + padding;
				shelves.emplace_back(std::move(currentShelf));
			}

			neededPrimary -= padding;
			neededSecondary -= padding;

			if constexpr (horizontal) {
				out.neededHeight = neededPrimary;
				out.neededWidth = neededSecondary;
			}
			else {
				out.neededWidth = neededPrimary;
				out.neededHeight = neededSecondary;
			}

			size_t primary = 0;
			size_t secondary = 0;

			for (const auto& shelf : shelves) {
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
				secondary = 0;
			}

			return out;
		}
	};
}