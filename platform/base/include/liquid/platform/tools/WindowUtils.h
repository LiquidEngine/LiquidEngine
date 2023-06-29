#pragma once

#include <GLFW/glfw3.h>

namespace liquid::platform {

/**
 * @brief Platform window utilities
 */
class WindowUtils {
public:
  /**
   * @brief Enable dark mode
   *
   * @param window GLFW window
   */
  static void enableDarkMode(GLFWwindow *window);
};

} // namespace liquid::platform
