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

TEST(GJK, circleAndTriangleCollide2) {
	Circle3D circle{ glm::vec3{4.41356277f, 3.2411015f, -2.31648755f}, 0.5f };
	const std::array<glm::vec3, 3> triangle{
		glm::vec3{1.0f, 1.0f, -1.0f},
		glm::vec3{-1.0f, 1.0f, -1.0f},
		glm::vec3{-1.0f, 1.0f, 1.0f},
	};

	glm::mat4 transform = glm::translate(glm::mat4{ 1.0f }, glm::vec3(5.0, 1.5, 0.0));
	transform = glm::scale(transform, glm::vec3(2.0, 1.5, 5.0));
	auto inv = glm::inverse(transform);

	auto ttri = transformSupportFn(getSupportFn(std::span{ triangle }), transform, inv);

	auto result = gjk(getSupportFn(circle), ttri);
	EXPECT_TRUE(result);

	result = gjk(ttri, getSupportFn(circle));
	EXPECT_TRUE(result);
}

// because EXPECT_FLOAT_EQ is WAYYYYY too strict.
bool floatEq(float a, float b) {
	return std::abs(a - b) <= 0.00005f;
}

TEST(Dist_GJK, simpleBoxesGiveDist) {
	Rect3D boxA{ glm::vec3{-0.5f}, glm::vec3{0.1f} };
	Rect3D boxB{ glm::vec3{0.11f}, glm::vec3{0.5f} };

	auto result = gjkDist(getSupportFn(boxA), getSupportFn(boxB));
	EXPECT_FALSE(result.hit);
	EXPECT_TRUE(boxA.contains(result.a));
	EXPECT_TRUE(boxB.contains(result.b));

	auto dist = glm::distance(result.a, result.b);
	EXPECT_TRUE(floatEq(dist, 0.01732f));

	result = gjkDist(getSupportFn(boxB), getSupportFn(boxA));
	EXPECT_FALSE(result.hit);
	EXPECT_TRUE(boxB.contains(result.a));
	EXPECT_TRUE(boxA.contains(result.b));

	dist = glm::distance(result.a, result.b);
	EXPECT_TRUE(floatEq(dist, 0.01732f));
}

TEST(Dist_GJK, axisAlignedSpheresGiveDist) {
	Circle3D circleA{ glm::vec3{-1.0f, 0.0f, 0.0f}, 1.0f };
	Circle3D circleB{ glm::vec3{-3.0f, 0.0f, 0.0f}, 0.5f };

	auto result = gjkDist(getSupportFn(circleA), getSupportFn(circleB));
	EXPECT_FALSE(result.hit);
	EXPECT_TRUE(circleA.contains(result.a));
	EXPECT_TRUE(circleB.contains(result.b));

	auto dist = glm::distance(result.a, result.b);
	EXPECT_TRUE(floatEq(dist, 0.5f));

	result = gjkDist(getSupportFn(circleB), getSupportFn(circleA));
	EXPECT_FALSE(result.hit);
	EXPECT_TRUE(circleB.contains(result.a));
	EXPECT_TRUE(circleA.contains(result.b));

	dist = glm::distance(result.a, result.b);
	EXPECT_TRUE(floatEq(dist, 0.5f));
}

TEST(Dist_GJK, MixedShapesGiveDist) {
	Circle3D circle{ glm::vec3{ 0.0f, 1.0f, 0.0f }, 0.5f };
	Tri3D tri{ glm::vec3{-1.0f, 0.0f, 1.0f}, glm::vec3{1.0f, 0.0f, 1.0f} , glm::vec3{1.0f, 0.0f, -1.0f} };

	auto result = gjkDist(getSupportFn(circle), getSupportFn(tri));
	EXPECT_FALSE(result.hit);
	EXPECT_LE(circle.distance(result.a), 0.00001f);
	EXPECT_TRUE(tri.contains(result.b));

	auto dist = glm::distance(result.a, result.b);
	EXPECT_TRUE(floatEq(dist, 0.5f));

	result = gjkDist(getSupportFn(tri), getSupportFn(circle));
	EXPECT_FALSE(result.hit);
	EXPECT_TRUE(tri.contains(result.a));
	EXPECT_TRUE(circle.contains(result.b));

	dist = glm::distance(result.a, result.b);
	EXPECT_TRUE(floatEq(dist, 0.5f));
}

TEST(Dist_GJK, MixedShapesGiveDist2) {
	Circle3D circle{ glm::vec3{ 0.0f, 1.0f, 0.0f }, 0.5f };
	Rect3D rect{ glm::vec3{-20.0f, -20.0f, -20.0f}, glm::vec3{20.0f, 0.0f, 20.0f}};

	auto result = gjkDist(getSupportFn(circle), getSupportFn(rect));
	EXPECT_FALSE(result.hit);
	EXPECT_LE(circle.distance(result.a), 0.00001f);
	EXPECT_TRUE(rect.contains(result.b));

	auto dist = glm::distance(result.a, result.b);
	EXPECT_TRUE(floatEq(dist, 0.5f));

	result = gjkDist(getSupportFn(rect), getSupportFn(circle));
	EXPECT_FALSE(result.hit);
	EXPECT_TRUE(rect.contains(result.a));
	EXPECT_TRUE(circle.contains(result.b));

	dist = glm::distance(result.a, result.b);
	EXPECT_TRUE(floatEq(dist, 0.5f));
}


TEST(EPA, simpleBoxesGiveCorrectDirection) {
	Rect3D boxA{ glm::vec3{-1.0f}, glm::vec3{0.0f} };
	Rect3D boxB{ glm::vec3{-0.5f}, glm::vec3{0.5f} };

	auto simplex = gjk(getSupportFn(boxA), getSupportFn(boxB));
	ASSERT_TRUE(simplex);

	auto result = epa(*simplex, getSupportFn(boxA), getSupportFn(boxB));
	ASSERT_LE(std::abs(result.depth - 0.50001f), 0.0001f);

	boxB.translate(result.normal * (result.depth + 0.0001f));
	EXPECT_FALSE(gjk(getSupportFn(boxA), getSupportFn(boxB)));
}

TEST(EPA, simpleSpheresGiveCorrectDirection) {
	Circle3D circleA{ glm::vec3{-1.0f, 0.0f, 0.0f}, 1.0f };
	Circle3D circleB{ glm::vec3{-2.0f, 0.0f, 0.0f}, 0.5f };

	auto sptA = getSupportFn(circleA);
	auto sptB = getSupportFn(circleB);

	auto simplex = gjk(sptA, sptB);
	ASSERT_TRUE(simplex);

	auto result = epa(*simplex, sptA, sptB);
	EXPECT_TRUE(floatEq(result.depth, 0.5));
}

TEST(EPA, mixedShapesGiveCorrectDirection) {
	Circle3D circle{ glm::vec3{-1.0f, 1.0f, 0.0f}, 1.0f };
	Rect3D rect{ glm::vec3{-0.5f, 0.0f, 0.0f}, glm::vec3{-0.5f, 2.0f, 1.0f} };

	auto sptA = getSupportFn(circle);
	auto sptB = getSupportFn(rect);

	auto simplex = gjk(sptA, sptB);
	ASSERT_TRUE(simplex);

	auto result = epa(*simplex, sptA, sptB);
	EXPECT_TRUE(floatEq(result.depth, 0.5));
}