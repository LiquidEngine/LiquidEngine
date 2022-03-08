#pragma once

#include <imgui.h>

#include "liquid/window/glfw/GLFWWindow.h"
#include "liquid/renderer/RenderCommandList.h"
#include "liquid/renderer/Pipeline.h"

#include "../../rhi/vulkan/VulkanRenderDevice.h"

namespace liquid {

class ImguiRenderer {
  struct FrameData {
    BufferHandle vertexBuffer = 0;
    size_t vertexBufferSize = 0;
    void *vertexBufferData = nullptr;

    BufferHandle indexBuffer = 0;
    void *indexBufferData = nullptr;
    size_t indexBufferSize = 0;

    bool firstTime = true;
  };

public:
  ImguiRenderer(GLFWWindow *window, experimental::VulkanRenderDevice *device,
                experimental::ResourceRegistry &registry);
  ~ImguiRenderer();

  ImguiRenderer(const ImguiRenderer &rhs) = delete;
  ImguiRenderer(ImguiRenderer &&rhs) = delete;
  ImguiRenderer &operator=(const ImguiRenderer &rhs) = delete;
  ImguiRenderer &operator=(ImguiRenderer &&rhs) = delete;

  static void beginRendering();
  static void endRendering();

  void draw(RenderCommandList &commandList,
            const SharedPtr<Pipeline> &pipeline);

private:
  void loadFonts();

  void setupRenderStates(ImDrawData *draw_data, RenderCommandList &commandList,
                         int fbWidth, int fbHeight,
                         const SharedPtr<Pipeline> &pipeline);

private:
  experimental::VulkanRenderDevice *device = nullptr;
  experimental::ResourceRegistry &registry;

  TextureHandle fontTexture = 0;

  std::vector<FrameData> frameData;
  uint32_t currentFrame = 0;
};

} // namespace liquid
