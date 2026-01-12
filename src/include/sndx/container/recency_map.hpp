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

		void erase(const KeyT& key) {
			if (auto it = mapping.find(key); it != mapping.end()) {
				container.erase(it->second);
				mapping.erase(it);
			}
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

		[[nodiscard]] decltype(auto) end() {
			return container.end();
		}

		[[nodiscard]] decltype(auto) end() const {
			return container.end();
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

		void erase(const KeyT& key) {
			underlying.erase(key);
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

		[[nodiscard]] decltype(auto) end() {
			return underlying.end();
		}

		[[nodiscard]] decltype(auto) end() const {
			return underlying.end();
		}
	};
}
