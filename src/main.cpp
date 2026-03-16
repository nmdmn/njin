#include "config.h"
#include <exception>
#include <stdexcept>
#include <vulkan/vulkan.h>
#include <iostream>
#include <cstdlib>
#include <sstream>

class HelloTriangleApplication {
public:
  void run() {
    initVulkan();
    mainLoop();
    cleanup();
  }

private:
  void initVulkan() { throw std::invalid_argument("Some error occured ~!@#$%^&*()_+"); }

  void mainLoop() {}

  void cleanup() {}
};

int main() {
  std::stringstream ss;
  ss << NJIN_NAME << " v" << NJIN_VERSION;
  std::cout << ss.str() << std::endl;

  HelloTriangleApplication app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
