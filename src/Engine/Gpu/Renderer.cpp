#include <stdexcept>
#include <array>
#include <imgui_impl_vulkan.h>
#include "Engine/Core/Logger.h"
#include "Context.h"
#include "Engine/Gpu/Vulkan/Context.h"

#include "Renderer.h"

namespace Lgt::Gpu {

Extent RendererClass::GetCurrentExtent() const {
    if (_renderToSwapchain) {
        auto ext = _swapchain.GetExtent();
        return Extent{ext.width, ext.height};
    }
    return _sceneTarget.extent;
}

void RendererClass::Init(GLFWwindow* window) {

    _window = window;
    glfwSetWindowUserPointer(_window, this);
    glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);

    _swapchain.Init(Lgt::Vulkan::g_Device->Physical(),
                    Lgt::Vulkan::g_Device->Logical(),
                    Lgt::Vulkan::g_Surface->Handle(),
                    _window,
                    Lgt::Vulkan::g_Device->GraphicsFamily(),
                    Lgt::Vulkan::g_Device->PresentFamily());

    for (uint32_t i = 0; i < _swapchain.ImageCount(); ++i) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType     = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width  = _swapchain.GetExtent().width;
        imageInfo.extent.height = _swapchain.GetExtent().height;
        imageInfo.extent.depth  = 1;
        imageInfo.mipLevels     = 1;
        imageInfo.arrayLayers   = 1;
        imageInfo.format        = VK_FORMAT_D32_SFLOAT;
        imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

        VkImage       image;
        VmaAllocation alloc;
        Vulkan::g_Allocator->createImage(imageInfo, VMA_MEMORY_USAGE_GPU_ONLY, 0, image, alloc);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                           = image;
        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                          = VK_FORMAT_D32_SFLOAT;
        viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

        VkImageView view;
        vkCreateImageView(Vulkan::g_Device->Logical(), &viewInfo, nullptr, &view);

        _swapchainDepthImages.push_back(image);
        _swapchainDepthAllocs.push_back(alloc);
        _swapchainDepthViews.push_back(view);
    }

    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
    createUBOS();

    createTestResources();
}

void RendererClass::ShutDown() {
    vkDeviceWaitIdle(Vulkan::g_Device->Logical());

    if (_sceneTarget.colorImage != VK_NULL_HANDLE) {
        ImGui_ImplVulkan_RemoveTexture(_sceneTarget.imguiDescriptor);
        vkDestroyImageView(Vulkan::g_Device->Logical(), _sceneTarget.colorView, nullptr);
        Vulkan::g_Allocator->destroyImage(_sceneTarget.colorImage, _sceneTarget.colorAlloc);
        _sceneTarget.colorImage = VK_NULL_HANDLE;
    }

    if (_sceneTarget.depthImage != VK_NULL_HANDLE) {
        vkDestroyImageView(Vulkan::g_Device->Logical(), _sceneTarget.depthView, nullptr);
        Vulkan::g_Allocator->destroyImage(_sceneTarget.depthImage, _sceneTarget.depthAlloc);
        _sceneTarget.depthImage = VK_NULL_HANDLE;
    }

    for (size_t i = 0; i < _swapchainDepthImages.size(); ++i) {
        vkDestroyImageView(Vulkan::g_Device->Logical(), _swapchainDepthViews[i], nullptr);
        Vulkan::g_Allocator->destroyImage(_swapchainDepthImages[i], _swapchainDepthAllocs[i]);
    }
    _swapchainDepthImages.clear();
    _swapchainDepthAllocs.clear();
    _swapchainDepthViews.clear();

    _swapchain.Cleanup(Vulkan::g_Device->Logical());
    vkDestroyPipeline(Vulkan::g_Device->Logical(), TraingleGfxPipeline_, nullptr);
    if (_wireframePipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(Vulkan::g_Device->Logical(), _wireframePipeline, nullptr);
    }

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroySemaphore(Lgt::Vulkan::g_Device->Logical(), _imageAvailableSems[i], nullptr);
        vkDestroySemaphore(Lgt::Vulkan::g_Device->Logical(), _renderFinishedSems[i], nullptr);
        vkDestroyFence(Lgt::Vulkan::g_Device->Logical(), _inFlightFences[i], nullptr);
    }
    vkDestroyCommandPool(Lgt::Vulkan::g_Device->Logical(), _commandPool, nullptr);
}

