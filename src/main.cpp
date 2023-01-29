#include "util/datafile.hpp"
#include "util/logging.hpp"
#include "util/stringmanip.hpp"
#include "util/window.hpp"
#include "util/lines.hpp"

#include "audio/alcontext.hpp"

#include "render/texture.hpp"

#include "3d/model.hpp"

using namespace sndx;

void doThing() {
	auto d = getAlDevices();

	ALContext alcontext{};
	alcontext.bind();

	auto testmp3 = loadAudioFile("test.mp3").value();
	testmp3.normalize();

	auto testbuf = alcontext.createBuffer("test", testmp3);

	testmp3.destroy();

	auto testsrc = alcontext.createSource("test");

	alcontext.setVolume(0.3f);

	testsrc.setPos(glm::vec3(0.0));
	testsrc.setParam(AL_ROLLOFF_FACTOR, 0.0f);
	testsrc.setParam(AL_SOURCE_RELATIVE, AL_TRUE);
	testsrc.setParam(AL_LOOPING, AL_TRUE);
	testsrc.setSpeed(1.0f);

	testsrc.setBuffer(testbuf);
	testsrc.play();

	while (testsrc.playing()) {
		

		auto dataO = loadDataTree("in.json", LayoutJSON<char>);
		if (dataO.has_value()) {
			auto data = std::move(dataO.value());
			
			auto n = data.getOrElse<int>("n", '.', 0, [](const std::string& in) {return std::stoi(in); });

			std::vector<glm::vec2> points{};
			points.reserve(n);

			for (int i = 0; i < n; ++i) {
				auto x = data.getOrElse<float>(std::to_string(i) + ".x", '.', 0.0f, [](const std::string& in) { return std::stof(in); });
				auto y = data.getOrElse<float>(std::to_string(i) + ".y", '.', 0.0f, [](const std::string& in) { return std::stof(in); });

				points.emplace_back(x, y);
			}

			ImageData tmp{};
			tmp.width = 1000;
			tmp.height = 1000;
			tmp.channels = 3;
			tmp.data.resize(tmp.width * tmp.height * tmp.channels);
				
			for (int y = 0; y < 10; ++y) {
				for (int x = 0; x < 1000; ++x) {
					int index = (x + (y * 100) * tmp.width) * tmp.channels;
					tmp.data[index] = 125;
					tmp.data[index] = 125;
					tmp.data[index] = 125;
				}
			}

			for (int x = 0; x < 10; ++x) {
				for (int y = 0; y < 1000; ++y) {
					int index = ((x * 100) + y * tmp.width) * tmp.channels;
					tmp.data[index] = 80;
					tmp.data[index] = 80;
					tmp.data[index] = 80;
				}
			}


			for (float t = 0.0f; t <= 1.0f; t += 0.0005f) {
				glm::vec2 res = bezier(t, points);

				for (int y = -1; y <= 1; ++y) {
					for (int x = -1; x <= 1; ++x) {
						glm::vec2 pos = res * 10.0f + glm::vec2(x, y);

						if (pos.x >= tmp.width || pos.x < 0.0f) continue;
						if (pos.y >= tmp.height || pos.y < 0.0f) continue;

						int index = (floor(pos.x) + floor(pos.y) * tmp.width) * tmp.channels;

						unsigned char color = 255u - abs(x) * 100u - abs(y) * 100u;
						tmp.data[index + 2] = std::max(color, tmp.data[index + 2]);
					}
				}
			}

			for (glm::vec2 point : points) {
				for (int y = -1; y <= 1; ++y) {
					for (int x = -1; x <= 1; ++x) {
						glm::vec2 pos = point * 10.0f + glm::vec2(x, y);

						if (pos.x < tmp.width && pos.x >= 0.0f) {
							if (pos.y < tmp.height && pos.y >= 0.0f) {
								int index = (floor(pos.x) + floor(pos.y) * tmp.width) * tmp.channels;
								tmp.data[index + 1] = 255;
							}
						}
					}
				}
			}

			tmp.asGrayScale().save("out.jpg");
		}
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
}

int main() {


	
	doThing();
	

	return 0;
}