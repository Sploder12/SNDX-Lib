#pragma once

#include <cstdint>
#include <variant>
#include <string>
#include <stdexcept>
#include <memory>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <concepts>
#include <initializer_list>

#define SNDX_CREATE_GET(type, cnst, cnstxpr) \
	[[nodiscard]] cnstxpr cnst type##T* get##type##() cnst { \
		return get<type##T>(); \
	}

#define SNDX_CREATE_GET_OR(type, cnstxpr) \
	[[nodiscard]] cnstxpr type##T get##type##Or(const type##T& _alt) const { \
		if (auto _val = get##type##()) { \
			return *_val; } \
		return _alt; \
	}

#define SNDX_CREATE_GETS(type, cnstxpr) \
	SNDX_CREATE_GET(type, , cnstxpr) \
	SNDX_CREATE_GET(type, const, cnstxpr) \
	SNDX_CREATE_GET_OR(type, cnstxpr) 

namespace sndx::data {
	template<class... Ts>
	struct overloads : Ts... { using Ts::operator()...; };

	template <class T>
	concept numeric = std::integral<T> || std::floating_point<T>;
	
	class Number {
	public:
		using IntT = intmax_t;
		using FloatT = long double;

		template <class T>
		using is_internal_t = std::bool_constant<
			std::is_same_v<T, IntT> || 
			std::is_same_v<T, FloatT>>;

		template <class T>
		static constexpr bool is_internal_t_v = is_internal_t<T>();

		friend class Value;
	private:
		std::variant<IntT, FloatT> m_value;

	public:
		constexpr Number(const Number&) noexcept = default;
		constexpr Number(Number&&) noexcept = default;
		constexpr Number& operator=(const Number&) noexcept = default;
		constexpr Number& operator=(Number&&) noexcept = default;
		constexpr ~Number() noexcept = default;

		template <numeric T>
		constexpr Number(T&& value) noexcept :
			m_value(std::forward<T>(value)) {}

		template <numeric T>
		constexpr Number& operator=(T&& value) noexcept {
			m_value = std::forward<T>(value);
			return *this;
		}

		template <numeric T>
		constexpr bool operator==(const T& val) const noexcept {
			return std::visit([val](auto v) {return v == val; }, m_value);
		}

		constexpr bool operator==(const Number& other) const noexcept {
			return std::visit([this](auto v) {return *this == v; }, other.m_value);
		}

		template <numeric T>
		constexpr bool operator<(const T& val) const noexcept {
			return std::visit([val](auto v) {return v < val; }, m_value);
		}

		constexpr bool operator<(const Number& other) const noexcept {
			return std::visit([this](auto v) {return *this < v; }, other.m_value);
		}

		[[nodiscard]]
		constexpr bool isFloat() const noexcept {
			return std::holds_alternative<FloatT>(m_value);
		}

		[[nodiscard]]
		constexpr bool isInt() const noexcept {
			return !isFloat();
		}

		template <class T>
			requires is_internal_t_v<T> [[nodiscard]]
		constexpr T* get() noexcept {
			return std::get_if<T>(&m_value);
		}

		template <class T>
			requires is_internal_t_v<T> [[nodiscard]]
		constexpr const T* get() const noexcept {
			return std::get_if<T>(&m_value);
		}

		template <class T>
		constexpr T getOr(T alt) const noexcept {
			if (auto v = get<T>())
				return *v;

			return alt;
		}

		SNDX_CREATE_GETS(Int, constexpr);
		SNDX_CREATE_GETS(Float, constexpr);

		template <class T> [[nodiscard]]
		constexpr T getAs() const noexcept {
			if (auto v = get<T>())
				return *v;

			if constexpr (std::is_same_v<T, IntT>) {
				return std::get<FloatT>(m_value);
			}
			else {
				return std::get<IntT>(m_value);
			}
		}

		[[nodiscard]]
		constexpr IntT getAsInt() const noexcept {
			return getAs<IntT>();
		}

		[[nodiscard]]
		constexpr const FloatT getAsFloat() const noexcept {
			return getAs<FloatT>();
		}
	};


