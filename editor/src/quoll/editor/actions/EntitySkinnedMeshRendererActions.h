#pragma once

#include "quoll/renderer/SkinnedMeshRenderer.h"
#include "Action.h"

namespace quoll::editor {

class EntitySetSkinnedMeshRendererMaterial : public Action {
public:
  EntitySetSkinnedMeshRendererMaterial(Entity entity, usize slot,
                                       AssetHandle<MaterialAsset> handle);

  ActionExecutorResult onExecute(WorkspaceState &state,
                                 AssetCache &assetCache) override;

  ActionExecutorResult onUndo(WorkspaceState &state,
                              AssetCache &assetCache) override;

  bool predicate(WorkspaceState &state, AssetCache &assetCache) override;

private:
  Entity mEntity;
  usize mSlot;
  AssetHandle<MaterialAsset> mOldMaterial;
  AssetHandle<MaterialAsset> mNewMaterial;
};

class EntityAddSkinnedMeshRendererMaterialSlot : public Action {
public:
  EntityAddSkinnedMeshRendererMaterialSlot(Entity entity,
                                           AssetHandle<MaterialAsset> handle);

  ActionExecutorResult onExecute(WorkspaceState &state,
                                 AssetCache &assetCache) override;

  ActionExecutorResult onUndo(WorkspaceState &state,
                              AssetCache &assetCache) override;

  bool predicate(WorkspaceState &state, AssetCache &assetCache) override;

private:
  Entity mEntity;
  AssetHandle<MaterialAsset> mNewMaterial;
};

class EntityRemoveLastSkinnedMeshRendererMaterialSlot : public Action {
public:
  EntityRemoveLastSkinnedMeshRendererMaterialSlot(Entity entity);

  ActionExecutorResult onExecute(WorkspaceState &state,
                                 AssetCache &assetCache) override;

  ActionExecutorResult onUndo(WorkspaceState &state,
                              AssetCache &assetCache) override;

  bool predicate(WorkspaceState &state, AssetCache &assetCache) override;

private:
  Entity mEntity;
  AssetHandle<MaterialAsset> mOldMaterial;
};

} // namespace quoll::editor
