#pragma once

#include "liquid/rhi/DeviceStats.h"
#include "liquid/rhi/NativeRenderCommandListInterface.h"

#include "VulkanResourceRegistry.h"
#include "VulkanDescriptorManager.h"
#include "VulkanDescriptorPool.h"

namespace liquid::rhi {

/**
 * @brief Vulkan command buffer
 */
class VulkanCommandBuffer : public NativeRenderCommandListInterface {
public:
  /**
   * @brief Create Vulkan command buffer
   *
   * @param commandBuffer Command buffer
   * @param registry Resource registry
   * @param descriptorManager Vulkan descriptor manager
   * @param descriptorPool Descriptor pool
   * @param stats Device stats
   */
  VulkanCommandBuffer(VkCommandBuffer commandBuffer,
                      const VulkanResourceRegistry &registry,
                      const VulkanDescriptorPool &descriptorPool,
                      VulkanDescriptorManager &descriptorManager,
                      DeviceStats &stats);

  /**
   * @brief Destructor
   */
  ~VulkanCommandBuffer() = default;

  VulkanCommandBuffer(const VulkanCommandBuffer &) = delete;
  VulkanCommandBuffer &operator=(const VulkanCommandBuffer &) = delete;
  VulkanCommandBuffer(VulkanCommandBuffer &&) = delete;
  VulkanCommandBuffer &operator=(VulkanCommandBuffer &&) = delete;

  /**
   * @brief Get Vulkan command buffer
   *
   * @return Vulkan command buffer
   */
  inline VkCommandBuffer getVulkanCommandBuffer() const {
    return mCommandBuffer;
  }

  /**
   * @brief Begin render pass
   *
   * @param renderPass Render pass
   * @param framebuffer Framebuffer
   * @param renderAreaOffset Render area offset
   * @param renderAreaSize Render area size
   */
  void beginRenderPass(rhi::RenderPassHandle renderPass,
                       FramebufferHandle framebuffer,
                       const glm::ivec2 &renderAreaOffset,
                       const glm::uvec2 &renderAreaSize) override;

  /**
   * @brief End render pass
   */
  void endRenderPass() override;

  /**
   * @brief Bind pipeline
   *
   * @param pipeline Pipeline
   */
  void bindPipeline(PipelineHandle pipeline) override;

  /**
   * @brief Bind descriptor
   *
   * @param pipeline Pipeline
   * @param firstSet First set
   * @param descriptor Descriptor
   */
  void bindDescriptor(PipelineHandle pipeline, uint32_t firstSet,
                      const Descriptor &descriptor) override;

  /**
   * @brief Bind descriptor
   *
   * @param pipeline Pipeline
   * @param firstSet First set
   * @param descriptor Descriptor
   */
  void bindDescriptor(PipelineHandle pipeline, uint32_t firstSet,
                      const n::Descriptor &descriptor) override;

  /**
   * @brief Bind vertex buffer
   *
   * @param buffer Vertex buffer
   */
  void bindVertexBuffer(BufferHandle buffer) override;

  /**
   * @brief Bind index buffer
   *
   * @param buffer Index buffer
   * @param indexType Index buffer data type
   */
  void bindIndexBuffer(BufferHandle buffer, IndexType indexType) override;

  /**
   * @brief Push constants
   *
   * @param pipeline Pipeline
   * @param shaderStage Shader stage
   * @param offset Offset
   * @param size Size
   * @param data Data
   */
  void pushConstants(PipelineHandle pipeline, ShaderStage shaderStage,
                     uint32_t offset, uint32_t size, void *data) override;

  /**
   * @brief Draw
   *
   * @param vertexCount Vertex count
   * @param firstVertex First vertex
   * @param instanceCount Instance count
   * @param firstInstance First instance
   */
  void draw(uint32_t vertexCount, uint32_t firstVertex, uint32_t instanceCount,
            uint32_t firstInstance) override;

  /**
   * @brief Draw indexed
   *
   * @param indexCount Index count
   * @param firstIndex Offset of first index
   * @param vertexOffset Vertex offset
   * @param instanceCount Instance count
   * @param firstInstance First instance
   */
  void drawIndexed(uint32_t indexCount, uint32_t firstIndex,
                   int32_t vertexOffset, uint32_t instanceCount,
                   uint32_t firstInstance) override;

  /**
   * @brief Set viewport
   *
   * @param offset Viewport offset
   * @param size Viewport size
   * @param depthRange Viewport depth range
   */
  void setViewport(const glm::vec2 &offset, const glm::vec2 &size,
                   const glm::vec2 &depthRange) override;

  /**
   * @brief Set scissor
   *
   * @param offset Scissor offset
   * @param size Scissor size
   */
  void setScissor(const glm::ivec2 &offset, const glm::uvec2 &size) override;

  /**
   * @brief Pipeline barrier
   *
   * @param srcStage Source pipeline stage
   * @param dstStage Destination pipeline stage
   * @param memoryBarriers Memory barriers
   * @param imageBarriers Image barriers
   */
  void pipelineBarrier(PipelineStage srcStage, PipelineStage dstStage,
                       const std::vector<MemoryBarrier> &memoryBarriers,
                       const std::vector<ImageBarrier> &imageBarriers) override;

private:
  VkCommandBuffer mCommandBuffer = VK_NULL_HANDLE;

  const VulkanResourceRegistry &mRegistry;
  const VulkanDescriptorPool &mDescriptorPool;
  DeviceStats &mStats;
  VulkanDescriptorManager &mDescriptorManager;
};

} // namespace liquid::rhi
