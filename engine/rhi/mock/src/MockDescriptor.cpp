#include "liquid/core/Base.h"
#include "MockDescriptor.h"

namespace liquid::rhi {

template <class T>
static constexpr std::vector<T> vectorFrom(std::span<T> data) {
  return std::vector<T>(data.begin(), data.end());
}

MockDescriptor::MockDescriptor(DescriptorLayoutHandle layout)
    : mLayout(layout) {}

void MockDescriptor::write(uint32_t binding, std::span<TextureHandle> textures,
                           DescriptorType type, uint32_t start) {
  mBindings.push_back({binding, type, start, vectorFrom(textures)});
}

void MockDescriptor::write(uint32_t binding,
                           std::span<TextureViewHandle> textureViews,
                           DescriptorType type, uint32_t start) {
  mBindings.push_back({binding, type, start, vectorFrom(textureViews)});
}

void MockDescriptor::write(uint32_t binding, std::span<BufferHandle> buffers,
                           DescriptorType type, uint32_t start) {
  mBindings.push_back({binding, type, start, vectorFrom(buffers)});
}

void MockDescriptor::write(uint32_t binding,
                           std::span<DescriptorBufferInfo> bufferInfos,
                           DescriptorType type, uint32_t start) {
  mBindings.push_back({binding, type, start, vectorFrom(bufferInfos)});
}

} // namespace liquid::rhi