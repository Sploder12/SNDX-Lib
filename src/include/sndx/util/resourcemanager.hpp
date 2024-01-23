#pragma once

#include <vector>
#include <unordered_map>
#include <string>

namespace sndx {

	template <class T>
	decltype(auto) destroy(T& obj) {
		return obj.destroy();
	}

	template <class ResourceT, class IndexT = std::string, template<class, class> class InternalStorageT = std::unordered_map>
	class ResourceManager {
	public:
		using key_type = IndexT;
		using value_type = ResourceT;

		using internal_storage_type = InternalStorageT<IndexT, ResourceT>;

		internal_storage_type data;

		ResourceManager() = default;

		ResourceManager(const ResourceManager&) = delete;
		ResourceManager(ResourceManager&& other) noexcept :
			data(std::exchange(other.data, {})) {}

		ResourceManager& operator=(const ResourceManager&) = delete;
		ResourceManager& operator=(ResourceManager&& other) noexcept {
			std::swap(data, other.data);

			return *this;
		}

	public:
		bool contains(const key_type& key) const {
			return data.contains(key);
		}

		decltype(auto) size() const {
			return data.size();
		}

		decltype(auto) begin() const {
			return data.begin();
		}

		decltype(auto) begin() {
			return data.begin();
		}

		decltype(auto) end() const {
			return data.begin();
		}

		decltype(auto) end() {
			return data.begin();
		}

		decltype(auto) find(const key_type& key) const {
			return data.find(key);
		}

		decltype(auto) find(const key_type& key) {
			return data.find(key);
		}

		decltype(auto) at(const key_type& key) const {
			return data.at(key);
		}

		decltype(auto) at(const key_type& key) {
			return data.at(key);
		}

		void clear() {
			if constexpr (requires(ResourceT obj) { sndx::destroy<ResourceT>(obj); }) {
				for (auto& [id, obj] : data) {
					destroy(obj);
				}
			}

			data.clear();
		}

		~ResourceManager() {
			clear();
		}
	};

	template <class ResourceT, template<class> class InternalStorageT = std::vector>
	class LinearResourceManager {
	public:
		using key_type = size_t;
		using value_type = ResourceT;

		using internal_storage_type = InternalStorageT<ResourceT>;

		internal_storage_type data;

		LinearResourceManager() = default;

		LinearResourceManager(const LinearResourceManager&) = delete;
		LinearResourceManager(LinearResourceManager&& other) noexcept :
			data(std::exchange(other.data, {})) {}

		LinearResourceManager& operator=(const LinearResourceManager&) = delete;
		LinearResourceManager& operator=(LinearResourceManager&& other) noexcept {
			std::swap(data, other.data);

			return *this;
		}

	public:
		bool contains(const key_type& key) const {
			return key < data.size() && key >= 0;
		}

		decltype(auto) size() const {
			return data.size();
		}

		decltype(auto) begin() const {
			return data.begin();
		}

		decltype(auto) begin() {
			return data.begin();
		}

		decltype(auto) end() const {
			return data.begin();
		}

		decltype(auto) end() {
			return data.begin();
		}

		decltype(auto) find(const key_type& key) const {
			if (contains(key)) {
				return data.begin() + key;
			}

			return data.end();
		}

		decltype(auto) find(const key_type& key) {
			if (contains(key)) {
				return data.begin() + key;
			}

			return data.end();
		}

		decltype(auto) at(const key_type& key) const {
			return data.at(key);
		}

		decltype(auto) at(const key_type& key) {
			return data.at(key);
		}

		void clear() {
			if constexpr (requires(ResourceT obj) { sndx::destroy<ResourceT>(obj); }) {
				for (auto& obj : data) {
					destroy(obj);
				}
			}

			data.clear();
		}

		~LinearResourceManager() {
			clear();
		}
	};
}