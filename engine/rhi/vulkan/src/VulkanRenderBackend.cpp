#include "liquid/core/Base.h"
#include "liquid/core/Engine.h"

#include "VulkanRenderBackend.h"
#include "VulkanWindowExtensions.h"
#include "VulkanWindow.h"
#include "VulkanTexture.h"
#include "VulkanTextureView.h"
#include "VulkanBuffer.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanPipeline.h"
#include "VulkanShader.h"
#include <GLFW/glfw3.h>
#include "VulkanRenderDevice.h"
#include "VulkanError.h"
#include "VulkanLog.h"

namespace liquid::rhi {

static const String LIQUID_ENGINE_NAME = "Liquid";

VulkanRenderBackend::VulkanRenderBackend(Window &window, bool enableValidations)
    : mWindow(window) {
  createInstance("RHI", enableValidations);

  mSurface = createSurfaceFromWindow(mInstance, window);
  LOG_DEBUG_VK("Surface created", mSurface);

  mResizeListener = window.addResizeHandler(
      [this](auto width, auto height) { mFramebufferResized = true; });
}

VulkanRenderBackend::~VulkanRenderBackend() {
  mWindow.removeResizeHandler(mResizeListener);

  mDevice.reset();

  if (mSurface) {
    vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
    LOG_DEBUG_VK("Surface destroyed", mSurface);
  }

  mValidator.detachFromInstance(mInstance);

  if (mInstance) {
    vkDestroyInstance(mInstance, nullptr);
    LOG_DEBUG_VK("Instance destroyed", mInstance);
  }
}

RenderDevice *VulkanRenderBackend::createDefaultDevice() {
  if (!mDevice) {
    auto &&physicalDevice = pickPhysicalDevice();
    mDevice.reset(new VulkanRenderDevice(*this, physicalDevice));
  }

  return mDevice.get();
}

void VulkanRenderBackend::finishFramebufferResize() {
  mFramebufferResized = false;
}

void VulkanRenderBackend::createInstance(StringView applicationName,
                                         bool enableValidations) {
  VkResult result = volkInitialize();
  LIQUID_ASSERT(result == VK_SUCCESS, "Cannot initialize Vulkan loader");

  std::vector<const char *> extensions;
  extensions.resize(vulkanWindowExtensions.size());
  std::transform(vulkanWindowExtensions.begin(), vulkanWindowExtensions.end(),
                 extensions.begin(),
                 [](const String &ext) { return ext.c_str(); });

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pNext = nullptr;
  appInfo.pApplicationName = String(applicationName).c_str();
  appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 0, 12, 0);
  appInfo.pEngineName = LIQUID_ENGINE_NAME.c_str();
  appInfo.engineVersion = VK_MAKE_API_VERSION(0, 0, 12, 0);
  appInfo.apiVersion = VK_API_VERSION_1_2;

  VkInstanceCreateInfo createInstanceInfo{};
  createInstanceInfo.flags = 0;
  createInstanceInfo.pNext = nullptr;
  createInstanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInstanceInfo.pApplicationInfo = &appInfo;
  createInstanceInfo.enabledLayerCount = 0;
  createInstanceInfo.ppEnabledLayerNames = nullptr;

  if (enableValidations) {
    mValidator.attachToInstanceCreateConfig(createInstanceInfo);
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  createInstanceInfo.enabledExtensionCount =
      static_cast<uint32_t>(extensions.size());
  createInstanceInfo.ppEnabledExtensionNames = extensions.data();

  checkForVulkanError(
      vkCreateInstance(&createInstanceInfo, nullptr, &mInstance),
      "Failed to create instance");
  volkLoadInstance(mInstance);

  if (enableValidations) {
    mValidator.attachToInstance(mInstance);
    Engine::getLogger().info() << "Vulkan validations enabled";
  }

  LOG_DEBUG_VK("Vulkan instance created", mInstance);
}

VulkanPhysicalDevice VulkanRenderBackend::pickPhysicalDevice() {
  VulkanPhysicalDevice physicalDevice;

  auto &&devices =
      VulkanPhysicalDevice::getPhysicalDevices(mInstance, mSurface);
  auto it =
      std::find_if(devices.begin(), devices.end(), [this](const auto &device) {
        return device.getQueueFamilyIndices().isComplete() &&
               device.supportsSwapchain() &&
               !device.getSurfaceFormats(mSurface).empty() &&
               !device.getPresentModes(mSurface).empty();
      });

  LIQUID_ASSERT(it != devices.end(), "No suitable physical device found");

  physicalDevice = *it;
  LOG_DEBUG_VK_NO_HANDLE(
      "Physical device selected: " << physicalDevice.getName());
  return physicalDevice;
}

} // namespace liquid::rhi
