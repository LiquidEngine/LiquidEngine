#include "liquid/core/Base.h"
#include "VulkanResourceRegistry.h"
#include "VulkanTexture.h"
#include "VulkanBuffer.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanPipeline.h"
#include "VulkanShader.h"

namespace liquid::rhi {

void VulkanResourceRegistry::setShader(ShaderHandle handle,
                                       std::unique_ptr<VulkanShader> &&shader) {
  mShaders.insert_or_assign(handle, std::move(shader));
}

void VulkanResourceRegistry::deleteShader(ShaderHandle handle) {
  mShaders.erase(handle);
}

BufferHandle
VulkanResourceRegistry::setBuffer(std::unique_ptr<VulkanBuffer> &&buffer) {
  auto handle = BufferHandle{mLastBuffer};
  mLastBuffer++;
  mBuffers.insert_or_assign(handle, std::move(buffer));

  return handle;
}

void VulkanResourceRegistry::deleteBuffer(BufferHandle handle) {
  mBuffers.erase(handle);
}

void VulkanResourceRegistry::setTexture(
    TextureHandle handle, std::unique_ptr<VulkanTexture> &&texture) {
  if (texture->isFramebufferRelative()) {
    mSwapchainRelativeTextures.insert(handle);
  }

  mTextures.insert_or_assign(handle, std::move(texture));
}

void VulkanResourceRegistry::deleteTexture(TextureHandle handle) {
  mTextures.erase(handle);
}

void VulkanResourceRegistry::deleteDanglingSwapchainRelativeTextures() {
  for (auto it = mSwapchainRelativeTextures.begin();
       it != mSwapchainRelativeTextures.end(); ++it) {
    auto textureIt = mTextures.find(*it);
    if (textureIt == mTextures.end() ||
        !textureIt->second->isFramebufferRelative()) {
      it = mSwapchainRelativeTextures.erase(it);
    }
  }
}

void VulkanResourceRegistry::setRenderPass(
    rhi::RenderPassHandle handle,
    std::unique_ptr<VulkanRenderPass> &&renderPass) {
  mRenderPasses.insert_or_assign(handle, std::move(renderPass));
}

void VulkanResourceRegistry::deleteRenderPass(RenderPassHandle handle) {
  mRenderPasses.erase(handle);
}

void VulkanResourceRegistry::setFramebuffer(
    FramebufferHandle handle,
    std::unique_ptr<VulkanFramebuffer> &&framebuffer) {
  mFramebuffers.insert_or_assign(handle, std::move(framebuffer));
}

void VulkanResourceRegistry::deleteFramebuffer(FramebufferHandle handle) {
  mFramebuffers.erase(handle);
}

void VulkanResourceRegistry::setPipeline(
    PipelineHandle handle, std::unique_ptr<VulkanPipeline> &&pipeline) {
  mPipelines.insert_or_assign(handle, std::move(pipeline));
}

void VulkanResourceRegistry::deletePipeline(PipelineHandle handle) {
  mPipelines.erase(handle);
}

} // namespace liquid::rhi
