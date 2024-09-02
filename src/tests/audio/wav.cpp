#include "audio/wav.hpp"

#include <gtest/gtest.h>

#include "utility/stream.hpp"

using namespace sndx::audio;
using namespace sndx::utility;

uint8_t goodHeader[] =
	"RIFF"
	"\x32\x0\x0\x0"
	"WAVE"
	"fmt "
	"\x10\x0\x0\x0"
	"\x01\x0" // PCM int
	"\x01\x0" // 1 channel
	"\x44\xAC\x0\x0" // 44100 hz
	"\x0\x0\x0\x0"
	"\x0\x0"
	"\x08\x0" // 8bit
	"data"
	"\x0A\x0\x0\x0" // 10 bytes
	"\x0\x1\x2\x3\x4"
	"\x5\x6\x7\x8\x9";

TEST(WAVE, GoodHeader) {
	MemoryIStream buf(goodHeader, sizeof(goodHeader));

	WAVdecoder dec(buf);

	auto data = dec.readBytes(10);

	ASSERT_EQ(data.size(), 10);

	for (size_t i = 0; i < data.size(); ++i) {
		ASSERT_EQ(uint8_t(data[i]), i);
	}

	ASSERT_TRUE(dec.done());

	dec.seek(2);

	data = dec.readBytes(2);
	
	ASSERT_EQ(data.size(), 2);
	ASSERT_EQ(uint8_t(data[0]), 2);
	ASSERT_EQ(uint8_t(data[1]), 3);

	ASSERT_FALSE(dec.done());

	dec.seek(10);
	
	ASSERT_TRUE(dec.done());
}

TEST(WAVE, fullDeserialize) {
	MemoryIStream buf(goodHeader, sizeof(goodHeader));

	sndx::serialize::Deserializer deserializer(buf);

	WAVfile out;
	out.deserialize(deserializer);

	const auto& data = out.getData().data;

	ASSERT_EQ(data.size(), 10);

	for (size_t i = 0; i < data.size(); ++i) {
		ASSERT_EQ(data[i], i);
	}

	const auto& format = out.getFormat();

	ASSERT_EQ(format.channels, 1);
	ASSERT_EQ(format.bitDepth, 8);
	ASSERT_EQ(format.format, WAVE_PCM_INT);
	ASSERT_TRUE(std::holds_alternative<FMTchunk::ExtendedNone>(format.ext));
}

TEST(WAVE, lazyDeserialize) {
	MemoryIStream buf(goodHeader, sizeof(goodHeader));

	sndx::serialize::Deserializer deserializer(buf);

	sndx::RIFF::LazyFile out;
	out.deserialize(deserializer);

	auto dataChunkPtr = out.getChunk<DATAchunk>(deserializer);
	ASSERT_NE(dataChunkPtr, nullptr);
	
	const auto& data = dataChunkPtr->data;

	ASSERT_EQ(data.size(), 10);

	for (size_t i = 0; i < data.size(); ++i) {
		ASSERT_EQ(data[i], i);
	}

	auto fmtChunkPtr = out.getChunk<FMTchunk>(deserializer);
	ASSERT_NE(fmtChunkPtr, nullptr);

	const auto& format = *fmtChunkPtr;

	ASSERT_EQ(format.channels, 1);
	ASSERT_EQ(format.bitDepth, 8);
	ASSERT_EQ(format.format, WAVE_PCM_INT);
	ASSERT_TRUE(std::holds_alternative<FMTchunk::ExtendedNone>(format.ext));
}