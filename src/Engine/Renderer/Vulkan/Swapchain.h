#pragma once
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <vector>

#include "Helpers.h"

#include <GLFW/glfw3.h>

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR        capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
};

class VulkanSwapchain {
public:
    // Call once to create; call again after cleanupSwapchain() to recreate.
    void Init(VkPhysicalDevice physical,
              VkDevice         logical,
              VkSurfaceKHR     surface,
              GLFWwindow*      window,
              uint32_t         graphicsFamily,
              uint32_t         presentFamily);

    // Destroys image views + swapchain; keeps the object reusable.
    void Cleanup(VkDevice logical);

    // Convenience: Cleanup then Init (same args kept internally).
    void Recreate(GLFWwindow* window);

    // Accessors
    VkSwapchainKHR                  Handle() const { return _swapchain; }
    VkFormat                        Format() const { return _format; }
    VkExtent2D                      Extent() const { return _extent; }
    const std::vector<VkImage>&     Images() const { return _images; }
    const std::vector<VkImageView>& ImageViews() const { return _imageViews; }
    uint32_t                        ImageCount() const { return static_cast<uint32_t>(_images.size()); }

    static SwapchainSupportDetails QuerySupport(VkPhysicalDevice device, VkSurfaceKHR surface);

private:
    VkSwapchainKHR           _swapchain = VK_NULL_HANDLE;
    std::vector<VkImage>     _images;
    std::vector<VkImageView> _imageViews;
    VkFormat                 _format = VK_FORMAT_UNDEFINED;
    VkExtent2D               _extent{};

    // Cached for Recreate()
    VkPhysicalDevice physical_       = VK_NULL_HANDLE;
    VkDevice         logical_        = VK_NULL_HANDLE;
    VkSurfaceKHR     _surface        = VK_NULL_HANDLE;
    uint32_t         graphicsFamily_ = 0;
    uint32_t         presentFamily_  = 0;

    void createSwapchain(GLFWwindow* window);
    void createImageViews();

    static VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    static VkPresentModeKHR   choosePresentMode(const std::vector<VkPresentModeKHR>& modes);
    static VkExtent2D         chooseExtent(const VkSurfaceCapabilitiesKHR& caps, GLFWwindow* window);
};