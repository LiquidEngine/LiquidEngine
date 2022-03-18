#pragma once

#include "RenderGraph.h"
#include "RenderGraphPipelineDescription.h"

#include "liquid/rhi/RenderCommandList.h"
#include "liquid/rhi/RenderPassDescription.h"
#include "liquid/rhi/ResourceRegistry.h"

namespace liquid {

class RenderGraphEvaluator {
  struct VulkanAttachmentInfo {
    VkClearValue clearValue;
    std::vector<rhi::TextureHandle> framebufferAttachments;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t layers = 0;

    rhi::RenderPassAttachmentDescription attachment;
  };

public:
  /**
   * @brief Create render graph evaluator
   *
   * @param registry Resource registry
   */
  RenderGraphEvaluator(rhi::ResourceRegistry &registry);

  /**
   * @brief Build passes
   *
   * @param compiled Compiled passes
   * @param graph Render graph
   * @param swapchainRecreated Rebuild swapchain related passes
   * @param numSwapchainImages Number of swapchain images
   * @param extent Swapchain extent
   */
  void build(std::vector<RenderGraphPassBase *> &compiled, RenderGraph &graph,
             bool swapchainRecreated, uint32_t numSwapchainImages,
             const glm::uvec2 &extent);

  /**
   * @brief Execute render graph
   *
   * @param commandList Command list
   * @param passes Topologically sorted passes
   * @param graph Render graph
   * @param imageIdx Swapchain image index
   */
  void execute(rhi::RenderCommandList &commandList,
               const std::vector<RenderGraphPassBase *> &passes,
               RenderGraph &graph, uint32_t imageIdx);

  /**
   * @brief Get resource registry
   *
   * @return Resouce registry
   */
  inline rhi::ResourceRegistry &getRegistry() { return mRegistry; }

private:
  /**
   * @brief Build render pass resources
   *
   * @param pass Render pass
   * @param graph Render graph
   * @param force Force build even if the pass resources exist
   * @param numSwapchainImages Number of swapchain images
   * @param extent Swapchain extent
   */
  void buildPass(RenderGraphPassBase *pass, RenderGraph &graph, bool force,
                 uint32_t numSwapchainImages, const glm::uvec2 &extent);

  /**
   * @brief Create swapchain attachment
   *
   * @param attachment Attachment description
   * @param numSwapchainAttachments Number of swapchain attachments
   * @param extent Swapchain extent
   * @return Attachment info
   */
  VulkanAttachmentInfo
  createSwapchainAttachment(const RenderPassAttachment &attachment,
                            uint32_t numSwapchainAttachments,
                            const glm::uvec2 &extent);

  /**
   * @brief Create color attachment
   *
   * @param attachment Attachment description
   * @param texture Texture
   * @param extent Swapchain extent
   * @return Attachment info
   */
  VulkanAttachmentInfo
  createColorAttachment(const RenderPassAttachment &attachment,
                        rhi::TextureHandle texture, const glm::uvec2 &extent);

  /**
   * @brief Create depth attachment
   *
   * @param attachment Attachment description
   * @param texture Texture
   * @param extent Swapchain extent
   * @return Attachment info
   */
  VulkanAttachmentInfo
  createDepthAttachment(const RenderPassAttachment &attachment,
                        rhi::TextureHandle texture, const glm::uvec2 &extent);

  /**
   * @brief Check if pass is has swapchain relative resources
   *
   * @param pass Render pass
   * @retval true Has swapchain resources
   * @retval false Does not have swapchain relative resources
   */
  bool hasSwapchainRelativeResources(RenderGraphPassBase *pass);

private:
  rhi::ResourceRegistry &mRegistry;
};

} // namespace liquid
