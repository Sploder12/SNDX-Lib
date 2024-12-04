#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace sndx::render {
	struct Camera {
		glm::vec3 pos;
		glm::quat orientation;

		constexpr Camera() :
			pos(), orientation(1.0f, 0.0f, 0.0f, 0.0f) {}

		[[nodiscard]]
		glm::mat4 getViewMatrix() const {
			auto out = glm::translate(glm::mat4(1.0), pos);
			return glm::mat4_cast(orientation) * out;
		}

		[[nodiscard]]
		glm::vec3 getForward() const {
			return orientation * glm::vec3(0.0, 0.0, 1.0);
		}

		[[nodiscard]]
		glm::vec3 getUp() const {
			return orientation * glm::vec3(0.0, 1.0, 0.0);
		}

		[[nodiscard]]
		glm::vec3 getRight() const {
			return orientation * glm::vec3(-1.0, 0.0, 0.0);
		}

		// in degrees! not radians!
		[[nodiscard]]
		auto getPitch() const {
			return glm::degrees(glm::pitch(orientation));
		}

		[[nodiscard]]
		auto getRoll() const {
			return glm::degrees(glm::roll(orientation));
		}

		[[nodiscard]]
		auto getYaw() const {
			return glm::degrees(glm::yaw(orientation));
		}

		Camera& rotate(float angleDeg, glm::vec3 axis) {
			orientation = glm::rotate(orientation, glm::radians(angleDeg), axis);
			return *this;
		}

		Camera& rotatePitch(float angleDeg) {
			return rotate(angleDeg, glm::vec3(1.0, 0.0, 0.0));
		}

		Camera& rotateRoll(float angleDeg) {
			return rotate(angleDeg, glm::vec3(0.0, 0.0, 1.0));
		}

		Camera& rotateYaw(float angleDeg) {
			return rotate(angleDeg, glm::vec3(0.0, 1.0, 0.0));
		}

		// negative values move backwards
		Camera& moveForward(float dist) {
			pos += getForward() * dist;
			return *this;
		}

		Camera& lookAt(const glm::vec3& at) {
			auto direction = at - pos;
			auto len = glm::length(direction);

			if (len < 0.0001f)
				return *this;

			direction /= len;

			orientation = glm::quatLookAt(direction, getUp());
			return *this;
		}
	};
}