	class Value {
	public:
		using NumberT = Number;
		using StringT = std::string;
		using BoolT = bool;

		template <class T>
		using is_internal_t = std::bool_constant<
			std::is_same_v<T, NumberT> || 
			std::is_same_v<T, StringT> || 
			std::is_same_v<T, BoolT>>;

		template <class T>
		static constexpr bool is_internal_t_v = is_internal_t<T>();

		using IntT = NumberT::IntT;
		using FloatT = NumberT::FloatT;

	private:
		std::variant<NumberT, StringT, BoolT> m_value;

	public:
		Value(const Value&) = default;
		Value(Value&&) noexcept = default;
		Value& operator=(const Value&) = default;
		Value& operator=(Value&&) noexcept = default;
		~Value() noexcept = default;

		template <numeric T>
		Value(T&& value) noexcept :
			m_value(std::forward<T>(value)) {}

		template <class T>
			requires std::constructible_from<StringT, T>
		Value(T&& value) noexcept :
			m_value(std::in_place_type_t<StringT>{}, std::forward<T>(value)) {}

		template <numeric T>
		Value& operator=(T&& value) noexcept {
			m_value = std::forward<T>(value);
			return *this;
		}

		template <class T>
			requires std::constructible_from<StringT, T>
		Value& operator=(T&& value) noexcept {
			m_value.emplace<StringT>(std::forward<T>(value));
			return *this;
		}

		template <class T>
			requires numeric<T> || std::same_as<T, NumberT>
		bool operator==(const T& i) const noexcept {
			return getAsNumber() == i;
		}

		template <std::equality_comparable_with<StringT> T>
		bool operator==(const T& str) const noexcept {
			return getAsString() == str;
		}

		bool operator==(const BoolT& b) const noexcept {
			return getAsBool() == b;
		}

		bool operator==(const Value& other) const noexcept {
			return std::visit([this](const auto& v) {
				return *this == v;
			}, other.m_value);
		}

		[[nodiscard]]
		bool isNumber() const noexcept {
			return std::holds_alternative<NumberT>(m_value);
		}

		[[nodiscard]]
		bool isString() const noexcept {
			return std::holds_alternative<StringT>(m_value);
		}

		[[nodiscard]]
		bool isBool() const noexcept {
			return std::holds_alternative<BoolT>(m_value);
		}

		template <class T>
			requires is_internal_t_v<T> [[nodiscard]]
		constexpr T* get() noexcept {
			return std::get_if<T>(&m_value);
		}

		template <class T>
			requires is_internal_t_v<T> [[nodiscard]]
		constexpr const T* get() const noexcept {
			return std::get_if<T>(&m_value);
		}

		template <class T>
			requires NumberT::is_internal_t_v<T> [[nodiscard]]
		constexpr T* get() noexcept {
			if (auto n = getNumber())
				return n->get<T>();

			return nullptr;
		}

		template <class T>
			requires NumberT::is_internal_t_v<T> [[nodiscard]]
		constexpr const T* get() const noexcept {
			if (auto n = getNumber())
				return n->get<T>();

			return nullptr;
		}

		template <class T> [[nodiscard]]
		T getOr(const T& alt) const noexcept {
			if (auto v = get<T>())
				return *v;

			return alt;
		}

		SNDX_CREATE_GETS(Number, constexpr);
		SNDX_CREATE_GETS(Float, constexpr);
		SNDX_CREATE_GETS(Int, constexpr);

		[[nodiscard]]
		NumberT getAsNumber() const {
			return std::visit(overloads{
				[](const NumberT& n) { return n; },
				[](const BoolT& b) -> NumberT { return NumberT::IntT(b); },
				[](const StringT& s) -> NumberT { return std::stold(s); } },
				m_value
			);
		}

		SNDX_CREATE_GETS(String, );

		[[nodiscard]]
		StringT getAsString() const noexcept {
			return std::visit(overloads{
				[](const StringT& s) { return s; },
				[](const BoolT& b) -> StringT { return b ? "true" : "false"; },
				[](const NumberT& n) -> StringT {
					return std::visit(
						[](const auto& v) { return std::to_string(v); },
						n.m_value);
				}},
				m_value
			);
		}

