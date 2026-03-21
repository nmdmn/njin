#include <iostream>
#include <exception>
#include <vulkan/vulkan_core.h>

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

  void initVulkan() {
    createInstance();
    // TODO ...
  }

  void mainLoop() {
    while (!glfwWindowShouldClose(window_)) {
      glfwPollEvents();
    }
  }

  void cleanup() {
    glfwDestroyWindow(window_);
    glfwTerminate();
  }

  void createInstance() {
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = NJIN_NAME;
    app_info.applicationVersion = VK_MAKE_VERSION(NJIN_VERSION_MAJOR, NJIN_VERSION_MINOR, NJIN_VERSION_PATCH);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(NJIN_VERSION_MAJOR, NJIN_VERSION_MINOR, NJIN_VERSION_PATCH);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    uint32_t glfw_extension_count = 0;
    const char **glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    create_info.enabledExtensionCount = glfw_extension_count;
    create_info.ppEnabledExtensionNames = glfw_extensions;
    create_info.enabledLayerCount = 0;

    if (vkCreateInstance(&create_info, nullptr, &instance_) != VK_SUCCESS) {
      throw std::runtime_error(" Vulkan instance creation failed!");
    }
  }

  static constexpr uint32_t WIDTH = 800;
  static constexpr uint32_t HEIGHT = 600;

  GLFWwindow *window_;
  VkInstance instance_;
};
} // namespace njin

int main() {
  njin::App app;

  try {
    app.run();
  } catch (const std::exception &e) {
    njin::Logger::instance().error << e.what();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
