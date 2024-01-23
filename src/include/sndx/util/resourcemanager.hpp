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
	class ResourceManager : public InternalStorageT<IndexT, ResourceT> {
	public:

		using internal_storage_type = InternalStorageT<IndexT, ResourceT>;

		ResourceManager() = default;

		ResourceManager(const ResourceManager&) = delete;

		ResourceManager& operator=(const ResourceManager&) = delete;

		void clear() {
			if constexpr (requires(ResourceT obj) { sndx::destroy<ResourceT>(obj); }) {
				for (auto& [id, obj] : *this) {
					destroy(obj);
				}
			}

			internal_storage_type::clear();
		}

		~ResourceManager() {
			clear();
		}
	};

	template <class ResourceT, template<class> class InternalStorageT = std::vector>
	class LinearResourceManager : public InternalStorageT<ResourceT> {
	public:
		using internal_storage_type = InternalStorageT<ResourceT>;

		LinearResourceManager() = default;

		LinearResourceManager(const LinearResourceManager&) = delete;

		LinearResourceManager& operator=(const LinearResourceManager&) = delete;

		bool contains(internal_storage_type::size_type index) const {
			return index >= 0 && index < this->size();
		}

		void clear() {
			if constexpr (requires(ResourceT obj) { sndx::destroy<ResourceT>(obj); }) {
				for (auto& [id, obj] : *this) {
					destroy(obj);
				}
			}

			internal_storage_type::clear();
		}

		~LinearResourceManager() {
			clear();
		}
	};
}