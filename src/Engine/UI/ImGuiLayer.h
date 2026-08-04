#pragma once
#include <volk.h>

struct GLFWwindow;

namespace Lgt {

class ImGuiLayer {
public:
    void Init(GLFWwindow* window, VkFormat colorFormat);
    void BeginFrame();
    void EndFrame(VkCommandBuffer cmd);
    void Shutdown();

private:
    VkDescriptorPool _descriptorPool = VK_NULL_HANDLE;
};

} // namespace Lgt
