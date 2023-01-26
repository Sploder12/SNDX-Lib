#include "util/datafile.hpp"
#include "util/logging.hpp"
#include "util/stringmanip.hpp"
#include "util/window.hpp"

#include "audio/alcontext.hpp"

using namespace sndx;
int main() {

	

	auto data = loadDataTree("in.json", LayoutJSON<char>);

	std::cout << *data.getData("pizza.cheese", '.') << '\n';

	auto sauceNode = data.add("NONE", "pizza.toppings.0", '.');

	std::cout << *data.getData("pizza.toppings.0", '.') << '\n';

	data.save("out.sndx");

	auto d = getAlDevices();

	ALContext alcontext{};
	alcontext.bind();

	auto testmp3 = loadMP3("test.mp3");

	auto testbuf = alcontext.createBuffer("test", testmp3);

	testmp3.destroy();

	auto testsrc = alcontext.createSource("test");

	alcontext.setVolume(0.3f);

	testsrc.setPos(glm::vec3(0.0));
	testsrc.setParam(AL_ROLLOFF_FACTOR, 0.0f);
	testsrc.setParam(AL_SOURCE_RELATIVE, (int)AL_TRUE);
	testsrc.setParam(AL_LOOPING, (int)AL_TRUE);
	testsrc.setSpeed(1.2f);

	testsrc.setBuffer(testbuf);
	testsrc.play();
	
	while (testsrc.playing()) {
		std::this_thread::sleep_for(testbuf.lengthSeconds() / 1.2f);
	}

	return 0;
}