#include "audio/vorbis.hpp"

#include <gtest/gtest.h>

#include <fstream>

using namespace sndx::audio;
using namespace sndx::utility;

TEST(Vorbis, GoodFile) {
	std::ifstream file{ "test_data/audio/good.ogg", std::ios_base::binary };
	
	ASSERT_TRUE(file.is_open());

	VorbisDecoder dec{ file };
	auto data = dec.readSamples(std::numeric_limits<size_t>::max());

	EXPECT_EQ(data.frequency(), 44100);
	EXPECT_EQ(data.channels(), 1);
	EXPECT_EQ(data.totalSamples(), 49856);
}