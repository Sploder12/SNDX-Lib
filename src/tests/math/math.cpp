#include "math/math.hpp"

#include <gtest/gtest.h>

using namespace sndx::math;

TEST(Math, Factorial) {
	EXPECT_EQ(factorial(0), 1);
	EXPECT_EQ(factorial(1), 1);
	EXPECT_EQ(factorial(2), 2);
	EXPECT_EQ(factorial(3), 6);
	EXPECT_EQ(factorial(4), 24);
	EXPECT_EQ(factorial(5), 120);
	EXPECT_EQ(factorial(6), 720);
	EXPECT_EQ(factorial(7), 5040);
	EXPECT_EQ(factorial(8), 40320);
	EXPECT_EQ(factorial(9), 362880);
	EXPECT_EQ(factorial(10), 3628800);
	EXPECT_EQ(factorial(11), 39916800);
	EXPECT_EQ(factorial(12), 479001600);
}

TEST(Math, Factorials) {
	static constexpr size_t n = 12;
	static constexpr auto facts = factorials<n>();

	for (size_t i = 0; i <= n; ++i) {
		EXPECT_EQ(factorial(i), facts[i]);
	}
}

TEST(Math, BinomialCoefficient) {
	EXPECT_EQ(binomialCoefficient(0, 0), 1);

	EXPECT_EQ(binomialCoefficient(1, 0), 1);
	EXPECT_EQ(binomialCoefficient(1, 1), 1);

	EXPECT_EQ(binomialCoefficient(2, 0), 1);
	EXPECT_EQ(binomialCoefficient(2, 1), 2);
	EXPECT_EQ(binomialCoefficient(2, 2), 1);

	EXPECT_EQ(binomialCoefficient(3, 0), 1);
	EXPECT_EQ(binomialCoefficient(3, 1), 3);
	EXPECT_EQ(binomialCoefficient(3, 2), 3);
	EXPECT_EQ(binomialCoefficient(3, 3), 1);

	EXPECT_EQ(binomialCoefficient(4, 0), 1);
	EXPECT_EQ(binomialCoefficient(4, 1), 4);
	EXPECT_EQ(binomialCoefficient(4, 2), 6);
	EXPECT_EQ(binomialCoefficient(4, 3), 4);
	EXPECT_EQ(binomialCoefficient(4, 4), 1);
}

TEST(Math, BinomialCoefficients) {
	static constexpr auto coef0 = binomialCoefficients<0>();
	static constexpr auto coef1 = binomialCoefficients<1>();
	static constexpr auto coef2 = binomialCoefficients<2>();
	static constexpr auto coef3 = binomialCoefficients<3>();
	static constexpr auto coef4 = binomialCoefficients<4>();

	EXPECT_EQ(coef0[0], 1);

	EXPECT_EQ(coef1[0], 1);
	EXPECT_EQ(coef1[1], 1);

	EXPECT_EQ(coef2[0], 1);
	EXPECT_EQ(coef2[1], 2);
	EXPECT_EQ(coef2[2], 1);

	EXPECT_EQ(coef3[0], 1);
	EXPECT_EQ(coef3[1], 3);
	EXPECT_EQ(coef3[2], 3);
	EXPECT_EQ(coef3[3], 1);

	EXPECT_EQ(coef4[0], 1);
	EXPECT_EQ(coef4[1], 4);
	EXPECT_EQ(coef4[2], 6);
	EXPECT_EQ(coef4[3], 4);
	EXPECT_EQ(coef4[4], 1);
}


TEST(Math, Fibonacci) {
	EXPECT_EQ(fibonacci(0), 0);
	EXPECT_EQ(fibonacci(1), 1);
	EXPECT_EQ(fibonacci(2), 1);
	EXPECT_EQ(fibonacci(3), 2);
	EXPECT_EQ(fibonacci(4), 3);
	EXPECT_EQ(fibonacci(5), 5);
	EXPECT_EQ(fibonacci(6), 8);
	EXPECT_EQ(fibonacci(7), 13);
	EXPECT_EQ(fibonacci(8), 21);
	EXPECT_EQ(fibonacci(9), 34);
	EXPECT_EQ(fibonacci(10), 55);
	EXPECT_EQ(fibonacci(11), 89);
	EXPECT_EQ(fibonacci(12), 144);
}

TEST(Math, Fibonaccis) {
	static constexpr size_t n = 12;
	static constexpr auto facts = fibonacci<n>();

	for (size_t i = 0; i <= n; ++i) {
		EXPECT_EQ(fibonacci(i), facts[i]);
	}
}

template <class T>
T min() noexcept {
	return std::numeric_limits<T>::lowest();
}

template <class T>
T max() noexcept {
	return std::numeric_limits<T>::max();
}

template <class I, class O>
void testRemapLimits() {
	EXPECT_EQ(remap<O>(min<I>()), min<O>());
	EXPECT_EQ(remap<O>(max<I>()), max<O>());

	EXPECT_EQ(remap<I>(min<O>()), min<I>());
	EXPECT_EQ(remap<I>(max<O>()), max<I>());
}

TEST(Math, RemapEqualSize) {
	testRemapLimits<unsigned char, char>();

	EXPECT_EQ(remap<unsigned char>('\0'), 128);
	
	EXPECT_EQ(remap<char>((unsigned char)(0)), -128);
	EXPECT_EQ(remap<char>((unsigned char)(128)), 0);
}

TEST(Math, RemapDifferentSize) {
	testRemapLimits<int, char>();
	testRemapLimits<unsigned int, char>();
	testRemapLimits<int, unsigned char>();
	testRemapLimits<unsigned int, unsigned char>();

	EXPECT_EQ(remap<char>(short(0)), 0);
	EXPECT_EQ(remap<char>(int(0)), 0);

	EXPECT_EQ(remap<short>(int(0)), 0);
}

template <class I, class O>
void testRemapBalancedLimits() {
	EXPECT_EQ(remapBalanced<O>(min<I>()), min<O>());
	EXPECT_EQ(remapBalanced<O>(max<I>()), max<O>());

	EXPECT_EQ(remapBalanced<I>(min<O>()), min<I>());
	EXPECT_EQ(remapBalanced<I>(max<O>()), max<I>());
}

TEST(Math, RemapBalanced) {
	testRemapBalancedLimits<char, int>();
	EXPECT_EQ(remapBalanced<int>('\0'), 0);

	testRemapBalancedLimits<short, int>();
	EXPECT_EQ(remapBalanced<short>(0), 0);

	EXPECT_EQ(remapBalanced<unsigned char>('\0'), 0);
	EXPECT_EQ(remapBalanced<unsigned char>('\0', '\0', 128), 128);
	EXPECT_EQ(remapBalanced<unsigned char>(char(127), '\0', 128), 255);
	EXPECT_EQ(remapBalanced<char>((unsigned char)(255), (unsigned char)(128), 0), 127);
}