#pragma once

#include <glm/glm.hpp>

namespace sndx {

	template <class T>
	concept Vector = requires(const T& vector) {
		{ T::length() } -> std::convertible_to<size_t>;
		T::length() > 0;

		{ vector[0] } -> std::convertible_to<float>;
	};

	template <class T, size_t n>
	concept VectorN = Vector<T> && requires() {
		T::length() == n;
	};

	template <class T>
	concept Volume = requires(const T& t) {
		{ T::dimensionality() } -> std::convertible_to<size_t>;
		T::dimensionality() > 0;

		{ t.getSize() } -> VectorN<T::dimensionality()>;
		{ t.getCenter() } -> VectorN<T::dimensionality()>;
		{ t.getArea() } -> std::convertible_to<float>;
		{ t.contains(t.getCenter()) } -> std::convertible_to<bool>;
		{ t.distance(t.getCenter()) } -> std::convertible_to<bool>;
	};

	template <class T, size_t n>
	concept VolumeN = Volume<T> && requires() {
		T::dimensionality() == n;
	};
}