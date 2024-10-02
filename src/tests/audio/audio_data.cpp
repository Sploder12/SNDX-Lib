#include "audio/al/audio_data.hpp"

#include <gtest/gtest.h>

using namespace sndx::audio;

TEST(AL_Audio_Data, AudioData) {
	ALaudioData data{ ALaudioMeta{}, std::vector<std::byte>{
		(std::byte)0x0, (std::byte)0xffu, (std::byte)0xffu,
	} };

	ASSERT_EQ(data.getSampleCount(), 3);
	ASSERT_EQ(data.getByteSize(), 3);

	ASSERT_EQ(data.getSample(0, 0), 0x0);
	ASSERT_EQ(data.getSample(1, 0), 0xff);
	ASSERT_EQ(data.getSample(2, 0), 0xff);

	long double v = 0.0l;
	ASSERT_THROW(v = data.getSample(3, 0), std::out_of_range);

	auto conv = data.asFormat(ALformat::stereo16);

	ASSERT_EQ(conv.getSampleCount(), 3);
	ASSERT_EQ(conv.getByteSize(), 3 * 2 * 2);

	ASSERT_EQ(conv.getSample(0, 0), -32768);
	ASSERT_EQ(conv.getSample(1, 0), 32767);
	ASSERT_EQ(conv.getSample(2, 0), 32767);
	ASSERT_EQ(conv.getSample(0, 1), -32768);
	ASSERT_EQ(conv.getSample(1, 1), 32767);
	ASSERT_EQ(conv.getSample(2, 1), 32767);

	conv.setSample(2, 0, -100);
	conv.setSample(2, 1, 100);

	ASSERT_EQ(conv.getSample(2, 0), -100);
	ASSERT_EQ(conv.getSample(2, 1), 100);

	ASSERT_THROW(v = conv.getSample(0, 2), std::out_of_range);

	ASSERT_THROW(conv.setSample(0, 0, 32768.0l), std::domain_error);
	ASSERT_THROW(conv.setSample(0, 0, -32768.1l), std::domain_error);

	auto back = conv.asFormat(ALformat::mono8);

	ASSERT_EQ(back.getSampleCount(), 3);
	ASSERT_EQ(back.getSample(0, 0), 0x0);
	ASSERT_EQ(back.getSample(1, 0), 0xff);
	ASSERT_EQ(back.getSample(2, 0), 128);

	ASSERT_THROW(back.setSample(0, 0, 256.0l), std::domain_error);
	ASSERT_THROW(back.setSample(0, 0, -0.1l), std::domain_error);
}