#pragma once

namespace liquid {

enum class MaterialAssetHandle : uint32_t { Invalid = 0 };

enum class TextureAssetHandle : uint32_t { Invalid = 0 };

enum class MeshAssetHandle : uint32_t { Invalid = 0 };

enum class SkinnedMeshAssetHandle : uint32_t { Invalid = 0 };

enum class SkeletonAssetHandle : uint32_t { Invalid = 0 };

enum class AnimationAssetHandle : uint32_t { Invalid = 0 };

enum class AssetType {
  None,
  Material,
  Texture,
  Mesh,
  SkinnedMesh,
  Skeleton,
  Animation
};

} // namespace liquid
