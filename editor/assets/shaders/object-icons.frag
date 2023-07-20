#version 450 core

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

#include "bindless-editor.glsl"

layout(set = 0, binding = 0) uniform sampler2D uGlobalTextures[];
layout(set = 0, binding = 1) writeonly uniform image2D uGlobalImages[];

layout(set = 1, binding = 0) uniform DrawParameters {
  Empty gizmoTransforms;
  Empty skeletonTransforms;
  Empty debugSkeletons;
  Empty collidableParams;
  Empty camera;
  Empty gridData;
  Empty pad0;
  Empty pad1;
}
uDrawParams;

/**
 * @brief Push for icons
 */
layout(push_constant) uniform IconParams { uvec4 icon; }
uIconParams;

void main() {
  outColor = texture(uGlobalTextures[uIconParams.icon.x], inTexCoord);
}
