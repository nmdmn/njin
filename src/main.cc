#include <iostream>
#include <thread>
#include <string>
#include <exception>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "config.hh"
#include "logger.hh"

// TODO Proper RAII! c-tor/d-tor usage - implement via unique_ptr/shared_ptr with custom deleter!

namespace njin {
class App {
public:
  void run() {
    auto &logger = Logger::instance();
    logger.info << NJIN_NAME << " v" << NJIN_VERSION;

    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }

private:
  void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(WIDTH, HEIGHT, NJIN_NAME, nullptr, nullptr);
  }

  void initVulkan() {}

  void mainLoop() {
    while (!glfwWindowShouldClose(window_)) {
      glfwPollEvents();
    }
  }

  void cleanup() {
    glfwDestroyWindow(window_);
    glfwTerminate();
  }

  static constexpr uint32_t WIDTH = 800;
  static constexpr uint32_t HEIGHT = 600;

  GLFWwindow *window_;
};
} // namespace njin

void test_logger() {
  auto &logger = njin::Logger::instance();
  auto worker = [&](int id) {
    for (int i = 0; i < 5; ++i)
      logger.trace << "thread " << std::to_string(id) << " msg " << std::to_string(i);
  };

  std::thread t1(worker, 1);
  std::thread t2(worker, 2);
  std::thread t3(worker, 3);

  t1.join();
  t2.join();
  t3.join();
}

int main() {
  njin::App app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
