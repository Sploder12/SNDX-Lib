#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "./windows.h"
#ifndef _WIN32
#include <dlfcn.h>
#endif

namespace sndx::platform {
	class SharedLib {
	public:
	#ifdef _WIN32
		using UnderlyingT = HMODULE;
		using FlagT = DWORD;
		static constexpr FlagT defaultFlags = 0;
	#else
		using UnderlyingT = void*;
		using FlagT = int;
		static constexpr FlagT defaultFlags = RTLD_NOW;
	#endif
	private:
		UnderlyingT m_lib{};

	public:
		SharedLib(std::nullptr_t, FlagT) = delete;
		SharedLib(const char* filename, FlagT flags = defaultFlags) :
		#ifdef _WIN32
			m_lib{ LoadLibraryExA(filename, NULL, flags) } {}
		#else
			m_lib{ dlopen(filename, flags) } {}
		#endif

		// returns nullptr on error
		[[nodiscard]] void* load(std::nullptr_t) const = delete;
		[[nodiscard]] void* load(const char* symbol) const {
			if (!valid()) return nullptr;
		#ifdef _WIN32
			return (void*)GetProcAddress(m_lib, symbol);
		#else
			return dlsym(m_lib, symbol);
		#endif
		}

		[[nodiscard]] // WARNING: call ASAP if another function indicates error.
		static auto getLastError() {
			#ifdef _WIN32
				return GetLastError();
			#else
				return dlerror();
			#endif
		}

		[[nodiscard]]
		bool valid() const noexcept {
			return m_lib != nullptr;
		}

		[[nodiscard]]
		auto getUnderlying() const noexcept {
			return m_lib;
		}

		[[nodiscard]]
		operator UnderlyingT() noexcept {
			return getUnderlying();
		}

		void close() noexcept {
			if (m_lib) {
				#ifdef _WIN32
					FreeLibrary(m_lib);
				#else 
					dlclose(m_lib);
				#endif
				m_lib = nullptr;
			}
		}

		SharedLib() noexcept = default;

		SharedLib(const SharedLib&) = delete;
		SharedLib(SharedLib&& other) noexcept :
			m_lib(std::exchange(other.m_lib, nullptr)) {}

		SharedLib& operator=(const SharedLib&) = delete;
		SharedLib& operator=(SharedLib&& other) noexcept {
			std::swap(m_lib, other.m_lib);
			return *this;
		}

		~SharedLib() noexcept {
			close();
		}
	};

	class LibLoader {
	private:
		struct Data {
			const void** dest;
			const void* fallback;
		};

		std::unordered_map<std::string, Data> m_funcs{};
	public:

		enum class ErrorType : uint8_t {
			BadLibrary,
			BadFunction,
		};

		[[nodiscard]]
		bool contains(const std::string& id) const {
			return m_funcs.contains(id);
		}

		[[nodiscard]]
		size_t size() const {
			return m_funcs.size();
		}

		void reserve(size_t count) {
			m_funcs.reserve(count);
		}

		template <class Str, class T>
		void bind(Str&& id, T*& dest, T* fallback) {
			m_funcs[std::forward<Str>(id)] = Data{
				(const void**)(std::addressof(dest)),
				(const void*)(fallback) 
			};
		}

		template <class Str, class T>
		void bind(Str&& id, T*& dest, std::nullptr_t) {
			bind(std::forward<Str>(id), dest, (const T*)(nullptr));
		}

		bool remove(const std::string& id) {
			return m_funcs.erase(id) > 0;
		}

		void clear() {
			m_funcs.clear();
		}

		template <class T>
		size_t load(const SharedLib& lib, T errorCallback) {
			size_t fails = 0;

			for (auto& [id, data] : m_funcs) {
				if (lib.valid()) {
					auto val = lib.load(id.c_str());
					if (val) {
						*data.dest = val;
						continue;
					}
					else {
						errorCallback(id, ErrorType::BadFunction);
					}
				}
				
				*data.dest = data.fallback;
				++fails;
			}

			if (!lib.valid()) {
				errorCallback("", ErrorType::BadLibrary);
			}

			return fails;
		}

		size_t load(const SharedLib& lib) {
			return load(lib, [](std::string_view, ErrorType) {});
		}
	};
}