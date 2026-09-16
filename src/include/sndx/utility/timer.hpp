#pragma once

#include <algorithm>
#include <cmath>

namespace sndx::utility {
	struct IntervalTimer {
		// behavior undefined if start > end
		double start = 0.0, end = 0.0;
		double time = 0.0;
		double speed = 1.0;

		constexpr operator double() const noexcept {
			return time;
		}

		constexpr explicit IntervalTimer() noexcept = default;

		constexpr explicit IntervalTimer(double start, double end, double speed = 1.0):
			start(std::min(start, end)), end(std::max(start, end)), time(0.0), speed(speed) {

			if (this->start != start) {
				this->speed = -this->speed;
			}

			this->time = this->speed >= 0.0 ? this->start : this->end;
		}

		void correctTime(bool loop) {
			if (loop) {
				auto duration = end - start;
				if (duration == 0.0) return;
				auto adj = time - start;
				time = std::fmod(adj, duration) + (adj >= 0.0 ? start : end);
			}
			else {
				time = std::clamp(time, start, end);
			}
		}

		void advance(double dt, bool loop = false) {
			time += dt * speed;
			correctTime(loop);
		}

		constexpr void gotoStart() {
			if (speed >= 0.0) {
				time = start;
			}
			else {
				time = end;
			}
		}

		constexpr void gotoEnd() {
			if (speed >= 0.0) {
				time = end;
			}
			else {
				time = start;
			}
		}

		// not time, but t
		constexpr void gotoT(double t) {
			auto duration = end - start;
			time = start + std::clamp(t, 0.0, 1.0) * duration;
		}

		[[nodiscard]]
		constexpr double getT() const noexcept {
			auto duration = end - start;
			if (duration == 0.0) return 0.0;
			return (time - start) / duration;
		}

		[[nodiscard]]
		constexpr bool atStart() const noexcept {
			return speed >= 0.0 ? time <= start : time >= end;
		}

		[[nodiscard]]
		constexpr bool atEnd() const noexcept {
			return speed >= 0.0 ? time >= end : time <= start;
		}
	};
}