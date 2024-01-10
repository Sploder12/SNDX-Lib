#pragma once

#include <vector>
#include <algorithm>
#include <optional>
#include <stdexcept>

namespace sndx {

	template <class DataT, typename IndexT = size_t>
	class WeightedVector {
	public:
		using value_type = DataT;
		using index_type = IndexT;

		struct Entry {
			//index_type from; 
			index_type to;

			DataT data;

			static bool comp(const IndexT& index, const Entry& entry) {
				return index < entry.to;
			}
		};

	protected:
		std::vector<Entry> entries;
		IndexT last = IndexT(0);
		
	public:
		WeightedVector() :
			entries{}, last(IndexT(0)) {}

		
		template<class... Ts>
		decltype(auto) emplace_back(const IndexT& weight, Ts&&... data) {
			if (weight <= IndexT(0.0)) [[unlikely]] {
				throw std::invalid_argument("Weight must be greater than 0");
			}

			last += weight;
			return entries.emplace_back(last, std::forward<Ts...>(data...));
		}

		decltype(auto) push_back(const IndexT& weight, const DataT& data) {
			return this->emplace_back(weight, data);
		}

		void pop_back() {
			if (entries.empty()) [[unlikely]] return;

			entries.pop_back();

			last = entries.empty() ? IndexT(0) : entries.back().to;
		}

		[[nodiscard]]
		DataT* at(const IndexT& index) {
			auto it = std::upper_bound(entries.begin(), entries.end(), index, Entry::comp);
			return it != entries.end() ? std::addressof(it->data) : nullptr;
		}

		[[nodiscard]]
		const DataT* at(const IndexT& index) const {
			auto it = std::upper_bound(entries.begin(), entries.end(), index, Entry::comp);
			return it != entries.end() ? std::addressof(it->data) : nullptr;
		}

		[[nodiscard]]
		DataT* operator[](const IndexT& index) {
			return at(index);
		}

		[[nodiscard]]
		const DataT* operator[](const IndexT& index) const {
			return at(index);
		}

		void clear() {
			entries.clear();
			last = IndexT(0);
		}

		void reserve(size_t count) {
			entries.reserve(count);
		}

		[[nodiscard]]
		decltype(auto) count() const {
			return entries.size();
		}

		[[nodiscard]]
		auto size() const {
			return last;
		}

		[[nodiscard]]
		decltype(auto) empty() const {
			return entries.empty();
		}

		[[nodiscard]]
		decltype(auto) front() {
			return entries.front();
		}

		[[nodiscard]]
		decltype(auto) front() const {
			return entries.front();
		}

		[[nodiscard]]
		decltype(auto) back() {
			return entries.back();
		}

		[[nodiscard]]
		decltype(auto) back() const {
			return entries.back();
		}

		[[nodiscard]]
		decltype(auto) begin() {
			return entries.begin();
		}

		[[nodiscard]]
		decltype(auto) begin() const {
			return entries.begin();
		}

		[[nodiscard]]
		decltype(auto) rbegin() {
			return entries.rbegin();
		}

		[[nodiscard]]
		decltype(auto) rbegin() const {
			return entries.rbegin();
		}

		[[nodiscard]]
		decltype(auto) end() {
			return entries.end();
		}

		[[nodiscard]]
		decltype(auto) end() const {
			return entries.end();
		}

		[[nodiscard]]
		decltype(auto) rend() {
			return entries.rend();
		}

		[[nodiscard]]
		decltype(auto) rend() const {
			return entries.rend();
		}

		decltype(auto) data() {
			return entries.data();
		}
	};
}