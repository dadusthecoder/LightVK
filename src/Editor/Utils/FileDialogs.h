#pragma once
#include <string>

struct GLFWwindow;

namespace Lgt {

extern GLFWwindow* g_WindowHandle;

class FileDialogs {
public:
    // Returns empty string if cancelled
    static std::string OpenFile(const char* filter);
    static std::string SaveFile(const char* filter);
};

} // namespace Lgt