void RendererClass::recreateSwapchain() {
    int w = 0, h = 0;
    glfwGetFramebufferSize(_window, &w, &h);
    while (w == 0 || h == 0) {
        glfwGetFramebufferSize(_window, &w, &h);
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(Lgt::Vulkan::g_Device->Logical());

    for (size_t i = 0; i < _swapchainDepthImages.size(); ++i) {
        vkDestroyImageView(Vulkan::g_Device->Logical(), _swapchainDepthViews[i], nullptr);
        Vulkan::g_Allocator->destroyImage(_swapchainDepthImages[i], _swapchainDepthAllocs[i]);
    }
    _swapchainDepthImages.clear();
    _swapchainDepthAllocs.clear();
    _swapchainDepthViews.clear();

    _swapchain.Recreate(_window);

    for (uint32_t i = 0; i < _swapchain.ImageCount(); ++i) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType     = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width  = _swapchain.GetExtent().width;
        imageInfo.extent.height = _swapchain.GetExtent().height;
        imageInfo.extent.depth  = 1;
        imageInfo.mipLevels     = 1;
        imageInfo.arrayLayers   = 1;
        imageInfo.format        = VK_FORMAT_D32_SFLOAT;
        imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

        VkImage       image;
        VmaAllocation alloc;
        Vulkan::g_Allocator->createImage(imageInfo, VMA_MEMORY_USAGE_GPU_ONLY, 0, image, alloc);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                           = image;
        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                          = VK_FORMAT_D32_SFLOAT;
        viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

        VkImageView view;
        vkCreateImageView(Vulkan::g_Device->Logical(), &viewInfo, nullptr, &view);

        _swapchainDepthImages.push_back(image);
        _swapchainDepthAllocs.push_back(alloc);
        _swapchainDepthViews.push_back(view);
    }
}

void RendererClass::Render(const DrawList& list, const glm::mat4& viewProj) {
    DrawList emptyWireframeList;
    Render(list, emptyWireframeList, viewProj);
}

void RendererClass::Render(const DrawList& solidList, const DrawList& wireframeList, const glm::mat4& viewProj) {

    // Upload camera view-projection matrix to per-frame UBO
    auto* ubo = Resources->GetBuffer(_frameUBO[_currentFrame]);
    LGT_ASSERT(ubo, "");
    FrameUBO ubodata{viewProj};
    memcpy(ubo->mapped, &ubodata, sizeof(FrameUBO));
    //------------------------------------------------

    auto cmd = _commandBuffers[_currentFrame];

    uint32_t targetWidth  = _swapchain.GetExtent().width;
    uint32_t targetHeight = _swapchain.GetExtent().height;
    if (_sceneTarget.colorImage != VK_NULL_HANDLE) {
        targetWidth  = _sceneTarget.extent.width;
        targetHeight = _sceneTarget.extent.height;
    }

    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(targetWidth);
    viewport.height   = static_cast<float>(targetHeight);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {targetWidth, targetHeight};

    vkCmdSetScissor(cmd, 0, 1, &scissor);

    BeginRendering(cmd, true);

    // Draw solid objects
    if (!solidList.empty()) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, TraingleGfxPipeline_);
        for (size_t i = 0; i < solidList.commands.size(); ++i) {
            auto command       = solidList.commands[i];
            command.frameIndex = _currentFrame;

            VkHostAddressRangeConstEXT cpuPushDataInfo{};
            cpuPushDataInfo.size    = sizeof(DrawCommand);
            cpuPushDataInfo.address = &command;

            VkPushDataInfoEXT pushDataInfo{};
            pushDataInfo.sType  = VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT;
            pushDataInfo.offset = 0;
            pushDataInfo.data   = cpuPushDataInfo;

            vkCmdPushDataEXT(cmd, &pushDataInfo);
            vkCmdDraw(cmd, solidList.indexCounts[i], 1, 0, 0);
        }
    }

    // Draw wireframe objects
    if (!wireframeList.empty() && _wireframePipeline != VK_NULL_HANDLE) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _wireframePipeline);
        for (size_t i = 0; i < wireframeList.commands.size(); ++i) {
            auto command       = wireframeList.commands[i];
            command.frameIndex = _currentFrame;

            VkHostAddressRangeConstEXT cpuPushDataInfo{};
            cpuPushDataInfo.size    = sizeof(DrawCommand);
            cpuPushDataInfo.address = &command;

            VkPushDataInfoEXT pushDataInfo{};
            pushDataInfo.sType  = VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT;
            pushDataInfo.offset = 0;
            pushDataInfo.data   = cpuPushDataInfo;

            vkCmdPushDataEXT(cmd, &pushDataInfo);
            vkCmdDraw(cmd, wireframeList.indexCounts[i], 1, 0, 0);
        }
    }

    EndRendering(cmd);
}

