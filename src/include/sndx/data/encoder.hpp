#pragma once

#include "datatree.hpp"

#include <iomanip>
#include <sstream>
#include <filesystem>
#include <fstream>

namespace sndx {

	template <class Enc>
	struct Encoder {
		
		static auto encode(const Primitive& prim, int depth = 0, bool spaceFirst = true) {
			if constexpr (requires(decltype(prim)) { Enc::encode(prim, 0, true); }) {
				return Enc::encode(prim, depth, spaceFirst);
			}
			else if constexpr (requires(decltype(prim)) { Enc::encode(prim, 0); }) {
				return Enc::encode(prim, depth);
			}
			else if constexpr (requires(decltype(prim)) { Enc::encode(prim); }) {
				return Enc::encode(prim);
			}
			else {
				return std::visit([](auto&& val) {
					using T = std::decay_t<decltype(val)>;

					if constexpr (std::is_same_v<T, std::string>) {
						std::stringstream ss;
						ss << std::quoted(val);
						return ss.str();
					}
					else if constexpr (std::is_same_v<T, bool>) {
						if (val) {
							return std::string("true");
						}

						return std::string("false");
					}
					else if constexpr (std::is_arithmetic_v<T>) {
						return std::to_string(val);
					}
					else {
						return std::string("");
					}
				}, prim.data);
			}
		}

		static auto encode(const DataDict& dict, int depth = 0, bool spaceFirst = true) {
			return Enc::encode(dict, depth, spaceFirst);
		}

		static auto encode(const DataArray& arr, int depth = 0, bool spaceFirst = true) {
			return Enc::encode(arr, depth, spaceFirst);
		}

		static auto encode(const Data& data, int depth = 0, bool spaceFirst = true) {
			return std::visit([depth, spaceFirst](auto&& val) {
				return encode(val, depth, spaceFirst);
			}, data.data);
		}
	};

	template <GenericEncodingScheme scheme>
	struct GenericEncoder : public Encoder<GenericEncoder<scheme>> {

		using Base = Encoder<GenericEncoder<scheme>>;

		static constexpr auto encoderScheme = scheme;

		static auto encode(const DataDict& dict, int depth = 0, bool spaceFirst = true) {
			std::string out = "";
			if (spaceFirst) {
				for (int i = 0; i < depth; ++i) {
					out += scheme.depthSpacer;
				}
			}

			out += scheme.beginDir;
			out += scheme.spacer;

			for (const auto& [key, val] : dict) {
				std::stringstream ss;
				ss << std::quoted(key);

				for (int i = 0; i < depth + 1; ++i) {
					out += scheme.depthSpacer;
				}

				out += ss.str();
				out += scheme.keyDelim;
				out += ' ';
				out += Encoder<GenericEncoder<scheme>>::encode(val, depth + 1, false);
				out += scheme.primDelim;
				out += scheme.spacer;
			}

			if (dict.size() > 0) {
				out.pop_back();
				out.pop_back();
				out += scheme.spacer;
			}

			for (int i = 0; i < depth; ++i) {
				out += scheme.depthSpacer;
			}
			out += scheme.endDir;

			return out;
		}

		static auto encode(const DataArray& arr, int depth = 0, bool spaceFirst = true) {
			std::string out = "";

			if (spaceFirst) {
				for (int i = 0; i < depth; ++i) {
					out += scheme.depthSpacer;
				}
			}

			out += scheme.beginArr;
			out += scheme.spacer;

			for (const auto& val : arr) {

				for (int i = 0; i < depth + 1; ++i) {
					out += scheme.depthSpacer;
				}

				out += Encoder<GenericEncoder<scheme>>::encode(val, depth + 1);
				out += scheme.primDelim;
				out += scheme.spacer;
			}

			if (arr.size() > 0) {
				out.pop_back();
				out.pop_back();
				out += scheme.spacer;
			}

			for (int i = 0; i < depth; ++i) {
				out += scheme.depthSpacer;
			}
			out += scheme.endArr;

			return out;
		}
	};

	using SNDXencoder = GenericEncoder<GenericEncodingScheme{'='}>;
	using JSONencoder = GenericEncoder<GenericEncodingScheme{':'}>;

	template <class Enc>
	bool encodeData(const std::filesystem::path& path, const Data& data) {
		std::ofstream ofile{ path };

		if (ofile.is_open()) {
			ofile << Enc::Base::encode(data);
			return true;
		}

		return false;
	}
}