#pragma once

#include "VulkanPhysicalDevice.h"
#include "VulkanRenderBackend.h"
#include "VulkanDeviceObject.h"

#include <vk_mem_alloc.h>

namespace quoll::rhi {

class VulkanResourceAllocator {
public:
  VulkanResourceAllocator(VulkanRenderBackend &backend,
                          VulkanPhysicalDevice &physicalDevice,
                          VulkanDeviceObject &device);

  ~VulkanResourceAllocator();

  VulkanResourceAllocator(const VulkanResourceAllocator &) = delete;
  VulkanResourceAllocator &operator=(const VulkanResourceAllocator &) = delete;
  VulkanResourceAllocator(VulkanResourceAllocator &&) = delete;
  VulkanResourceAllocator &operator=(VulkanResourceAllocator &&) = delete;

  inline operator VmaAllocator() { return mAllocator; }

private:
  VmaAllocator mAllocator = VK_NULL_HANDLE;
};

} // namespace quoll::rhi
