#pragma once

#include "liquid/asset/AssetManager.h"
#include "liquid/platform-tools/NativeFileDialog.h"

namespace liquidator {

/**
 * @brief Asset loader
 *
 * Loads all supported asset types
 * from the editor
 */
class AssetLoader {
  static const std::vector<liquid::String> ScriptExtensions;
  static const std::vector<liquid::String> AudioExtensions;
  static const std::vector<liquid::String> SceneExtensions;
  static const std::vector<liquid::String> FontExtensions;

public:
  /**
   * @brief Create asset loader
   *
   * @param assetManager Asset manager
   * @param device Render device
   */
  AssetLoader(liquid::AssetManager &assetManager,
              liquid::rhi::RenderDevice *device);

  /**
   * @brief Load asset from path
   *
   * @param path Path to asset
   * @param directory Target directory path
   * @return Asset load result
   */
  liquid::Result<bool> loadFromPath(const liquid::Path &path,
                                    const liquid::Path &directory);

  /**
   * @brief Load asset from native file dialog
   *
   * @param directory Target directory path
   * @return Asset load result
   */
  liquid::Result<bool> loadFromFileDialog(const liquid::Path &directory);

private:
  liquid::AssetManager &mAssetManager;
  liquid::platform_tools::NativeFileDialog mNativeFileDialog;

  liquid::rhi::RenderDevice *mDevice;
};

} // namespace liquidator
