#include "liquid/core/Base.h"
#include "liquid/asset/AssetCache.h"

#include "liquid-tests/Testing.h"
#include "liquid-tests/test-utils/AssetCacheTestBase.h"

class AssetCacheFontTest : public AssetCacheTestBase {
public:
};

TEST_F(AssetCacheFontTest, CreatesFontFromSource) {
  auto sourcePath = FixturesPath / "valid-font.ttf";
  auto filePath = cache.createFontFromSource(sourcePath, "");

  EXPECT_TRUE(filePath.hasData());
  EXPECT_FALSE(filePath.hasError());
  EXPECT_FALSE(filePath.hasWarnings());

  EXPECT_EQ(filePath.getData().filename().string().size(), 38);

  auto meta = cache.getMetaFromUuid(filePath.getData().stem().string());

  EXPECT_EQ(meta.type, liquid::AssetType::Font);
  EXPECT_EQ(meta.name, "valid-font.ttf");
}

TEST_F(AssetCacheFontTest, LoadsTTFFontFromFile) {
  auto sourcePath = FixturesPath / "valid-font.ttf";
  auto filePath = cache.createFontFromSource(sourcePath, "");

  auto result = cache.loadFontFromFile(filePath.getData());

  EXPECT_TRUE(result.hasData());
  EXPECT_FALSE(result.hasError());
  EXPECT_FALSE(result.hasWarnings());

  auto handle = result.getData();
  EXPECT_NE(handle, liquid::FontAssetHandle::Null);
  const auto &asset = cache.getRegistry().getFonts().getAsset(handle);

  EXPECT_EQ(asset.path, filePath.getData());
  EXPECT_EQ(asset.name, "valid-font.ttf");
  EXPECT_EQ(asset.type, liquid::AssetType::Font);
}

TEST_F(AssetCacheFontTest, LoadsOTFFontFromFile) {
  auto sourcePath = FixturesPath / "valid-font.otf";
  auto filePath = cache.createFontFromSource(sourcePath, "");

  auto result = cache.loadFontFromFile(filePath.getData());

  EXPECT_TRUE(result.hasData());
  EXPECT_FALSE(result.hasError());
  EXPECT_FALSE(result.hasWarnings());

  auto handle = result.getData();
  EXPECT_NE(handle, liquid::FontAssetHandle::Null);
  const auto &asset = cache.getRegistry().getFonts().getAsset(handle);

  EXPECT_EQ(asset.path, filePath.getData());
  EXPECT_EQ(asset.type, liquid::AssetType::Font);
  EXPECT_EQ(asset.name, "valid-font.otf");
}

TEST_F(AssetCacheFontTest, FileReturnsErrorIfAudioFileCannotBeOpened) {
  auto filePath = CachePath / "non-existent-file.asset";

  auto result = cache.loadFontFromFile(filePath);
  EXPECT_TRUE(result.hasError());
  EXPECT_FALSE(result.hasWarnings());
  EXPECT_FALSE(result.hasData());
}