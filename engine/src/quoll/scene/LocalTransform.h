#pragma once

namespace quoll {

struct LocalTransform {
  glm::vec3 localPosition{0.0f};

  glm::quat localRotation{1.0f, 0.0f, 0.0f, 0.0f};

  glm::vec3 localScale{1.0f};
};

} // namespace quoll
