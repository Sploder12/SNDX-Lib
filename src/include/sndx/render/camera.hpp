#pragma once

#include <array>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace sndx::render {
	struct Perspective {
		float fov;
		float aspectRatio; // width / height
		float near, far;

		template <bool ZO = false> [[nodiscard]]
		glm::mat4 getMatrix() const {
			if constexpr (ZO) {
				return glm::perspectiveZO(fov, aspectRatio, near, far);
			}
			else {
				return glm::perspectiveNO(fov, aspectRatio, near, far);
			}
		}

		[[nodiscard]]
		float getFovX() const {
			return 2.0f * std::atan(std::tan(fov * 0.5f) * aspectRatio);
		}
	};

	struct Orthographic {
		glm::vec3 near, far;

		template <bool ZO = false> [[nodiscard]]
		glm::mat4 getMatrix() const {
			if constexpr (ZO) {
				return glm::orthoZO(near.x, far.x, near.y, far.y, near.z, far.z);
			}
			else {
				return glm::orthoNO(near.x, far.x, near.y, far.y, near.z, far.z);
			}
		}
	};

	struct View {
		glm::vec3 pos{};
		glm::quat orientation{ 1.0f, 0.0f, 0.0f, 0.0f };

		[[nodiscard]]
		glm::mat4 getMatrix() const {
			auto out = glm::translate(glm::mat4(1.0), -pos);
			return glm::mat4_cast(glm::conjugate(orientation)) * out;
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
			return orientation * glm::vec3(1.0, 0.0, 0.0);
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

		View& rotate(float angleDeg, glm::vec3 axis) {
			orientation = glm::rotate(orientation, glm::radians(angleDeg), axis);
			return *this;
		}

		View& rotatePitch(float angleDeg) {
			return rotate(angleDeg, glm::vec3(1.0, 0.0, 0.0));
		}

		View& rotateRoll(float angleDeg) {
			return rotate(angleDeg, glm::vec3(0.0, 0.0, 1.0));
		}

		View& rotateYaw(float angleDeg) {
			return rotate(angleDeg, glm::vec3(0.0, 1.0, 0.0));
		}

		// negative values move backwards
		View& moveForward(float dist) {
			pos += getForward() * dist;
			return *this;
		}

		View& lookAt(const glm::vec3& at, const glm::vec3& up) {
			auto direction = at - pos;
			auto len = glm::length(direction);

			if (len < 0.0001f)
				return *this;

			direction /= len;
			orientation = glm::quatLookAt(direction, up);
			return *this;
		}

		View& lookAt(const glm::vec3& at) {
			return lookAt(at, getUp());
		}
	};

	template <bool ZO = false> [[nodiscard]]
	std::array<glm::vec3, 8> getFrustrumCorners(const glm::mat4& projView) {
		std::array<glm::vec3, 8> out{};

		auto inverse = glm::inverse(projView);
		for (uint16_t z = 0; z < 2; ++z) {
			float baseZ = ZO ? z : 2.0f * z - 1.0f;
			for (uint16_t y = 0; y < 2; ++y) {
				for (uint16_t x = 0; x < 2; ++x) {
					glm::vec4 corner{
						2.0f * x - 1.0f,
						2.0f * y - 1.0f,
						baseZ,
						1.0f
					};
					auto world = inverse * corner;

					out[z * 4 + y * 2 + x] = glm::vec3(world / world.w);
				}
			}
		}

		return out;
	}
}