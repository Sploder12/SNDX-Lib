#include "math/integration.hpp"

#include <gtest/gtest.h>

#include <cmath>

using namespace sndx::math;

namespace {
	// this is the differential equation wikipedia uses for the runge-kutta page
	const auto yt = [](float t) -> float {
		return 2.0f * std::exp(0.5f * (t - std::sin(t) * std::cos(t)));
	};

	const float ys0 = yt(0.0f);
	const auto dydt = [](float t, float y) -> float {
		return std::pow(std::sin(t), 2.0f) * y;
	};
}

TEST(Integration, forwardEuler) {
	float p = 1.0f;
	float v = 3.0f;
	float dt = 2.0f;

	auto r = forwardEuler(p, dt, v);
	EXPECT_EQ(r, 7.0f);
}

TEST(Integration, verlet) {
	float p = 1.0f;
	const float accel = -1.0f;
	auto truth = [i = p, a = accel](float t) {
		return a * 0.5f * t * t + i;
	};

	float dt = 0.5f;
	float pp = truth(-dt);

	for (size_t i = 0; i < 10; ++i) {
		auto exact = truth(i * dt);
		auto error = std::abs(exact - p);
		EXPECT_FLOAT_EQ(error, 0.0);

		pp = std::exchange(p, verlet(p, pp, accel, dt));
	}

	auto fin = truth(10.0f * dt);
	EXPECT_FLOAT_EQ(std::abs(fin - p), 0.0);
}

TEST(Integration, forwardEulerFn) {
	const auto fin = yt(5.0f);
	float p = ys0;
	for (float t = 0.0f; t < 5.0f; t += 0.5f) {
		auto exact = yt(t);
		auto error = std::abs(p - exact);
		EXPECT_LE(error, 7.0f);

		p = forwardEuler(p, t, 0.5f, dydt);
	}
	// we know about how inaccuract we should be
	EXPECT_LE(std::abs(fin - p), 14.0f);
	EXPECT_GE(std::abs(fin - p), 10.0f);

	// we are expecting to see better results with lower timestep
	p = ys0;
	for (float t = 0.0f; t < 5.0f; t += 0.25f) {
		auto exact = yt(t);
		auto error = std::abs(p - exact);
		EXPECT_LE(error, 6.0f);

		p = forwardEuler(p, t, 0.25f, dydt);
	}

	EXPECT_LE(std::abs(fin - p), 8.0f);
	EXPECT_GE(std::abs(fin - p), 5.0f);
}


TEST(Integration, rk4) {
	const auto fin = yt(5.0f);
	float p = ys0;

	for (float t = 0.0f; t < 5.0f; t += 0.5f) {
		auto exact = yt(t);
		auto error = std::abs(p - exact);

		EXPECT_LE(error, 0.025f);
		p = rungeKutta4(p, t, 0.5f, dydt);
	}
	EXPECT_LE(std::abs(fin - p), 0.05f);
	EXPECT_GE(std::abs(fin - p), 0.01f);
}