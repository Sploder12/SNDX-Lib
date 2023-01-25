#include "util/datafile.hpp"
#include "util/logging.hpp"
#include "util/stringmanip.hpp"
#include "util/window.hpp"

using namespace sndx;
int main() {

	

	auto data = loadDataTree("in.json", LayoutJSON<char>);

	std::cout << *data.getData("pizza.cheese", '.') << '\n';

	auto sauceNode = data.add("NONE", "pizza.toppings.0", '.');

	std::cout << *data.getData("pizza.toppings.0", '.') << '\n';

	data.save("out.sndx");

	return 0;
}