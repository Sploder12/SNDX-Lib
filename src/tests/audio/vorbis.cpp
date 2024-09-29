#include "audio/vorbis.hpp"

#include <gtest/gtest.h>

#include "utility/stream.hpp"

#include <fstream>


#include "audio/al_context.hpp"

using namespace sndx::audio;
using namespace sndx::utility;


TEST(Vorbis, GoodFile) {
	std::ifstream file("test_data/audio/good.ogg", std::ios::binary);

	ASSERT_TRUE(file.is_open());
	
	VorbisDecoder dec(file);

	auto data = dec.readAll();

	EXPECT_EQ(data.getByteSize(), 99712);
	EXPECT_EQ(data.getFormat(), ALformat::mono16);
	EXPECT_EQ(data.getFrequency(), 44100);
	EXPECT_EQ(data.getChannels(), 1);
	EXPECT_EQ(data.getSampleCount(), 99712 / 2);
}