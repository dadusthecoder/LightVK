#include <stdexcept>
#include "Surface.h"

void VulkanSurface::Init(VkInstance instance, GLFWwindow* window) {
    _instance = instance;
    if (glfwCreateWindowSurface(_instance, window, nullptr, &_surface) != VK_SUCCESS)
        throw std::runtime_error("glfwCreateWindowSurface failed");
}

void VulkanSurface::ShutDown() {
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
}