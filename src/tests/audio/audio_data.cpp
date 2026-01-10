#include "audio/audio_data.hpp"

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