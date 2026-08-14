#pragma once

#include "Helpers.h"
#include <GLFW/glfw3.h>

class VulkanSurface {
public:
    void Init(VkInstance instance, GLFWwindow* window);
    void ShutDown();

    VkSurfaceKHR Handle() const { return _surface; }

private:
    VkInstance   _instance = VK_NULL_HANDLE;
    VkSurfaceKHR _surface  = VK_NULL_HANDLE;
};