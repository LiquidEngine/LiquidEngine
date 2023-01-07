#include "liquid/core/Base.h"
#include "liquid/rhi/Descriptor.h"

namespace liquid::rhi {

Descriptor &Descriptor::bind(uint32_t binding,
                             const std::vector<TextureHandle> &textures,
                             DescriptorType type) {
  LIQUID_ASSERT(type == DescriptorType::CombinedImageSampler,
                "Descriptor type for binding " + std::to_string(binding) +
                    " must be combined image sampler");
  bindings.insert({binding, DescriptorBinding{type, textures}});
  std::stringstream ss;
  ss << "b:" << binding << ";t:" << static_cast<uint32_t>(type) << ";";
  for (auto x : textures) {
    ss << "d:" << rhi::castHandleToUint(x) << ";";
  }
  ss << "|";
  hashCode += ss.str();
  return *this;
}

Descriptor &Descriptor::bind(uint32_t binding, BufferHandle buffer,
                             DescriptorType type) {
  LIQUID_ASSERT(type == DescriptorType::UniformBuffer ||
                    type == DescriptorType::StorageBuffer,
                "Descriptor type for binding " + std::to_string(binding) +
                    " must be uniform or storage buffer");

  bindings.insert({binding, DescriptorBinding{type, buffer}});
  std::stringstream ss;
  ss << "b:" << binding << ";t:" << static_cast<uint32_t>(type)
     << ";d:" << rhi::castHandleToUint(buffer) << "|";
  hashCode += ss.str();
  return *this;
}

} // namespace liquid::rhi
