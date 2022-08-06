#include "liquid/core/Base.h"

#include <ktx.h>
#include <vulkan/vulkan.hpp>
#include <ktxvulkan.h>
#include "KtxTextureLoader.h"
#include "KtxError.h"

namespace liquid {

KtxTextureLoader::KtxTextureLoader(rhi::ResourceRegistry &registry)
    : mRegistry(registry) {}

rhi::TextureHandle KtxTextureLoader::loadFromFile(const String &filename) {
  ktxTexture *ktxTextureData = nullptr;
  KTX_error_code result = ktxTexture_CreateFromNamedFile(
      filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
      &ktxTextureData);

  LIQUID_ASSERT(result == KTX_SUCCESS,
                KtxError("Failed to load KTX file", result).what());

  LIQUID_ASSERT(ktxTextureData->numDimensions == 2,
                "Only 2D textures are supported");

  LIQUID_ASSERT(
      !ktxTextureData->isArray,
      KtxError("Texture arrays are not supported", KTX_UNSUPPORTED_FEATURE)
          .what());

  static constexpr uint32_t CubemapSides = 6;

  rhi::TextureDescription description;
  description.type = ktxTextureData->isCubemap ? rhi::TextureType::Cubemap
                                               : rhi::TextureType::Standard;
  description.width = ktxTextureData->baseWidth;
  description.height = ktxTextureData->baseHeight;
  description.depth = ktxTextureData->baseDepth;
  description.format = ktxTexture_GetVkFormat(ktxTextureData);
  description.layers = ktxTextureData->numLayers *
                       (ktxTextureData->isCubemap ? CubemapSides : 1);
  description.size = ktxTexture_GetDataSizeUncompressed(ktxTextureData);
  description.usage = rhi::TextureUsage::Sampled | rhi::TextureUsage::Color |
                      rhi::TextureUsage::TransferDestination;
  description.data = new char[description.size];

  char *srcData = reinterpret_cast<char *>(ktxTexture_GetData(ktxTextureData));

  if (ktxTextureData->isCubemap) {
    size_t faceSize = ktxTexture_GetImageSize(ktxTextureData, 0);

    char *dstData = static_cast<char *>(description.data);

    for (size_t i = 0; i < CubemapSides; ++i) {
      size_t offset = 0;
      ktxTexture_GetImageOffset(ktxTextureData, 0, 0,
                                static_cast<ktx_uint32_t>(i), &offset);

      memcpy(dstData + faceSize * i, srcData + offset, faceSize);
    }
  } else {
    memcpy(description.data, srcData, description.size);
  }

  ktxTexture_Destroy(ktxTextureData);

  return mRegistry.setTexture(description);
}

} // namespace liquid
