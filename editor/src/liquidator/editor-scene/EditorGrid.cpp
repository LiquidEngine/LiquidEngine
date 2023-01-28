#include "liquid/core/Base.h"
#include "EditorGrid.h"

namespace liquid::editor {

void EditorGrid::setGridLinesFlag(bool flag) {
  mData.gridLines.x = static_cast<uint32_t>(flag);
}

void EditorGrid::setAxisLinesFlag(bool flag) {
  mData.gridLines.y = static_cast<uint32_t>(flag);
}

} // namespace liquid::editor
