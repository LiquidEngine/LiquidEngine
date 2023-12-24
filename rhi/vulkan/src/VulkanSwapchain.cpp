#include "quoll/core/Base.h"
#include "quoll/core/Engine.h"

#include "VulkanSwapchain.h"
#include "VulkanError.h"
#include "VulkanLog.h"

namespace quoll::rhi {

VulkanSwapchain::VulkanSwapchain(VulkanRenderBackend &backend,
                                 const VulkanPhysicalDevice &physicalDevice,
                                 VulkanDeviceObject &device,
                                 VulkanResourceRegistry &registry,
                                 VulkanResourceAllocator &allocator)
    : mDevice(device), mRegistry(registry) {
  create(backend, physicalDevice, allocator);
}

VulkanSwapchain::~VulkanSwapchain() { destroy(); }

void VulkanSwapchain::recreate(VulkanRenderBackend &backend,
                               const VulkanPhysicalDevice &physicalDevice,
                               VulkanResourceAllocator &allocator) {
  VkSwapchainKHR oldSwapchain = mSwapchain;

  create(backend, physicalDevice, allocator);

  vkDestroySwapchainKHR(mDevice, oldSwapchain, nullptr);
  LOG_DEBUG_VK("Swapchain destroyed", mSwapchain);
}

void VulkanSwapchain::destroy() {
  if (mSwapchain) {
    vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);
    mSwapchain = VK_NULL_HANDLE;
    LOG_DEBUG_VK("Swapchain destroyed", mSwapchain);
  }
}

void VulkanSwapchain::create(VulkanRenderBackend &backend,
                             const VulkanPhysicalDevice &physicalDevice,
                             VulkanResourceAllocator &allocator) {
  VkSwapchainKHR oldSwapchain = mSwapchain;
  const auto &surfaceCapabilities =
      physicalDevice.getSurfaceCapabilities(backend.getSurface());

  pickMostSuitableSurfaceFormat(
      physicalDevice.getSurfaceFormats(backend.getSurface()));
  pickMostSuitablePresentMode(
      physicalDevice.getPresentModes(backend.getSurface()));
  calculateExtent(surfaceCapabilities, backend.getFramebufferSize());

  u32 imageCount = std::min(surfaceCapabilities.minImageCount + 1,
                            surfaceCapabilities.maxImageCount);

  bool sameQueueFamily =
      physicalDevice.getQueueFamilyIndices().getGraphicsFamily() ==
      physicalDevice.getQueueFamilyIndices().getPresentFamily();

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.pNext = nullptr;
  createInfo.flags = 0;
  createInfo.surface = backend.getSurface();
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = mSurfaceFormat.format;
  createInfo.imageColorSpace = mSurfaceFormat.colorSpace;
  createInfo.imageExtent = VkExtent2D{mExtent.x, mExtent.y};
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  if (sameQueueFamily) {
    auto &array = physicalDevice.getQueueFamilyIndices().toArray();
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = static_cast<u32>(array.size());
    createInfo.pQueueFamilyIndices = array.data();
  } else {
    // TODO: Handle the case where graphics
    // and present queues are not the same
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
  }

  createInfo.preTransform = surfaceCapabilities.currentTransform;
  createInfo.compositeAlpha = getSuitableCompositeAlpha(surfaceCapabilities);
  createInfo.presentMode = mPresentMode;
  createInfo.clipped = true;
  createInfo.oldSwapchain = oldSwapchain;

  checkForVulkanError(
      vkCreateSwapchainKHR(mDevice, &createInfo, nullptr, &mSwapchain),
      "Failed to create swapchain");

  vkGetSwapchainImagesKHR(mDevice, mSwapchain, &imageCount, nullptr);
  std::vector<VkImage> images(imageCount, VK_NULL_HANDLE);
  vkGetSwapchainImagesKHR(mDevice, mSwapchain, &imageCount, images.data());

  usize oldSize = mTextures.size();

  // Delete old textures
  for (usize i = oldSize + 1; i < images.size(); ++i) {
    mRegistry.deleteTexture(static_cast<TextureHandle>(i));
  }

  mTextures.resize(images.size());

  for (usize i = 0; i < mTextures.size(); ++i) {
    VkImage image = images.at(i);
    VkImageView imageView = VK_NULL_HANDLE;

    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image;

    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = mSurfaceFormat.format;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.layerCount = 1;

    checkForVulkanError(
        vkCreateImageView(mDevice, &createInfo, nullptr, &imageView),
        "Failed to create image views for swapchain");

    mTextures.at(i) = static_cast<rhi::TextureHandle>(i);

    mRegistry.setTexture(std::make_unique<VulkanTexture>(
                             image, imageView, VK_NULL_HANDLE,
                             mSurfaceFormat.format, allocator, mDevice),
                         mTextures.at(i));
  }

  LOG_DEBUG_VK("Swapchain created. Images: " << mTextures.size()
                                             << "; Extent: [" << mExtent.x
                                             << ", " << mExtent.y << "]",
               mSwapchain);
}

void VulkanSwapchain::pickMostSuitableSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &surfaceFormats) {

  auto it = std::find_if(
      surfaceFormats.begin(), surfaceFormats.end(),
      [](auto surfaceFormat) -> bool {
        return (surfaceFormat.format == VK_FORMAT_R8G8B8A8_SRGB ||
                surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB) &&
               surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
      });

  QuollAssert(it != surfaceFormats.end(),
              "Most suitable surface format not found");

  mSurfaceFormat = it != surfaceFormats.end() ? *it : surfaceFormats[0];
}

void VulkanSwapchain::pickMostSuitablePresentMode(
    const std::vector<VkPresentModeKHR> &presentModes) {

  auto it = std::find_if(presentModes.begin(), presentModes.end(),
                         [](auto presentMode) {
                           return presentMode == VK_PRESENT_MODE_MAILBOX_KHR;
                         });

  mPresentMode = it != presentModes.end() ? *it : VK_PRESENT_MODE_FIFO_KHR;
}

void VulkanSwapchain::calculateExtent(
    const VkSurfaceCapabilitiesKHR &capabilities, const glm::uvec2 &size) {

  u32 width = std::max(capabilities.minImageExtent.width,
                       std::min(capabilities.maxImageExtent.width, size.x));
  u32 height = std::max(capabilities.minImageExtent.height,
                        std::min(capabilities.maxImageExtent.height, size.y));

  mExtent = glm::uvec2{width, height};
}

VkCompositeAlphaFlagBitsKHR VulkanSwapchain::getSuitableCompositeAlpha(
    const VkSurfaceCapabilitiesKHR &capabilities) const {
  if (capabilities.supportedCompositeAlpha &
      VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) {
    return VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
  }

  if (capabilities.supportedCompositeAlpha &
      VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) {
    return VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
  }

  if (capabilities.supportedCompositeAlpha &
      VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) {
    return VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
  }

  return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
}

u32 VulkanSwapchain::acquireNextImage(VkSemaphore imageAvailableSemaphore) {
  QUOLL_PROFILE_EVENT("VulkanSwapchain::acquireNextImage");
  u32 imageIndex = 0;
  VkResult result = vkAcquireNextImageKHR(
      mDevice, mSwapchain, std::numeric_limits<u64>::max(),
      imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    return std::numeric_limits<u32>::max();
  }

  return imageIndex;
}

} // namespace quoll::rhi
