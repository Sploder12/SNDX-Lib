#include "util/datafile.hpp"
#include "util/logging.hpp"
#include "util/stringmanip.hpp"
#include "util/window.hpp"

#include "render/vbo.hpp"
#include "render/vao.hpp"

using namespace sndx;

int main() {

	VBO<VboLayout<int, int>> a{};

	VAO b;
	b.bindVBO(a);

	return 0;
}