bool RendererClass::BeginFrame(uint32_t frameindex) {
    _currentFrame = frameindex;

    vkWaitForFences(Vulkan::g_Device->Logical(), 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(Vulkan::g_Device->Logical(),
                                            _swapchain.GetHandle(),
                                            UINT64_MAX,
                                            _imageAvailableSems[_currentFrame],
                                            VK_NULL_HANDLE,
                                            &_currentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return false;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        throw std::runtime_error("vkAcquireNextImageKHR failed");

    vkResetFences(Lgt::Vulkan::g_Device->Logical(), 1, &_inFlightFences[_currentFrame]);

    // resource heap bind info
    VkDeviceAddressRangeEXT resourceDeviceAdderRange{};
    resourceDeviceAdderRange.size    = ResourceHeap->GetSize();
    resourceDeviceAdderRange.address = ResourceHeap->GetBufferAddress();

    VkBindHeapInfoEXT resourceBind{};
    resourceBind.sType               = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT;
    resourceBind.heapRange           = resourceDeviceAdderRange;
    resourceBind.reservedRangeOffset = ResourceHeap->GerReservedRangeOffset();
    resourceBind.reservedRangeSize   = ResourceHeap->GetReservedRangeSize();

    // smapler heap bind info
    VkDeviceAddressRangeEXT samplerDeviceAdderRange{};
    samplerDeviceAdderRange.size    = SamplerHeap->GetSize();
    samplerDeviceAdderRange.address = SamplerHeap->GetBufferAddress();

    VkBindHeapInfoEXT samplerBind{};
    samplerBind.sType               = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT;
    samplerBind.heapRange           = samplerDeviceAdderRange;
    samplerBind.reservedRangeOffset = SamplerHeap->GerReservedRangeOffset();
    samplerBind.reservedRangeSize   = SamplerHeap->GetReservedRangeSize();

    VkCommandBuffer cmd = _commandBuffers[_currentFrame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;

    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS)
        throw std::runtime_error("vkBeginCommandBuffer failed");

    VkCommandBuffer uiCmd = _uiCommandBuffers[_currentFrame];
    vkResetCommandBuffer(uiCmd, 0);
    if (vkBeginCommandBuffer(uiCmd, &beginInfo) != VK_SUCCESS)
        throw std::runtime_error("vkBeginCommandBuffer failed");

    vkCmdBindResourceHeapEXT(cmd, &resourceBind);
    vkCmdBindSamplerHeapEXT(cmd, &samplerBind);

    VkImageMemoryBarrier renderBarrier{};
    renderBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    renderBarrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED; // Safe catch-all for acquired swapchain images
    renderBarrier.newLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    renderBarrier.srcAccessMask                   = 0;
    renderBarrier.dstAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    renderBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    renderBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    renderBarrier.image                           = _swapchain.Images()[_currentImageIndex];
    renderBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    renderBarrier.subresourceRange.baseMipLevel   = 0;
    renderBarrier.subresourceRange.levelCount     = 1;
    renderBarrier.subresourceRange.baseArrayLayer = 0;
    renderBarrier.subresourceRange.layerCount     = 1;

    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &renderBarrier);

    return true;
}

void RendererClass::ResizeSceneTarget(Extent extent) {
    if (_sceneTarget.extent.width == extent.width && _sceneTarget.extent.height == extent.height)
        return;

    // Destroy old resources safely
    if (_sceneTarget.colorImage != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(Vulkan::g_Device->Logical()); // Ensure GPU is not using the image
        ImGui_ImplVulkan_RemoveTexture(_sceneTarget.imguiDescriptor);
        vkDestroyImageView(Vulkan::g_Device->Logical(), _sceneTarget.colorView, nullptr);
        Vulkan::g_Allocator->destroyImage(_sceneTarget.colorImage, _sceneTarget.colorAlloc);
    }
    if (_sceneTarget.depthImage != VK_NULL_HANDLE) {
        vkDestroyImageView(Vulkan::g_Device->Logical(), _sceneTarget.depthView, nullptr);
        Vulkan::g_Allocator->destroyImage(_sceneTarget.depthImage, _sceneTarget.depthAlloc);
        _sceneTarget.depthImage = VK_NULL_HANDLE;
    }

    _sceneTarget.extent.width  = extent.width;
    _sceneTarget.extent.height = extent.height;

    if (extent.width == 0 || extent.height == 0)
        return;

    // Create new color image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = extent.width;
    imageInfo.extent.height = extent.height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = _swapchain.GetFormat();
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;

    Vulkan::g_Allocator->createImage(imageInfo, VMA_MEMORY_USAGE_GPU_ONLY, 0, _sceneTarget.colorImage, _sceneTarget.colorAlloc);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                           = _sceneTarget.colorImage;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = _swapchain.GetFormat();
    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    vkCreateImageView(Vulkan::g_Device->Logical(), &viewInfo, nullptr, &_sceneTarget.colorView);

    // Create new depth image
    VkImageCreateInfo depthInfo{};
    depthInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depthInfo.imageType     = VK_IMAGE_TYPE_2D;
    depthInfo.extent.width  = extent.width;
    depthInfo.extent.height = extent.height;
    depthInfo.extent.depth  = 1;
    depthInfo.mipLevels     = 1;
    depthInfo.arrayLayers   = 1;
    depthInfo.format        = VK_FORMAT_D32_SFLOAT;
    depthInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    depthInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depthInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    depthInfo.samples       = VK_SAMPLE_COUNT_1_BIT;

    Vulkan::g_Allocator->createImage(depthInfo, VMA_MEMORY_USAGE_GPU_ONLY, 0, _sceneTarget.depthImage, _sceneTarget.depthAlloc);

    VkImageViewCreateInfo depthViewInfo{};
    depthViewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depthViewInfo.image                           = _sceneTarget.depthImage;
    depthViewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    depthViewInfo.format                          = VK_FORMAT_D32_SFLOAT;
    depthViewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthViewInfo.subresourceRange.baseMipLevel   = 0;
    depthViewInfo.subresourceRange.levelCount     = 1;
    depthViewInfo.subresourceRange.baseArrayLayer = 0;
    depthViewInfo.subresourceRange.layerCount     = 1;

    vkCreateImageView(Vulkan::g_Device->Logical(), &depthViewInfo, nullptr, &_sceneTarget.depthView);

    // Register with ImGui
    _sceneTarget.imguiDescriptor = ImGui_ImplVulkan_AddTexture(_sceneTarget.colorView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void RendererClass::BeginRendering(VkCommandBuffer cmd, bool clearColor) {
    bool isUiPass       = (cmd == _uiCommandBuffers[_currentFrame]);
    bool useSceneTarget = !isUiPass && !_renderToSwapchain;

    if (!clearColor) {
        VkImageMemoryBarrier syncBarrier{};
        syncBarrier.sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        syncBarrier.oldLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        syncBarrier.newLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        syncBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        syncBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        syncBarrier.image         = useSceneTarget ? _sceneTarget.colorImage : _swapchain.Images()[_currentImageIndex];
        syncBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        syncBarrier.subresourceRange.baseMipLevel   = 0;
        syncBarrier.subresourceRange.levelCount     = 1;
        syncBarrier.subresourceRange.baseArrayLayer = 0;
        syncBarrier.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &syncBarrier);
    }

    if (useSceneTarget) {
        // Transition offscreen target from whatever it was (usually SHADER_READ_ONLY_OPTIMAL or UNDEFINED) to
        // COLOR_ATTACHMENT_OPTIMAL
        VkImageMemoryBarrier transitionBarrier{};
        transitionBarrier.sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        transitionBarrier.oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED; // We don't care about previous contents before clearing
        transitionBarrier.newLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        transitionBarrier.srcAccessMask = 0;
        transitionBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        transitionBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        transitionBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        transitionBarrier.image                           = _sceneTarget.colorImage;
        transitionBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        transitionBarrier.subresourceRange.baseMipLevel   = 0;
        transitionBarrier.subresourceRange.levelCount     = 1;
        transitionBarrier.subresourceRange.baseArrayLayer = 0;
        transitionBarrier.subresourceRange.layerCount     = 1;

        if (!clearColor) {
            transitionBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &transitionBarrier);

        // Depth barrier
        VkImageMemoryBarrier depthBarrier{};
        depthBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        depthBarrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        depthBarrier.newLayout                       = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depthBarrier.srcAccessMask                   = 0;
        depthBarrier.dstAccessMask                   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        depthBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        depthBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        depthBarrier.image                           = _sceneTarget.depthImage;
        depthBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthBarrier.subresourceRange.baseMipLevel   = 0;
        depthBarrier.subresourceRange.levelCount     = 1;
        depthBarrier.subresourceRange.baseArrayLayer = 0;
        depthBarrier.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &depthBarrier);
    } else if (!isUiPass) {
        // Depth barrier for swapchain in runtime mode
        VkImageMemoryBarrier depthBarrier{};
        depthBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        depthBarrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        depthBarrier.newLayout                       = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depthBarrier.srcAccessMask                   = 0;
        depthBarrier.dstAccessMask                   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        depthBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        depthBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        depthBarrier.image                           = _swapchainDepthImages[_currentImageIndex];
        depthBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthBarrier.subresourceRange.baseMipLevel   = 0;
        depthBarrier.subresourceRange.levelCount     = 1;
        depthBarrier.subresourceRange.baseArrayLayer = 0;
        depthBarrier.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &depthBarrier);
    }

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType            = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp           = clearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp          = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = {{0.4f, 0.2f, 0.0f, 1.0f}};

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.layerCount           = 1;
    renderingInfo.colorAttachmentCount = 1;

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType                   = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageLayout             = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp                  = clearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    depthAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil = {1.0f, 0};

    if (useSceneTarget) {
        // Render to offscreen scene target
        colorAttachment.imageView = _sceneTarget.colorView;
        renderingInfo.renderArea  = {{0, 0}, {_sceneTarget.extent.width, _sceneTarget.extent.height}};

        depthAttachment.imageView      = _sceneTarget.depthView;
        renderingInfo.pDepthAttachment = &depthAttachment;
    } else {
        // Render directly to swapchain
        colorAttachment.imageView = _swapchain.ImageViews()[_currentImageIndex];
        renderingInfo.renderArea  = {{0, 0}, _swapchain.GetExtent()};

        if (!isUiPass) {
            depthAttachment.imageView      = _swapchainDepthViews[_currentImageIndex];
            renderingInfo.pDepthAttachment = &depthAttachment;
        }
    }

    renderingInfo.pColorAttachments = &colorAttachment;

    vkCmdBeginRenderingKHR(cmd, &renderingInfo);
}

