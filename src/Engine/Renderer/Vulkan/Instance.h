#pragma once
#include <vector>
#include "Helpers.h"

class VulkanInstance {
public:
    void Init(bool enableValidation, const std::vector<const char*>& validationLayers);
    void ShutDown();

    VkInstance Handle() const { return _instance; }

private:
    VkInstance               _instance       = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;
    bool                     validation_     = false;

    void createInstance(const std::vector<const char*>& layers);
    void setupDebugMessenger();

    bool                                      checkValidationLayerSupport(const std::vector<const char*>& layers);
    std::vector<const char*>                  getRequiredExtensions();
    static VkDebugUtilsMessengerCreateInfoEXT makeDebugCI();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT,
                                                        VkDebugUtilsMessageTypeFlagsEXT,
                                                        const VkDebugUtilsMessengerCallbackDataEXT*,
                                                        void*);
};