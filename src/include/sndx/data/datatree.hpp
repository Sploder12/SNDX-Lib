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

		constexpr Primitive(bool val) noexcept:
			data(val) {}

		template <std::integral T>
		constexpr Primitive(T val) noexcept :
			data(val) {}

		template <std::floating_point T>
		constexpr Primitive(T val) noexcept :
			data(val) {}

		Primitive(const std::string& val) :
			data(val) {}

		Primitive(std::string&& val) noexcept:
			data(std::move(val)) {}

		Primitive(Primitive&& other) noexcept:
			data(std::exchange(other.data, false)) {}

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

		constexpr operator decltype(data)& (){
			return data;
		}

		constexpr operator const decltype(data)& () const {
			return data;
		}
	};

	struct Data;

	using DataArray = std::vector<Data>;
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

		[[nodiscard]]
		Primitive* get() {
			return std::get_if<Primitive>(&data);
		}

		[[nodiscard]]
		Data* get(size_t index) {
			auto arr = std::get_if<DataArray>(&data);

			if (arr) {
				return &arr->at(index);
			}

			return nullptr;
		}

		[[nodiscard]]
		Data* get(const std::string& key) {
			auto dict = std::get_if<DataDict>(&data);
			
			if (dict) {
				if (auto it = dict->find(key); it != dict->end()) {
					return &it->second;
				}
			}

			return nullptr;
		}

		[[nodiscard]]
		Data& operator[](size_t index) {
			auto& arr = std::get<DataArray>(data);
			return arr[index];
		}

		[[nodiscard]]
		bool has(size_t index) const {
			auto arr = std::get_if<DataArray>(&data);

			if (arr) {
				return index < arr->size();
			}

			return false;
		}

		[[nodiscard]]
		bool has(const std::string& key) const {
			auto dict = std::get_if<DataDict>(&data);

			if (dict) {
				return dict->find(key) != dict->cend();
			}

			return false;
		}

		[[nodiscard]]
		Data* find(const std::string& key) {
			if (holdsAlternative<DataDict>()) {
				auto& dict = std::get<DataDict>(data);

				if (auto it = dict.find(key); it != dict.end()) {
					return &it->second;
				}
			}
			
			return nullptr;
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

		template <class T>
		auto begin() {
			if constexpr (std::is_same_v<T, Primitive>) {
				return &std::get<Primitive>(data);
			}
			else if constexpr (std::is_same_v<T, DataDict>) {
				return std::get<DataDict>(data).begin();
			}
			if constexpr (std::is_same_v<T, DataArray>) {
				return std::get<DataArray>(data).begin();
			}
			else {
				return nullptr;
			}
		}

		template <class T>
		auto end() {
			if constexpr (std::is_same_v<T, Primitive>) {
				return &std::get<Primitive>(data) + 1;
			}
			else if constexpr (std::is_same_v<T, DataDict>) {
				return std::get<DataDict>(data).end();
			}
			if constexpr (std::is_same_v<T, DataArray>) {
				return std::get<DataArray>(data).end();
			}
			else {
				return nullptr;
			}
		}
	};

	struct EncodingScheme {
		char keyDelim, primDelim = ',';
		char beginDir = '{', endDir = '}';
		char beginArr = '[', endArr = ']';
		char spacer = '\n', depthSpacer = '\t';
	};
}
