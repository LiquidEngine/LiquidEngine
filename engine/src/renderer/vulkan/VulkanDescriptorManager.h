#pragma once

#include "renderer/Descriptor.h"
#include "renderer/Pipeline.h"

#include <vulkan/vulkan.hpp>

namespace liquid {

class VulkanDescriptorManager {
public:
  /**
   * @brief Create Vulkan descriptor manager
   *
   * @param device Vulkan device
   */
  VulkanDescriptorManager(VkDevice device);

  VulkanDescriptorManager(const VulkanDescriptorManager &) = delete;
  VulkanDescriptorManager(VulkanDescriptorManager &&) = delete;
  VulkanDescriptorManager &operator=(const VulkanDescriptorManager &) = delete;
  VulkanDescriptorManager &operator=(VulkanDescriptorManager &&) = delete;

  /**
   * @brief Destroy desceriptor manager
   */
  ~VulkanDescriptorManager();

  /**
   * @brief Get Vulkan descriptor set
   *
   * Gets descriptor set from cache or creates
   * descriptors and returns it
   *
   * @param descriptor Descriptor
   * @param layout Vulkan descriptor layout
   * @return Vulkan descriptor set
   */
  VkDescriptorSet getOrCreateDescriptor(const Descriptor &descriptor,
                                        VkDescriptorSetLayout layout);

private:
  /**
   * @brief Create descriptor set
   *
   * @param descriptor Descriptor
   * @param layout Descriptor layout
   * @return Vulkan descriptor set
   */
  VkDescriptorSet createDescriptorSet(const Descriptor &descriptor,
                                      VkDescriptorSetLayout layout);

  /**
   * @brief Allocate descriptor set
   *
   * @param layout Vulkan descriptor layout
   * @return Vulkan descriptor set
   */
  VkDescriptorSet allocateDescriptorSet(VkDescriptorSetLayout layout);

  /**
   * @brief Create descriptor pool
   */
  void createDescriptorPool();

  /**
   * @brief Create hash from descriptor and layout
   *
   * @param descriptor Descriptor
   * @param layout Descriptor layout
   * @return Hash code
   */
  String createHash(const Descriptor &descriptor, VkDescriptorSetLayout layout);

private:
  std::unordered_map<String, VkDescriptorSet> descriptorCache;
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  VkDevice device;
};

} // namespace liquid
