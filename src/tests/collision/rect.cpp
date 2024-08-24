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

	ASSERT_EQ(r.getSize(), size);
	ASSERT_FLOAT_EQ(r.getArea(), 1.0f);

	ASSERT_TRUE(r.contains(r.getCenter()));
	ASSERT_FLOAT_EQ(r.getDistance(r.getCenter()), -0.5f);

	ASSERT_TRUE(r.overlaps(r));
	ASSERT_TRUE(r.contains(r));
	ASSERT_TRUE(r.contains(size));
	ASSERT_TRUE(r.contains(TypeParam{}));

	ASSERT_FALSE(r.contains(TypeParam{ -1.0f }));
	ASSERT_FALSE(r.contains(TypeParam{ 1.1f }));

	if constexpr (std::is_same_v<TypeParam, glm::vec3>) {
		static_assert(r.getDimensionality() == 3);

		ASSERT_FALSE(r.contains(glm::vec3{ 1.1f, 0.0f, 0.0f }));
		ASSERT_FALSE(r.contains(glm::vec3{ 0.0f, 1.1f, 0.0f }));
		ASSERT_FALSE(r.contains(glm::vec3{ 0.0f, 0.0f, 1.1f }));
		ASSERT_FALSE(r.contains(glm::vec3{ -1.1f, 0.0f, 0.0f }));
		ASSERT_FALSE(r.contains(glm::vec3{ 0.0f, -1.1f, 0.0f }));
		ASSERT_FALSE(r.contains(glm::vec3{ 0.0f, 0.0f, -1.1f }));

		ASSERT_FLOAT_EQ(r.getDistance(glm::vec3{ 2.0f, 1.0f, 1.0f }), 1.0f);
		ASSERT_FLOAT_EQ(r.getDistance(glm::vec3{ -1.0f, 1.0f, 1.0f }), 1.0f);
	}

	ASSERT_FLOAT_EQ(r.getDistance(TypeParam{}), 0.0f);
	ASSERT_FLOAT_EQ(r.getDistance(TypeParam{1.0f}), 0.0f);
}

TEST(Rect, VolumeCollision) {
	static constexpr auto size = glm::vec2(1.0f);

	Rect2D r{ size };
	Circle2D c{ glm::vec2(0.5f), 0.5f };

	ASSERT_TRUE(r.contains(c));
	ASSERT_LE(c.getDistance(r), 0.0);
	ASSERT_TRUE(c.overlaps(r));

	Rect2D o{ glm::vec2(0.5f), glm::vec2(1.5f) };

	ASSERT_FLOAT_EQ(o.getArea(), 1.0f);

	ASSERT_TRUE(r.overlaps(o));
	ASSERT_FALSE(r.contains(o));
	ASSERT_TRUE(o.overlaps(r));
	ASSERT_FALSE(o.contains(r));

	auto b = r.combine(o);

	ASSERT_FLOAT_EQ(b.getArea(), 1.5f * 1.5f);

	ASSERT_TRUE(b.contains(r));
	ASSERT_TRUE(b.contains(o));

	Rect2D x{ glm::vec2(2.0f), glm::vec2(3.0f) };

	ASSERT_FALSE(r.contains(x));
	ASSERT_FALSE(r.overlaps(x));
	ASSERT_FALSE(o.contains(x));
	ASSERT_FALSE(o.overlaps(x));
}