#include <stdexcept>
#include <cstring>

#include <GLFW/glfw3.h>

#include "Engine/Core/Logger.h"
#include "Instance.h"
    
void VulkanInstance::Init(bool enableValidation, const std::vector<const char*>& validationLayers) {
    validation_ = enableValidation;
    if (validation_ && !checkValidationLayerSupport(validationLayers))
        LIGHTVK_ERROR("Validation layers requested but not available");

    createInstance(validationLayers);
    setupDebugMessenger();
}

void VulkanInstance::ShutDown() {
    if (validation_ && debugMessenger_ != VK_NULL_HANDLE) {
        auto fn = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT"));
        if (fn)
            fn(instance_, debugMessenger_, nullptr);
    }
    vkDestroyInstance(instance_, nullptr);
}

void VulkanInstance::createInstance(const std::vector<const char*>& layers) {
    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "MyGame";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "DOGGEngine";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_4;

    auto extensions = getRequiredExtensions();

    VkInstanceCreateInfo ci{};
    ci.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo        = &appInfo;
    ci.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    ci.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCI = makeDebugCI();
    if (validation_) {
        ci.enabledLayerCount   = static_cast<uint32_t>(layers.size());
        ci.ppEnabledLayerNames = layers.data();
        ci.pNext               = &debugCI;
    }

    if (vkCreateInstance(&ci, nullptr, &instance_) != VK_SUCCESS)
        throw std::runtime_error("vkCreateInstance failed");
}

void VulkanInstance::setupDebugMessenger() {
    if (!validation_)
        return;
    auto ci = makeDebugCI();
    auto fn =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance_, "vkCreateDebugUtilsMessengerEXT"));
    if (!fn || fn(instance_, &ci, nullptr, &debugMessenger_) != VK_SUCCESS)
        throw std::runtime_error("Failed to create debug messenger");
}

bool VulkanInstance::checkValidationLayerSupport(const std::vector<const char*>& layers) {
    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> available(count);
    vkEnumerateInstanceLayerProperties(&count, available.data());

    for (const char* name : layers) {
        bool found = false;
        for (const auto& props : available)
            if (strcmp(name, props.layerName) == 0) {
                found = true;
                break;
            }
        if (!found)
            return false;
    }
    return true;
}

std::vector<const char*> VulkanInstance::getRequiredExtensions() {
    uint32_t                 glfwCount = 0;
    const char**             glfwExts  = glfwGetRequiredInstanceExtensions(&glfwCount);
    std::vector<const char*> exts(glfwExts, glfwExts + glfwCount);
    if (validation_)
        exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    return exts;
}

VkDebugUtilsMessengerCreateInfoEXT VulkanInstance::makeDebugCI() {
    VkDebugUtilsMessengerCreateInfoEXT ci{};
    ci.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    ci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    ci.pfnUserCallback = debugCallback;
    return ci;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanInstance::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                             VkDebugUtilsMessageTypeFlagsEXT type,
                                                             const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                             void*) {
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        LIGHTVK_ERROR("[Validation] {}", pCallbackData->pMessage);
    } else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        LIGHTVK_WARN("[Validation] {}", pCallbackData->pMessage);
    } else {
        LIGHTVK_TRACE("[Validation] {}", pCallbackData->pMessage);
    }
    return VK_FALSE;
}