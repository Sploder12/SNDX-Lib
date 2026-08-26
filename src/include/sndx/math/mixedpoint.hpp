#pragma once

#include <cassert>
#include <bit>
#include <cmath>
#include <compare>
#include <concepts>
#include <cstdint>
#include <limits>
#include <numbers>

#include "./math.hpp"

namespace sndx::math {
	
	// Vaguely similar to base 10 IEEE-754 but it is not.
	// Everything is stored in "canonical" form, value * 10^17 * 10^exponent
	// Except 0, 0 has both value and exponent as 0.
	// This means each number has exactly 1 valid representation
	// (this does waste a fair amount of bits)
	struct MixedPoint {
	private:
		using ValueT = int64_t;

		ValueT value = 0;
		int64_t exponent = 0;

		static constexpr ValueT digits = std::numeric_limits<ValueT>::digits10;
		static constexpr ValueT highThreshold = fPow10(digits);
		static constexpr ValueT lowThreshold = highThreshold / 10;

		static constexpr ValueT multiplyDigits = digits / 2;
		static constexpr ValueT multiplyTruncation = fPow10(multiplyDigits);
		static_assert((highThreshold / multiplyTruncation) * (highThreshold / multiplyTruncation) <= std::numeric_limits<ValueT>::max());

		constexpr void correctExponent() noexcept {
			if (value == 0) {
				exponent = 0;
				return;
			}

			ValueT tmpVal = value < 0 ? -value : value;
			if (tmpVal >= highThreshold) {
				value /= 10;
				++exponent;
				assert(tmpVal / 10 < highThreshold);
			}
			else if (tmpVal < lowThreshold) {
				auto delta = digits - 1 - fLog10(tmpVal);
				value *= fPow10(delta);
				exponent -= delta;
				assert(tmpVal * fPow10(delta) >= lowThreshold);
			}
		}

		[[nodiscard]] // does nothing if exponent is less than the current exponent
		constexpr MixedPoint increaseExponent(int64_t exponent) const noexcept {
			MixedPoint out = *this;

			if (exponent > this->exponent) {
				out.exponent = exponent;
				auto delta = exponent - this->exponent;
				if (delta > digits) {
					out.value = 0;
				}
				else {
					out.value /= fPow10(delta);
				}
			}

			return out;
		}

	public:
		[[nodiscard]]
		ValueT getValue() const noexcept {
			return value;
		}

		[[nodiscard]]
		int64_t getExponent() const noexcept {
			return exponent;
		}

		constexpr MixedPoint() noexcept = default;

		template <std::signed_integral T>
		constexpr MixedPoint(T value) noexcept:
			value(std::forward<T>(value)) {

			static_assert(sizeof(T) <= sizeof(ValueT));
			correctExponent();
		}

		template <std::unsigned_integral T>
		constexpr MixedPoint(T value) noexcept:
			value(value) {

			static_assert(sizeof(T) <= sizeof(ValueT));
			if constexpr (sizeof(T) == sizeof(ValueT)) {
				if (value > static_cast<T>(std::numeric_limits<ValueT>::max())) {
					this->value = value / 10;
					this->exponent = 1;
				}
			}

			correctExponent();
		}

		template <std::floating_point T>
		MixedPoint(T val):
			value(0), exponent(0) {
			if (val == T(0.0)) {
				return;
			}
			
			auto positive = std::abs(val);
			auto l10 = std::log10(positive);
			exponent = l10 - digits + 1;

			positive /= std::pow(10, exponent);
			value = std::trunc(positive);
			if (val < T(0.0)) {
				value = -value;
			}

			correctExponent();
		}

		constexpr MixedPoint(ValueT value, int64_t exponent) noexcept:
			value(value), exponent(exponent) {
			correctExponent();
		}

		constexpr MixedPoint& operator++() noexcept {
			return *this += MixedPoint{ 1 };
		}

		constexpr MixedPoint operator++(int) noexcept {
			auto temp = *this;
			++(*this);
			return temp;
		}

		constexpr MixedPoint& operator--() noexcept {
			return *this += MixedPoint{ -1 };
		}

		constexpr MixedPoint& operator--(int) noexcept {
			auto temp = *this;
			--(*this);
			return temp;
		}

