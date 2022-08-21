#include "liquid/core/Base.h"
#include "NativeWindowTools.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <dwmapi.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

namespace liquid::platform_tools {

void NativeWindowTools::enableDarkMode(GLFWwindow *window) {
  HWND hWnd = glfwGetWin32Window(window);
  BOOL value = TRUE;
  ::DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value,
                          sizeof(value));
}

} // namespace liquid::platform_tools
