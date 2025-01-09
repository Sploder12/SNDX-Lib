#pragma once

#include "./data_tree.hpp"

#include <string_view>
#include <array>
#include <bit>

namespace sndx::data {

	enum class TokenType : unsigned short {
		string, decimal, integer, boolean, null,

		map_key, 
		map_start, map_end, map_seperator,

		array_start, array_end, array_seperator,

		error,
	};


	struct TokenData {
		TokenType type{ TokenType::null };

		union {
			::std::nullptr_t null;
			Data::ValueT::FloatT decimal;
			Data::ValueT::IntT integer;
			Data::ValueT::BoolT boolean;
			::std::string_view string;
		};
	};

	struct Token {
		int depth;
		TokenData data;
	};
	
	namespace detail {
		inline void tokenize(const Data&, std::vector<Token>&, int);

		inline void tokenize(const Data::ValueT& data, std::vector<Token>& tokens, int depth) {
			TokenData t{};

			if (auto str = data.getString()) {
				t.type = TokenType::string;
				t.string = *str;
			}
			else if (auto f = data.getFloat()) {
				t.type = TokenType::decimal;
				t.decimal = *f;
			}
			else if (auto i = data.getInt()) {
				t.type = TokenType::integer;
				t.integer = *i;
			}
			else if (auto b = data.getBool()) {
				t.type = TokenType::boolean;
				t.boolean = *b;
			}
			
			tokens.emplace_back(depth, std::move(t));
		}

		inline void tokenize(const Data::NullT&, std::vector<Token>& tokens, int depth) {
			tokens.emplace_back(depth, TokenData{});
		}

		inline void tokenize(const Data::ArrayT& array, std::vector<Token>& tokens, int depth) {
			tokens.emplace_back(depth, TokenData{TokenType::array_start});

			for (size_t i = 0; i < array.size(); ++i) {

				tokenize(array[i], tokens, depth + 1);

				if (i < array.size() - 1) {
					tokens.emplace_back(depth + 1, TokenData{ TokenType::array_seperator });
				}
			}

			tokens.emplace_back(depth, TokenData{ TokenType::array_end });
		}

		inline void tokenize(const Data::MapT& map, std::vector<Token>& tokens, int depth) {
			tokens.emplace_back(depth, TokenData{ TokenType::map_start });

			size_t i = 0;
			for (const auto& [id, value] : map) {
				TokenData key{ TokenType::map_key };
				key.string = id;

				tokens.emplace_back(depth + 1, std::move(key));

				tokenize(value, tokens, depth + 1);

				if (i++ < map.size() - 1) {
					tokens.emplace_back(depth + 1, TokenData{ TokenType::map_seperator });
				}
			}

			tokens.emplace_back(depth, TokenData{ TokenType::map_end });
		}

		inline void tokenize(const Data& data, std::vector<Token>& tokens, int depth) {
			if (auto map = data.getMap()) {
				tokenize(*map, tokens, depth);
			}
			else if (auto arr = data.getArray()) {
				tokenize(*arr, tokens, depth);
			}
			else if (auto val = data.getValue()) {
				tokenize(*val, tokens, depth);
			}
			else {
				tokenize(nullptr, tokens, depth);
			}
		}

		[[nodiscard]]
		inline std::string quote(std::string_view in) {
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

		[[nodiscard]]
		inline std::string floatToString(Data::ValueT::FloatT f) {
			auto tmp = std::to_string(f);

			// remove trailing zeros
			auto end = tmp.find_last_not_of('0');
			if (end != std::string::npos) {
				tmp.erase(tmp[end] == '.' ? end + 2 : end + 1);
			}

			return tmp;
		}
	}

	class packedJSONencoder {
	private:
		static void encode(std::string& buf, const Token& token) {
			switch (token.data.type)
			{
			case TokenType::map_key:
				buf += detail::quote(token.data.string) + ':';
				break;
			case TokenType::string:
				buf += detail::quote(token.data.string);
				break;
			case TokenType::decimal:
				buf += detail::floatToString(token.data.decimal);
				break;
			case TokenType::integer:
				buf += std::to_string(token.data.integer);
				break;
			case TokenType::boolean:
				buf += token.data.boolean ? "true" : "false";
				break;
			case TokenType::null:
				buf += "null";
				break;
			case TokenType::array_start:
				buf += '[';
				break;
			case TokenType::array_end:
				buf += ']';
				break;
			case TokenType::map_start:
				buf += '{';
				break;
			case TokenType::map_end:
				buf += '}';
				break;
			case TokenType::array_seperator:
			case TokenType::map_seperator:
				buf += ',';
				break;
			default:
				break;
			}
		}

	public:
		[[nodiscard]]
		static std::string encode(const std::vector<Token>& tokens) {
			std::string out{};

			for (const auto& token : tokens) {
				encode(out, token);
			}

			return out;
		}
	};

	class prettyJSONencoder {
	private:
		static void addTabs(std::string& buf, int tabs) {
			for (size_t i = 0; i < tabs; ++i) {
				buf += '\t';
			}
		}

		static void encode(std::string& buf, const Token& token) {
			switch (token.data.type)
			{
			case TokenType::map_key:
				addTabs(buf, token.depth);
				buf += detail::quote(token.data.string) + ": ";
				break;
			case TokenType::string:
				buf += detail::quote(token.data.string);
				break;
			case TokenType::decimal:
				buf += detail::floatToString(token.data.decimal);
				break;
			case TokenType::integer:
				buf += std::to_string(token.data.integer);
				break;
			case TokenType::boolean:
				buf += token.data.boolean ? "true" : "false";
				break;
			case TokenType::null:
				buf += "null";
				break;
			case TokenType::array_start:
				buf += "[\n";
				addTabs(buf, token.depth + 1);
				break;
			case TokenType::array_end:
				buf += '\n';
				addTabs(buf, token.depth);
				buf += ']';
				break;
			case TokenType::map_start:
				buf += "{\n";
				break;
			case TokenType::map_end:
				buf += '\n';
				addTabs(buf, token.depth);
				buf += '}';
				break;
			case TokenType::array_seperator:
				buf += ",\n";
				addTabs(buf, token.depth);
				break;
			case TokenType::map_seperator:
				buf += ",\n";
				break;
			default:
				break;
			}
		}

	public:
		[[nodiscard]]
		static std::string encode(const std::vector<Token>& tokens) {
			std::string out{};

			for (const auto& token : tokens) {
				encode(out, token);
			}

			return out;
		}
	};

	[[nodiscard]]
	inline std::vector<Token> tokenize(const auto& data) {
		std::vector<Token> out{};
		detail::tokenize(data, out, 0);
		return out;
	}

	template <class Encoder = packedJSONencoder> [[nodiscard]]
	decltype(auto) encodeData(const auto& data, const Encoder& encoder = Encoder{}) {
		return encoder.encode(tokenize(data));
	}
}