		[[nodiscard]]
		friend constexpr std::strong_ordering operator<=>(const MixedPoint& a, const MixedPoint& b) noexcept {
			if (a.value == 0) {
				return 0 <=> b.value;
			}
			if (b.value == 0) {
				return a.value <=> 0;
			}

			if (a.exponent < b.exponent) {
				return std::strong_ordering::less;
			}
			else if (a.exponent > b.exponent) {
				return std::strong_ordering::greater;
			}
			return a.value <=> b.value;
		}

		[[nodiscard]]
		friend constexpr bool operator==(const MixedPoint& a, const MixedPoint& b) noexcept = default;

		[[nodiscard]]
		MixedPoint operator-() const noexcept {
			return MixedPoint{ -value, exponent };
		}

		constexpr MixedPoint& operator+=(const MixedPoint& other) noexcept {
			if (other.value == 0) {
				return *this;
			}
			else if (value == 0) {
				*this = other;
				return *this;
			}

			if (other.exponent < exponent) {
				value += other.increaseExponent(exponent).value;
			}
			else {
				value = other.value + increaseExponent(other.exponent).value;
				exponent = other.exponent;
			}

			correctExponent();
			return *this;
		}

		constexpr MixedPoint& operator-=(const MixedPoint& other) noexcept {
			return *this += -other;
		}

		constexpr MixedPoint& operator*=(const MixedPoint& other) noexcept {
			if (value == 0 || other.value == 0) {
				value = 0;
				exponent = 0;
				return *this;
			}

			// the math behind this relies on the idea that value = high * base + low
			// then you multiply those together to get sane additions
			auto a = value / multiplyTruncation;
			auto ra = value % multiplyTruncation;

			auto b = other.value / multiplyTruncation;
			auto rb = other.value % multiplyTruncation;

			auto baseExponent = exponent + other.exponent;

			// purposely adding least to greatest for a sliver of extra precision
			value = ra * rb;
			exponent = baseExponent;
			correctExponent();

			*this += MixedPoint{ a * rb + b * ra, baseExponent + multiplyDigits };
			*this += MixedPoint{ a * b , baseExponent + multiplyDigits + multiplyDigits };

			return *this;
		}

		// evil long division :(
		constexpr MixedPoint& operator/=(const MixedPoint& other) noexcept {
			if (value == 0 || other.value == 0) {
				value = 0;
				exponent = 0;
				return *this;
			}

			exponent -= other.exponent;

			ValueT result = value / other.value;
			auto remain = value % other.value;
			auto against = other.value;

			for (int i = 0; i < digits - 1; ++i) {
				if (remain == 0) {
					break;
				}
				result *= 10;
				against /= 10;
				--exponent;
				result += remain / against;
				remain %= against;
			}
			value = result;

			correctExponent();
			return *this;
		}

		constexpr MixedPoint& operator%=(const MixedPoint& other) noexcept {
			// = a - b * floor(a / b)
			// if b > a then a / b == 0 so = a

			auto order = other <=> *this;
			if (order == 0) {
				value = 0;
				exponent = 0;
				return *this;
			}
			if ((order > 0) ^ (this->sign() != other.sign())) {
				return *this;
			}

			*this -= other * (*this / other).floor();
			return *this;
		}

		[[nodiscard]]
		friend constexpr MixedPoint operator+(MixedPoint a, const MixedPoint& b) noexcept {
			a += b;
			return a;
		}

		[[nodiscard]]
		friend constexpr MixedPoint operator-(MixedPoint a, const MixedPoint& b) noexcept {
			a -= b;
			return a;
		}

		[[nodiscard]]
		friend constexpr MixedPoint operator*(MixedPoint a, const MixedPoint& b) noexcept {
			a *= b;
			return a;
		}

		[[nodiscard]]
		friend constexpr MixedPoint operator/(MixedPoint a, const MixedPoint& b) noexcept {
			a /= b;
			return a;
		}

		[[nodiscard]]
		friend constexpr MixedPoint operator%(MixedPoint a, const MixedPoint& b) noexcept {
			a %= b;
			return a;
		}

