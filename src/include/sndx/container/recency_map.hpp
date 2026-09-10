#pragma once

#include <functional>
#include <list>
#include <unordered_map>
#include <utility>

namespace sndx {
	// ONLY .get, .poke, and .insert_or_update will update recency
	template <class KeyT, class ItemT>
	class RecencyMap {
	public:
		using key_type = KeyT;
		using value_type = std::pair<const KeyT*, ItemT>;

	private:
		// front is most recent, back is oldest
		std::list<std::pair<const KeyT*, ItemT>> container{};
		using ContainerIt = typename decltype(container)::iterator;

		std::unordered_map<KeyT, ContainerIt> mapping{};

		void updateRecency(ContainerIt& it) {
			container.splice(container.begin(), container, it);
			it = container.begin();
		}
	public:

		[[nodiscard]]
		ItemT* get(const KeyT& key) {
			if (auto it = mapping.find(key); it != mapping.end()) {
				auto& containerIt = it->second;

				updateRecency(containerIt);
				return &containerIt->second;
			}
			return nullptr;
		}

		bool poke(const KeyT& key) {
			return get(key) != nullptr;
		}

		template <class M>
		std::pair<ContainerIt, bool> insert_or_assign(const KeyT& key, M&& value) {
			if (auto it = mapping.find(key); it != mapping.end()) {
				auto& containerIt = it->second;

				updateRecency(containerIt);
				containerIt->second = std::forward<M>(value);
				return { containerIt, false };
			}

			container.emplace_front(nullptr, std::forward<M>(value));
			auto mit = mapping.emplace(key, container.begin()).first;
			container.front().first = &mit->first;
			return { container.begin(), true };
		}

		void pop_most_recent() {
			mapping.erase(*container.front().first);
			container.pop_front();
		}

		void pop_least_recent() {
			mapping.erase(*container.back().first);
			container.pop_back();
		}

		size_t erase(const KeyT& key) {
			if (auto it = mapping.find(key); it != mapping.end()) {
				container.erase(it->second);
				mapping.erase(it);
				return 1;
			}
			return 0;
		}

		ContainerIt erase(ContainerIt it) {
			mapping.erase(*it->first);
			return container.erase(it);
		}

		void clear() {
			container.clear();
			mapping.clear();
		}

		[[nodiscard]] bool contains(const KeyT& key) const {
			return mapping.contains(key);
		}

		[[nodiscard]] size_t size() const {
			return container.size();
		}

		[[nodiscard]] bool empty() const {
			return container.empty();
		}

		[[nodiscard]] decltype(auto) front() {
			return container.front();
		}

		[[nodiscard]] decltype(auto) front() const {
			return container.front();
		}

		[[nodiscard]] decltype(auto) back() {
			return container.back();
		}

		[[nodiscard]] decltype(auto) back() const {
			return container.back();
		}

		[[nodiscard]] decltype(auto) begin() {
			return container.begin();
		}

		[[nodiscard]] decltype(auto) begin() const {
			return container.begin();
		}

		[[nodiscard]] decltype(auto) rbegin() {
			return container.rbegin();
		}

		[[nodiscard]] decltype(auto) rbegin() const {
			return container.rbegin();
		}

		[[nodiscard]] decltype(auto) end() {
			return container.end();
		}

		[[nodiscard]] decltype(auto) end() const {
			return container.end();
		}

		[[nodiscard]] decltype(auto) rend() {
			return container.rend();
		}

		[[nodiscard]] decltype(auto) rend() const {
			return container.rend();
		}
	};

	// ONLY .get, .poke, and .insert_or_update will update recency
	template <class KeyT, class ItemT, class TimeT>
	class TimeAwareRecencyMap {
	public:
		using TimestampedT = std::pair<TimeT, ItemT>;
		using key_type = KeyT;
		using value_type = std::pair<const KeyT*, TimestampedT>;

	private:
		RecencyMap<KeyT, TimestampedT> underlying{};

