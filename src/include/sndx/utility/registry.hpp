#pragma once

#include <mutex>
#include <shared_mutex>
#include <unordered_map>

namespace sndx::utility {
	template <class KeyT, class ValT>
	class FactoryRegistry {
	protected:
		std::unordered_map<KeyT, ValT> m_registry{};
		std::shared_mutex m_mtx;

	public:
		struct no_factory_error : public std::invalid_argument {
			using std::invalid_argument::invalid_argument;
		};

		bool add(const KeyT& key, ValT fn) {
			std::unique_lock lock(m_mtx);

			return m_registry.emplace(key, std::move(fn)).second;
		}

		bool remove(const KeyT& key) {
			std::unique_lock lock(m_mtx);

			return m_registry.erase(key) > 0;
		}

		void clear() {
			std::unique_lock lock(m_mtx);

			m_registry.clear();
		}

		template <class... Args>
		decltype(auto) apply(const KeyT& key, Args&&... args) {
			std::shared_lock lock(m_mtx);

			if (auto it = m_registry.find(key); it != m_registry.end()) {
				auto func = it->second;
				lock.unlock();

				return func(std::forward<Args>(args)...);
			}
			
			throw no_factory_error("Factory could not be found in registry");
		}
	};
}