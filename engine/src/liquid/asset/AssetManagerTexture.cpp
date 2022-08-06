#include "liquid/core/Base.h"
#include "liquid/core/Version.h"

#include "AssetManager.h"

#include "AssetFileHeader.h"
#include "OutputBinaryStream.h"
#include "InputBinaryStream.h"

#include <ktx.h>
#include <vulkan/vulkan.h>
#include <ktxvulkan.h>
#include "liquid/loaders/KtxError.h"

namespace liquid {

Result<Path>
AssetManager::createTextureFromAsset(const AssetData<TextureAsset> &asset) {
  ktxTextureCreateInfo createInfo{};
  createInfo.baseWidth = asset.data.width;
  createInfo.baseHeight = asset.data.height;
  createInfo.baseDepth = 1;
  createInfo.numDimensions = 2;
  createInfo.numFaces = 1;
  createInfo.numLayers = 1;
  createInfo.numLevels = 1;
  createInfo.isArray = KTX_FALSE;
  createInfo.generateMipmaps = KTX_FALSE;
  createInfo.vkFormat = VK_FORMAT_R8G8B8A8_SRGB;

  Path assetPath = (mAssetsPath / (asset.name + ".ktx2")).make_preferred();

  ktxTexture2 *texture = nullptr;
  {
    auto res = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                  &texture);

    if (res != KTX_SUCCESS) {
      return Result<Path>::Error(
          KtxError("Cannot create KTX texture", res).what());
    }

    LIQUID_ASSERT(res == KTX_SUCCESS, "Cannot create KTX texture");
  }

  auto *baseTexture = reinterpret_cast<ktxTexture *>(texture);

  ktxTexture_SetImageFromMemory(
      baseTexture, 0, 0, 0, static_cast<const ktx_uint8_t *>(asset.data.data),
      asset.size);

  {
    auto res =
        ktxTexture_WriteToNamedFile(baseTexture, assetPath.string().c_str());

    if (res != KTX_SUCCESS) {
      return Result<Path>::Error(
          KtxError("Cannot write KTX texture to a file", res).what());
    }
  }

  ktxTexture_Destroy(baseTexture);

  return Result<Path>::Ok(assetPath);
}

Result<TextureAssetHandle>
AssetManager::loadTextureFromFile(const Path &filePath) {
  static constexpr uint32_t CubemapSides = 6;

  ktxTexture *ktxTextureData = nullptr;
  KTX_error_code result = ktxTexture_CreateFromNamedFile(
      filePath.string().c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
      &ktxTextureData);

  if (result != KTX_SUCCESS) {
    return Result<TextureAssetHandle>::Error(
        KtxError("Cannot create KTX texture", result).what());
  }

  if (ktxTextureData->numDimensions != 2) {
    return Result<TextureAssetHandle>::Error("Only 2D textures are supported");
  }

  if (ktxTextureData->isArray) {
    return Result<TextureAssetHandle>::Error(
        "Texture arrays are not supported");
  }

  AssetData<TextureAsset> texture{};
  texture.size = ktxTexture_GetDataSizeUncompressed(ktxTextureData);
  texture.path = filePath;
  texture.relativePath = std::filesystem::relative(filePath, mAssetsPath);
  texture.name = texture.relativePath.string();
  texture.data.data = new char[texture.size];
  texture.data.width = ktxTextureData->baseWidth;
  texture.data.height = ktxTextureData->baseHeight;
  texture.data.layers = ktxTextureData->numLayers *
                        (ktxTextureData->isCubemap ? CubemapSides : 1);
  texture.data.type = ktxTextureData->isCubemap ? TextureAssetType::Cubemap
                                                : TextureAssetType::Standard;
  texture.data.format = ktxTexture_GetVkFormat(ktxTextureData);

  char *srcData = reinterpret_cast<char *>(ktxTexture_GetData(ktxTextureData));

  if (ktxTextureData->isCubemap) {
    size_t faceSize = ktxTexture_GetImageSize(ktxTextureData, 0);

    char *dstData = static_cast<char *>(texture.data.data);

    for (size_t i = 0; i < CubemapSides; ++i) {
      size_t offset = 0;
      ktxTexture_GetImageOffset(ktxTextureData, 0, 0,
                                static_cast<ktx_uint32_t>(i), &offset);

      memcpy(dstData + faceSize * i, srcData + offset, faceSize);
    }
  } else {
    memcpy(texture.data.data, srcData, texture.size);
  }

  ktxTexture_Destroy(ktxTextureData);

  return Result<TextureAssetHandle>::Ok(
      mRegistry.getTextures().addAsset(texture));
}

Result<TextureAssetHandle>
AssetManager::getOrLoadTextureFromPath(StringView relativePath) {
  if (relativePath.empty()) {
    return Result<TextureAssetHandle>::Ok(TextureAssetHandle::Invalid);
  }

  Path fullPath = (mAssetsPath / relativePath).make_preferred();

  for (auto &[handle, asset] : mRegistry.getTextures().getAssets()) {
    if (asset.path == fullPath) {
      return Result<TextureAssetHandle>::Ok(handle);
    }
  }

  return loadTextureFromFile(fullPath);
}

} // namespace liquid
