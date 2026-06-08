#pragma once

#include "./imglm.hpp"

#include "../collision.hpp"

namespace ImGui {
	template <class T>
	bool StructWidget(sndx::collision::Rect<T>& rect, float v_speed = 0.05f) {
		T a = rect.getP1();
		T b = rect.getP2();
		auto any = DragVec("A", a, v_speed);
		any |= DragVec("B", b, v_speed);

		if (any) {
			rect.setPoints(a, b);
		}
		return any;
	}

	template <class T>
	bool StructWidget(sndx::collision::OriRect<T>& obb, float v_speed = 0.05f) {
		using VT = typename T::value_type;
		T center = obb.getCenter();
		T halfs = obb.getHalfExtents();
		glm::quat rot = obb.getRotation();

		VT min{ 0.0 };

		auto any = DragVec("Center", center, v_speed);
		any |= DragVec("Half Extents", halfs, v_speed, &min);
		any |= DragEulerDegrees("Rotation", rot, v_speed);

		any &= glm::all(glm::greaterThan(halfs, T{ 0.0 }));
		if (any) {
			obb = sndx::collision::OriRect<T>{ center, halfs, rot };
		}
		return any;
	}

	template <class T>
	bool StructWidget(sndx::collision::Circle<T>& sphere, float v_speed = 0.05f) {
		T center = sphere.getCenter();
		auto radius = sphere.getRadius();

		auto any = DragVec("Center", center, v_speed);
		any |= DragFloat("Radius", &radius, v_speed);

		any &= radius > 0.0;
		if (any) {
			sphere = sndx::collision::Circle<T>{ center, radius };
		}
		return any;
	}

	template <class T>
	bool StructWidget(sndx::collision::Capsule<T>& capsule, float v_speed = 0.05f) {
		T a = capsule.getPointA();
		T b = capsule.getPointB();
		auto radius = capsule.getRadius();

		auto any = DragVec("A", a, v_speed);
		any |= DragVec("B", b, v_speed);
		any |= DragFloat("Radius", &radius, v_speed);

		any &= radius > 0.0;
		if (any) {
			capsule = sndx::collision::Capsule<T>{ a, b, radius };
		}
		return any;
	}

	template <class T>
	bool StructWidget(sndx::collision::Tri<T>& tri, float v_speed = 0.05f) {
		T a = tri.getP1();
		T b = tri.getP2();
		T c = tri.getP3();

		auto any = DragVec("A", a, v_speed);
		any |= DragVec("B", b, v_speed);
		any |= DragVec("C", c, v_speed);

		if (any) {
			tri = sndx::collision::Tri<T>{ a, b, c };
		}
		if (tri.isDegenerate()) {
			TextColored(ImVec4(1.0, 0.0, 0.0, 1.0), "WARNING: Triangle is degenerate!");
		}
		return any;
	}
}