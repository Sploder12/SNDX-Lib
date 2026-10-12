#pragma once

#include <cassert>
#include <cstdint>
#include <numeric>
#include <utility>
#include <variant>
#include <vector>

namespace sndx::container {
	struct EntityHandle {
		uint32_t id = 0, generation = 0;

		constexpr auto operator<=>(const EntityHandle&) const = default;

		[[nodiscard]]
		constexpr uint64_t as_uint64_t() const noexcept {
			uint64_t combined = (uint64_t(id) << 32u) | generation;
			return combined;
		}
	};

	template <class T>
	struct EntityVector {
		struct Entity {
			std::variant<uint32_t, T> value{};
			uint32_t generation = 1;
		};

		std::vector<Entity> entities{};
		uint32_t freeHead = std::numeric_limits<uint32_t>::max();

		template <class... Args> [[nodiscard]]
		EntityHandle emplace(Args&&... args) {
			Entity* target;
			uint32_t idx = static_cast<uint32_t>(entities.size());
			if (freeHead >= entities.size()) {
				target = std::addressof(entities.emplace_back());
			}
			else {
				assert(entities[freeHead].value.index() == 0);
				idx = std::exchange(freeHead, std::get<0>(entities[freeHead].value));
				target = std::addressof(entities[idx]);
			}

			target->value.template emplace<1>(std::forward<Args>(args)...);
			return EntityHandle{ idx, target->generation };
		}

		bool erase(EntityHandle handle) {
			if (!isAlive(handle)) return false;

			auto& e = entities[handle.id];
			++e.generation;
			if (e.generation == 0) [[unlikely]] {
				e.generation = 1; // hopefully no super old handles are in use...
			}
			e.value.template emplace<0>(std::exchange(freeHead, handle.id));
			return true;
		}

		[[nodiscard]]
		T* get(EntityHandle handle) {
			if (!isAlive(handle)) return nullptr;
			return std::addressof(std::get<1>(entities[handle.id].value));
		}

		[[nodiscard]]
		const T* get(EntityHandle handle) const {
			if (!isAlive(handle)) return nullptr;
			return std::addressof(std::get<1>(entities[handle.id].value));
		}

		template <class Fn>
		void foreach(const Fn& fn) {
			for (auto& e : entities) {
				if (e.value.index() == 1) {
					fn(std::get<1>(e.value));
				}
			}
		}

		template <class Fn>
		void foreach(const Fn& fn) const {
			for (const auto& e : entities) {
				if (e.value.index() == 1) {
					fn(std::get<1>(e.value));
				}
			}
		}

		template <class Fn>
		void foreachHandle(const Fn& fn) const {
			for (uint32_t i = 0; i < static_cast<uint32_t>(entities.size()); ++i) {
				const auto& e = entities[i];
				if (e.value.index() == 1) {
					fn(EntityHandle{ i, e.generation });
				}
			}
		}

		template <class Pred>
		void erase_if(const Pred& pred) {
			foreachHandle([&](const auto& handle) {
				if (pred(std::get<1>(entities[handle.id].value))) {
					erase(handle);
				}
			});
		}

		[[nodiscard]]
		bool isAlive(EntityHandle handle) const {
			if (handle.generation == 0 || handle.id >= entities.size()) return false;
			return entities[handle.id].generation == handle.generation;
		}
	};
}

namespace std {
	template <>
	struct hash<sndx::container::EntityHandle> {
		std::size_t operator()(const sndx::container::EntityHandle& e) const noexcept {
			return e.as_uint64_t();
		}
	};
}