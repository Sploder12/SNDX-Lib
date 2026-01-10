#include "audio/audiodata.hpp"

#include <gtest/gtest.h>

using namespace sndx::audio;

TEST(Audio_Data, AudioData) {
	AudioData data{ 1, 1, std::vector<uint8_t>{
		0x0u, 0xffu, 0xffu,
	} };

	ASSERT_EQ(data.totalSamples(), 3);
	ASSERT_EQ(data.byteSize(), 3);

	EXPECT_EQ(data.getSample(0, 0), 0x0);
	EXPECT_EQ(data.getSample(1, 0), 0xff);
	EXPECT_EQ(data.getSample(2, 0), 0xff);

	[[maybe_unused]] uint8_t v{};
	ASSERT_THROW(v = data.getSample(3, 0), std::out_of_range);

	data.setSample(2, 0, 10);

	EXPECT_EQ(data.getSample(2, 0), 10);
	ASSERT_THROW(v = data.getSample(0, 1), std::out_of_range);
}

TEST(Audio_Data, self_conversion_does_nothing) {
	AudioData data{ 2, 44100, std::vector<uint8_t>{
		0x02u, 0xffu, 0xffu, 0x12u, 0xeau, 0x14
	} };

	// copy is intentional
	auto converted = convert<uint8_t>(data);

	EXPECT_EQ(data.frequency(), converted.frequency());
	EXPECT_EQ(data.channels(), converted.channels());

	EXPECT_EQ(data.byteSize(), converted.byteSize());

	ASSERT_EQ(data.totalSamples(), converted.totalSamples());
	for (size_t i = 0; i < data.totalSamples(); ++i) {
		EXPECT_EQ(data.data()[i], converted.data()[i]);
	}
}

TEST(Audio_Data, mono_conversion_works) {
	AudioData data{ 2, 44100, std::vector<uint8_t>{
		0x02u, 0xffu, 0xffu, 0x12u, 0xeau, 0x14
	} };

	auto converted = asMono(data);

	EXPECT_EQ(data.frequency(), converted.frequency());
	EXPECT_NE(data.channels(), converted.channels());
	EXPECT_EQ(converted.channels(), 1);

	EXPECT_EQ(data.byteSize() / 2, converted.byteSize());

	ASSERT_EQ(data.sampleFrames(), converted.sampleFrames());
	for (size_t i = 0; i < data.sampleFrames(); ++i) {
		auto stereo = std::midpoint(data.getSample(i, 0), data.getSample(i, 1));
		EXPECT_EQ(stereo, converted.getSample(i, 0));
	}
}

TEST(Audio_Data, float_conversion_works) {
	AudioData data{ 2, 44100, std::vector<uint8_t>{
		0x00u, 0xffu, 0xffu, 127u, 63u, 167u
	} };

	auto converted = convert<float>(data);

	EXPECT_EQ(data.frequency(), converted.frequency());
	EXPECT_EQ(data.channels(), converted.channels());

	EXPECT_EQ(data.byteSize() * sizeof(float), converted.byteSize());

	ASSERT_EQ(data.sampleFrames(), converted.sampleFrames());
	
	EXPECT_FLOAT_EQ(converted.getSample(0, 0), -1.0f);
	EXPECT_FLOAT_EQ(converted.getSample(0, 1), 1.0f);
	EXPECT_FLOAT_EQ(converted.getSample(1, 0), 1.0f);
	EXPECT_FLOAT_EQ(converted.getSample(1, 1), 0.0f);
	EXPECT_NEAR(converted.getSample(2, 0), -0.5f, 0.005f);
	EXPECT_NEAR(converted.getSample(2, 1), 0.3125f, 0.005f);
}

TEST(Audio_Data, int_conversion_works) {
	AudioData data{ 2, 44100, std::vector<uint8_t>{
		0x00u, 0xffu, 0xffu, 127u, 63u, 167u
	} };

	auto converted = convert<int16_t>(data);

	EXPECT_EQ(data.frequency(), converted.frequency());
	EXPECT_EQ(data.channels(), converted.channels());

	EXPECT_EQ(data.byteSize() * sizeof(int16_t), converted.byteSize());

	ASSERT_EQ(data.sampleFrames(), converted.sampleFrames());

	EXPECT_EQ(converted.getSample(0, 0), std::numeric_limits<int16_t>::min());
	EXPECT_EQ(converted.getSample(0, 1), std::numeric_limits<int16_t>::max());
	EXPECT_EQ(converted.getSample(1, 0), std::numeric_limits<int16_t>::max());
	EXPECT_EQ(converted.getSample(1, 1), 0);
	EXPECT_NEAR(converted.getSample(2, 0), -16513, 128);
	EXPECT_NEAR(converted.getSample(2, 1), 10239, 128);
}