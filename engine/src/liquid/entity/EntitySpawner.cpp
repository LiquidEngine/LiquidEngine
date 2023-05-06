#include "liquid/core/Base.h"
#include "EntitySpawner.h"

namespace liquid {

EntitySpawner::EntitySpawner(EntityDatabase &entityDatabase,
                             AssetRegistry &assetRegistry)
    : mEntityDatabase(entityDatabase), mAssetRegistry(assetRegistry) {}

Entity EntitySpawner::spawnEmpty(LocalTransform transform) {
  auto entity = mEntityDatabase.create();
  mEntityDatabase.set(entity, transform);
  mEntityDatabase.set<WorldTransform>(entity, {});
  mEntityDatabase.set<Name>(entity, {"New entity"});
  return entity;
}

std::vector<Entity> EntitySpawner::spawnPrefab(PrefabAssetHandle handle,
                                               LocalTransform transform) {
  LIQUID_ASSERT(mAssetRegistry.getPrefabs().hasAsset(handle),
                "Prefab not found");

  const auto &asset = mAssetRegistry.getPrefabs().getAsset(handle).data;

  std::unordered_map<uint32_t, size_t> entityMap;
  std::vector<Entity> entities;

  auto getOrCreateEntity = [&entityMap, &entities,
                            this](uint32_t localId) mutable {
    if (entityMap.find(localId) == entityMap.end()) {
      auto entity = mEntityDatabase.create();
      mEntityDatabase.set<LocalTransform>(entity, {});
      mEntityDatabase.set<WorldTransform>(entity, {});

      entities.push_back(entity);
      entityMap.insert_or_assign(localId, entities.size() - 1);
      return entity;
    }

    return entities.at(entityMap.at(localId));
  };

  for (const auto &pTransform : asset.transforms) {
    if (pTransform.value.parent >= 0) {
      auto parent = getOrCreateEntity(pTransform.value.parent);
      auto entity = getOrCreateEntity(pTransform.entity);
      mEntityDatabase.set(entity, Parent{parent});

      if (!mEntityDatabase.has<Children>(entity)) {
        mEntityDatabase.set<Children>(parent, {{entity}});
      } else {
        mEntityDatabase.get<Children>(parent).children.push_back(entity);
      }
    }
  }

  for (const auto &pTransform : asset.transforms) {
    auto entity = getOrCreateEntity(pTransform.entity);
    LocalTransform transform{};
    transform.localPosition = pTransform.value.position;
    transform.localRotation = pTransform.value.rotation;
    transform.localScale = pTransform.value.scale;

    mEntityDatabase.set<LocalTransform>(entity, transform);
  }

  for (const auto &pMesh : asset.meshes) {
    auto entity = getOrCreateEntity(pMesh.entity);
    mEntityDatabase.set<Mesh>(entity, {pMesh.value});
  }

  for (const auto &pSkinnedMesh : asset.skinnedMeshes) {
    auto entity = getOrCreateEntity(pSkinnedMesh.entity);
    mEntityDatabase.set<SkinnedMesh>(entity, {pSkinnedMesh.value});
  }

  for (const auto &pSkeleton : asset.skeletons) {
    auto entity = getOrCreateEntity(pSkeleton.entity);

    const auto &asset =
        mAssetRegistry.getSkeletons().getAsset(pSkeleton.value).data;

    size_t numJoints = asset.jointLocalPositions.size();

    Skeleton skeleton{};
    skeleton.assetHandle = pSkeleton.value;
    skeleton.numJoints = static_cast<uint32_t>(numJoints);
    skeleton.jointNames.resize(numJoints);
    skeleton.jointParents.resize(numJoints);
    skeleton.jointLocalPositions.resize(numJoints);
    skeleton.jointLocalRotations.resize(numJoints);
    skeleton.jointLocalScales.resize(numJoints);
    skeleton.jointInverseBindMatrices.resize(numJoints);
    skeleton.jointWorldTransforms.resize(numJoints, glm::mat4{1.0f});
    skeleton.jointFinalTransforms.resize(numJoints, glm::mat4{1.0f});

    for (size_t i = 0; i < numJoints; ++i) {
      skeleton.jointNames.at(i) = asset.jointNames.at(i);
      skeleton.jointParents.at(i) = asset.jointParents.at(i);
      skeleton.jointLocalPositions.at(i) = asset.jointLocalPositions.at(i);
      skeleton.jointLocalRotations.at(i) = asset.jointLocalRotations.at(i);
      skeleton.jointLocalScales.at(i) = asset.jointLocalScales.at(i);
      skeleton.jointInverseBindMatrices.at(i) =
          asset.jointInverseBindMatrices.at(i);
    }

    mEntityDatabase.set(entity, skeleton);
  }

  for (auto &item : asset.animators) {
    auto entity = getOrCreateEntity(item.entity);
    mEntityDatabase.set<Animator>(entity, item.value);
  }

  for (auto &item : asset.directionalLights) {
    auto entity = getOrCreateEntity(item.entity);
    mEntityDatabase.set<DirectionalLight>(entity, item.value);
  }

  for (auto &item : asset.pointLights) {
    auto entity = getOrCreateEntity(item.entity);
    mEntityDatabase.set<PointLight>(entity, item.value);
  }

  std::vector<Entity> rootEntities;
  for (auto entity : entities) {
    if (!mEntityDatabase.has<Parent>(entity)) {
      rootEntities.push_back(entity);
    }
  }

  LIQUID_ASSERT(!rootEntities.empty(),
                "Nothing is spawned. Check that prefab is not empty.");

  auto rootNode = Entity::Null;
  // If more than one root exists,
  // create root node
  if (rootEntities.size() > 1) {
    rootNode = mEntityDatabase.create();
    mEntityDatabase.set<Children>(rootNode, {rootEntities});
    for (auto entity : rootEntities) {
      mEntityDatabase.set<Parent>(entity, {rootNode});
    }
    entities.push_back(rootNode);
  } else {
    rootNode = rootEntities.at(0);
  }

  mEntityDatabase.set(rootNode, transform);
  mEntityDatabase.set<WorldTransform>(rootNode, {});
  mEntityDatabase.set(rootNode, Name{"New entity"});

  return entities;
}

} // namespace liquid