		std::function<TimeT()> timeProvider;
		
	public:
		TimeAwareRecencyMap(std::function<TimeT()> timeProvider):
			timeProvider(timeProvider) {}

		[[nodiscard]]
		TimeT getNow() {
			return timeProvider();
		}

		[[nodiscard]]
		ItemT* get(const KeyT& key) {
			if (auto timestamped = underlying.get(key)) {
				timestamped->first = timeProvider();
				return &timestamped->second;
			}
			return nullptr;
		}

		bool poke(const KeyT& key) {
			return get(key) != nullptr;
		}

		template <class M>
		auto insert_or_assign(const KeyT& key, M&& value) {
			return underlying.insert_or_assign(key, std::make_pair<TimeT, ItemT>(timeProvider(), std::forward<M>(value)));
		}

		void pop_most_recent() {
			underlying.pop_most_recent();
		}

		void pop_least_recent() {
			underlying.pop_least_recent();
		}

		// inclusive
		template <class DurationT>
		size_t erase_older_than(const DurationT& duration) {
			size_t count = 0;
			auto now = timeProvider();
			while (!underlying.empty()) {
				auto delta = now - underlying.back().second.first;
				if (delta >= duration) {
					underlying.pop_least_recent();
					++count;
				}
				else {
					break;
				}
			}
			return count;
		}

		// inclusive
		template <class DurationT, class Pred>
		size_t erase_older_than_and(const DurationT& duration, Pred pred) {
			size_t old = size();
			auto now = timeProvider();
			// takes advantage of being sorted by age
			for (auto it = end(); it != begin();) {
				--it;

				auto delta = now - it->second.first;
				if (delta >= duration) {
					if (pred(*it)) {
						it = underlying.erase(it);
					}
				}
				else {
					break;
				}
			}
			return old - size();
		}

		// exclusive
		template <class DurationT>
		size_t erase_newer_than(const DurationT& duration) {
			size_t count = 0;
			auto now = timeProvider();
			while (!underlying.empty()) {
				auto delta = now - underlying.front().second.first;
				if (delta < duration) {
					underlying.pop_most_recent();
					++count;
				}
				else {
					break;
				}
			}
			return count;
		}

		template <class Pred>
		size_t erase_if(Pred pred) {
			size_t old = size();
			for (auto it = begin(), last = end(); it != last;) {
				if (pred(*it)) {
					it = underlying.erase(it);
				}
				else {
					++it;
				}
			}
			return old - size();
		}

		auto erase(const KeyT& key) {
			return underlying.erase(key);
		}

		void clear() {
			underlying.clear();
		}

		[[nodiscard]] bool contains(const KeyT& key) const {
			return underlying.contains(key);
		}

		[[nodiscard]] size_t size() const {
			return underlying.size();
		}

		[[nodiscard]] bool empty() const {
			return underlying.empty();
		}

		[[nodiscard]] decltype(auto) front() {
			return underlying.front();
		}

		[[nodiscard]] decltype(auto) front() const {
			return underlying.front();
		}

		[[nodiscard]] decltype(auto) back() {
			return underlying.back();
		}

		[[nodiscard]] decltype(auto) back() const {
			return underlying.back();
		}

		[[nodiscard]] decltype(auto) begin() {
			return underlying.begin();
		}

		[[nodiscard]] decltype(auto) begin() const {
			return underlying.begin();
		}

		[[nodiscard]] decltype(auto) rbegin() {
			return underlying.rbegin();
		}

		[[nodiscard]] decltype(auto) rbegin() const {
			return underlying.rbegin();
		}

		[[nodiscard]] decltype(auto) end() {
			return underlying.end();
		}

		[[nodiscard]] decltype(auto) end() const {
			return underlying.end();
		}

		[[nodiscard]] decltype(auto) rend() {
			return underlying.rend();
		}

		[[nodiscard]] decltype(auto) rend() const {
			return underlying.rend();
		}
	};
}
