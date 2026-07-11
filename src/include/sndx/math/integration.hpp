#pragma once

#include <concepts>

namespace sndx::math {

	template <class T>
	constexpr auto forwardEuler(const T& yn, float t, const T& derivitive) {
		return yn + t * derivitive;
	}

	// derivitive is a function(time, y)
	template <class T, std::regular_invocable<float, const T&> Fn>
	constexpr auto forwardEuler(const T& yn, float t, float dt, const Fn& derivitive) {
		return yn + dt * derivitive(t, yn);
	}

	template <class T> // acceleration aka. the 2nd derivitive
	constexpr auto verlet(const T& current, const T& prev, const T& acceleration, float dt) {
		return 2.0f * current - prev + acceleration * dt * dt;
	}

	// derivitive is a function(time, y)
	template <class T, std::regular_invocable<float, const T&> Fn>
	constexpr auto rungeKutta4(const T& yn, float t, float dt, const Fn& derivitive) {
		auto halfH = dt * 0.5f;
		auto k1 = derivitive(t, yn);
		auto k2 = derivitive(t + halfH, yn + k1 * halfH);
		auto k3 = derivitive(t + halfH, yn + k2 * halfH);
		auto k4 = derivitive(t + dt, yn + k3 * dt);

		return yn + dt / 6.0f * (k1 + 2.0f * k2 + 2.0f * k3 + k4);
	}
}