		SNDX_CREATE_GETS(Bool, constexpr);

		[[nodiscard]]
		BoolT getAsBool() const noexcept {
			return std::visit(overloads{
				[](const BoolT& b) { return b; },
				[](const NumberT& n) {
					return std::visit(
						[](const auto& v) { return static_cast<BoolT>(v); },
						n.m_value);
				},
				[](const StringT& s) {
					return
						s.size() == 4 &&
						std::tolower(s[0]) == 't' &&
						std::tolower(s[1]) == 'r' &&
						std::tolower(s[2]) == 'u' &&
						std::tolower(s[3]) == 'e';
				}},
				m_value
			);
		}
	};

	class Data;

	template <class DataT = Data>
	class DataArray {
	private:
		std::vector<DataT> m_data{};

	public:
		template <class... Ts>
		DataArray(Ts&&... args) :
			m_data{ DataT(std::forward<Ts>(args))... } {
		}

		DataArray(const DataArray& other) {
			m_data.reserve(other.size());
			for (const auto& data : other.m_data) {
				m_data.emplace_back(data);
			}
		}

		DataArray(DataArray&& other) noexcept :
			m_data(std::exchange(other.m_data, {})) {
		}

		DataArray& operator=(const DataArray& other) {
			m_data.clear();
			m_data.reserve(other.size());
			for (const auto& data : other.m_data) {
				m_data.emplace_back(data);
			}
			return *this;
		}

		DataArray& operator=(DataArray&& other) noexcept {
			std::swap(m_data, other.m_data);
			return *this;
		}

		~DataArray() noexcept = default;

		template <class T>
		bool operator==(const T& arr) const {
			if (size() != arr.size())
				return false;

			size_t i = 0;
			for (const auto& val : arr) {
				if (m_data[i++] != val)
					return false;
			}

			return true;
		}

		bool operator==(const DataT& d) const {
			if (auto arr = d.getArray())
				return *this == *arr;

			return false;
		}

		template <class... Args>
		void emplace_back(Args&&... args) {
			m_data.emplace_back(DataT(std::forward<Args>(args)...));
		}

		void reserve(size_t size) {
			m_data.reserve(size);
		}

		void clear() {
			m_data.clear();
		}

		[[nodiscard]]
		size_t size() const {
			return m_data.size();
		}

		[[nodiscard]]
		DataT& at(size_t pos) {
			return m_data.at(pos);
		}

		[[nodiscard]]
		const DataT& at(size_t pos) const {
			return m_data.at(pos);
		}

		[[nodiscard]]
		DataT& operator[](size_t pos) {
			return m_data[pos];
		}

		[[nodiscard]]
		const DataT& operator[](size_t pos) const {
			return m_data[pos];
		}

		auto begin() noexcept {
			return m_data.begin();
		}

		auto begin() const noexcept {
			return m_data.begin();
		}

		auto end() noexcept {
			return m_data.end();
		}

		auto end() const noexcept {
			return m_data.end();
		}
	};

	template <class DataT = Data, class IdT = std::string>
	class DataDict {
	private:
		std::unordered_map<IdT, DataT> m_data{};

	public:

		DataDict(std::initializer_list<std::pair<const IdT, DataT>> list) :
			m_data{ list } {
		}

		DataDict(const DataDict& other) {
			m_data.reserve(other.size());
			for (const auto& data : other.m_data) {
				m_data.emplace(data);
			}
		}

		DataDict(DataDict&& other) noexcept :
			m_data(std::exchange(other.m_data, {})) {
		}

		DataDict& operator=(const DataDict& other) {
			m_data.clear();
			m_data.reserve(other.size());
			for (const auto& data : other.m_data) {
				m_data.emplace(data);
			}
			return *this;
		}

		DataDict& operator=(DataDict&& other) noexcept {
			std::swap(m_data, other.m_data);
			return *this;
		}

		~DataDict() noexcept = default;

		bool operator==(const auto& dict) const {
			if (size() != dict.size())
				return false;

			for (const auto& [id, val] : dict) {
				if (auto it = m_data.find(id); it != m_data.end()) {
					if (it->second != val)
						return false;
				}
				else {
					return false;
				}
			}

			return true;
		}

