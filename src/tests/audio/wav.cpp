#include "audio/wav.hpp"

#include <gtest/gtest.h>

#include "utility/stream.hpp"

using namespace sndx::audio;
using namespace sndx::utility;

uint8_t goodHeader[] =
	"RIFF"
	"\x2E\x0\x0\x0"
	"WAVE"
	"fmt "
	"\x10\x0\x0\x0"
	"\x01\x0" // PCM int
	"\x01\x0" // 1 channel
	"\x44\xAC\x0\x0" // 44100 hz
	"\x44\xAC\x0\x0" // avgBytesPerSec
	"\x01\x0" // blockAlign
	"\x08\x0" // 8bit
	"data"
	"\x0A\x0\x0\x0" // 10 bytes
	"\x0\x1\x2\x3\x4"
	"\x5\x6\x7\x8\x9";

TEST(WAVE, GoodHeader) {
	ASSERT_TRUE(_wavDecoderRegisterer);

	MemoryStream buf(goodHeader, sizeof(goodHeader) - 1);

	WAVdecoder dec(buf);

	auto data = dec.readRawBytes(10);

	ASSERT_EQ(data.size(), 10);

	for (size_t i = 0; i < data.size(); ++i) {
		ASSERT_EQ(uint8_t(data[i]), i);
	}

	ASSERT_TRUE(dec.done());

	dec.seek(2);

	data = dec.readRawBytes(2);
	
	ASSERT_EQ(data.size(), 2);
	EXPECT_EQ(uint8_t(data[0]), 2);
	EXPECT_EQ(uint8_t(data[1]), 3);

	EXPECT_FALSE(dec.done());

	dec.seek(10);
	
	EXPECT_TRUE(dec.done());
}

TEST(WAVE, fullDeserialize) {
	auto it = goodHeader;
	
	WAVfile out;
	out.deserialize(it, it + sizeof(goodHeader) - 1);

	const auto& data = out.getData().data;

	ASSERT_EQ(data.size(), 10);

	for (size_t i = 0; i < data.size(); ++i) {
		EXPECT_EQ(data[i], i);
	}

	const auto& format = out.getFormat();

	EXPECT_EQ(format.channels, 1);
	EXPECT_EQ(format.bitDepth, 8);
	EXPECT_EQ(format.format, WAVE_PCM_INT);
	EXPECT_TRUE(std::holds_alternative<FMTchunk::ExtendedNone>(format.ext));

	std::vector<uint8_t> outData{};
	outData.reserve(sizeof(goodHeader) - 1);

	auto i = std::back_inserter(outData);
	out.serialize(i);

	ASSERT_EQ(outData.size(), sizeof(goodHeader) - 1);
	for (size_t i = 0; i < outData.size(); ++i) {
		EXPECT_EQ(unsigned char(goodHeader[i]), outData[i]);
	}
}

uint8_t badHeaderWAVE[] =
	"RIFF"
	"\x2E\x0\x0\x0"
	"WAVe" // WAVE incorrect
	"fmt "
	"\x10\x0\x0\x0"
	"\x01\x0" // PCM int
	"\x01\x0" // 1 channel
	"\x44\xAC\x0\x0" // 44100 hz
	"\x44\xAC\x0\x0" // avgBytesPerSec
	"\x01\x0" // blockAlign
	"\x08\x0" // 8bit
	"data"
	"\x0A\x0\x0\x0" // 10 bytes
	"\x0\x1\x2\x3\x4"
	"\x5\x6\x7\x8\x9";

uint8_t badHeaderFMT[] =
	"RIFF"
	"\x2E\x0\x0\x0"
	"WAVE"
	"fmt "
	"\x11\x0\x0\x0" //fmt size incorrect
	"\x01\x0" // PCM int
	"\x01\x0" // 1 channel
	"\x44\xAC\x0\x0" // 44100 hz
	"\x44\xAC\x0\x0" // avgBytesPerSec
	"\x01\x0" // blockAlign
	"\x08\x0" // 8bit
	"data"
	"\x0A\x0\x0\x0" // 10 bytes
	"\x0\x1\x2\x3\x4"
	"\x5\x6\x7\x8\x9";

TEST(WAVE, badHeaderWAVE) {
	auto it = badHeaderWAVE;

	WAVfile file;
	ASSERT_THROW(file.deserialize(it, badHeaderWAVE + sizeof(badHeaderWAVE) - 1), sndx::bad_field_error);
}

TEST(WAVE, badHeaderFMT) {
	auto it = badHeaderFMT;

	WAVfile file;
	ASSERT_THROW(file.deserialize(it, badHeaderFMT + sizeof(badHeaderFMT) - 1), sndx::bad_field_error);
}