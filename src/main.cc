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
  static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                       VkDebugUtilsMessageTypeFlagsEXT message_type,
                                                       const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                                                       void *user_data) {
    std::string log_prefix;
    switch (message_type) {
    case (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT):
      log_prefix = "GENERAL vulkan debug: ";
      break;
    case (VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT):
      log_prefix = "VALIDATION vulkan debug: ";
      break;
    case (VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT):
      log_prefix = "PERFORMANCE vulkan debug: ";
      break;
    default:
      log_prefix = "not valid message type in validation layer debug callback: ";
      break;
    }

    switch (message_severity) {
    case (VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT):
      logger.trace << log_prefix << callback_data->pMessage;
      break;
    case (VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT):
      logger.info << log_prefix << callback_data->pMessage;
      break;
    case (VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT):
      logger.warning << log_prefix << callback_data->pMessage;
      break;
    case (VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT):
      logger.error << log_prefix << callback_data->pMessage;
      break;
    default:
      logger.error << "not a valid message severity in validation layer debug callback!";
      break;
    }

    return false;
  }

  auto run() -> void {
    logger.info << NJIN_NAME << " v" << NJIN_VERSION;

    init_window();
    init_vulkan();
    main_loop();
    cleanup();
  }

private:
  auto init_window() -> void {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(WIDTH_, HEIGHT_, NJIN_NAME, nullptr, nullptr);
  }

  auto init_vulkan() -> void {
    create_instance();
    setup_debug_messenger();
    select_physical_device();
    create_logical_device();
  }

  auto main_loop() -> void {
    while (!glfwWindowShouldClose(window_)) {
      glfwPollEvents();
    }
  }

  auto cleanup() -> void {
    if (is_validation_layer_) {
      destroy_debug_utils_messenger_EXT(instance_, debug_messenger_, nullptr);
    }
    vkDestroyInstance(instance_, nullptr);
    glfwDestroyWindow(window_);
    glfwTerminate();
  }

  auto check_extension_support() {
    uint32_t vk_extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &vk_extension_count, nullptr);
    std::vector<VkExtensionProperties> vk_extensions{vk_extension_count};
    vkEnumerateInstanceExtensionProperties(nullptr, &vk_extension_count, vk_extensions.data());
    logger.info << "available extensions:";
    std::ranges::for_each(vk_extensions, [&](const auto &element) { logger.info << "\t" << element.extensionName; });

    // NOTE needed on MacOS
    required_extensions_.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    if (is_validation_layer_) {
      required_extensions_.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    logger.info << "required extensions:";
    bool all_found = true;
    std::ranges::for_each(required_extensions_, [&](const auto &required_extension) {
      logger.info << "\t" << required_extension;
      bool found = std::ranges::any_of(vk_extensions, [&](const auto &available_extension) {
        // NOTE is this ugly? strcomp? or extract this into a util function???
        return std::string(required_extension) == std::string(available_extension.extensionName);
      });
      if (!found) {
        all_found = false;
        logger.warning << "missing extension: " << required_extension;
      }
    });

    if (all_found) {
      logger.info << "all extensions found";
      return true;
    }
    return false;
  }

  auto check_layer_support() {
    uint32_t vk_layer_count = 0;
    vkEnumerateInstanceLayerProperties(&vk_layer_count, nullptr);
    std::vector<VkLayerProperties> vk_layers(vk_layer_count);
    vkEnumerateInstanceLayerProperties(&vk_layer_count, vk_layers.data());

    logger.info << "available layers:";
    std::ranges::for_each(vk_layers, [&](const auto &element) { logger.info << "\t" << element.layerName; });

    logger.info << "required extensions:";
    bool all_found = true;
    std::ranges::for_each(required_layers_, [&](const auto &required_layer) {
      logger.info << "\t" << required_layer;
      bool found = std::ranges::any_of(vk_layers, [&](const auto &available_layer) {
        return std::string(required_layer) == std::string(available_layer.layerName);
      });
      if (!found) {
        all_found = false;
        logger.warning << "missing layer: " << required_layer;
      }
    });

    if (all_found) {
      logger.info << "all layer found";
      return true;
    }

    return false;
  }

  auto create_instance() -> void {
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = NJIN_NAME;
    app_info.applicationVersion = VK_MAKE_VERSION(NJIN_VERSION_MAJOR, NJIN_VERSION_MINOR, NJIN_VERSION_PATCH);
    app_info.pEngineName = NJIN_NAME;
    app_info.engineVersion = VK_MAKE_VERSION(NJIN_VERSION_MAJOR, NJIN_VERSION_MINOR, NJIN_VERSION_PATCH);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    uint32_t glfw_extension_count = 0;
    const char **glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    std::ranges::transform(std::views::iota(static_cast<uint32_t>(0), glfw_extension_count),
                           std::back_inserter(required_extensions_), [&](int i) { return glfw_extensions[i]; });

    if (!check_extension_support()) {
      throw std::runtime_error("required extension(s) no supported!");
    }
    create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    create_info.enabledExtensionCount = static_cast<uint32_t>(required_extensions_.size());
    create_info.ppEnabledExtensionNames = required_extensions_.data();

    if (is_validation_layer_ && !check_layer_support()) {
      throw std::runtime_error("required layer(s) no supported!");
    }
    if (is_validation_layer_) {
      create_info.enabledLayerCount = static_cast<uint32_t>(required_layers_.size());
      create_info.ppEnabledLayerNames = required_layers_.data();
      VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
      populate_debug_messenger_create_info(debug_create_info);
      create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debug_create_info;
    } else {
      create_info.enabledLayerCount = 0;
      create_info.pNext = nullptr;
    }

    if (vkCreateInstance(&create_info, nullptr, &instance_) != VK_SUCCESS) {
      throw std::runtime_error("vulkan instance creation failed!");
    }
  }

  VkResult create_debug_utils_messenger_EXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *create_info,
                                            const VkAllocationCallbacks *allocator,
                                            VkDebugUtilsMessengerEXT *debug_messenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
      return func(instance, create_info, allocator, debug_messenger);
    } else {
      return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
  }

  void destroy_debug_utils_messenger_EXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger,
                                         const VkAllocationCallbacks *allocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
      func(instance, debug_messenger, allocator);
    }
  }

  static auto populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT &create_info) -> void {
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    if (NJIN_LOGGER >= static_cast<int>(Logger::Level::ERROR)) {
      create_info.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    }
    if (NJIN_LOGGER >= static_cast<int>(Logger::Level::WARNING)) {
      create_info.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    }
    if (NJIN_LOGGER >= static_cast<int>(Logger::Level::INFO)) {
      create_info.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    }
    if (NJIN_LOGGER >= static_cast<int>(Logger::Level::TRACE)) {
      create_info.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    }
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = debug_callback;
    create_info.pUserData = nullptr;
  }

  auto setup_debug_messenger() -> void {
    if (!is_validation_layer_) {
      return;
    }

    VkDebugUtilsMessengerCreateInfoEXT create_info{};
    populate_debug_messenger_create_info(create_info);

    if (create_debug_utils_messenger_EXT(instance_, &create_info, nullptr, &debug_messenger_) != VK_SUCCESS) {
      throw std::runtime_error("failed to create debug messenger!");
    }
  }

  struct Queue_family_indices {
    std::optional<uint32_t> graphics_family;

    auto is_complete() const { return graphics_family.has_value(); }
  };

  static auto find_queue_families(VkPhysicalDevice physical_device) {
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family_properties.data());

    const auto graphics_family = std::ranges::find_if(
        queue_family_properties, [&](const auto &property) { return property.queueFlags & VK_QUEUE_GRAPHICS_BIT; });

    Queue_family_indices family_indices;
    if (graphics_family != queue_family_properties.end()) {
      family_indices.graphics_family = std::distance(queue_family_properties.begin(), graphics_family);
    }
    return family_indices;
  }

  static auto is_device_suitable(VkPhysicalDevice physical_device) {
    VkPhysicalDeviceProperties physical_device_properties{};
    vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);

    VkPhysicalDeviceFeatures physical_device_features{};
    vkGetPhysicalDeviceFeatures(physical_device, &physical_device_features);

    logger.info << "GPU: " << physical_device_properties.deviceName
                << ", API version: " << physical_device_properties.apiVersion
                << ", driver version: " << physical_device_properties.driverVersion;

    const auto family_indices = find_queue_families(physical_device);
    return family_indices.is_complete();
  }

  auto select_physical_device() -> void {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
    if (0 == device_count) {
      throw std::runtime_error("no GPU found with vulkan support!");
    }
    std::vector<VkPhysicalDevice> physical_devices(device_count);
    vkEnumeratePhysicalDevices(instance_, &device_count, physical_devices.data());
    // TODO implement a scoring logic based on avail memory, discrete, etc. and choose the most "useful" GPU
    auto suitable_device = std::ranges::find_if(physical_devices, is_device_suitable);
    if (suitable_device != physical_devices.end()) {
      physical_device_ = *suitable_device;
    } else {
      throw std::runtime_error("failed to find suitable GPU!");
    }
  }

  auto create_logical_device() -> void {
    // TODO
    // ASD
  }

  static constexpr uint32_t WIDTH_ = 800;
  static constexpr uint32_t HEIGHT_ = 600;

  GLFWwindow *window_;
  VkInstance instance_;
  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT debug_messenger_;
  std::vector<const char *> required_extensions_;
  const std::vector<const char *> required_layers_ = {"VK_LAYER_KHRONOS_validation"};

// NOTE not optimal? validation layer enabled only in DEBUG!
#if DEBUG && NJIN_LOGGER > 0
  static constexpr bool is_validation_layer_ = true;
#else
  static constexpr bool is_validation_layer_ = false;
#endif
};
} // namespace njin

auto main() -> int {
  njin::App app;

  try {
    app.run();
  } catch (const std::exception &e) {
    logger.error << e.what();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
