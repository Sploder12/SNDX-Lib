#include "audio/vorbis.hpp"

#include <gtest/gtest.h>

#include <fstream>

using namespace sndx::audio;
using namespace sndx::utility;

TEST(Vorbis, GoodFile) {
	ASSERT_TRUE(_vorbisDecoderRegisterer);

	std::ifstream file{ "test_data/audio/good.ogg", std::ios_base::binary };
	
	ASSERT_TRUE(file.is_open());

	auto decptr = tryCreateDecoder(".ogg", file);

	ASSERT_TRUE(decptr);
	ASSERT_TRUE(dynamic_cast<VorbisDecoder*>(decptr.get()));

	auto* dec = dynamic_cast<VorbisDecoder*>(decptr.get());
	auto data = dec->readAll();

	EXPECT_EQ(data.getByteSize(), 99712);
	EXPECT_EQ(data.getFormat(), ALformat::mono16);
	EXPECT_EQ(data.getFrequency(), 44100);
	EXPECT_EQ(data.getChannels(), 1);
	EXPECT_EQ(data.getSampleCount(), 99712 / 2);
}