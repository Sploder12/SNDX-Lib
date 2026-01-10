#define MINIMP3_IMPLEMENTATION
#include "audio/mp3.hpp"

#include <gtest/gtest.h>

#include <fstream>

using namespace sndx::audio;
using namespace sndx::utility;

TEST(MP3, GoodFile) {
	std::ifstream file{ "test_data/audio/good.mp3", std::ios_base::binary };

	ASSERT_TRUE(file.is_open());

	MP3decoder dec{ file };
	auto data = dec.readSamples(std::numeric_limits<size_t>::max());
	
	EXPECT_EQ(data.byteSize(), 64512);
	EXPECT_EQ(data.frequency(), 44100);
	EXPECT_EQ(data.channels(), 1);
	EXPECT_EQ(data.totalSamples(), 32256);
}