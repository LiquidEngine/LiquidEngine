#include "liquid/core/Base.h"
#include "AssetCache.h"

#include <ft2build.h>
#include <freetype/freetype.h>

#include "liquid/text/MsdfAtlas.h"
#include "liquid/text/MsdfLoader.h"

#include "FontAsset.h"

namespace liquid {

Result<Path> AssetCache::createFontFromSource(const Path &sourcePath,
                                              const Uuid &uuid) {
  if (uuid.isEmpty()) {
    LIQUID_ASSERT(false, "Invalid uuid provided");
    return Result<Path>::Error("Invalid uuid provided");
  }

  using co = std::filesystem::copy_options;

  auto assetPath = getPathFromUuid(uuid);

  if (!std::filesystem::copy_file(sourcePath, assetPath,
                                  co::overwrite_existing)) {
    return Result<Path>::Error("Cannot create font from source: " +
                               sourcePath.stem().string());
  }

  auto metaRes = createAssetMeta(AssetType::Font,
                                 sourcePath.filename().string(), assetPath);

  if (!metaRes.hasData()) {
    std::filesystem::remove(assetPath);
    return Result<Path>::Error("Cannot create font from source: " +
                               sourcePath.stem().string());
  }

  return Result<Path>::Ok(assetPath);
}

Result<FontAssetHandle> AssetCache::loadFont(const Uuid &uuid) {
  auto filePath = getPathFromUuid(uuid);

  MsdfLoader loader;

  auto res = loader.loadFontData(filePath);

  if (res.hasError()) {
    return Result<FontAssetHandle>::Error(res.getError());
  }

  auto meta = getAssetMeta(uuid);

  auto &data = res.getData();
  data.type = AssetType::Font;
  data.name = meta.name;
  data.path = filePath;
  data.uuid = Uuid(filePath.stem().string());

  auto handle = mRegistry.getFonts().addAsset(data);

  return Result<FontAssetHandle>::Ok(handle);
}

} // namespace liquid
