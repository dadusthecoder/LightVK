#pragma once
#include <memory>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

#include "Engine/Renderer/Vulkan/Swapchain.h"
#include "Resource.h"

namespace Lgt::Gpu {

struct DrawCommand {
    uint32_t  frameIndex        = 0;
    uint32_t  vertexBufferIndex = 0;
    uint32_t  indexBufferIndex  = 0;
    uint32_t  materialIndex     = 0;
    glm::mat4 transform         = glm::mat4(1.0f);
};

struct DrawList {
    DrawCommand* commands    = nullptr;
    uint32_t*    indexCounts = nullptr;
    uint32_t     count       = 0;
};

struct FrameUBO {
    glm::mat4 viewProj; // Camera view-projection matrix
};

struct OffscreenTarget {
    VkImage         colorImage = VK_NULL_HANDLE;
    VmaAllocation   colorAlloc = VK_NULL_HANDLE;
    VkImageView     colorView  = VK_NULL_HANDLE;
    
    VkImage         depthImage = VK_NULL_HANDLE;
    VmaAllocation   depthAlloc = VK_NULL_HANDLE;
    VkImageView     depthView  = VK_NULL_HANDLE;

    VkDescriptorSet imguiDescriptor = VK_NULL_HANDLE;

    uint32_t width = 0, height = 0;
};

class RendererClass {
public:
    void Init(GLFWwindow* window);
    void ShutDown();
    void Render(DrawList* list, const glm::mat4& viewProj);
    void BeginRendering(VkCommandBuffer cmd, bool clearColor = true);
    void EndRendering(VkCommandBuffer cmd);
    bool BeginFrame(uint32_t frameIndex);
    void EndFrame();

    void ResizeSceneTarget(uint32_t width, uint32_t height);
    VkDescriptorSet GetSceneTexture() const { return sceneTarget_.imguiDescriptor; }
    uint32_t GetSceneWidth() const { return sceneTarget_.width; }
    uint32_t GetSceneHeight() const { return sceneTarget_.height; }

    VkCommandBuffer GetCurrentCommandBuffer() const { return commandBuffers_[currentFrame_]; }
    VkCommandBuffer GetUICommandBuffer() const { return uiCommandBuffers_[currentFrame_]; }
    VkFormat        SwapchainFormat() const { return swapchain_.Format(); }

private:
    GLFWwindow* window_ = nullptr;

    VulkanSwapchain swapchain_;
    OffscreenTarget sceneTarget_;

    // Frame resources
    VkCommandPool                commandPool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers_;
    std::vector<VkCommandBuffer> uiCommandBuffers_;
    std::vector<BufferHandle>    frameUBO_;

    // Swapchain depth buffers
    std::vector<VkImage>         swapchainDepthImages_;
    std::vector<VmaAllocation>   swapchainDepthAllocs_;
    std::vector<VkImageView>     swapchainDepthViews_;

    // Sync
    std::vector<VkSemaphore> imageAvailableSems_;
    std::vector<VkSemaphore> renderFinishedSems_;
    std::vector<VkFence>     inFlightFences_;

    uint32_t currentFrame_       = 0;
    uint32_t currentImageIndex_  = 0;
    bool     framebufferResized_ = false;

    VkPipeline   TraingleGfxPipeline_ = VK_NULL_HANDLE;
    BufferHandle vertSSBO_;
    uint32_t     vertGpuIndex = 0;

    void        createCommandPool();
    void        createCommandBuffers();
    void        createSyncObjects();
    void        createUBOS();
    void        recreateSwapchain();
    static void framebufferResizeCallback(GLFWwindow* w, int, int);

    // testing purpose only
    void createTestResources();
};
} // namespace Lgt::Gpu