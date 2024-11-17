#include "render/image/imagedata.hpp"
#include <array>

#include "../../common.hpp"

using namespace sndx::render;

constexpr std::array<std::byte, 12> testArray {
	std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff},
	std::byte{0xff}, std::byte{0x0},  std::byte{0x0},  std::byte{0xff},
	std::byte{0x0},  std::byte{0x0},  std::byte{0xff}, std::byte{0x0}
};

constexpr std::span testData{ testArray };

TEST(ImageDataTest, InvalidChannelsThrows) {
	EXPECT_THROW(ImageData(4, 3, 0, testData), std::invalid_argument);
	EXPECT_THROW(ImageData(1, 3, 5, testData), std::invalid_argument);
}

TEST(ImageDataTest, InvalidSizeThrows) {
	EXPECT_THROW(ImageData(13, 1, 1, testData), std::domain_error);
	EXPECT_THROW(ImageData(1, 13, 1, testData), std::domain_error);

	EXPECT_THROW(ImageData(7, 1, 2, testData), std::domain_error);
	EXPECT_THROW(ImageData(1, 7, 2, testData), std::domain_error);

	EXPECT_THROW(ImageData(5, 1, 3, testData), std::domain_error);
	EXPECT_THROW(ImageData(1, 5, 3, testData), std::domain_error);

	EXPECT_THROW(ImageData(4, 1, 4, testData), std::domain_error);
	EXPECT_THROW(ImageData(1, 4, 4, testData), std::domain_error);
}

TEST(ImageDataTest, AtOutOfBounds) {
	auto data = ImageData(12, 1, 1, testData);

	EXPECT_THROW([[maybe_unused]] auto _ = data.at(13, 0, 0), std::domain_error);
	EXPECT_THROW([[maybe_unused]] auto _ = data.at(0, 1, 0), std::domain_error);
	EXPECT_THROW([[maybe_unused]] auto _ = data.at(0, 0, 1), std::domain_error);

	const auto& cdata = data;
	EXPECT_THROW([[maybe_unused]] auto _ = cdata.at(13, 0, 0), std::domain_error);
	EXPECT_THROW([[maybe_unused]] auto _ = cdata.at(0, 1, 0), std::domain_error);
	EXPECT_THROW([[maybe_unused]] auto _ = cdata.at(0, 0, 1), std::domain_error);

	EXPECT_THROW([[maybe_unused]] auto _ = cdata.at<2>(0, 0), std::invalid_argument);

	EXPECT_THROW([[maybe_unused]] auto _ = cdata.at<1>(13, 0), std::domain_error);
	EXPECT_THROW([[maybe_unused]] auto _ = cdata.at<1>(0, 1), std::domain_error);
}

TEST(ImageDataTest, SpanCopied) {
	auto data = ImageData(12, 1, 1, testData);

	EXPECT_EQ(data.channels(), 1);
	EXPECT_EQ(data.width(), 12);
	EXPECT_EQ(data.height(), 1);
	EXPECT_EQ(data.pixels(), 12);
	EXPECT_EQ(data.bytes(), 12);

	for (size_t i = 0; i < testData.size(); ++i) {
		EXPECT_EQ(data.at(i, 0, 0), testData[i]);
	}
}

TEST(ImageDataTest, GrayscalesRGBA) {
	auto data = ImageData(3, 1, 4, testData);

	auto gray = data.asGrayscale();

	EXPECT_EQ(data.width(), gray.width());
	EXPECT_EQ(data.height(), gray.height());
	EXPECT_EQ(data.pixels(), gray.pixels());
	EXPECT_EQ(gray.channels(), 1);
	EXPECT_EQ(gray.bytes(), 3);

	EXPECT_EQ(gray.at(0, 0, 0), std::byte(0xff));
	EXPECT_EQ(gray.at(1, 0, 0), std::byte(0xff / 3));
	EXPECT_EQ(gray.at(2, 0, 0), std::byte(0xff / 3));
}

TEST(ImageDataTest, GrayscalesRGB) {
	auto data = ImageData(2, 2, 3, testData);

	auto gray = data.asGrayscale();

	EXPECT_EQ(data.width(), gray.width());
	EXPECT_EQ(data.height(), gray.height());
	EXPECT_EQ(data.pixels(), gray.pixels());
	EXPECT_EQ(gray.channels(), 1);
	EXPECT_EQ(gray.bytes(), 4);

	EXPECT_EQ(gray.at(0, 0, 0), std::byte(0xff));
	EXPECT_EQ(gray.at(1, 0, 0), std::byte((0xff + 0xff) / 3));
	EXPECT_EQ(gray.at(0, 1, 0), std::byte(0xff / 3));
	EXPECT_EQ(gray.at(1, 1, 0), std::byte(0xff / 3));
}

TEST(ImageDataTest, Transforms) {
	auto data = ImageData(3, 1, 4, testData);

	auto gbr = data.transform(glm::mat3x4{
		0.0, 0.0, 1.0, 0.0,
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0
	});

	EXPECT_EQ(data.width(), gbr.width());
	EXPECT_EQ(data.height(), gbr.height());
	EXPECT_EQ(data.pixels(), gbr.pixels());
	EXPECT_EQ(gbr.channels(), 3);
	EXPECT_EQ(gbr.bytes(), 9);

	using Vec = glm::vec<3, std::byte>;

	EXPECT_EQ(gbr.at<3>(0, 0), (Vec{ 255, 255, 255 }));
	EXPECT_EQ(gbr.at<3>(1, 0), (Vec{ 0, 255, 0 }));
	EXPECT_EQ(gbr.at<3>(2, 0), (Vec{ 255, 0, 0 }));
}

TEST(ImageDataTest, InvalidTransformThrows) {
	auto data = ImageData(3, 1, 4, testData);

	EXPECT_THROW([[maybe_unused]] auto _ = data.transform(glm::vec1(1.0)), std::invalid_argument);
	EXPECT_THROW([[maybe_unused]] auto _ = data.transform(glm::vec2(1.0)), std::invalid_argument);
	EXPECT_THROW([[maybe_unused]] auto _ = data.transform(glm::vec3(1.0)), std::invalid_argument);

	EXPECT_THROW([[maybe_unused]] auto _ = data.transform(glm::mat4x3(1.0)), std::invalid_argument);
	EXPECT_THROW([[maybe_unused]] auto _ = data.transform(glm::mat3x3(1.0)), std::invalid_argument);
	EXPECT_THROW([[maybe_unused]] auto _ = data.transform(glm::mat2x3(1.0)), std::invalid_argument);

	EXPECT_THROW([[maybe_unused]] auto _ = data.transform(glm::mat4x2(1.0)), std::invalid_argument);
	EXPECT_THROW([[maybe_unused]] auto _ = data.transform(glm::mat3x2(1.0)), std::invalid_argument);
	EXPECT_THROW([[maybe_unused]] auto _ = data.transform(glm::mat2x2(1.0)), std::invalid_argument);
}