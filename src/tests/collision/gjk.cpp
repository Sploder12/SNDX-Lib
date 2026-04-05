#include "collision/gjk.hpp"
#include "collision/rect.hpp"
#include "collision/circle.hpp"

#include <gtest/gtest.h>

using namespace sndx::collision;

TEST(GJK, simpleBoxesCollide) {
	Rect3D boxA{ glm::vec3{-0.5f}, glm::vec3{0.1f} };
	Rect3D boxB{ glm::vec3{-0.1f}, glm::vec3{0.5f} };

	auto result = gjk(getSupportFn(boxA), getSupportFn(boxB));
	EXPECT_TRUE(result);

	result = gjk(getSupportFn(boxB), getSupportFn(boxA));
	EXPECT_TRUE(result);
}

TEST(GJK, simpleBoxesDontCollide) {
	Rect3D boxA{ glm::vec3{-0.5f}, glm::vec3{0.1f} };
	Rect3D boxB{ glm::vec3{0.11f}, glm::vec3{0.5f} };

	auto result = gjk(getSupportFn(boxA), getSupportFn(boxB));
	EXPECT_FALSE(result);

	result = gjk(getSupportFn(boxB), getSupportFn(boxA));
	EXPECT_FALSE(result);
}

TEST(GJK, simpleSpheresCollide) {
	Circle3D circleA{ glm::vec3{-0.5f, -1.0f, 0.4f}, 0.6f };
	Circle3D circleB{ glm::vec3{-0.1f, -0.9f, 1.1f}, 0.3f };

	assert(circleA.overlaps(circleB) && "test writer error");

	auto result = gjk(getSupportFn(circleA), getSupportFn(circleB));
	EXPECT_TRUE(result);

	result = gjk(getSupportFn(circleB), getSupportFn(circleA));
	EXPECT_TRUE(result);
}


TEST(GJK, handlesOriginAlignedSpheres) {
	Circle3D circleA{ glm::vec3{-1.0f, 0.0f, 0.0f}, 1.0f };
	Circle3D circleB{ glm::vec3{-2.0f, 0.0f, 0.0f}, 0.5f };

	auto sptA = getSupportFn(circleA);
	auto sptB = getSupportFn(circleB);

	auto simplex = gjk(sptA, sptB);
	EXPECT_TRUE(simplex);

	simplex = gjk(sptB, sptA);
	EXPECT_TRUE(simplex);
}

TEST(GJK, simpleSpheresDontCollide) {
	Circle3D circleA{ glm::vec3{-0.5f, -1.0f, 0.4f}, 0.5f };
	Circle3D circleB{ glm::vec3{-0.1f, -0.9f, 1.1f}, 0.3f };

	assert(!circleA.overlaps(circleB) && "test writer error");

	auto result = gjk(getSupportFn(circleA), getSupportFn(circleB));
	EXPECT_FALSE(result);

	result = gjk(getSupportFn(circleB), getSupportFn(circleA));
	EXPECT_FALSE(result);
}

TEST(GJK, boxAndSphereCollide) {
	Rect3D box{ glm::vec3{0.5f}, glm::vec3{1.0f} };
	Circle3D circle{ glm::vec3{2.15f}, 2.0f };

	assert(circle.overlaps(box) && "test writer error");

	auto result = gjk(getSupportFn(circle), getSupportFn(box));
	EXPECT_TRUE(result);

	result = gjk(getSupportFn(box), getSupportFn(circle));
	EXPECT_TRUE(result);
}

TEST(GJK, circleAndTriangleCollide) {
	Circle3D circle{ glm::vec3{0.7f, 1.0f, 1.5f}, 0.9f };
	const std::array<glm::vec3, 3> triangle{
		glm::vec3{2.0f, -1.0f, 1.0f},
		glm::vec3{2.0f, 2.0f, 0.5f},
		glm::vec3{1.0f, -0.1f, 2.0f},
	};

	auto result = gjk(getSupportFn(circle), getSupportFn(std::span{ triangle }));
	EXPECT_TRUE(result);

	result = gjk(getSupportFn(std::span{ triangle }), getSupportFn(circle));
	EXPECT_TRUE(result);
}

/*
TEST(EPA, simpleBoxesGiveCorrectDirection) {
	Rect3D boxA{ glm::vec3{-1.0f}, glm::vec3{0.0f} };
	Rect3D boxB{ glm::vec3{-0.5f}, glm::vec3{0.5f} };

	auto simplex = gjk(getSupportFn(boxA), getSupportFn(boxB));
	ASSERT_TRUE(simplex);

	auto result = epa(*simplex, getSupportFn(boxA), getSupportFn(boxB));
	ASSERT_LE(std::abs(result.depth - 0.5f), 0.0001f);

	boxB.translate(result.normal * (result.depth + 0.0001f));
	EXPECT_FALSE(gjk(getSupportFn(boxA), getSupportFn(boxB)));
}

TEST(EPA, simpleSpheresGiveCorrectDirection) {
	Circle3D circleA{ glm::vec3{-1.0f, 0.0f, 0.0f}, 1.0f };
	Circle3D circleB{ glm::vec3{-2.0f, 0.0f, 0.0f}, 0.5f };

	auto sptA = getSupportFn(circleA);
	auto sptB = getSupportFn(circleA);

	auto simplex = gjk(sptA, sptB);
	ASSERT_TRUE(simplex);

	auto result = epa(*simplex, sptA, sptB);
}

TEST(EPA, mixedShapesGiveCorrectDirection) {
	Circle3D circle{ glm::vec3{-1.0f, 1.0f, 0.0f}, 1.0f };
	Rect3D rect{ glm::vec3{-0.5f, 0.0f, 0.0f}, glm::vec3{-0.5f, 2.0f, 1.0f} };

	auto sptA = getSupportFn(circle);
	auto sptB = getSupportFn(rect);

	auto simplex = gjk(sptA, sptB);
	ASSERT_TRUE(simplex);

	auto result = epa(*simplex, sptA, sptB);
}*/