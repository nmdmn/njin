#include "core.h"

#include <iostream>
#include <string>

namespace njin {
core::core() {
	constexpr std::string_view njin_name = NJIN_NAME;
	constexpr std::string_view njin_version = NJIN_VERSION;
	std::cout << std::string{njin_name} + " v" + std::string{njin_version} << std::endl;

	renderer renderer{njin_name, 800, 600};
	renderer.set_cursor(GLFW_CROSSHAIR_CURSOR);
	renderer.set_clear_color({.87f, .23f, .16f, 1.f});
	renderer.set_polygon_mode(GL_FRONT_AND_BACK, GL_FILL);
	std::cout << renderer;

	constexpr std::string_view njin_shader_folder = NJIN_SHADER_FOLDER;
	program program{njin_shader_folder.data() + std::string("default.vs"),
									njin_shader_folder.data() + std::string("default.fs")};
	std::cout << program;

	while (renderer.is_running()) {
		renderer.clear();
		renderer.handle_events();
		// TODO
		renderer.swap();
	}
}
} // namespace njin
