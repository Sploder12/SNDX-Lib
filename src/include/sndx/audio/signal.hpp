#pragma once

#include <vector>
#include <span>
#include <complex>
#include <numbers>

namespace sndx {

	template <class T> [[nodiscard]]
	std::vector<std::complex<T>> fft(std::span<std::complex<T>> data, int effectiveSize = -1, int offset = 0, int stride = 1) {
		if (effectiveSize == 1) return { data[offset] };
		if (effectiveSize == 0) return {};

		if (effectiveSize == -1) effectiveSize = data.size();
		
		std::vector<std::complex<T>> out{};
		out.resize(effectiveSize);

		
		auto even = fft<T>(data, effectiveSize / 2, offset, stride * 2);
		auto odd = fft<T>(data, effectiveSize / 2, offset + stride, stride * 2);

		for (int i = 0; i < effectiveSize / 2; ++i) {
			using namespace std::complex_literals;

			auto twiddle = std::exp(-2.0 * i * std::numbers::pi * 1i / double(effectiveSize));
			auto q = twiddle * odd[i];
			
			out[i] = even[i] + q;
			out[i + effectiveSize / 2] = even[i] - q;
		}

		return out;
	}

	template <class T> [[nodiscard]]
	std::vector<std::complex<T>> ifft(std::span<std::complex<T>> data) {
		std::vector<std::complex<T>> out{};
		out.reserve(data.size());

		for (const auto& val : data) {
			out.emplace_back(std::conj(val));
		}

		out = fft(std::span(out));

		for (auto& val : out) {
			val = std::conj(val) / double(out.size());

		}

		return out;
	}
}