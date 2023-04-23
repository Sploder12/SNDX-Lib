#pragma once

#include <vector>
#include <span>
#include <complex>
#include <numbers>

namespace sndx {


	// see https://en.wikipedia.org/wiki/Cooley%E2%80%93Tukey_FFT_algorithm
	template <class T> [[nodiscard]]
	std::vector<std::complex<T>> fft(std::span<std::complex<T>> data, int effectiveSize = -1, int offset = 0, int stride = 1) {
		if (effectiveSize == 1) return { data[offset] };
		if (effectiveSize == 0) return {};

		if (effectiveSize == -1) effectiveSize = int(data.size());
		
		std::vector<std::complex<T>> out{};
		out.resize(effectiveSize);

		
		auto even = fft<T>(data, effectiveSize / 2, offset, stride * 2);
		auto odd = fft<T>(data, effectiveSize / 2, offset + stride, stride * 2);

		for (int i = 0; i < effectiveSize / 2; ++i) {
			using namespace std::complex_literals;

			auto twiddle = std::exp(T(-2.0) * T(i) * std::numbers::pi_v<T> * std::complex<T>(1i) / T(effectiveSize));
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
			val = std::conj(val) / T(out.size());
		}

		return out;
	}
}