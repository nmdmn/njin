#include <vector>
#include <ranges>
#include <algorithm>
#include <exception>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "config.hh"
#include "logger.hh"

// TODO Proper RAII! c-tor/d-tor usage - implement via unique_ptr/shared_ptr with custom deleter!
// NOTE on the official Khronos Vulkan tutorial there is something about vk::raii::... CHECK IT!

static auto &logger = njin::Logger::instance();

namespace njin {
class App {
public:
  void run() {
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
    vkDestroyInstance(instance_, nullptr);
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
    create_info.enabledLayerCount = 0;

    uint32_t glfw_extension_count = 0;
    const char **glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    std::vector<const char *> glfw_required_extensions;
    std::ranges::transform(std::views::iota(static_cast<uint32_t>(0), glfw_extension_count),
                           std::back_inserter(glfw_required_extensions), [&](int i) { return glfw_extensions[i]; });
    glfw_required_extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    create_info.enabledExtensionCount = static_cast<uint32_t>(glfw_required_extensions.size());
    create_info.ppEnabledExtensionNames = glfw_required_extensions.data();

    uint32_t vk_extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &vk_extension_count, nullptr);
    std::vector<VkExtensionProperties> vk_extensions{vk_extension_count};
    vkEnumerateInstanceExtensionProperties(nullptr, &vk_extension_count, vk_extensions.data());
    logger.info << "available extensions:";
    std::ranges::for_each(vk_extensions, [&](const auto &element) { logger.info << "\t" << element.extensionName; });

    bool all_found = true;
    std::ranges::for_each(glfw_required_extensions, [&](const auto &glfw_extension) {
      bool found = std::ranges::any_of(vk_extensions, [&](const auto &vk_extension) {
        return std::string(glfw_extension) == std::string(vk_extension.extensionName);
      });
      if (!found) {
        all_found = false;
        logger.warning << "missing extension: " << glfw_extension;
      }
    });

    if (all_found) {
      logger.info << "all extensions found";
    }

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
    logger.error << e.what();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