		[[nodiscard]]
		constexpr MixedPoint floor() const noexcept {
			if (exponent >= 0) {
				return *this;
			}
			if (exponent <= -digits) {
				return { value < 0 ? -1 : 0 };
			}

			auto truncator = fPow10(-exponent);
			auto adjusted = (value / truncator) * truncator;
			if (adjusted < 0 && adjusted != value) {
				adjusted -= truncator;
			}

			return { adjusted, exponent };
		}

		[[nodiscard]]
		constexpr MixedPoint ceil() const noexcept {
			if (exponent >= 0) {
				return *this;
			}
			if (exponent <= -digits) {
				return { value < 0 ? 0 : 1 };
			}

			auto truncator = fPow10(-exponent);
			auto adjusted = (value / truncator) * truncator;
			if (adjusted >= 0 && adjusted != value) {
				adjusted += truncator;
			}

			return { adjusted, exponent };
		}

		[[nodiscard]]
		constexpr MixedPoint abs() const noexcept {
			if (value >= 0) {
				return *this;
			}
			return MixedPoint{ -value, exponent };
		}

		[[nodiscard]] // 0 is considered positive
		constexpr int sign() const noexcept {
			return value >= 0 ? 1 : -1;
		}

		[[nodiscard]] // negative numbers work as if they were positive (0 always returns 0)
		constexpr auto ilog10() const noexcept {
			if (value == 0) return exponent;
			return exponent + digits - 1;
		}

		[[nodiscard]] // negative numbers work as if they were positive (0 always returns 0)
		constexpr MixedPoint log10() const noexcept {
			if (value == 0) return 0;

			// relies heavily on the log rule:
			// log(x + 10^y) = log(x) + y
	
			// range reduction table, 10^n/10 (in log terms [0, 1])
			constexpr std::array<MixedPoint, 11> rangeTable{
				MixedPoint{1},
				MixedPoint{125892541179416721, -17},
				MixedPoint{158489319246111349, -17},
				MixedPoint{199526231496887960, -17},
				MixedPoint{251188643150958011, -17},
				MixedPoint{316227766016837933, -17},
				MixedPoint{398107170553497251, -17},
				MixedPoint{501187233627272285, -17},
				MixedPoint{630957344480193249, -17},
				MixedPoint{794328234724281502, -17},
				MixedPoint{10},
			};

			constexpr auto invRangeTable = [&rangeTable]() {
				std::decay_t<decltype(rangeTable)> out{};
				for (size_t i = 0; i < rangeTable.size(); ++i) {
					out[i] = MixedPoint{ 1 } / rangeTable[i];
				}
				return out;
			}();

			auto exp = exponent + digits - 1;
			auto x = MixedPoint{ value >= 0 ? value : -value, 1 - digits };

			// Remez algorithm [1, 10^1/10] to approximate mantissa
			// generated with Maple using `minimax(log10(x), x = 1 .. 10^1/10, 10, 1, 'maxerror')` (18 digits)
			// theoretical max error is around 1.821e-15, may be slightly larger in practice
			constexpr std::array<MixedPoint, 11> coefficients{
				MixedPoint{-122215493561194887, -17},
				MixedPoint{387044076498592176, -17},
				MixedPoint{-775587727877832188, -17},
				MixedPoint{122718016631259263, -16},
				MixedPoint{-143258249015752698, -16},
				MixedPoint{122240506361746277, -16},
				MixedPoint{-754031424704987048, -17},
				MixedPoint{327834199022210113, -17},
				MixedPoint{-954237461086014350, -18},
				MixedPoint{167096745973453202, -18},
				MixedPoint{-133229763806028993, -19},
			};

			uint32_t r = 0;
			for (; r + 1 < rangeTable.size(); ++r) {
				if (x < rangeTable[r + 1]) break;
			}
			x *= invRangeTable[r];

			auto result = coefficients.back();
			for (size_t i = 1; i < coefficients.size(); ++i) {
				const auto& c = coefficients[coefficients.size() - 1 - i];
				result = c + result * x;
			}
			result += exp;

			auto remapFactor = MixedPoint{ r };
			remapFactor.exponent -= 1;
			result += remapFactor;
			return result;
		}
		
