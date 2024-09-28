#define MINIMP3_IMPLEMENTATION
#include "audio/mp3.hpp"

#include <gtest/gtest.h>

#include "utility/stream.hpp"

#include <fstream>

using namespace sndx::audio;
using namespace sndx::utility;

TEST(MP3, GoodFile) {
	std::ifstream file("test_data/audio/good.mp3", std::ios::binary);

	MP3decoder dec(file);

	auto data = dec.readAll();
	
	EXPECT_EQ(data.getByteSize(), 64512);
	EXPECT_EQ(data.getFormat(), ALformat::mono16);
	EXPECT_EQ(data.getFrequency(), 44100);
	EXPECT_EQ(data.getChannels(), 1);
	EXPECT_EQ(data.getSampleCount(), 64512 / 2);
}