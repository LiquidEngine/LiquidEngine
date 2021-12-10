#pragma once

#include "renderer/ResourceManager.h"
#include "VulkanDescriptorManager.h"

namespace liquid {

class VulkanResourceManager : public ResourceManager {
public:
  /**
   * @brief Create Vulkan resource manager
   *
   * @param descriptorManager Descriptor manager
   */
  VulkanResourceManager(VulkanDescriptorManager *descriptorManager);

  /**
   * @brief Create Vulkan material resource binder
   *
   * @param material Material
   * @return Vulkan material resource binder
   */
  SharedPtr<MaterialResourceBinder>
  createMaterialResourceBinder(Material *material) override;

private:
  VulkanDescriptorManager *descriptorManager;
};

} // namespace liquid
