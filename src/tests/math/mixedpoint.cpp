#include "math/mixedpoint.hpp"

#include <gtest/gtest.h>

using namespace sndx::math;

TEST(MixedPoint, float_ctor_works) {
	EXPECT_EQ((MixedPoint{ 37.0 }), (MixedPoint{ 37 }));
	EXPECT_EQ((MixedPoint{ 2.51188643151 }), (MixedPoint{ 251188643151, -11 }));
	EXPECT_EQ((MixedPoint{ 1e15 }), (MixedPoint{ 1, 15 }));
	EXPECT_EQ((MixedPoint{ -0.258 }), (MixedPoint{ -258, -3 }));
	EXPECT_EQ((MixedPoint{ -1.245285874467082e-9 }), (MixedPoint{ -12452858744670818, -25 }));
}

TEST(MixedPoint, floor_works) {
	EXPECT_EQ(MixedPoint{ 42 }.floor(), 42);
	EXPECT_EQ((MixedPoint{ 99999999999999, 100 }).floor(), (MixedPoint{ 99999999999999, 100 }));
	EXPECT_EQ(MixedPoint{ 42 }.floor(), 42);
	EXPECT_EQ(MixedPoint{ 0 }.floor(), 0);
	EXPECT_EQ(MixedPoint{ -0.257 }.floor(), -1);
	EXPECT_EQ(MixedPoint{ -1.257 }.floor(), -2);
	EXPECT_EQ(MixedPoint{ -42.0 }.floor(), -42);
	EXPECT_EQ(MixedPoint{ 0.275 }.floor(), 0);
	EXPECT_EQ(MixedPoint{ 1.275 }.floor(), 1);
}

TEST(MixedPoint, ceil_works) {
	EXPECT_EQ(MixedPoint{ 42 }.ceil(), 42);
	EXPECT_EQ((MixedPoint{ 99999999999999, 100 }).ceil(), (MixedPoint{ 99999999999999, 100 }));
	EXPECT_EQ(MixedPoint{ 42 }.ceil(), 42);
	EXPECT_EQ(MixedPoint{ 0 }.ceil(), 0);
	EXPECT_EQ(MixedPoint{ -0.257 }.ceil(), 0);
	EXPECT_EQ(MixedPoint{ -1.257 }.ceil(), -1);
	EXPECT_EQ(MixedPoint{ -42.0 }.ceil(), -42);
	EXPECT_EQ(MixedPoint{ 0.275 }.ceil(), 1);
	EXPECT_EQ(MixedPoint{ 1.275 }.ceil(), 2);
}

TEST(MixedPoint, ilog10_works) {
	EXPECT_EQ(MixedPoint{ 42 }.ilog10(), 1);
	EXPECT_EQ((MixedPoint{ 9, 100 }).ilog10(), 100);
	EXPECT_EQ(MixedPoint{ 42 }.ilog10(), 1);
	EXPECT_EQ(MixedPoint{ 0 }.ilog10(), 0);
	EXPECT_EQ(MixedPoint{ -0.257 }.ilog10(), -1);
	EXPECT_EQ(MixedPoint{ -1.257 }.ilog10(), 0);
	EXPECT_EQ(MixedPoint{ -420.0 }.ilog10(), 2);
	EXPECT_EQ(MixedPoint{ 0.275 }.ilog10(), -1);
	EXPECT_EQ(MixedPoint{ 1.275 }.ilog10(), 0);
}

TEST(MixedPoint, log10_works) {
	// log10 should be accurate to 15 digits
	constexpr auto epsilon = MixedPoint{ 5, -15 };

	EXPECT_LE((MixedPoint{ 42 }.log10() - std::log10(42)).abs(), epsilon);
	EXPECT_LE((MixedPoint{ 9, 100 }.log10() - MixedPoint{100.954242509439}).abs(), epsilon * 100);
	EXPECT_LE(MixedPoint{ 0 }.log10(), 0);
	EXPECT_LE((MixedPoint{ -0.257 }.log10() - std::log10(0.257)).abs(), epsilon);
	EXPECT_LE((MixedPoint{ -1.257 }.log10() - std::log10(1.257)).abs(), epsilon);
	EXPECT_LE((MixedPoint{ -420.0 }.log10() - std::log10(420)).abs(), epsilon);
	EXPECT_LE((MixedPoint{ 0.275 }.log10() - std::log10(0.275)).abs(), epsilon);
	EXPECT_LE((MixedPoint{ 1.275 }.log10() - std::log10(1.275)).abs(), epsilon);
}

TEST(MixedPoint, pow_works) {
	EXPECT_EQ(MixedPoint{ 42 }.pow(1), 42);
	EXPECT_EQ(MixedPoint{ 42 }.pow(0), 1);
	EXPECT_EQ(MixedPoint{ 42 }.pow(-1), (MixedPoint{ 238095238095238100, -19 }));
	EXPECT_EQ(MixedPoint{ 10 }.pow(2), 100);
	EXPECT_EQ(MixedPoint{ -5 }.pow(3), -125);
	EXPECT_EQ(MixedPoint{ 5 }.pow(4), 625);
}

