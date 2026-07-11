#pragma once

#include <glm/glm.hpp>

namespace sndx::math {

	[[nodiscard]] // relativeContact is the point of contact relative to the center of mass
	constexpr auto torque(const glm::vec3& force, const glm::vec3& relativeContact) {
		return glm::cross(relativeContact, force);
	}

	template <class T> [[nodiscard]] // a = F/m
	constexpr T linearAcceleration(const T& force, float mass) {
		return force / mass;
	}

	[[nodiscard]] // relativeContact is the point of contact relative to the center of mass
	constexpr auto angularAcceleration(const glm::vec3& force, const glm::vec3& relativeContact, const glm::mat3& invInertiaTensor) {
		return invInertiaTensor * torque(force, relativeContact);
	}

	template <class T> [[nodiscard]] // returns force of an ideal spring
	constexpr T hookesLaw(float stiffness, const T& displacement) {
		return -stiffness * displacement;
	}

	template <class T> [[nodiscard]] // returns force of a dampened spring
	constexpr T dampenedHookes(float stiffness, const T& displacement, float dampening, const T& velocity) {
		return hookesLaw(stiffness, displacement) - dampening * velocity;
	}

	// as described by the virtual work velocity of points formula
	[[nodiscard]] // velocity at a relative contact point
	constexpr auto contactVelocity(const glm::vec3& linearVelocity, const glm::vec3& angularVelocity, const glm::vec3& relativeContact) {
		return linearVelocity + glm::cross(angularVelocity, relativeContact);
	}

	template <class T> [[nodiscard]]
	constexpr T rigidbodyImpulse(const T& relativeVelocity, const T& collisionNormal, float restitution, float invMassA, float invMassB = 0.0f) {
		auto alignment = glm::dot(relativeVelocity, collisionNormal);
		if (alignment >= 0.0f) {
			return T{};
		}

		auto denom = invMassA + invMassB;
		auto j = -(1.0f + restitution) * alignment;

		return collisionNormal * j / denom;
	}
}