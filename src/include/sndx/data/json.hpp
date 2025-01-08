#pragma once

#include "./data_tree.hpp"

namespace sndx::data {
	namespace detail {
		template <class Encoder>
		void toJSON(std::string&, const Data&, const Encoder&, int);

		template <class Encoder>
		void toJSON(std::string& out, const Data::NullT&, const Encoder& encoder, int depth) {
			encoder.encodeNull(out, depth);
		}

		template <class Encoder>
		void toJSON(std::string& out, const Data::ValueT& value , const Encoder& encoder, int depth) {
			encoder.encodeValue(out, value, depth);
		}

		template <class Encoder>
		void toJSON(std::string& out, const Data::ArrayT& array, const Encoder& encoder, int depth) {
			encoder.encodeArrayStart(out, depth);

			for (size_t i = 0; i < array.size(); ++i) {
				toJSON<Encoder>(out, array[i], encoder, depth + 1);

				if (i < array.size() - 1) {
					encoder.encodeArraySeperator(out, depth + 1);
				}
			}

			encoder.encodeArrayEnd(out, depth);
		}

		template <class Encoder>
		void toJSON(std::string& out, const Data::MapT& map, const Encoder& encoder, int depth) {
			encoder.encodeMapStart(out, depth);

			size_t i = 0;
			for (const auto& [id, value] : map) {

				encoder.encodeKey(out, id, depth + 1);

				toJSON<Encoder>(out, value, encoder, depth + 1);

				if (i++ < map.size() - 1) {
					encoder.encodeMapSeperator(out, depth + 1);
				}
			}

			encoder.encodeMapEnd(out, depth);
		}

		template <class Encoder>
		void toJSON(std::string& out, const Data& data, const Encoder& encoder, int depth) {
			if (auto map = data.getMap()) {
				toJSON<Encoder>(out, *map, encoder, depth);
			}
			else if (auto arr = data.getArray()) {
				toJSON<Encoder>(out, *arr, encoder, depth);
			}
			else if (auto val = data.getValue()) {
				toJSON<Encoder>(out, *val, encoder, depth);
			}
			else {
				toJSON<Encoder>(out, nullptr, encoder, depth);
			}
		}

		inline std::string quote(const std::string& in) {
			std::string out = "\"";
			out.reserve(in.size() + 2);

			bool escaped = false;
			for (auto c : in) {
				switch (c) {
				case '"':
					out += "\\\"";
					break;
				case '\\':
					out += "\\\\";
					break;
				case '\n':
					out += "\\n";
					break;
				case '\b':
					out += "\\b";
					break;
				case '\f':
					out += "\\f";
					break;
				case '\r':
					out += "\\r";
					break;
				case '\t':
					out += "\\t";
					break;
				default:
					out += c;
					break;
				}
			}

			out += '"';
			return out;
		}
	}

	struct packedJSONencoder {
		void encodeArrayStart(std::string& out, int) const {
			out += '[';
		}

		void encodeArraySeperator(std::string& out, int) const {
			out += ',';
		}

		void encodeArrayEnd(std::string& out, int) const {
			out += ']';
		}

		void encodeMapStart(std::string& out, int) const {
			out += '{';
		}

		void encodeMapSeperator(std::string& out, int) const {
			out += ',';
		}

		void encodeMapEnd(std::string& out, int) const {
			out += '}';
		}

		void encodeKey(std::string& out, const std::string& key, int) const {
			out += detail::quote(key) + ':';
		}

		void encodeValue(std::string& out, const Data::ValueT& value, int) const {
			if (auto str = value.getString()) {
				out += detail::quote(*str);
			}
			else {
				out += value.getAsString();
			}
		}

		void encodeNull(std::string& out, int) const {
			out += "null";
		}
	};

	template <class Encoder = packedJSONencoder>
	std::string toJSON(const Data::MapT& data, const Encoder& encoder = packedJSONencoder{}) {
		std::string out{};
		detail::toJSON(out, data, encoder, 0);
		return out;
	}
}