		bool operator==(const DataT& d) const {
			if (auto map = d.getMap())
				return *this == *map;

			return false;
		}

		template <class... Args>
		bool emplace(const IdT& id, Args&&... args) {
			return m_data.emplace(id, DataT{ std::forward<Args>(args)... }).second;
		}

		decltype(auto) erase(const IdT& id) {
			return m_data.erase(id);
		}

		void reserve(size_t size) {
			m_data.reserve(size);
		}

		void clear() {
			m_data.clear();
		}

		[[nodiscard]]
		size_t size() const {
			return m_data.size();
		}

		[[nodiscard]]
		DataT& at(const IdT& id) {
			return m_data.at(id);
		}

		[[nodiscard]]
		const DataT& at(const IdT& id) const {
			return m_data.at(id);
		}

		[[nodiscard]]
		decltype(auto) find(const IdT& id) {
			return m_data.find(id);
		}

		[[nodiscard]]
		decltype(auto) find(const IdT& id) const {
			return m_data.find(id);
		}

		decltype(auto) operator[](const IdT& id) {
			return m_data[id];
		}

		auto begin() noexcept {
			return m_data.begin();
		}

		auto begin() const noexcept {
			return m_data.begin();
		}

		auto end() noexcept {
			return m_data.end();
		}

		auto end() const noexcept {
			return m_data.end();
		}
	};

	class Data {
	public:
		using NullT = std::nullptr_t;
		using ValueT = Value;
		using MapT = DataDict<Data>;
		using ArrayT = DataArray<Data>;

		template <class T>
		using is_internal_t = std::bool_constant<
			std::is_same_v<T, NullT> ||
			std::is_same_v<T, ValueT> ||
			std::is_same_v<T, MapT> ||
			std::is_same_v<T, ArrayT>>;

		template <class T>
		static constexpr bool is_internal_t_v = is_internal_t<T>();

		using BoolT = ValueT::BoolT;
		using StringT = ValueT::StringT;
		using NumberT = ValueT::NumberT;

		using IntT = NumberT::IntT;
		using FloatT = NumberT::FloatT;

	private:
		std::variant<NullT, std::unique_ptr<MapT>, ArrayT, ValueT> m_data{ std::in_place_type_t<NullT>{}, nullptr };

	public:
		Data() noexcept = default;

		Data(NullT) noexcept :
			Data() {}

		template <class T>
			requires std::constructible_from<ValueT, T>
		Data(T&& value) noexcept :
			m_data(std::in_place_type_t<ValueT>{}, std::forward<T>(value)) {}

		explicit Data(const ArrayT& arr) :
			m_data{ std::in_place_type_t<ArrayT>{}, ArrayT(arr) } {}

		Data(const MapT& dict):
			m_data{ std::make_unique<MapT>(dict)} {}

		Data(const Data& d) {
			if (d.isArray()) {
				m_data.emplace<ArrayT>(*d.getArray());
			}
			else if (d.isMap()) {
				m_data.emplace<std::unique_ptr<MapT>>(std::make_unique<MapT>(*d.getMap()));
			}
			else if (d.isValue()) {
				m_data.emplace<ValueT>(*d.getValue());
			}
		}

		Data(Data&& other) noexcept:
			m_data(std::exchange(other.m_data, decltype(m_data){ std::in_place_type_t<NullT>{}, nullptr })) {}

		Data& operator=(const Data& d) {
			if (d.isArray()) {
				m_data.emplace<ArrayT>(*d.getArray());
			}
			else if (d.isMap()) {
				m_data.emplace<std::unique_ptr<MapT>>(std::make_unique<MapT>(*d.getMap()));
			}
			else if (d.isValue()) {
				m_data.emplace<ValueT>(*d.getValue());
			}
			else {
				m_data.emplace<NullT>(nullptr);
			}
			return *this;
		}

		Data& operator=(Data&& other) noexcept {
			std::swap(m_data, other.m_data);
			return *this;
		}

		~Data() noexcept = default;

