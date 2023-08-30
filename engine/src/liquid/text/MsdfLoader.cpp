#include "liquid/core/Base.h"
#include "liquid/rhi/RenderHandle.h"

#include "MsdfAtlas.h"
#include "MsdfLoader.h"

#include <ft2build.h>
#include <freetype/freetype.h>

namespace quoll {

Result<AssetData<FontAsset>> MsdfLoader::loadFontData(const Path &path) {
  static constexpr double MaxCornerAngle = 3.0;
  static constexpr double MinimumScale = 32.0;
  static constexpr double PixelRange = 2.0;
  static constexpr uint32_t NumChannels = 4;
  static constexpr double FontScale = 1.0;

  using namespace msdf_atlas;

  auto *ft = msdfgen::initializeFreetype();
  if (!ft) {
    return Result<AssetData<FontAsset>>::Error("Failed to initialize freetype");
  }

  auto *font = msdfgen::loadFont(ft, path.string().c_str());

  if (font == nullptr) {
    msdfgen::deinitializeFreetype(ft);
    return Result<AssetData<FontAsset>>::Error("Failed to load font: " +
                                               path.string());
  }

  std::vector<GlyphGeometry> msdfGlyphs;
  FontGeometry fontGeometry(&msdfGlyphs);
  fontGeometry.loadCharset(font, FontScale, Charset::ASCII);

  for (auto &glyph : msdfGlyphs) {
    glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, MaxCornerAngle, 0);
  }

  TightAtlasPacker packer;
  packer.setDimensionsConstraint(
      TightAtlasPacker::DimensionsConstraint::SQUARE);
  packer.setDimensionsConstraint(
      TightAtlasPacker::DimensionsConstraint::POWER_OF_TWO_SQUARE);
  packer.setMinimumScale(MinimumScale);
  packer.setPixelRange(PixelRange);
  packer.setMiterLimit(1.0);
  packer.setPadding(0);
  packer.pack(msdfGlyphs.data(), static_cast<int>(msdfGlyphs.size()));

  int width = 0, height = 0;
  packer.getDimensions(width, height);

  ImmediateAtlasGenerator<float, NumChannels, mtsdfGenerator,
                          BitmapAtlasStorage<byte, NumChannels>>
      generator(width, height);

  GeneratorAttributes attributes;
  attributes.config.overlapSupport = false;

  generator.setAttributes(attributes);
  generator.setThreadCount(1);
  generator.generate(msdfGlyphs.data(), static_cast<int>(msdfGlyphs.size()));

  std::unordered_map<uint32_t, FontGlyph> glyphs;

  auto fWidth = static_cast<float>(width);
  auto fHeight = static_cast<float>(height);

  for (auto &msdfGlyph : msdfGlyphs) {
    FontGlyph glyph{};

    const auto &rect = msdfGlyph.getBoxRect();

    {
      double top = 0.0, left = 0.0, bottom = 0.0, right = 0.0;
      msdfGlyph.getQuadAtlasBounds(left, bottom, right, top);

      glyph.bounds = glm::vec4(static_cast<float>(left),
                               static_cast<float>(fHeight - bottom),
                               static_cast<float>(right),
                               static_cast<float>(fHeight - top)) /
                     fWidth;
    }

    {
      double top = 0.0, left = 0.0, bottom = 0.0, right = 0.0;
      msdfGlyph.getQuadPlaneBounds(left, top, right, bottom);

      glyph.planeBounds =
          glm::vec4(static_cast<float>(left), static_cast<float>(top),
                    static_cast<float>(right), static_cast<float>(bottom));
    }

    glyph.advanceX = static_cast<float>(msdfGlyph.getAdvance());

    glyphs.insert_or_assign(msdfGlyph.getCodepoint(), glyph);
  }

  auto storage = generator.atlasStorage();

  msdfgen::BitmapConstRef<byte, NumChannels> bitmap = storage;

  std::vector<std::byte> pixels(static_cast<size_t>(bitmap.width) *
                                bitmap.height * NumChannels);

  for (int y = 0; y < bitmap.height; ++y) {
    memcpy(&pixels[NumChannels * static_cast<size_t>(bitmap.width) * y],
           bitmap(0, bitmap.height - y - 1),
           NumChannels * static_cast<size_t>(bitmap.width));
  }

  AssetData<FontAsset> fontAsset{};
  fontAsset.path = path;
  fontAsset.type = AssetType::Font;
  fontAsset.size =
      sizeof(std::byte) * bitmap.width * bitmap.height * NumChannels;
  fontAsset.data.glyphs = glyphs;
  fontAsset.data.atlas = pixels;
  fontAsset.data.atlasDimensions = glm::uvec2{bitmap.width, bitmap.height};
  fontAsset.data.fontScale = static_cast<float>(FontScale);

  msdfgen::destroyFont(font);
  msdfgen::deinitializeFreetype(ft);

  return Result<AssetData<FontAsset>>::Ok(fontAsset);
}

} // namespace quoll
