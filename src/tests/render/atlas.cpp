#include "render/atlas.hpp"
#include "render/gl/texture.hpp"

#include "render/image/stbimage.hpp"
#include "../common.hpp"

#include "fake_texture.hpp"
#include "../math/mock_binpack.hpp"
#include "../render/image/image_helper.hpp"

using namespace ::testing;

using namespace sndx::render;

const std::filesystem::path test_data_path{ u8"test_data/visual/rgbbw_test_img☃.png" };

class ImageAtlasTest : public ::testing::Test {};

TEST_F(ImageAtlasTest, emptyThrows) {
	AtlasBuilder<size_t> builder{};

	MockBinPacker<size_t> mockPacker{};

	sndx::math::Packing<size_t> out{};

	EXPECT_CALL(mockPacker, mock_constructor);
	EXPECT_CALL(mockPacker, mock_destructor);
	EXPECT_CALL(mockPacker, add).Times(0);
	EXPECT_CALL(mockPacker, pack(10, 5)).WillOnce(Return(out));

	EXPECT_THROW(auto ign = builder.build<ProxyBinPacker<size_t>>(10, 5), std::logic_error);
}

TEST_F(ImageAtlasTest, zeroWidthThrows) {
	AtlasBuilder<size_t> builder{};

	MockBinPacker<size_t> mockPacker{};

	sndx::math::Packing<size_t> out{};
	out.positions[0] = {};
	out.neededHeight = 20;
	out.neededWidth = 0;

	EXPECT_CALL(mockPacker, mock_constructor);
	EXPECT_CALL(mockPacker, mock_destructor);
	EXPECT_CALL(mockPacker, add).Times(0);
	EXPECT_CALL(mockPacker, pack(20, 10)).WillOnce(Return(out));

	EXPECT_THROW(auto ign = builder.build<ProxyBinPacker<size_t>>(20, 10), std::logic_error);
}

TEST_F(ImageAtlasTest, zeroHeightThrows) {
	AtlasBuilder<size_t> builder{};

	MockBinPacker<size_t> mockPacker{};

	sndx::math::Packing<size_t> out{};
	out.positions[0] = {};
	out.neededHeight = 0;
	out.neededWidth = 20;

	EXPECT_CALL(mockPacker, mock_constructor);
	EXPECT_CALL(mockPacker, mock_destructor);
	EXPECT_CALL(mockPacker, add).Times(0);
	EXPECT_CALL(mockPacker, pack(10, 20)).WillOnce(Return(out));

	EXPECT_THROW(auto ign = builder.build<ProxyBinPacker<size_t>>(10, 20), std::logic_error);
}

TEST_F(ImageAtlasTest, addsEntry) {
	AtlasBuilder<size_t> builder{};

	auto img = createCheckeredImage(3, 7, glm::vec<1, std::byte>{std::byte(0xff)});
	builder.add(42, img);

	MockBinPacker<size_t> mockPacker{};

	sndx::math::Packing<size_t> out{};
	out.positions[0] = {};
	out.neededWidth = 3;
	out.neededHeight = 7;

	EXPECT_CALL(mockPacker, mock_constructor);
	EXPECT_CALL(mockPacker, mock_destructor);
	EXPECT_CALL(mockPacker, add(0, 3, 7));
	EXPECT_CALL(mockPacker, pack(10, 5)).WillOnce(Return(out));

	auto atlas = builder.build<ProxyBinPacker<size_t>>(10, 5);

	EXPECT_EQ(atlas.size(), 1);
	EXPECT_NO_THROW(std::ignore = atlas.getEntry(42));
}

class IntenseImageAtlasTest : public ImageAtlasTest {
public:
	void SetUp() override {
		set_test_weight(TestWeight::BasicIntegration);
	}
};

TEST_F(IntenseImageAtlasTest, canBuildSingleImage) {
	AtlasBuilder<std::string> builder{};

	auto img = loadImageFile(test_data_path, 4, STBimageLoader{false});
	ASSERT_TRUE(img.has_value());

	builder.add("single", *img);

	auto atlas = builder.build(img->width(), 0);
	const auto& outImg = atlas.getImage();

	EXPECT_EQ(outImg.width(), 225);
	EXPECT_EQ(outImg.height(), 100);
	EXPECT_EQ(outImg.channels(), 4);

	EXPECT_EQ(atlas.getEntry("single").pos, (glm::vec<2, size_t>{0, 0}));
	EXPECT_EQ(atlas.getEntry("single").dims, (glm::vec<2, size_t>{225, 100}));
}

TEST_F(IntenseImageAtlasTest, canBuildMultiImage) {
	AtlasBuilder<int> builder{};

	auto imgRGB = loadImageFile(test_data_path, 3, STBimageLoader{ true });
	ASSERT_TRUE(imgRGB.has_value());

	auto imgR = loadImageFile(test_data_path, 1, STBimageLoader{ false });
	ASSERT_TRUE(imgR.has_value());

	builder.add(0, *imgRGB);
	builder.add(1, *imgR);
	builder.add(2, *imgR);
	builder.add(3, *imgR);

	auto atlas = builder.build(imgR->width() * 2 + 1, 1);
	const auto& outImg = atlas.getImage();

	EXPECT_EQ(outImg.channels(), 3);
	EXPECT_EQ(outImg.width(), 225 * 2 + 2);
	EXPECT_EQ(outImg.height(), 100 * 2 + 2);

	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			if (i == j) continue;

			EXPECT_NE(atlas.getEntry(int(i)).pos, atlas.getEntry(int(j)).pos);
		}
	}

	EXPECT_EQ(atlas.getEntry(0).dims, (glm::vec<2, size_t>{225, 100}));
	EXPECT_EQ(atlas.getEntry(1).dims, (glm::vec<2, size_t>{225, 100}));
	EXPECT_EQ(atlas.getEntry(2).dims, (glm::vec<2, size_t>{225, 100}));
	EXPECT_EQ(atlas.getEntry(3).dims, (glm::vec<2, size_t>{225, 100}));
}

class TextureAtlasTest : public ::testing::Test {
public:
	void SetUp() override {
		set_test_weight(TestWeight::BasicIntegration)
	}
};

TEST_F(TextureAtlasTest, BasicTextureAtlas) {
	AtlasBuilder<std::string> builder{};

	auto img = loadImageFile(test_data_path, 4, STBimageLoader{ false });
	ASSERT_TRUE(img.has_value());

	builder.add("single", *img);

	auto atlas = builder.buildTexture<FakeTexture>(img->width(), 0);
	const auto& texture = atlas.getTexture();

	EXPECT_EQ(texture.width(), img->width());
	EXPECT_EQ(texture.height(), img->height());

	const auto& asImg = texture.asImage(img->channels(), 0);

	ASSERT_EQ(asImg.bytes(), img->bytes());

	for (size_t i = 0; i < asImg.bytes(); ++i) {
		EXPECT_EQ(asImg.data()[i], img->data()[i]);
	}
}