		[[nodiscard]]
		constexpr MixedPoint log2() const noexcept {
			// 1 / log(2)
			constexpr MixedPoint l2{ 3321928094887362, -15 };
			return log10() * l2;
		}

		[[nodiscard]]
		constexpr MixedPoint ln() const noexcept {
			// 1 / log(e)
			constexpr MixedPoint le{ 2302585092994046, -15 };
			return log10() * le;
		}

		template <std::integral V> [[nodiscard]]
		constexpr MixedPoint pow(V exp) const noexcept {
			if constexpr (std::is_signed_v<V>) {
				if (exp < 0) {
					return (MixedPoint{ 1 } / *this).pow(-exp);
				}
			}
			if (exp == 0) {
				return 1;
			}
			if (exp == 1) {
				return *this;
			}

			MixedPoint result = 1;
			auto base = *this;
			while (exp > 0) {
				if (exp % 2 == 1) {
					result *= base;
				}

				base *= base;
				exp /= 2;
			}
			return result;
		}

		[[nodiscard]] // returns 0 when base is negative and exp is not an integer
		constexpr MixedPoint pow(MixedPoint exp) const noexcept {
			if (*this == 1) return *this;
			if (*this == 0 && exp != 0) return *this;

			auto calcWholeNum = [](MixedPoint base, MixedPoint whole) {
				if (whole == 0) {
					return MixedPoint{ 1 };
				}
				if (whole == 1) {
					return base;
				}

				if (whole < 0) {
					base = MixedPoint{ 1 } / base;
					whole = -whole;
				}

				MixedPoint result = 1;
				while (whole > 0) {
					if (whole % 2 == 1) {
						result *= base;
					}

					base *= base;
					whole /= 2;
					whole = whole.floor();
				}
				return result;
			};

			// attempt whole number only exponent
			if (exp == exp.floor()) {
				return calcWholeNum(*this, exp);
			}
			else if (*this < 0) {
				// negative base to a non-integer exponent, abandon hope
				return 0;
			}

			// b^x = e^(xln(b))
			auto lnb = this->ln();
			exp *= lnb;

			// e^(a+b) = e^a * e^b
			auto whole = exp.floor();
			auto fract = exp - whole;
			auto wholePart = calcWholeNum(MixedPoint{ 271828182845904524, -17 }, whole);

			// Remez algorithm (-1, 1) to approximate mantissa
			// generated with Maple using `minimax(exp(x), x = -1 .. 1, 12, 1, 'maxerror')` (18 digits)
			// theoretical max error is around 3.998e-14, may be slightly larger in practice
			constexpr std::array<MixedPoint, 13> coefficients{
				MixedPoint{100000000000000285, -17},
				MixedPoint{999999999999481873, -18},
				MixedPoint{499999999999757888, -18},
				MixedPoint{166666666681167750, -18},
				MixedPoint{416666666700941491, -19},
				MixedPoint{833333321739788130, -20},
				MixedPoint{138888887041076249, -20},
				MixedPoint{198413095528527369, -21},
				MixedPoint{248016353113237456, -22},
				MixedPoint{275507111351845966, -23},
				MixedPoint{275508560962091967, -24},
				MixedPoint{255790719526347827, -25},
				MixedPoint{213110584847839724, -26},
			};

			auto result = coefficients.back();
			for (size_t i = 1; i < coefficients.size(); ++i) {
				const auto& c = coefficients[coefficients.size() - 1 - i];
				result = c + result * fract;
			}
			result *= wholePart;
			return result;
		}

		[[nodiscard]] // this relies on pow so is fairly slow.
		constexpr MixedPoint sqrt() const noexcept {
			constexpr MixedPoint half{5, -1};
			return pow(half);
		}

		[[nodiscard]]
		static constexpr MixedPoint maxValue() noexcept {
			return MixedPoint{ 
				highThreshold - 1,
				std::numeric_limits<std::decay_t<decltype(exponent)>>::max() 
			};
		}

		static constexpr MixedPoint minValue() noexcept {
			return MixedPoint{
				-(highThreshold - 1),
				std::numeric_limits<std::decay_t<decltype(exponent)>>::max()
			};
		}
	};
}