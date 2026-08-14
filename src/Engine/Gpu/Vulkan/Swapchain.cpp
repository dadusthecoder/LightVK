#include "Swapchain.h"

void VulkanSwapchain::Init(VkPhysicalDevice physical,
                           VkDevice         logical,
                           VkSurfaceKHR     surface,
                           GLFWwindow*      window,
                           uint32_t         graphicsFamily,
                           uint32_t         presentFamily) {
    physical_       = physical;
    logical_        = logical;
    _surface        = surface;
    graphicsFamily_ = graphicsFamily;
    presentFamily_  = presentFamily;

    createSwapchain(window);
    createImageViews();
}

void VulkanSwapchain::Cleanup(VkDevice logical) {
    for (auto iv : _imageViews)
        vkDestroyImageView(logical, iv, nullptr);
    _imageViews.clear();

    vkDestroySwapchainKHR(logical, _swapchain, nullptr);
    _swapchain = VK_NULL_HANDLE;
    _images.clear();
}

void VulkanSwapchain::Recreate(GLFWwindow* window) {
    Cleanup(logical_);
    createSwapchain(window);
    createImageViews();
}

void VulkanSwapchain::createSwapchain(GLFWwindow* window) {
    auto support = QuerySupport(physical_, _surface);
    auto sf      = chooseSurfaceFormat(support.formats);
    auto pm      = choosePresentMode(support.presentModes);
    auto ext     = chooseExtent(support.capabilities, window);

    uint32_t imageCount = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0)
        imageCount = std::min(imageCount, support.capabilities.maxImageCount);

    VkSwapchainCreateInfoKHR ci{};
    ci.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.surface          = _surface;
    ci.minImageCount    = imageCount;
    ci.imageFormat      = sf.format;
    ci.imageColorSpace  = sf.colorSpace;
    ci.imageExtent      = ext;
    ci.imageArrayLayers = 1;
    ci.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilies[] = {graphicsFamily_, presentFamily_};
    if (graphicsFamily_ != presentFamily_) {
        ci.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        ci.queueFamilyIndexCount = 2;
        ci.pQueueFamilyIndices   = queueFamilies;
    } else {
        ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    ci.preTransform   = support.capabilities.currentTransform;
    ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode    = pm;
    ci.clipped        = VK_TRUE;
    ci.oldSwapchain   = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(logical_, &ci, nullptr, &_swapchain) != VK_SUCCESS)
        throw std::runtime_error("vkCreateSwapchainKHR failed");

    uint32_t count = 0;
    vkGetSwapchainImagesKHR(logical_, _swapchain, &count, nullptr);
    _images.resize(count);
    vkGetSwapchainImagesKHR(logical_, _swapchain, &count, _images.data());

    _format = sf.format;
    _extent = ext;
}

void VulkanSwapchain::createImageViews() {
    _imageViews.resize(_images.size());
    for (size_t i = 0; i < _images.size(); ++i) {
        VkImageViewCreateInfo ci{};
        ci.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ci.image                           = _images[i];
        ci.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        ci.format                          = _format;
        ci.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        ci.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        ci.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        ci.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        ci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        ci.subresourceRange.baseMipLevel   = 0;
        ci.subresourceRange.levelCount     = 1;
        ci.subresourceRange.baseArrayLayer = 0;
        ci.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(logical_, &ci, nullptr, &_imageViews[i]) != VK_SUCCESS)
            throw std::runtime_error("vkCreateImageView failed");
    }
}

SwapchainSupportDetails VulkanSwapchain::QuerySupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapchainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr);
    if (count) {
        details.formats.resize(count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, details.formats.data());
    }

    count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr);
    if (count) {
        details.presentModes.resize(count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, details.presentModes.data());
    }
    return details;
}

VkSurfaceFormatKHR VulkanSwapchain::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
    for (const auto& f : formats)
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return f;
    return formats[0];
}

VkPresentModeKHR VulkanSwapchain::choosePresentMode(const std::vector<VkPresentModeKHR>& modes) {
    for (const auto& m : modes)
        if (m == VK_PRESENT_MODE_MAILBOX_KHR)
            return m;
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::chooseExtent(const VkSurfaceCapabilitiesKHR& caps, GLFWwindow* window) {
    if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return caps.currentExtent;

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    VkExtent2D actual = {static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
    actual.width      = std::clamp(actual.width, caps.minImageExtent.width, caps.maxImageExtent.width);
    actual.height     = std::clamp(actual.height, caps.minImageExtent.height, caps.maxImageExtent.height);
    return actual;
}