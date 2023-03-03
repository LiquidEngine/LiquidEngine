#include "liquid/core/Base.h"

#include "liquid/rhi/RenderHandle.h"
#include "SceneRendererFrameData.h"

namespace liquid {

SceneRendererFrameData::SceneRendererFrameData(RenderStorage &renderStorage,
                                               rhi::RenderDevice *device,
                                               size_t reservedSpace)
    : mReservedSpace(reservedSpace) {

  mLights.reserve(MaxNumLights);
  mShadowMaps.reserve(MaxShadowMaps);

  mTextTransforms.reserve(mReservedSpace);
  mTextGlyphs.reserve(mReservedSpace);

  rhi::BufferDescription defaultDesc{};
  defaultDesc.type = rhi::BufferType::Storage;
  defaultDesc.size = mReservedSpace * sizeof(glm::mat4);
  defaultDesc.mapped = true;

  mMeshTransformsBuffer = renderStorage.createBuffer(defaultDesc);
  mSkinnedMeshTransformsBuffer = renderStorage.createBuffer(defaultDesc);
  mTextTransformsBuffer = renderStorage.createBuffer(defaultDesc);

  {
    auto desc = defaultDesc;
    desc.size = mReservedSpace * MaxNumJoints * sizeof(glm::mat4);
    mSkeletonsBuffer = renderStorage.createBuffer(desc);
  }

  {
    auto desc = defaultDesc;
    desc.size = mReservedSpace * sizeof(GlyphData);
    mTextGlyphsBuffer = renderStorage.createBuffer(desc);
  }

  {
    auto desc = defaultDesc;
    desc.size = mLights.capacity() * sizeof(LightData);
    mLightsBuffer = renderStorage.createBuffer(desc);
  }

  {
    auto desc = defaultDesc;
    desc.size = sizeof(Camera);
    desc.type = rhi::BufferType::Uniform;
    mCameraBuffer = renderStorage.createBuffer(desc);
  }

  {
    auto desc = defaultDesc;
    desc.size = sizeof(SceneData);
    desc.type = rhi::BufferType::Uniform;
    mSceneBuffer = renderStorage.createBuffer(desc);
  }

  {
    auto desc = defaultDesc;
    desc.size = sizeof(SkyboxData);
    desc.type = rhi::BufferType::Uniform;
    mSkyboxBuffer = renderStorage.createBuffer(desc);
  }

  {
    auto desc = defaultDesc;
    desc.size = mShadowMaps.capacity() * sizeof(ShadowMapData);
    mShadowMapsBuffer = renderStorage.createBuffer(desc);
  }

  mDrawParams.index0 = rhi::castHandleToUint(mMeshTransformsBuffer.getHandle());
  mDrawParams.index1 =
      rhi::castHandleToUint(mSkinnedMeshTransformsBuffer.getHandle());
  mDrawParams.index2 = rhi::castHandleToUint(mSkeletonsBuffer.getHandle());
  mDrawParams.index3 = rhi::castHandleToUint(mTextTransformsBuffer.getHandle());
  mDrawParams.index4 = rhi::castHandleToUint(mTextGlyphsBuffer.getHandle());
  mDrawParams.index5 = rhi::castHandleToUint(mLightsBuffer.getHandle());
  mDrawParams.index6 = rhi::castHandleToUint(mShadowMapsBuffer.getHandle());
  mDrawParams.index7 = rhi::castHandleToUint(mCameraBuffer.getHandle());
  mDrawParams.index8 = rhi::castHandleToUint(mSceneBuffer.getHandle());
  mDrawParams.index9 = rhi::castHandleToUint(mSkyboxBuffer.getHandle());
}

void SceneRendererFrameData::updateBuffers() {
  {
    size_t offset = 0;
    auto *bufferData = static_cast<glm::mat4 *>(mMeshTransformsBuffer.map());
    for (auto &[_, data] : mMeshGroups) {
      memcpy(bufferData + offset, data.transforms.data(),
             data.transforms.size() * sizeof(glm::mat4));
      offset += data.transforms.size();
    }
  }

  {
    size_t transformsOffset = 0;
    auto *transformsBuffer =
        static_cast<glm::mat4 *>(mSkinnedMeshTransformsBuffer.map());

    size_t skeletonsOffset = 0;
    auto *skeletonsBuffer = static_cast<glm::mat4 *>(mSkeletonsBuffer.map());
    for (auto &[_, data] : mSkinnedMeshGroups) {
      memcpy(transformsBuffer + transformsOffset, data.transforms.data(),
             data.transforms.size() * sizeof(glm::mat4));
      transformsOffset += data.transforms.size();

      memcpy(skeletonsBuffer + skeletonsOffset, data.skeletons.get(),
             data.lastSkeleton * MaxNumJoints * sizeof(glm::mat4));
      skeletonsOffset += data.lastSkeleton * MaxNumJoints;
    }
  }

  mTextTransformsBuffer.update(mTextTransforms.data(),
                               mTextTransforms.size() * sizeof(glm::mat4));
  mTextGlyphsBuffer.update(mTextGlyphs.data(),
                           mTextGlyphs.size() * sizeof(GlyphData));
  mLightsBuffer.update(mLights.data(), mLights.size() * sizeof(LightData));
  mShadowMapsBuffer.update(mShadowMaps.data(),
                           mShadowMaps.size() * sizeof(ShadowMapData));
  mCameraBuffer.update(&mCameraData, sizeof(Camera));
  mSceneBuffer.update(&mSceneData, sizeof(SceneData));
  mSkyboxBuffer.update(&mSkyboxData, sizeof(SkyboxData));
}

void SceneRendererFrameData::addMesh(MeshAssetHandle handle,
                                     liquid::Entity entity,
                                     const glm::mat4 &transform) {
  if (mMeshGroups.find(handle) == mMeshGroups.end()) {
    MeshData data{};
    mMeshGroups.insert_or_assign(handle, data);
  }

  mMeshGroups.at(handle).entities.push_back(entity);
  mMeshGroups.at(handle).transforms.push_back(transform);
}

void SceneRendererFrameData::setBrdfLookupTable(rhi::TextureHandle brdfLut) {
  mSceneData.textures.z = static_cast<uint32_t>(brdfLut);
}

void SceneRendererFrameData::addSkinnedMesh(
    SkinnedMeshAssetHandle handle, Entity entity, const glm::mat4 &transform,
    const std::vector<glm::mat4> &skeleton) {

  if (mSkinnedMeshGroups.find(handle) == mSkinnedMeshGroups.end()) {
    mSkinnedMeshGroups.insert({handle, SkinnedMeshData{}});
  }

  auto &group = mSkinnedMeshGroups.at(handle);

  group.entities.push_back(entity);
  group.transforms.push_back(transform);

  size_t currentOffset = group.lastSkeleton * MaxNumJoints;
  size_t newSize = currentOffset + MaxNumJoints;

  // Resize skeletons if new skeleton does not fit
  if (group.skeletonCapacity < newSize) {
    group.skeletonCapacity = newSize * 2;
    auto *newVector = new glm::mat4[group.skeletonCapacity];

    memcpy(newVector, group.skeletons.get(), currentOffset * sizeof(glm::mat4));
    group.skeletons.reset(newVector);
  }

  auto *currentSkeleton = group.skeletons.get() + currentOffset;
  size_t dataSize = std::min(skeleton.size(), MaxNumJoints);
  memcpy(currentSkeleton, skeleton.data(), dataSize * sizeof(glm::mat4));
  group.lastSkeleton++;
}

void SceneRendererFrameData::addCascadedShadowMaps(
    const DirectionalLight &light, const CascadedShadowMap &shadowMap) {
  uint32_t numCascades = static_cast<uint32_t>(shadowMap.numCascades);

  // Calculate split distances by combining
  // logarithmic and uniform splitting
  //
  // log_i = near * (far/near)^(i / size)
  // uniform_i = near + (far - near) * (1 / size)
  // distance_i = lambda * log_i + (1 - lambda) * uniform_i
  std::array<float, CascadedShadowMap::MaxCascades + 1> splitDistances{};

  float splitLambda = shadowMap.splitLambda;

  float far = mCameraLens.far;
  float near = mCameraLens.near;

  float range = far - near;
  float ratio = far / near;

  for (size_t i = 0; i < static_cast<size_t>(numCascades + 1); ++i) {
    float p =
        static_cast<float>(i + 1) / static_cast<float>(splitDistances.size());
    float log = near * std::pow(ratio, p);
    float uniform = near + range * p;
    float d = splitLambda * log + (1.0f - splitLambda) * uniform;

    splitDistances.at(i) = mCameraLens.far * ((d - near) / range);
  }

  // Camera frustum NDC coordinates
  static constexpr std::array<glm::vec3, 8> FrustumCornersNDC{
      glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(1.0f, 1.0f, -1.0f),
      glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(-1.0f, -1.0f, -1.0f),
      glm::vec3(-1.0f, 1.0f, 1.0f),  glm::vec3(1.0f, 1.0f, 1.0f),
      glm::vec3(1.0f, -1.0f, 1.0f),  glm::vec3(-1.0f, -1.0f, 1.0f),
  };

  float prevSplitDistance = mCameraLens.near;
  for (size_t i = 0; i < numCascades; ++i) {
    float splitDistance = splitDistances.at(i);

    auto splitProjectionMatrix = glm::perspective(
        glm::radians(mCameraLens.fovY), mCameraLens.aspectRatio,
        prevSplitDistance, splitDistance);

    auto invProjView =
        glm::inverse(splitProjectionMatrix * mCameraData.viewMatrix);

    std::array<glm::vec4, FrustumCornersNDC.size()> frustumCorners{};

    for (size_t i = 0; i < frustumCorners.size(); ++i) {
      auto pt = invProjView * glm::vec4(FrustumCornersNDC.at(i), 1.0f);
      frustumCorners.at(i) = pt / pt.w;
    }

    // Calculate frustum center by adding
    // each corner and dividing the result by
    // number of corners
    glm::vec3 frustumCenter{0.0f};
    for (const auto &v : frustumCorners) {
      frustumCenter += glm::vec3(v);
    }
    frustumCenter /= static_cast<float>(frustumCorners.size());

    float radius = 0.0f;
    for (const auto &corner : frustumCorners) {
      float distance = glm::length(glm::vec3(corner) - frustumCenter);
      radius = glm::max(radius, distance);
    }

    static constexpr float Sixteen = 16.0f;
    radius = std::ceil(radius * Sixteen) / Sixteen;
    glm::vec3 maxBounds = glm::vec3(radius);
    glm::vec3 minBounds = -maxBounds;

    float cascadeZ = maxBounds.z - minBounds.z;
    auto lightViewMatrix =
        glm::lookAt(frustumCenter - light.direction * radius, frustumCenter,
                    glm::vec3(0.0f, 1.0f, 0.0f));
    auto lightProjectionMatrix = glm::ortho(
        minBounds.x, maxBounds.x, minBounds.y, maxBounds.y, 0.0f, cascadeZ);

    auto projectionViewMatrix = lightProjectionMatrix * lightViewMatrix;

    mShadowMaps.push_back(
        {projectionViewMatrix,
         glm::vec4{-splitDistance, shadowMap.softShadows ? 1.0f : 0.0f, 0.0f,
                   0.0f}});

    prevSplitDistance = splitDistance;
  }
}

void SceneRendererFrameData::addLight(const DirectionalLight &light,
                                      const CascadedShadowMap &shadowMap) {
  uint32_t shadowIndex = static_cast<uint32_t>(mShadowMaps.size());
  uint32_t numCascades = static_cast<uint32_t>(shadowMap.numCascades);

  bool canCastShadows =
      (shadowIndex + numCascades) <= static_cast<uint32_t>(MaxShadowMaps);

  if (canCastShadows) {
    addCascadedShadowMaps(light, shadowMap);
  }

  LightData data{
      glm::vec4(light.direction, light.intensity), light.color,
      glm::uvec4(canCastShadows ? 1 : 0, shadowIndex, numCascades, 0)};
  mLights.push_back(data);
  mSceneData.data.x = static_cast<int32_t>(mLights.size());
}

void SceneRendererFrameData::addLight(const DirectionalLight &light) {
  LightData data{glm::vec4(light.direction, light.intensity), light.color};
  mLights.push_back(data);

  mSceneData.data.x = static_cast<int32_t>(mLights.size());
}

void SceneRendererFrameData::addText(FontAssetHandle font,
                                     const std::vector<GlyphData> &glyphs,
                                     const glm::mat4 &transform) {
  mTextTransforms.push_back(transform);
  uint32_t index = static_cast<uint32_t>(mTextTransforms.size() - 1);

  TextData textData{};
  textData.index = index;
  textData.glyphStart = static_cast<uint32_t>(mTextGlyphs.size());
  textData.length = static_cast<uint32_t>(glyphs.size());

  for (auto &glyph : glyphs) {
    mTextGlyphs.push_back(glyph);
  }

  if (mTextGroups.find(font) == mTextGroups.end()) {
    mTextGroups.insert_or_assign(font, std::vector<TextData>{textData});
  } else {
    mTextGroups.at(font).push_back(textData);
  }
}

void SceneRendererFrameData::setSkyboxTexture(rhi::TextureHandle texture) {
  mSkyboxData.data.x = static_cast<uint32_t>(texture);
}

void SceneRendererFrameData::setSkyboxColor(const glm::vec4 &color) {
  mSkyboxData.color = color;
}

void SceneRendererFrameData::setEnvironmentTextures(
    rhi::TextureHandle irradianceMap, rhi::TextureHandle specularMap) {
  mSceneData.data.y = SceneData::EnvironmentLighting::Texture;
  mSceneData.textures.x = rhi::castHandleToUint(irradianceMap);
  mSceneData.textures.y = rhi::castHandleToUint(specularMap);
}

void SceneRendererFrameData::setEnvironmentColor(const glm::vec4 &color) {
  mSceneData.data.y = SceneData::EnvironmentLighting::Color;
  mSceneData.color = color;
}

void SceneRendererFrameData::setCameraData(const Camera &data,
                                           const PerspectiveLens &lens) {
  mCameraData = data;
  mCameraLens = lens;
}

void SceneRendererFrameData::setShadowMapTexture(rhi::TextureHandle shadowmap) {
  mSceneData.textures.w = rhi::castHandleToUint(shadowmap);
}

void SceneRendererFrameData::clear() {
  mTextTransforms.clear();
  mTextGroups.clear();
  mTextGlyphs.clear();

  mLights.clear();
  mShadowMaps.clear();
  mSceneData.data.x = 0;
  mSceneData.data.y = 0;
  mSceneData.textures.x = 0;
  mSceneData.textures.y = 0;
  // Do not unset remove BRDF Lookup table
  mSceneData.color = {};
  mSkyboxData.color = {};
  mSkyboxData.data.x = 0;
  mLastSkeleton = 0;

  mMeshGroups.clear();
  mSkinnedMeshGroups.clear();
}

} // namespace liquid
