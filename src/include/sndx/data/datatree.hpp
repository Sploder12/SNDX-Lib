#pragma once

#include <variant>
#include <string>
#include <functional>
#include <unordered_set>
#include <vector>
#include <optional>
#include <string_view>
#include <concepts>

namespace sndx {

	template <class T>
	concept Arithmetic = std::integral<T> || std::floating_point<T>;

	struct Primitive {
		std::variant<
			bool,
			int64_t,
			uint64_t,
			long double,
			std::string
		> data;

		constexpr Primitive() noexcept :
			data(false) {}

		constexpr Primitive(const Primitive& other) :
			data(false) {
		
			std::visit([this](auto&& alt) {
				using T = std::decay_t<decltype(alt)>;
				this->data.emplace<T>(alt);
			}, other.data);
		}

		Primitive(Primitive&& other) noexcept :
			data(std::exchange(other.data, false)) {}

		constexpr Primitive(bool val) noexcept:
			data(val) {}

		template <Arithmetic T>
		constexpr Primitive(T val) noexcept :
			data(val) {}

		Primitive(const std::string& val) :
			data(val) {}

		Primitive(std::string&& val) noexcept:
			data(std::move(val)) {}


		template <Arithmetic T>
		constexpr Primitive& operator=(T val) {
			data.emplace(val);
			return *this;
		}

		Primitive& operator=(const Primitive& other) {
			data = other.data;
			return *this;
		}

		constexpr Primitive& operator=(Primitive&& other) noexcept {
			std::swap(data, other.data);
			return *this;
		}

		template <class T>
		constexpr bool holdsAlternative() noexcept {
			return std::holds_alternative<T>(data);
		}

		template <class T>
		constexpr decltype(auto) get_if() noexcept {
			if constexpr (std::is_floating_point_v<T>) {
				return std::get_if<long double>(&data);
			}
			else if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
				return std::get_if<bool>(&data);
			}
			else if constexpr (std::is_convertible_v<std::string, std::decay_t<T>>) {
				return std::get_if<std::string>(&data);
			}
			else if constexpr (std::is_integral_v<T>) {
				if constexpr (std::is_unsigned_v<T>) {
					return std::get_if<uint64_t>(&data);
				}
				else {
					return std::get_if<int64_t>(&data);
				}
			}
			else {
				static_assert(!std::is_same_v<T, T>, "sndx::Primitive get_if has invalid type");
			}
		}

		template <class T>
		constexpr decltype(auto) get_if() const noexcept {
			if constexpr (std::is_floating_point_v<T>) {
				return std::get_if<long double>(&data);
			}
			else if constexpr (std::is_same_v<T, bool>) {
				return std::get_if<bool>(&data);
			}
			else if constexpr (std::is_convertible_v<std::string, T>) {
				return std::get_if<std::string>(&data);
			}
			else if constexpr (std::is_integral_v<T>) {
				if constexpr (std::is_unsigned_v<T>) {
					return std::get_if<uint64_t>(&data);
				}
				else {
					return std::get_if<int64_t>(&data);
				}
			}
			else {
				static_assert(!std::is_same_v<T, T>, "sndx::Primitive get_if has invalid type");
			}
		}

		constexpr operator decltype(data)& (){
			return data;
		}

