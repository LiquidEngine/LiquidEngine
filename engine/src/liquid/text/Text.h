#pragma once

#include "liquid/asset/Asset.h"

namespace liquid {

/**
 * @brief Text component
 */
struct Text {
  /**
   * Text contents
   */
  String text;

  /**
   * Line height
   */
  float lineHeight = 1.0f;

  /**
   * Font used for rendering
   */
  FontAssetHandle font = FontAssetHandle::Null;
};

} // namespace liquid
