#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace sndx {


	struct Camera {
		glm::vec3 pos;
		glm::quat orientation;

		[[nodiscard]]
		glm::mat4 getViewMatrix() const {
			auto out = glm::translate(glm::mat4(1.0), pos);
			return out * glm::mat4_cast(orientation);
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
	};
}