		constexpr operator const decltype(data)& () const {
			return data;
		}
	};

	struct Data;

	using DataArray = std::vector<Data>;
	
	// this is not portable and may only work on MSVC.
	using DataDict = std::unordered_map<std::string, Data>;

	struct Data {
		std::variant<Primitive, DataDict, DataArray> data;

		template <class T>
		Data(const T& val) :
			data(val) {}

		template <class T>
		Data(T&& val) noexcept:
			data(std::forward<T>(val)) {}

		Data(const std::string& key, const Data& val) :
			data(DataDict{}) {
			
			auto& dict = std::get<DataDict>(data);
			dict.emplace(key, val);
		}

		Data(const Data& other) :
			data() {

			std::visit([this](auto&& alt) {
				using T = std::decay_t<decltype(alt)>;
				this->data.emplace<T>(alt);
			}, other.data);
		}

		Data(Data&& other) noexcept :
			data(std::exchange(other.data, Primitive())) {}

		template <class T>
		constexpr bool holdsAlternative() const noexcept {
			return std::holds_alternative<T>(data);
		}

		constexpr operator decltype(data)& () noexcept {
			return data;
		}

		constexpr operator const decltype(data)& () const noexcept {
			return data;
		}

		template <class T> [[nodiscard]]
		T* get() noexcept {
			if (auto pptr = get<Primitive>(); pptr) {
				return pptr->get_if<T>();
			}

			return nullptr;
		}

		template <class T> [[nodiscard]]
		const T* get() const noexcept {
			if (auto pptr = get<Primitive>(); pptr) {
				return pptr->get_if<T>();
			}

			return nullptr;
		}

		template <> [[nodiscard]]
		Primitive* get() noexcept {
			return std::get_if<Primitive>(&data);
		}

		template <> [[nodiscard]]
		const Primitive* get() const noexcept {
			return std::get_if<Primitive>(&data);
		}

		template <> [[nodiscard]]
		sndx::DataDict* get() noexcept {
			return std::get_if<sndx::DataDict>(&data);
		}

		template <> [[nodiscard]]
		const sndx::DataDict* get() const noexcept {
			return std::get_if<sndx::DataDict>(&data);
		}
	
		template <> [[nodiscard]]
		sndx::DataArray* get() noexcept {
			return std::get_if<sndx::DataArray>(&data);
		}

		template <> [[nodiscard]]
		const sndx::DataArray* get() const noexcept {
			return std::get_if<sndx::DataArray>(&data);
		}

		[[nodiscard]]
		Data* get(size_t index) noexcept {
			if (auto arr = std::get_if<DataArray>(&data); arr) {
				if (index < arr->size()) {
					return &((*arr)[index]);
				}
			}

			return nullptr;
		}

		[[nodiscard]]
		const Data* get(size_t index) const noexcept {
			if (auto arr = std::get_if<DataArray>(&data); arr) {
				if (index < arr->size()) {
					return &((*arr)[index]);
				}
			}

			return nullptr;
		}

		template <class T> [[nodiscard]]
		T* get(size_t index) {
			if (auto elem = get(index); elem) {
				return elem->get<T>();
			}

			return nullptr;
		}

		template <class T> [[nodiscard]]
		const T* get(size_t index) const {
			if (auto elem = get(index); elem) {
				return elem->get<T>();
			}

			return nullptr;
		}

		[[nodiscard]]
		Data* get(const std::string& key) {
			if (auto dict = std::get_if<DataDict>(&data); dict) {
				if (auto it = dict->find(key); it != dict->end()) {
					return &it->second;
				}
			}

			return nullptr;
		}

		[[nodiscard]]
		const Data* get(const std::string& key) const {
			if (auto dict = std::get_if<DataDict>(&data); dict) {
				if (auto it = dict->find(key); it != dict->end()) {
					return &it->second;
				}
			}

			return nullptr;
		}

		template <class T> [[nodiscard]]
		T* get(const std::string& key) {
			if (auto elem = get(key); elem) {
				return elem->get<T>();
			}

			return nullptr;
		}

		template <class T> [[nodiscard]]
		const T* get(const std::string& key) const {
			if (auto elem = get(key); elem) {
				return elem->get<T>();
			}

			return nullptr;
		}

		template <class First, class... Args> [[nodiscard]]
		Data* get(const First& first, const Args&... args) 
			requires (
				(sizeof...(Args) > 0) &&
				((std::convertible_to<std::decay_t<Args>, std::string>
				|| (std::convertible_to<std::decay_t<Args>, size_t>)) && ...)) {

			if (auto cur = get(first); cur) {
				return cur->get(args...);
			}

			return nullptr;
		}

		template <class First, class... Args> [[nodiscard]]
		const Data* get(const First& first, const Args&... args) const
			requires (
				(sizeof...(Args) > 0) &&
				((std::convertible_to<std::decay_t<Args>, std::string>
				|| (std::convertible_to<std::decay_t<Args>, size_t>)) && ...)) {

			if (auto cur = get(first); cur) {
				return cur->get(args...);
			}

			return nullptr;
		}

		template <class T, class First, class... Args> [[nodiscard]]
		T* get(const First& first, const Args&... args)
			requires (
				(sizeof...(Args) > 0) &&
				((std::convertible_to<std::decay_t<Args>, std::string>
				|| (std::convertible_to<std::decay_t<Args>, size_t>)) && ...)) {

			if (auto elem = get(first, args...); elem) {
				return elem->get<T>();
			}

			return nullptr;
		}

		template <class T, class First, class... Args> [[nodiscard]]
		const T* get(const First& first, const Args&... args) const
			requires (
				(sizeof...(Args) > 0) &&
				((std::convertible_to<std::decay_t<Args>, std::string>
				|| (std::convertible_to<std::decay_t<Args>, size_t>)) && ...)) {

			if (auto elem = get(first, args...); elem) {
				return elem->get<T>();
			}

			return nullptr;
		}

		template <class T, class... Args>
		T get_or_else(const T& other, Args&&... args) const {
			auto dat = get<T>(std::forward<Args>(args)...);
			if (dat) {
				return *dat;
			}

			return other;
		}

		template <class T, class O, class... Args>
		T get_or_else(const O& other, Args&&... args) const {
			auto dat = get<T>(std::forward<Args>(args)...);
			if (dat) {
				return *dat;
			}

			return T(other);
		}

		[[nodiscard]]
		bool has(size_t index) const {
			return get(index) != nullptr;
		}

		[[nodiscard]]
		bool has(const std::string& key) const {
			return get(key) != nullptr;
		}

		[[nodiscard]]
		size_t size() const {
			if (holdsAlternative<DataDict>()) {
				return std::get<DataDict>(data).size();
			}
			else if (holdsAlternative<DataArray>()) {
				return std::get<DataArray>(data).size();
			}
			else {
				return 1;
			}
		}

		bool append(const Data& val) {
			if (holdsAlternative<DataArray>()) {
				auto& arr = std::get<DataArray>(data);

				arr.emplace_back(val);
				return true;
			}

			return false;
		}

		bool append(Data&& val) {
			if (holdsAlternative<DataArray>()) {
				auto& arr = std::get<DataArray>(data);

				arr.emplace_back(std::move(val));
				return true;
			}

			return false;
		}

		bool append(const std::string& key, const Data& val) {
			if (holdsAlternative<DataDict>()) {
				auto& dict = std::get<DataDict>(data);

				return dict.emplace(key, val).second;
			}

			return false;
		}

		bool append(const std::string& key, Data&& val) {
			if (holdsAlternative<DataDict>()) {
				auto& dict = std::get<DataDict>(data);

				return dict.emplace(key, std::move(val)).second;
			}

			return false;
		}
	};

	struct GenericEncodingScheme {
		char keyDelim, primDelim = ',';
		char beginDir = '{', endDir = '}';
		char beginArr = '[', endArr = ']';
		char spacer = '\n', depthSpacer = '\t';
	};
}