void RendererClass::EndRendering(VkCommandBuffer cmd) {
    vkCmdEndRenderingKHR(cmd);

    bool isUiPass       = (cmd == _uiCommandBuffers[_currentFrame]);
    bool useSceneTarget = !isUiPass && !_renderToSwapchain;

    // If we just rendered to the offscreen target, transition it to SHADER_READ_ONLY_OPTIMAL for ImGui
    if (useSceneTarget) {
        VkImageMemoryBarrier barrier{};
        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                           = _sceneTarget.colorImage;
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;

        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &barrier);
    }
}

void RendererClass::EndFrame() {
    auto cmd   = _commandBuffers[_currentFrame];
    auto uiCmd = _uiCommandBuffers[_currentFrame];

    VkImageMemoryBarrier barrier{};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout                       = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.srcAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask                   = 0;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = _swapchain.Images()[_currentImageIndex];
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;

    // We do the transition on uiCmd, which is the last buffer to execute
    vkCmdPipelineBarrier(uiCmd,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &barrier);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
        throw std::runtime_error("vkEndCommandBuffer failed");

    if (vkEndCommandBuffer(uiCmd) != VK_SUCCESS)
        throw std::runtime_error("vkEndCommandBuffer failed");

    VkCommandBuffer submitCmds[] = {cmd, uiCmd};

    VkSemaphore          waitSems[]   = {_imageAvailableSems[_currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore          signalSems[] = {_renderFinishedSems[_currentImageIndex]};

    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = waitSems;
    submitInfo.pWaitDstStageMask    = waitStages;
    submitInfo.commandBufferCount   = 2;
    submitInfo.pCommandBuffers      = submitCmds;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSems;

    if (vkQueueSubmit(Lgt::Vulkan::g_Device->GraphicsQueue(), 1, &submitInfo, _inFlightFences[_currentFrame]) != VK_SUCCESS)
        throw std::runtime_error("vkQueueSubmit failed");

    VkSwapchainKHR   swapchains[] = {_swapchain.GetHandle()}; // <-- .Handle()
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSems;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = swapchains;
    presentInfo.pImageIndices      = &_currentImageIndex;

    auto result = vkQueuePresentKHR(Lgt::Vulkan::g_Device->PresentQueue(), &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized_) {
        framebufferResized_ = false;
        recreateSwapchain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("vkQueuePresentKHR failed");
    }
}

void RendererClass::createTestResources() {

    // pipeline
    auto vertCode = readFile("shaders/shader.vert.spv");
    auto fragCode = readFile("shaders/shader.frag.spv");

    VkShaderModule vertModule = createShaderModule(Lgt::Vulkan::g_Device, vertCode);
    VkShaderModule fragModule = createShaderModule(Lgt::Vulkan::g_Device, fragCode);

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName  = "main";

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName  = "main";

    VkPipelineShaderStageCreateInfo stages[] = {vertStage, fragStage};

    // Vertex input — positions hardcoded in shader
    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth   = 1.0f;
    rasterizer.cullMode    = VK_CULL_MODE_NONE;
    rasterizer.frontFace   = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlend{};
    colorBlend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments    = &blendAttachment;

    std::array<VkDynamicState, 2>    dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates    = dynamicStates.data();

    VkPipelineCreateFlags2CreateInfo pipelineflags{};
    pipelineflags.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO;
    pipelineflags.flags = VK_PIPELINE_CREATE_2_DESCRIPTOR_HEAP_BIT_EXT;

    // TEMPORARY CODE: Hardcoded depth stencil state for the initial pipeline
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable       = VK_TRUE;
    depthStencil.depthWriteEnable      = VK_TRUE;
    depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable     = VK_FALSE;

    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount  = 1;
    renderingInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

    VkFormat colorFormat                  = _swapchain.GetFormat();
    renderingInfo.pColorAttachmentFormats = &colorFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo{};

    // CHAINING
    pipelineInfo.pNext  = &renderingInfo;
    renderingInfo.pNext = &pipelineflags;

    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount          = 2;
    pipelineInfo.pStages             = stages;
    pipelineInfo.pVertexInputState   = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = &depthStencil;
    pipelineInfo.pColorBlendState    = &colorBlend;
    pipelineInfo.pDynamicState       = &dynamicState;

    pipelineInfo.layout     = VK_NULL_HANDLE;
    pipelineInfo.renderPass = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(
            Lgt::Vulkan::g_Device->Logical(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &TraingleGfxPipeline_) != VK_SUCCESS)
        throw std::runtime_error("vkCreateGraphicsPipelines failed");

    rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    if (vkCreateGraphicsPipelines(
            Lgt::Vulkan::g_Device->Logical(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_wireframePipeline) != VK_SUCCESS)
        throw std::runtime_error("vkCreateGraphicsPipelines for wireframe failed");

    vkDestroyShaderModule(Lgt::Vulkan::g_Device->Logical(), fragModule, nullptr);
    vkDestroyShaderModule(Lgt::Vulkan::g_Device->Logical(), vertModule, nullptr);

    // vertex buffer -- ssbo

    float positions[6] = {0.0, -0.5, 0.5, 0.5, -0.5, 0.5};
    _vertSSBO          = Resources->CreateBuffer(BufferDesc::SSBO(sizeof(positions)));
    auto* dstgpubuffer = Resources->GetBuffer(_vertSSBO);
    vertGpuIndex       = ResourceHeap->AllocateSSBO(_vertSSBO);

    Buffer srcBuffer{};

    VkBufferCreateInfo bufferci{};
    bufferci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferci.size        = 24;
    bufferci.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferci.sharingMode = Vulkan::g_Device->GraphicsFamily() == Vulkan::g_Device->TransferFamily() ? VK_SHARING_MODE_CONCURRENT
                                                                                                    : VK_SHARING_MODE_EXCLUSIVE;

    uint32_t queuefamilyindices[]  = {Vulkan::g_Device->GraphicsFamily(), Vulkan::g_Device->TransferFamily()};
    bufferci.queueFamilyIndexCount = 2;
    bufferci.pQueueFamilyIndices   = queuefamilyindices;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    LGT_ASSERT(Vulkan::g_Allocator->createBuffer(bufferci, allocInfo, srcBuffer.buffer, srcBuffer.allocation), "");

    void* ptr = Vulkan::g_Allocator->map(srcBuffer.allocation);
    memcpy(ptr, positions, 24);
    Vulkan::g_Allocator->unmap(srcBuffer.allocation);

    VkBufferCopy buffercpy{};
    buffercpy.dstOffset = 0;
    buffercpy.srcOffset = 0;
    buffercpy.size      = 24;

    vkWaitForFences(Vulkan::g_Device->Logical(), 1, &_inFlightFences[0], VK_TRUE, UINT64_MAX);
    vkResetFences(Lgt::Vulkan::g_Device->Logical(), 1, &_inFlightFences[_currentFrame]);

    VkCommandBufferBeginInfo cmdBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(_commandBuffers[0], &cmdBeginInfo);
    vkCmdCopyBuffer(_commandBuffers[0], srcBuffer.buffer, dstgpubuffer->buffer, 1, &buffercpy);
    vkEndCommandBuffer(_commandBuffers[0]);

    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &_commandBuffers[0];

    if (vkQueueSubmit(Lgt::Vulkan::g_Device->GraphicsQueue(), 1, &submitInfo, _inFlightFences[0]) != VK_SUCCESS)
        throw std::runtime_error("vkQueueSubmit failed");

    vkQueueWaitIdle(Lgt::Vulkan::g_Device->GraphicsQueue());
    Vulkan::g_Allocator->destroyBuffer(srcBuffer.buffer, srcBuffer.allocation);
}

void RendererClass::framebufferResizeCallback(GLFWwindow* w, int /*width*/, int /*height*/) {
    auto* app                = reinterpret_cast<RendererClass*>(glfwGetWindowUserPointer(w));
    app->framebufferResized_ = true;
}

void RendererClass::createCommandPool() {
    VkCommandPoolCreateInfo ci{};
    ci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    ci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    ci.queueFamilyIndex = Lgt::Vulkan::g_Device->GraphicsFamily();

    if (vkCreateCommandPool(Lgt::Vulkan::g_Device->Logical(), &ci, nullptr, &_commandPool) != VK_SUCCESS)
        throw std::runtime_error("vkCreateCommandPool failed");
}

void RendererClass::createCommandBuffers() {
    _commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    _uiCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = _commandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());

    if (vkAllocateCommandBuffers(Lgt::Vulkan::g_Device->Logical(), &allocInfo, _commandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("vkAllocateCommandBuffers failed");

    if (vkAllocateCommandBuffers(Lgt::Vulkan::g_Device->Logical(), &allocInfo, _uiCommandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("vkAllocateCommandBuffers failed");
}

void RendererClass::createSyncObjects() {
    _imageAvailableSems.resize(MAX_FRAMES_IN_FLIGHT);
    _renderFinishedSems.resize(MAX_FRAMES_IN_FLIGHT);
    _inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semCI{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo     fenceCI{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (vkCreateSemaphore(Lgt::Vulkan::g_Device->Logical(), &semCI, nullptr, &_imageAvailableSems[i]) != VK_SUCCESS ||
            vkCreateSemaphore(Lgt::Vulkan::g_Device->Logical(), &semCI, nullptr, &_renderFinishedSems[i]) != VK_SUCCESS ||
            vkCreateFence(Lgt::Vulkan::g_Device->Logical(), &fenceCI, nullptr, &_inFlightFences[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create sync objects");
    }
}

void RendererClass::createUBOS() {
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        auto bufferHandle = Resources->CreateBuffer(BufferDesc::UBO(sizeof(FrameUBO)));
        ResourceHeap->AllocateUBO(bufferHandle);
        _frameUBO.push_back(std::move(bufferHandle));
    }
}

} // namespace Lgt::Gpu
