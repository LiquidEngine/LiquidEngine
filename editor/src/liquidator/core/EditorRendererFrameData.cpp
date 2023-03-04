#include "liquid/core/Base.h"
#include "EditorRendererFrameData.h"

namespace liquid::editor {

EditorRendererFrameData::EditorRendererFrameData(RenderStorage &renderStorage,
                                                 size_t reservedSpace)
    : mReservedSpace(reservedSpace) {
  mSkeletonTransforms.reserve(mReservedSpace);
  mNumBones.reserve(mReservedSpace);
  mGizmoTransforms.reserve(reservedSpace);
  mSkeletonVector.reset(new glm::mat4[mReservedSpace * MaxNumBones]);

  rhi::BufferDescription defaultDesc{};
  defaultDesc.usage = rhi::BufferUsage::Storage;
  defaultDesc.size = mReservedSpace * sizeof(glm::mat4);
  defaultDesc.mapped = true;

  mSkeletonTransformsBuffer = renderStorage.createBuffer(defaultDesc);

  {
    auto desc = defaultDesc;
    desc.size = mReservedSpace * MaxNumBones * sizeof(glm::mat4);
    mSkeletonBoneTransformsBuffer = renderStorage.createBuffer(desc);
  }

  {
    auto desc = defaultDesc;
    desc.data = mGizmoTransforms.data();
    mGizmoTransformsBuffer = renderStorage.createBuffer(desc);
  }

  {
    auto desc = defaultDesc;
    desc.usage = rhi::BufferUsage::Uniform;
    desc.size = sizeof(Camera);
    mCameraBuffer = renderStorage.createBuffer(desc);
  }

  {
    auto desc = defaultDesc;
    desc.usage = rhi::BufferUsage::Uniform;
    desc.size = sizeof(EditorGridData);
    mEditorGridBuffer = renderStorage.createBuffer(desc);
  }

  {
    auto desc = defaultDesc;
    desc.size = sizeof(CollidableEntity);
    desc.usage = rhi::BufferUsage::Uniform;
    mCollidableEntityBuffer = renderStorage.createBuffer(desc);
  }

  mDrawParams.index0 =
      rhi::castHandleToUint(mGizmoTransformsBuffer.getHandle());
  mDrawParams.index1 =
      rhi::castHandleToUint(mSkeletonBoneTransformsBuffer.getHandle());
  mDrawParams.index2 = rhi::castHandleToUint(mEditorGridBuffer.getHandle());
  mDrawParams.index3 = rhi::castHandleToUint(mCameraBuffer.getHandle());
  mDrawParams.index4 =
      rhi::castHandleToUint(mCollidableEntityBuffer.getHandle());
  mDrawParams.index5 =
      rhi::castHandleToUint(mSkeletonTransformsBuffer.getHandle());
}

void EditorRendererFrameData::addSkeleton(
    const glm::mat4 &worldTransform,
    const std::vector<glm::mat4> &boneTransforms) {
  mSkeletonTransforms.push_back(worldTransform);
  mNumBones.push_back(static_cast<uint32_t>(boneTransforms.size()));

  auto *currentSkeleton = mSkeletonVector.get() + (mLastSkeleton * MaxNumBones);
  size_t dataSize = std::min(boneTransforms.size(), MaxNumBones);
  memcpy(currentSkeleton, boneTransforms.data(), dataSize * sizeof(glm::mat4));

  mLastSkeleton++;
}

void EditorRendererFrameData::setActiveCamera(const Camera &camera) {
  mCameraData = camera;
}

void EditorRendererFrameData::addGizmo(rhi::TextureHandle icon,
                                       const glm::mat4 &worldTransform) {
  mGizmoTransforms.push_back(worldTransform);
  mGizmoCounts[icon]++;
}

void EditorRendererFrameData::setEditorGrid(const EditorGridData &data) {
  mEditorGridData = data;
}

void EditorRendererFrameData::updateBuffers() {

  mCameraBuffer.update(&mCameraData, sizeof(Camera));
  mEditorGridBuffer.update(&mEditorGridData, sizeof(EditorGridData));

  if (!mSkeletonTransforms.empty()) {
    mSkeletonTransformsBuffer.update(mSkeletonTransforms.data(),
                                     mSkeletonTransforms.size() *
                                         sizeof(glm::mat4));
    mSkeletonBoneTransformsBuffer.update(
        mSkeletonVector.get(), mLastSkeleton * MaxNumBones * sizeof(glm::mat4));
  }

  mGizmoTransformsBuffer.update(mGizmoTransforms.data(),
                                mGizmoTransforms.size() * sizeof(glm::mat4));

  mCollidableEntityBuffer.update(&mCollidableEntityParams,
                                 sizeof(CollidableEntity));
}

void EditorRendererFrameData::clear() {
  mSkeletonTransforms.clear();
  mGizmoTransforms.clear();
  mNumBones.clear();
  mGizmoCounts.clear();
  mLastSkeleton = 0;

  mCollidableEntity = Entity::Null;
}

void EditorRendererFrameData::setCollidable(
    Entity entity, const Collidable &collidable,
    const WorldTransform &worldTransform) {
  mCollidableEntity = entity;
  mCollidableEntityParams.worldTransform = worldTransform.worldTransform;
  mCollidableEntityParams.type.x =
      static_cast<uint32_t>(collidable.geometryDesc.type);

  if (collidable.geometryDesc.type == PhysicsGeometryType::Box) {
    const auto &params =
        std::get<PhysicsGeometryBox>(collidable.geometryDesc.params);
    mCollidableEntityParams.params = glm::vec4(params.halfExtents, 0.0f);
  } else if (collidable.geometryDesc.type == PhysicsGeometryType::Sphere) {
    const auto &params =
        std::get<PhysicsGeometrySphere>(collidable.geometryDesc.params);
    mCollidableEntityParams.params = glm::vec4(params.radius);
  } else if (collidable.geometryDesc.type == PhysicsGeometryType::Capsule) {
    const auto &params =
        std::get<PhysicsGeometryCapsule>(collidable.geometryDesc.params);
    mCollidableEntityParams.params =
        glm::vec4(params.radius, params.halfHeight, 0.0f, 0.0f);
  }
}

} // namespace liquid::editor
