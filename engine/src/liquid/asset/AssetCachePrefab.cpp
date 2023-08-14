#include "liquid/core/Base.h"
#include "liquid/core/Version.h"

#include "AssetCache.h"

#include "AssetFileHeader.h"
#include "OutputBinaryStream.h"
#include "InputBinaryStream.h"

namespace liquid {

Result<Path>
AssetCache::createPrefabFromAsset(const AssetData<PrefabAsset> &asset,
                                  const String &uuid) {
  auto assetPath = createAssetPath(uuid);

  OutputBinaryStream file(assetPath);

  if (!file.good()) {
    return Result<Path>::Error("File cannot be opened for writing: " +
                               assetPath.string());
  }

  AssetFileHeader header{};
  header.type = AssetType::Prefab;
  header.magic = AssetFileHeader::MagicConstant;
  header.name = asset.name;
  file.write(header);

  std::map<MaterialAssetHandle, uint32_t> localMaterialMap;
  {
    std::vector<String> assetPaths;
    assetPaths.reserve(asset.data.meshes.size());

    for (auto &component : asset.data.meshRenderers) {
      for (auto material : component.value.materials) {
        if (localMaterialMap.find(material) == localMaterialMap.end()) {
          auto uuid = mRegistry.getMaterials().getAsset(material).uuid;
          localMaterialMap.insert_or_assign(
              material, static_cast<uint32_t>(assetPaths.size()));
          assetPaths.push_back(uuid);
        }
      }
    }

    for (auto &component : asset.data.skinnedMeshRenderers) {
      for (auto material : component.value.materials) {
        if (localMaterialMap.find(material) == localMaterialMap.end()) {
          auto uuid = mRegistry.getMaterials().getAsset(material).uuid;
          localMaterialMap.insert_or_assign(
              material, static_cast<uint32_t>(assetPaths.size()));
          assetPaths.push_back(uuid);
        }
      }
    }

    file.write(static_cast<uint32_t>(assetPaths.size()));
    file.write(assetPaths);
  }

  std::map<MeshAssetHandle, uint32_t> localMeshMap;
  {
    std::vector<String> assetPaths;
    assetPaths.reserve(asset.data.meshes.size());

    for (auto &component : asset.data.meshes) {
      if (localMeshMap.find(component.value) == localMeshMap.end()) {
        auto uuid = mRegistry.getMeshes().getAsset(component.value).uuid;
        localMeshMap.insert_or_assign(component.value,
                                      static_cast<uint32_t>(assetPaths.size()));
        assetPaths.push_back(uuid);
      }
    }

    file.write(static_cast<uint32_t>(assetPaths.size()));
    file.write(assetPaths);
  }

  std::map<SkinnedMeshAssetHandle, uint32_t> localSkinnedMeshMap;
  {
    std::vector<String> assetPaths;
    assetPaths.reserve(asset.data.skinnedMeshes.size());

    for (auto &component : asset.data.skinnedMeshes) {
      auto uuid = mRegistry.getSkinnedMeshes().getAsset(component.value).uuid;

      if (localSkinnedMeshMap.find(component.value) ==
          localSkinnedMeshMap.end()) {
        localSkinnedMeshMap.insert_or_assign(
            component.value, static_cast<uint32_t>(assetPaths.size()));
        assetPaths.push_back(uuid);
      }
    }

    file.write(static_cast<uint32_t>(assetPaths.size()));
    file.write(assetPaths);
  }

  std::map<SkeletonAssetHandle, uint32_t> localSkeletonMap;
  {
    std::vector<String> assetPaths;
    assetPaths.reserve(asset.data.skeletons.size());

    for (auto &component : asset.data.skeletons) {
      auto uuid = mRegistry.getSkeletons().getAsset(component.value).uuid;

      if (localSkeletonMap.find(component.value) == localSkeletonMap.end()) {
        localSkeletonMap.insert_or_assign(
            component.value, static_cast<uint32_t>(assetPaths.size()));
        assetPaths.push_back(uuid);
      }
    }

    file.write(static_cast<uint32_t>(assetPaths.size()));
    file.write(assetPaths);
  }

  std::map<AnimationAssetHandle, uint32_t> localAnimationMap;
  {
    std::vector<String> assetPaths;
    assetPaths.reserve(asset.data.animations.size());

    for (auto handle : asset.data.animations) {
      auto uuid = mRegistry.getAnimations().getAsset(handle).uuid;

      if (localAnimationMap.find(handle) == localAnimationMap.end()) {
        localAnimationMap.insert_or_assign(
            handle, static_cast<uint32_t>(assetPaths.size()));
        assetPaths.push_back(uuid);
      }
    }

    file.write(static_cast<uint32_t>(assetPaths.size()));
    file.write(assetPaths);
  }

  std::map<AnimatorAssetHandle, uint32_t> localAnimatorMap;
  {
    std::vector<String> assetPaths;
    assetPaths.reserve(asset.data.animators.size());

    for (auto &component : asset.data.animators) {
      auto uuid = mRegistry.getAnimators().getAsset(component.value).uuid;

      if (localAnimatorMap.find(component.value) == localAnimatorMap.end()) {
        localAnimatorMap.insert_or_assign(
            component.value, static_cast<uint32_t>(assetPaths.size()));
        assetPaths.push_back(uuid);
      }
    }

    file.write(static_cast<uint32_t>(assetPaths.size()));
    file.write(assetPaths);
  }

  // Load component data
  {
    auto numComponents = static_cast<uint32_t>(asset.data.transforms.size());

    file.write(numComponents);
    for (uint32_t i = 0; i < numComponents; ++i) {
      const auto &transform = asset.data.transforms.at(i).value;
      file.write(asset.data.transforms.at(i).entity);

      file.write(transform.position);
      file.write(transform.rotation);
      file.write(transform.scale);
      file.write(transform.parent);
    }
  }

  {
    auto numComponents = static_cast<uint32_t>(asset.data.names.size());
    file.write(numComponents);

    for (auto &name : asset.data.names) {
      file.write(name.entity);
      file.write(name.value);
    }
  }

  {
    auto numComponents = static_cast<uint32_t>(asset.data.meshes.size());
    file.write(numComponents);
    for (auto &component : asset.data.meshes) {
      file.write(component.entity);
      file.write(localMeshMap.at(component.value));
    }
  }

  {
    auto numComponents = static_cast<uint32_t>(asset.data.meshRenderers.size());
    file.write(numComponents);
    for (auto &component : asset.data.meshRenderers) {
      file.write(component.entity);

      file.write(static_cast<uint32_t>(component.value.materials.size()));
      for (auto handle : component.value.materials) {
        file.write(localMaterialMap.at(handle));
      }
    }
  }

  {
    auto numComponents = static_cast<uint32_t>(asset.data.skinnedMeshes.size());
    file.write(numComponents);
    for (auto &component : asset.data.skinnedMeshes) {
      file.write(component.entity);
      file.write(localSkinnedMeshMap.at(component.value));
    }
  }

  {
    auto numComponents =
        static_cast<uint32_t>(asset.data.skinnedMeshRenderers.size());
    file.write(numComponents);
    for (auto &component : asset.data.skinnedMeshRenderers) {
      file.write(component.entity);

      file.write(static_cast<uint32_t>(component.value.materials.size()));
      for (auto handle : component.value.materials) {
        file.write(localMaterialMap.at(handle));
      }
    }
  }

  {
    auto numComponents = static_cast<uint32_t>(asset.data.skeletons.size());
    file.write(numComponents);
    for (auto &component : asset.data.skeletons) {
      file.write(component.entity);
      file.write(localSkeletonMap.at(component.value));
    }
  }

  {
    auto numComponents = static_cast<uint32_t>(asset.data.animations.size());
    file.write(numComponents);

    for (auto &component : asset.data.animations) {
      file.write(localAnimationMap.at(component));
    }
  }

  {
    auto numComponents = static_cast<uint32_t>(asset.data.animators.size());
    file.write(numComponents);

    for (auto &component : asset.data.animators) {
      file.write(component.entity);
      file.write(localAnimatorMap.at(component.value));
    }
  }

  {
    auto numComponents =
        static_cast<uint32_t>(asset.data.directionalLights.size());
    file.write(numComponents);

    for (auto &light : asset.data.directionalLights) {
      file.write(light.entity);
      file.write(light.value.color);
      file.write(light.value.intensity);
    }
  }

  {
    auto numComponents = static_cast<uint32_t>(asset.data.pointLights.size());
    file.write(numComponents);

    for (auto &light : asset.data.pointLights) {
      file.write(light.entity);
      file.write(light.value.color);
      file.write(light.value.intensity);
      file.write(light.value.range);
    }
  }

  return Result<Path>::Ok(assetPath);
}

Result<PrefabAssetHandle>
AssetCache::loadPrefabDataFromInputStream(InputBinaryStream &stream,
                                          const Path &filePath,
                                          const AssetFileHeader &header) {

  std::vector<String> warnings;

  AssetData<PrefabAsset> prefab{};
  prefab.name = header.name;
  prefab.path = filePath;
  prefab.type = AssetType::Prefab;
  prefab.uuid = filePath.stem().string();

  std::vector<MaterialAssetHandle> localMaterialMap;
  {
    uint32_t numAssets = 0;
    stream.read(numAssets);
    std::vector<liquid::String> actual(numAssets);
    stream.read(actual);
    localMaterialMap.resize(numAssets, MaterialAssetHandle::Null);

    for (uint32_t i = 0; i < numAssets; ++i) {
      auto assetUuid = actual.at(i);
      const auto &res = getOrLoadMaterialFromUuid(assetUuid);
      if (res.hasData()) {
        localMaterialMap.at(i) = res.getData();
        warnings.insert(warnings.end(), res.getWarnings().begin(),
                        res.getWarnings().end());
      } else {
        warnings.push_back(res.getError());
      }
    }
  }

  std::vector<MeshAssetHandle> localMeshMap;
  {
    uint32_t numAssets = 0;
    stream.read(numAssets);
    std::vector<liquid::String> actual(numAssets);
    stream.read(actual);
    localMeshMap.resize(numAssets, MeshAssetHandle::Null);

    for (uint32_t i = 0; i < numAssets; ++i) {
      auto assetUuid = actual.at(i);
      const auto &res = getOrLoadMeshFromUuid(assetUuid);
      if (res.hasData()) {
        localMeshMap.at(i) = res.getData();
        warnings.insert(warnings.end(), res.getWarnings().begin(),
                        res.getWarnings().end());
      } else {
        warnings.push_back(res.getError());
      }
    }
  }

  std::vector<SkinnedMeshAssetHandle> localSkinnedMeshMap;
  {
    uint32_t numAssets = 0;
    stream.read(numAssets);
    std::vector<liquid::String> actual(numAssets);
    stream.read(actual);
    localSkinnedMeshMap.resize(numAssets, SkinnedMeshAssetHandle::Null);

    for (uint32_t i = 0; i < numAssets; ++i) {
      auto assetUuid = actual.at(i);
      const auto &res = getOrLoadSkinnedMeshFromUuid(assetUuid);
      if (res.hasData()) {
        localSkinnedMeshMap.at(i) = res.getData();
        warnings.insert(warnings.end(), res.getWarnings().begin(),
                        res.getWarnings().end());
      } else {
        warnings.push_back(res.getError());
      }
    }
  }

  std::vector<SkeletonAssetHandle> localSkeletonMap;
  {
    uint32_t numAssets = 0;
    stream.read(numAssets);
    std::vector<liquid::String> actual(numAssets);
    stream.read(actual);
    localSkeletonMap.resize(numAssets, SkeletonAssetHandle::Null);

    for (uint32_t i = 0; i < numAssets; ++i) {
      auto assetUuid = actual.at(i);
      const auto &res = getOrLoadSkeletonFromUuid(assetUuid);
      if (res.hasData()) {
        localSkeletonMap.at(i) = res.getData();
        warnings.insert(warnings.end(), res.getWarnings().begin(),
                        res.getWarnings().end());
      } else {
        warnings.push_back(res.getError());
      }
    }
  }

  std::vector<AnimationAssetHandle> localAnimationMap;
  {
    auto &map = mRegistry.getAnimations();

    uint32_t numAssets = 0;
    stream.read(numAssets);
    std::vector<liquid::String> actual(numAssets);
    stream.read(actual);
    localAnimationMap.resize(numAssets, AnimationAssetHandle::Null);

    for (uint32_t i = 0; i < numAssets; ++i) {
      auto assetUuid = actual.at(i);
      const auto &res = getOrLoadAnimationFromUuid(assetUuid);
      if (res.hasData()) {
        localAnimationMap.at(i) = res.getData();
        warnings.insert(warnings.end(), res.getWarnings().begin(),
                        res.getWarnings().end());

      } else {
        warnings.push_back(res.getError());
      }
    }
  }

  std::vector<AnimatorAssetHandle> localAnimatorMap;
  {
    auto &map = mRegistry.getAnimators();

    uint32_t numAssets = 0;
    stream.read(numAssets);
    std::vector<liquid::String> actual(numAssets);
    stream.read(actual);
    localAnimatorMap.resize(numAssets, AnimatorAssetHandle::Null);

    for (uint32_t i = 0; i < numAssets; ++i) {
      auto assetUuid = actual.at(i);
      const auto &res = getOrLoadAnimatorFromUuid(assetUuid);
      if (res.hasData()) {
        localAnimatorMap.at(i) = res.getData();
        warnings.insert(warnings.end(), res.getWarnings().begin(),
                        res.getWarnings().end());

      } else {
        warnings.push_back(res.getError());
      }
    }
  }

  {
    uint32_t numComponents = 0;
    stream.read(numComponents);
    prefab.data.transforms.resize(numComponents);
    for (uint32_t i = 0; i < numComponents; ++i) {
      uint32_t entity = 0;
      stream.read(entity);

      glm::vec3 position;
      glm::quat rotation;
      glm::vec3 scale;
      int32_t parent = -1;
      stream.read(position);
      stream.read(rotation);
      stream.read(scale);
      stream.read(parent);

      prefab.data.transforms.at(i).entity = entity;
      prefab.data.transforms.at(i).value = {position, rotation, scale, parent};
    }
  }

  {
    uint32_t numComponents = 0;
    stream.read(numComponents);
    prefab.data.names.resize(numComponents);
    for (uint32_t i = 0; i < numComponents; ++i) {
      uint32_t entity = 0;
      stream.read(entity);

      String name;
      stream.read(name);

      prefab.data.names.at(i).entity = entity;
      prefab.data.names.at(i).value = name;
    }
  }

  {
    uint32_t numComponents = 0;
    stream.read(numComponents);

    prefab.data.meshes.resize(numComponents);

    for (uint32_t i = 0; i < numComponents; ++i) {
      uint32_t entity = 0;
      stream.read(entity);

      uint32_t meshIndex = 0;
      stream.read(meshIndex);

      prefab.data.meshes.at(i).entity = entity;
      prefab.data.meshes.at(i).value = localMeshMap.at(meshIndex);
    }
  }

  {
    uint32_t numComponents = 0;
    stream.read(numComponents);

    prefab.data.meshRenderers.resize(numComponents);

    for (uint32_t i = 0; i < numComponents; ++i) {
      uint32_t entity = 0;
      stream.read(entity);

      uint32_t numMaterials = 0;
      stream.read(numMaterials);

      std::vector<uint32_t> materialIndices(numMaterials);
      stream.read(materialIndices);

      prefab.data.meshRenderers.at(i).entity = entity;

      for (auto materialIndex : materialIndices) {
        prefab.data.meshRenderers.at(i).value.materials.push_back(
            localMaterialMap.at(materialIndex));
      }
    }
  }

  {
    uint32_t numComponents = 0;
    stream.read(numComponents);

    prefab.data.skinnedMeshes.resize(numComponents);

    for (uint32_t i = 0; i < numComponents; ++i) {
      uint32_t entity = 0;
      stream.read(entity);

      uint32_t meshIndex = 0;
      stream.read(meshIndex);

      prefab.data.skinnedMeshes.at(i).entity = entity;
      prefab.data.skinnedMeshes.at(i).value = localSkinnedMeshMap.at(meshIndex);
    }
  }

  {
    uint32_t numComponents = 0;
    stream.read(numComponents);

    prefab.data.skinnedMeshRenderers.resize(numComponents);

    for (uint32_t i = 0; i < numComponents; ++i) {
      uint32_t entity = 0;
      stream.read(entity);

      uint32_t numMaterials = 0;
      stream.read(numMaterials);

      std::vector<uint32_t> materialIndices(numMaterials);
      stream.read(materialIndices);

      prefab.data.skinnedMeshRenderers.at(i).entity = entity;

      for (auto materialIndex : materialIndices) {
        prefab.data.skinnedMeshRenderers.at(i).value.materials.push_back(
            localMaterialMap.at(materialIndex));
      }
    }
  }

  {
    uint32_t numComponents = 0;
    stream.read(numComponents);

    prefab.data.skeletons.resize(numComponents);

    for (uint32_t i = 0; i < numComponents; ++i) {
      uint32_t entity = 0;
      stream.read(entity);

      uint32_t meshIndex = 0;
      stream.read(meshIndex);

      prefab.data.skeletons.at(i).entity = entity;
      prefab.data.skeletons.at(i).value = localSkeletonMap.at(meshIndex);
    }
  }

  {
    uint32_t numComponents = 0;
    stream.read(numComponents);

    prefab.data.animations.resize(numComponents);

    for (uint32_t i = 0; i < numComponents; ++i) {
      uint32_t animationIndex = 0;
      stream.read(animationIndex);

      prefab.data.animations.at(i) = localAnimationMap.at(animationIndex);
    }
  }

  {
    uint32_t numComponents = 0;
    stream.read(numComponents);

    prefab.data.animators.resize(numComponents);

    for (uint32_t i = 0; i < numComponents; ++i) {
      uint32_t entity = 0;
      stream.read(entity);

      uint32_t animatorIndex = 0;
      stream.read(animatorIndex);

      prefab.data.animators.at(i).entity = entity;
      prefab.data.animators.at(i).value = localAnimatorMap.at(animatorIndex);
    }
  }

  {
    uint32_t numComponents = 0;
    stream.read(numComponents);
    prefab.data.directionalLights.resize(numComponents);
    for (uint32_t i = 0; i < numComponents; ++i) {
      uint32_t entity = 0;
      stream.read(entity);

      glm::vec4 color;
      float intensity = 0.0f;
      stream.read(color);
      stream.read(intensity);

      prefab.data.directionalLights.at(i).entity = entity;
      prefab.data.directionalLights.at(i).value = {color, intensity};
    }
  }

  {
    uint32_t numComponents = 0;
    stream.read(numComponents);
    prefab.data.pointLights.resize(numComponents);
    for (uint32_t i = 0; i < numComponents; ++i) {
      uint32_t entity = 0;
      stream.read(entity);

      glm::vec4 color;
      float intensity = 0.0f;
      float range = 0.0f;
      stream.read(color);
      stream.read(intensity);
      stream.read(range);

      prefab.data.pointLights.at(i).entity = entity;
      prefab.data.pointLights.at(i).value = {color, intensity, range};
    }
  }

  if (prefab.data.transforms.empty() && prefab.data.directionalLights.empty() &&
      prefab.data.pointLights.empty() && prefab.data.meshes.empty() &&
      prefab.data.skinnedMeshes.empty() && prefab.data.skeletons.empty() &&
      prefab.data.animators.empty() && prefab.data.names.empty()) {
    return Result<PrefabAssetHandle>::Error("Prefab is empty");
  }

  return Result<PrefabAssetHandle>::Ok(mRegistry.getPrefabs().addAsset(prefab),
                                       warnings);
}

Result<PrefabAssetHandle> AssetCache::loadPrefabFromFile(const Path &filePath) {
  InputBinaryStream stream(filePath);

  const auto &header = checkAssetFile(stream, filePath, AssetType::Prefab);
  if (header.hasError()) {
    return Result<PrefabAssetHandle>::Error(header.getError());
  }

  return loadPrefabDataFromInputStream(stream, filePath, header.getData());
}

} // namespace liquid