		template <class T>
		Data& operator=(T&& value) noexcept {
			m_data = std::forward<T>(value);
			return *this;
		}

		Data& operator=(NullT) noexcept {
			*this = Data{};
			return *this;
		}

		Data& operator=(std::initializer_list<Data> arr_init) {
			m_data.emplace<ArrayT>(ArrayT(arr_init));
			return *this;
		}

		Data& operator=(std::initializer_list<std::pair<const std::string, Data>> dict_init) {
			m_data.emplace<std::unique_ptr<MapT>>(std::make_unique<MapT>(dict_init));
			return *this;
		}

		bool operator==(const Data& other) const noexcept {
			return std::visit([this](const auto& v) {
				return *this == v;
			}, other.m_data);
		}

		bool operator==(NullT) const noexcept {
			return isNull();
		}

		template <class T>
			requires std::convertible_to<T, ValueT>
		bool operator==(const T& val) const noexcept {
			if (auto v = getValue())
				return *v == val;

			return false;
		}

		template <class T>
			requires std::convertible_to<T, std::unique_ptr<MapT>>
		bool operator==(const T& val) const noexcept {
			return *this == *val;
		}

		explicit operator bool() const noexcept {
			if (isNull())
				return false;

			if (auto v = getValue()) {
				if (auto b = v->getBool())
					return *b;
			}

			return true;
		}

		[[nodiscard]]
		bool isNull() const noexcept {
			return std::holds_alternative<NullT>(m_data);
		}

		[[nodiscard]]
		bool isMap() const noexcept {
			return std::holds_alternative<std::unique_ptr<MapT>>(m_data);
		}

		[[nodiscard]]
		bool isArray() const noexcept {
			return std::holds_alternative<ArrayT>(m_data);
		}

		[[nodiscard]]
		bool isValue() const noexcept {
			return std::holds_alternative<ValueT>(m_data);
		}

		template <class T>
			requires std::same_as<T, ArrayT> || 
					 std::same_as<T, ValueT> ||
					 std::same_as<T, NullT> [[nodiscard]]
		constexpr T* get() noexcept {
			return std::get_if<T>(&m_data);
		}

		template <class T>
			requires std::same_as<T, ArrayT> ||
					 std::same_as<T, ValueT> ||
					 std::same_as<T, NullT> [[nodiscard]]
		constexpr const T* get() const noexcept {
			return std::get_if<T>(&m_data);
		}

		template <class T>
			requires std::same_as<T, MapT> [[nodiscard]]
		constexpr T* get() noexcept {
			if (auto m = std::get_if<std::unique_ptr<T>>(&m_data))
				return m->get();

			return nullptr;
		}

		template <class T>
			requires std::same_as<T, MapT> [[nodiscard]]
		constexpr const T* get() const noexcept {
			if (auto m = std::get_if<std::unique_ptr<T>>(&m_data))
				return m->get();

			return nullptr;
		}

		template <class T>
			requires ValueT::is_internal_t_v<T> ||
					 NumberT::is_internal_t_v<T> [[nodiscard]]
		constexpr T* get()noexcept {
			if (auto v = get<ValueT>())
				return v->get<T>();

			return nullptr;
		}

		template <class T>
			requires ValueT::is_internal_t_v<T> ||
					 NumberT::is_internal_t_v<T> [[nodiscard]]
		constexpr const T* get() const noexcept {
			if (auto v = get<ValueT>())
				return v->get<T>();

			return nullptr;
		}

		template <class T> [[nodiscard]]
		T getOr(const T& alt) const noexcept {
			if (auto v = get<T>())
				return *v;

			return alt;
		}

		SNDX_CREATE_GET(Map, , );
		SNDX_CREATE_GET(Map, const, );

		SNDX_CREATE_GET(Array, , );
		SNDX_CREATE_GET(Array, const, );

		SNDX_CREATE_GETS(Value, );

		SNDX_CREATE_GETS(String, );
		SNDX_CREATE_GETS(Bool, );
		SNDX_CREATE_GETS(Number, );
		
		SNDX_CREATE_GETS(Int, );
		SNDX_CREATE_GETS(Float, );
	};
}
