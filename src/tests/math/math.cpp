#include "math/math.hpp"

#include <gtest/gtest.h>

using namespace sndx::math;

TEST(Math, Factorial) {
	ASSERT_EQ(factorial(0), 1);
	ASSERT_EQ(factorial(1), 1);
	ASSERT_EQ(factorial(2), 2);
	ASSERT_EQ(factorial(3), 6);
	ASSERT_EQ(factorial(4), 24);
	ASSERT_EQ(factorial(5), 120);
	ASSERT_EQ(factorial(6), 720);
	ASSERT_EQ(factorial(7), 5040);
	ASSERT_EQ(factorial(8), 40320);
	ASSERT_EQ(factorial(9), 362880);
	ASSERT_EQ(factorial(10), 3628800);
	ASSERT_EQ(factorial(11), 39916800);
	ASSERT_EQ(factorial(12), 479001600);
}

TEST(Math, Factorials) {
	static constexpr size_t n = 12;
	static constexpr auto facts = factorials<n>();

	for (size_t i = 0; i <= n; ++i) {
		ASSERT_EQ(factorial(i), facts[i]);
	}
}

TEST(Math, BinomialCoefficient) {
	ASSERT_EQ(binomialCoefficient(0, 0), 1);

	ASSERT_EQ(binomialCoefficient(1, 0), 1);
	ASSERT_EQ(binomialCoefficient(1, 1), 1);

	ASSERT_EQ(binomialCoefficient(2, 0), 1);
	ASSERT_EQ(binomialCoefficient(2, 1), 2);
	ASSERT_EQ(binomialCoefficient(2, 2), 1);

	ASSERT_EQ(binomialCoefficient(3, 0), 1);
	ASSERT_EQ(binomialCoefficient(3, 1), 3);
	ASSERT_EQ(binomialCoefficient(3, 2), 3);
	ASSERT_EQ(binomialCoefficient(3, 3), 1);

	ASSERT_EQ(binomialCoefficient(4, 0), 1);
	ASSERT_EQ(binomialCoefficient(4, 1), 4);
	ASSERT_EQ(binomialCoefficient(4, 2), 6);
	ASSERT_EQ(binomialCoefficient(4, 3), 4);
	ASSERT_EQ(binomialCoefficient(4, 4), 1);
}

TEST(Math, BinomialCoefficients) {
	static constexpr auto coef0 = binomialCoefficients<0>();
	static constexpr auto coef1 = binomialCoefficients<1>();
	static constexpr auto coef2 = binomialCoefficients<2>();
	static constexpr auto coef3 = binomialCoefficients<3>();
	static constexpr auto coef4 = binomialCoefficients<4>();

	ASSERT_EQ(coef0[0], 1);

	ASSERT_EQ(coef1[0], 1);
	ASSERT_EQ(coef1[1], 1);

	ASSERT_EQ(coef2[0], 1);
	ASSERT_EQ(coef2[1], 2);
	ASSERT_EQ(coef2[2], 1);

	ASSERT_EQ(coef3[0], 1);
	ASSERT_EQ(coef3[1], 3);
	ASSERT_EQ(coef3[2], 3);
	ASSERT_EQ(coef3[3], 1);

	ASSERT_EQ(coef4[0], 1);
	ASSERT_EQ(coef4[1], 4);
	ASSERT_EQ(coef4[2], 6);
	ASSERT_EQ(coef4[3], 4);
	ASSERT_EQ(coef4[4], 1);
}


TEST(Math, Fibonacci) {
	ASSERT_EQ(fibonacci(0), 0);
	ASSERT_EQ(fibonacci(1), 1);
	ASSERT_EQ(fibonacci(2), 1);
	ASSERT_EQ(fibonacci(3), 2);
	ASSERT_EQ(fibonacci(4), 3);
	ASSERT_EQ(fibonacci(5), 5);
	ASSERT_EQ(fibonacci(6), 8);
	ASSERT_EQ(fibonacci(7), 13);
	ASSERT_EQ(fibonacci(8), 21);
	ASSERT_EQ(fibonacci(9), 34);
	ASSERT_EQ(fibonacci(10), 55);
	ASSERT_EQ(fibonacci(11), 89);
	ASSERT_EQ(fibonacci(12), 144);
}

TEST(Math, Fibonaccis) {
	static constexpr size_t n = 12;
	static constexpr auto facts = fibonacci<n>();

	for (size_t i = 0; i <= n; ++i) {
		ASSERT_EQ(fibonacci(i), facts[i]);
	}
}