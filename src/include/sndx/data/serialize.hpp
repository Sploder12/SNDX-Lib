#pragma once

#include <iostream>
#include <type_traits>
#include <concepts>
#include <stdexcept>

#include "../utility/endian.hpp"

namespace sndx {

	struct io_error : public std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	struct serialize_error : public io_error {
		using io_error::io_error;
	};

	struct deserialize_error : public io_error {
		using io_error::io_error;
	};

	struct identifier_error : public deserialize_error {
		using deserialize_error::deserialize_error;
	};

	namespace serialize {
		struct Serializer {
			std::ostream m_sink;

			Serializer(std::ostream& sink) :
				m_sink(sink.rdbuf()) {}

			template <class T>
			Serializer& serialize(const T& obj) {
				obj.serialize(*this);
				return *this;
			}

			template <class T>
			Serializer& serialize(const T& val)
				requires std::is_arithmetic_v<T> {

				m_sink.write(reinterpret_cast<const char*>(std::addressof(val)), sizeof(T));

				if (m_sink.bad())
					throw serialize_error("Serializer sink went bad!");

				return *this;
			}

			template <std::endian endianess = std::endian::native>
			Serializer& serialize(const std::integral auto* arr, size_t size) {
				for (size_t i = 0; i < size; ++i) {
					serialize<endianess>(arr[i]);
				}

				return *this;
			}

			template <std::endian endianess>
			decltype(auto) serialize(const std::integral auto& val) {
				return serialize(sndx::utility::fromEndianess<endianess>(val));
			}
		};

		struct Deserializer {
			std::istream m_source;

			Deserializer(std::istream& source) :
				m_source(source.rdbuf()) {}

			template <class T>
			decltype(auto) deserialize(T& obj) {
				return obj.deserialize(*this);
			}

			template <class T>
			auto deserialize(T& obj)
				requires std::is_arithmetic_v<T> {

				m_source.read(reinterpret_cast<char*>(std::addressof(obj)), sizeof(T));
				if (m_source.gcount() != sizeof(T))
					throw deserialize_error("Deserialize failed on arithmetic type!");
			}

			template <std::endian endianess = std::endian::native>
			auto deserialize(std::integral auto* arr, size_t size) {
				for (size_t i = 0; i < size; ++i) {
					deserialize<endianess>(arr[i]);
				}
			}

			template <std::endian endianess>
			auto deserialize(std::integral auto& obj) {
				deserialize(obj);
				obj = sndx::utility::fromEndianess<endianess>(obj);
			}


			bool discard(size_t bytes, bool seekable) {
				m_source.clear();

				if (seekable) {
					m_source.seekg(bytes, std::ios::cur);
					if (!m_source.fail())
						return true;
					else
						m_source.clear();
				}

				for (size_t i = 0; i < bytes; ++i) {
					char c;
					m_source.read(&c, sizeof(c));
				}

				return false;
			}
		};
	}
}