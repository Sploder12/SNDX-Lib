#include "util/datafile.hpp"
#include "util/logging.hpp"
#include "util/stringmanip.hpp"
#include "util/window.hpp"

#include "render/vbo.hpp"
#include "render/vao.hpp"
#include "render/shader.hpp"

using namespace sndx;

struct peoeple {
	int i;
	int y;
};

int main() {

	VBO<VboLayout<int, int>> a{};

	VAO b;
	b.bindVBO(a);

	std::vector<std::pair<int, int>> data1{ {1,2}, {3,5} };
	std::vector<peoeple> data2{ {1,2}, {3,5} };

	std::vector<std::vector<peoeple>> bbb{ { {1, 2}, { 2,2 }, {3,3}}};

	a.setData(bbb.begin(), bbb.end());

	return 0;
}