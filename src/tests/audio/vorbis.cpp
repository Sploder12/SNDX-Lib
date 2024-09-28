#include "audio/vorbis.hpp"

#include <gtest/gtest.h>

#include "utility/stream.hpp"

#include <fstream>


#include "audio/al_context.hpp"

using namespace sndx::audio;
using namespace sndx::utility;


TEST(Vorbis, GoodFile) {
	std::ifstream file("test_data/audio/good.ogg", std::ios::binary);

	/*
	VorbisDecoder dec(file);

	auto data = dec.readAll();

	ALcontext context{};
	context.bind();

	auto src = context.createSource("finger");

	auto buf = context.createBuffer("pingus", data);

	src->setBuffer(*buf);

	src->play();

	while (src->playing()) {

	}
	*/
}