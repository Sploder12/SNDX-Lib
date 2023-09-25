#include "util/logging.hpp"
#include "util/stringmanip.hpp"
#include "util/window.hpp"
#include "util/lines.hpp"

#include "audio/alcontext.hpp"
#include "audio/signal.hpp"

#include "render/texture.hpp"
#include "render/shader.hpp"
#include "render/font.hpp"

#include "3d/model.hpp"
#include "3d/camera.hpp"

#include "imgui/textureview.hpp"
#include "imgui/datatreeview.hpp"

#include "data.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

using namespace sndx;

Window win;

/*
ALContext alcontext{};

void makeNoise() {
	auto d = getAlDevices();

	alcontext.bind();

	auto testmp3 = loadAudioFile("tmp/test.mp3").value();
	testmp3.normalize();

	auto testbuf = alcontext.createBuffer("test", testmp3);

	testmp3.destroy();

	auto testsrc = alcontext.createSource("test");

	alcontext.setVolume(0.3f);

	testsrc.setPos(glm::vec3(0.0));
	testsrc.setGain(0.1f).setParam(AL_ROLLOFF_FACTOR, 0.0f).setParam(AL_SOURCE_RELATIVE, AL_TRUE)
		.setParam(AL_LOOPING, AL_TRUE).setSpeed(1.0f)
		.setBuffer(testbuf).play();
}*/

void window_size_callback(GLFWwindow* window, int width, int height) {
	win.resize(width, height);
	win.resetViewport();
}

void doThing() {
	auto dataO = decodeData<JSONdecoder>("tmp/in.json");
	if (dataO.has_value()) {
		auto data = std::move(dataO.value());

		size_t n = 0;
		auto nptr = data.get("n");
		if (nptr && nptr->holdsAlternative<Primitive>()) {
			if (std::get<Primitive>(nptr->data).holdsAlternative<int64_t>()) {
				n = std::get<int64_t>(std::get<Primitive>(nptr->data).data);
			}
		}

		std::vector<glm::vec2> points{};
		points.reserve(n);

		auto collectionptr = data.get("coords");
		if (collectionptr && collectionptr->holdsAlternative<DataArray>()) {
			auto& coords = std::get<DataArray>(collectionptr->data);
			for (auto& val : coords) {
				auto xptr = val.get("x");

				long double x = 0.0;
				if (xptr && xptr->holdsAlternative<Primitive>()) {
					if (std::get<Primitive>(xptr->data).holdsAlternative<long double>()) {
						x = std::get<long double>(std::get<Primitive>(nptr->data).data);
					}
				}

				auto yptr = val.get("y");

				long double y = 0.0;
				if (yptr && yptr->holdsAlternative<Primitive>()) {
					if (std::get<Primitive>(yptr->data).holdsAlternative<long double>()) {
						y = std::get<long double>(std::get<Primitive>(yptr->data).data);
					}
				}

				points.emplace_back(x, y);
			}
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

					auto index = size_t((floor(pos.x) + floor(pos.y) * tmp.width) * tmp.channels);

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
							int index = int((floor(pos.x) + floor(pos.y) * tmp.width) * tmp.channels);
							tmp.data[index + 1] = 255;
						}
					}
				}
			}
		}

		tmp.asGrayScale().save("tmp/out.jpg");
	}
}

bool firstMouse = true;
float lastX;
float lastY;
glm::mat4 mdl = glm::mat4(1.0f);
float yrot = 0.0f;
float xrot = 0.0f;
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;
	
	yrot += yoffset * 0.01f;
	xrot += xoffset * 0.01f;
}

int main() {

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	win = createWindow(600, 600, "window");
	
	glfwMakeContextCurrent(win.window);

	glfwSetCursorPosCallback(win.window, mouse_callback);
	glfwSetWindowSizeCallback(win.window, window_size_callback);

	glewInit();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(win, true);
	ImGui_ImplOpenGL3_Init("#version 330 core");

	glEnable(GL_DEPTH_TEST);

	//doThing();

	FreetypeContext FT_context{};
	auto font = loadFont(FT_context, "tmp/NotoSans-Regular.ttf", false).value();
	font.atlas.save("notoSans");

	ShaderProgram shdr = programFromFiles("tmp/model.vs", "tmp/model.fs").value();

	auto m = loadModelFromFile("tmp/duck.obj");
	if (!m.has_value()) return 1;

	Model model = m.value();

	

	shdr.use();

	glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

	shdr.uniform("projection", projection);
	shdr.uniform("view", glm::mat4(1.0f));

	auto data1 = decodeData<JSONdecoder>("tmp/in.json").value();

	//makeNoise();

	auto it = font.atlas.entries.begin();

	while (!glfwWindowShouldClose(win.window)) {
	

		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		

		
		mdl = glm::mat4(1.0);
		mdl = glm::translate(mdl, glm::vec3(0.0, -2.0, -7.0));
		mdl = glm::rotate(mdl, yrot, glm::vec3(cos(xrot), 0.0, sin(xrot)));
		mdl = glm::rotate(mdl, xrot, glm::vec3(0.0, -1.0, 0.0));
		
		shdr.uniform("model", mdl);
		


		model.onEachMesh([](Mesh<Vertex, glm::vec3, glm::vec3, glm::vec2>& mesh) {
			mesh.bind();
			glDrawElements(GL_TRIANGLES, GLsizei(mesh.indices.size()), GL_UNSIGNED_INT, 0);
		});

		ImGuiNewFrame();

		AtlasView(font.atlas, "Atlas", it);

		DataTreeView(data1, "Atlas Tree");

		ImGuiEndFrame();

		glfwSwapBuffers(win.window);
		glfwPollEvents();

	}

	ImGuiTerminate();

	//model.destroy();
	glfwTerminate();
	return 0;
}