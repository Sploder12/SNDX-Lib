#include "collision/rect.hpp"
#include "collision/circle.hpp"

#include <gtest/gtest.h>

using namespace sndx::collision;

template <class T>
class RectTest : public testing::Test {};

using Types = ::testing::Types<glm::vec1, glm::vec2, glm::vec3, glm::vec4>;
TYPED_TEST_SUITE(RectTest, Types);

TYPED_TEST(RectTest, Points) {

	static constexpr auto size = TypeParam(1.0f);

	Rect<TypeParam> r{ TypeParam(), size };

	EXPECT_EQ(r.getSize(), size);
	EXPECT_FLOAT_EQ(r.getArea(), 1.0f);

	EXPECT_TRUE(r.contains(r.getCenter()));
	EXPECT_FLOAT_EQ(r.distance(r.getCenter()), -0.5f);

	EXPECT_TRUE(r.overlaps(r));
	EXPECT_TRUE(r.contains(r));
	EXPECT_TRUE(r.contains(size));
	EXPECT_TRUE(r.contains(TypeParam{}));

	EXPECT_FALSE(r.contains(TypeParam{ -1.0f }));
	EXPECT_FALSE(r.contains(TypeParam{ 1.1f }));

	if constexpr (std::is_same_v<TypeParam, glm::vec3>) {
		static_assert(r.dimensionality() == 3);

		EXPECT_FALSE(r.contains(glm::vec3{ 1.1f, 0.0f, 0.0f }));
		EXPECT_FALSE(r.contains(glm::vec3{ 0.0f, 1.1f, 0.0f }));
		EXPECT_FALSE(r.contains(glm::vec3{ 0.0f, 0.0f, 1.1f }));
		EXPECT_FALSE(r.contains(glm::vec3{ -1.1f, 0.0f, 0.0f }));
		EXPECT_FALSE(r.contains(glm::vec3{ 0.0f, -1.1f, 0.0f }));
		EXPECT_FALSE(r.contains(glm::vec3{ 0.0f, 0.0f, -1.1f }));

		EXPECT_FLOAT_EQ(r.distance(glm::vec3{ 2.0f, 1.0f, 1.0f }), 1.0f);
		EXPECT_FLOAT_EQ(r.distance(glm::vec3{ -1.0f, 1.0f, 1.0f }), 1.0f);
	}

	EXPECT_FLOAT_EQ(r.distance(TypeParam{}), 0.0f);
	EXPECT_FLOAT_EQ(r.distance(TypeParam{1.0f}), 0.0f);
}

TEST(Rect, VolumeCollision) {
	static constexpr auto size = glm::vec2(1.0f);

	Rect2D r{ size };
	Circle2D c{ glm::vec2(0.5f), 0.5f };

	EXPECT_TRUE(r.contains(c));
	EXPECT_LE(c.distance(r), 0.0);
	EXPECT_TRUE(c.overlaps(r));

	Rect2D o{ glm::vec2(0.5f), glm::vec2(1.5f) };

	EXPECT_FLOAT_EQ(o.getArea(), 1.0f);

	EXPECT_TRUE(r.overlaps(o));
	EXPECT_FALSE(r.contains(o));
	EXPECT_TRUE(o.overlaps(r));
	EXPECT_FALSE(o.contains(r));

	auto b = r.combine(o);

	EXPECT_FLOAT_EQ(b.getArea(), 1.5f * 1.5f);

	EXPECT_TRUE(b.contains(r));
	EXPECT_TRUE(b.contains(o));

	Rect2D x{ glm::vec2(2.0f), glm::vec2(3.0f) };

	EXPECT_FALSE(r.contains(x));
	EXPECT_FALSE(r.overlaps(x));
	EXPECT_FALSE(o.contains(x));
	EXPECT_FALSE(o.overlaps(x));
}