TEST(MixedPoint, float_pow_works) {
	EXPECT_EQ(MixedPoint{ 42 }.pow(MixedPoint{ 1 }), 42);
	EXPECT_EQ(MixedPoint{ 42 }.pow(MixedPoint{ 0 }), 1);
	EXPECT_EQ(MixedPoint{ 42 }.pow(MixedPoint{ -1 }), (MixedPoint{ 238095238095238100, -19 }));
	EXPECT_EQ(MixedPoint{ 10 }.pow(MixedPoint{ 2 }), 100);
	EXPECT_EQ(MixedPoint{ -5 }.pow(MixedPoint{ 3 }), -125);

	// epsilon is for DIGITS so it must be rescaled.
	constexpr auto epsilon = MixedPoint{ 5, -14 };
	EXPECT_LE(MixedPoint{ 10 }.pow(5.2) - (MixedPoint{ 15848931924611135, -11 }).abs(), epsilon * fPow10(6));
	EXPECT_LE(MixedPoint{ 10 }.pow(5.2) - (MixedPoint{ 15848931924611135, -11 }).abs(), epsilon * fPow10(6));
	EXPECT_LE((MixedPoint{ 16 }.pow(-2.5) - 0.0009765625).abs(), epsilon / fPow10(3));
	EXPECT_EQ(MixedPoint{ -9.1 }.pow(5.1), 0);
	EXPECT_EQ(MixedPoint{ -9.1 }.pow(5), -62403.21451);
	
	EXPECT_LE((MixedPoint{ 101.1 }.pow(597.5) - MixedPoint{ 689946579034376804, 1197 - 17 }).ilog10(), 1197 - 11);
}

TEST(MixedPoint, add_works) {
	EXPECT_EQ(MixedPoint{ 0 } + 42, 42);
	EXPECT_EQ(MixedPoint{ 42 } + 0, 42);
	EXPECT_EQ(MixedPoint{ 42 } + 1, 43);
	EXPECT_EQ(MixedPoint{ 42 } - 1, 41);
	EXPECT_EQ((MixedPoint{ 42, 10000 }) + 0x1337, (MixedPoint{42, 10000}));
	EXPECT_EQ((MixedPoint{ 42, 100 }) + (MixedPoint{ 57, 10000 }), (MixedPoint{ 57, 10000 }));
	EXPECT_EQ(MixedPoint{ std::numeric_limits<uint64_t>::max() } + 100, (MixedPoint{ 1844674407370955171, 1 }));
}

TEST(MixedPoint, multiply_works) {
	EXPECT_EQ(MixedPoint{ 42 } * 0, 0);
	EXPECT_EQ(MixedPoint{ 0 } * 42, 0);
	EXPECT_EQ(MixedPoint{ 10 } * 10, (MixedPoint{ 1, 2 }));
	EXPECT_EQ(MixedPoint{ -0.257 } * -1, (MixedPoint{ 0.257 }));
	EXPECT_EQ(MixedPoint{ -0.275 } * 1, (MixedPoint{ -0.275 }));

	EXPECT_EQ(MixedPoint{ 9876543210 } * MixedPoint{ 9876543210 }, (MixedPoint{ 975461057789971041, 2 }));
	EXPECT_EQ(MixedPoint{ 9876543210111111ll } * MixedPoint{ 1234567891111111111ll }, (MixedPoint{ 121932631223746378, 17 }));
}

TEST(MixedPoint, divide_works) {
	EXPECT_EQ(MixedPoint{ 42 } / 42, 1);
	EXPECT_EQ(MixedPoint{ 10 } / 1000, (MixedPoint{ 1, -2 }));
	EXPECT_EQ(MixedPoint{ -0.257 } / -1, (MixedPoint{ 0.257 }));
	EXPECT_EQ(MixedPoint{ -0.275 } / 1, (MixedPoint{ -0.275 }));

	auto result = MixedPoint{ 9876543210 } / MixedPoint{ 1.0 / 9876543210.0 };
	constexpr auto exact = MixedPoint{ 975461057789971041, 2 };
	constexpr auto epsilon = MixedPoint{ 1, 4 };
	EXPECT_LE((result - exact).abs(), epsilon);

	result = 1 / (MixedPoint{ 251188643151, -11 });
	EXPECT_LE(result - (MixedPoint{ 39810717055343070, -17 }).abs(), (MixedPoint{ 1, -16 }));
}

TEST(MixedPoint, modulo_works) {
	EXPECT_EQ(MixedPoint{ 42 } % 42, 0);
	EXPECT_EQ(MixedPoint{ 10 } % 1000, 10);
	EXPECT_EQ(MixedPoint{ 0.256 } % -1, -0.744);
	EXPECT_EQ(MixedPoint{ -0.257 } % -1, -0.257);
	EXPECT_EQ(MixedPoint{ -0.275 } % 1, 0.725);
}