#include "quoll/core/Base.h"
#include "DeleteEntityAction.h"
#include "EntityLightActions.h"

namespace quoll::editor {

EntityDeleteDirectionalLight::EntityDeleteDirectionalLight(Entity entity)
    : mEntity(entity) {}

ActionExecutorResult
EntityDeleteDirectionalLight::onExecute(WorkspaceState &state,
                                        AssetCache &assetCache) {
  auto &scene = state.scene;

  mOldDirectionalLight = scene.entityDatabase.get<DirectionalLight>(mEntity);
  scene.entityDatabase.remove<DirectionalLight>(mEntity);

  if (scene.entityDatabase.has<CascadedShadowMap>(mEntity)) {
    mOldCascadedShadowMap =
        scene.entityDatabase.get<CascadedShadowMap>(mEntity);
    scene.entityDatabase.remove<CascadedShadowMap>(mEntity);
  }

  ActionExecutorResult res{};
  res.entitiesToSave.push_back(mEntity);
  res.addToHistory = true;
  return res;
}

ActionExecutorResult
EntityDeleteDirectionalLight::onUndo(WorkspaceState &state,
                                     AssetCache &assetCache) {
  auto &scene = state.scene;

  scene.entityDatabase.set(mEntity, mOldDirectionalLight);

  if (mOldCascadedShadowMap) {
    scene.entityDatabase.set(mEntity, mOldCascadedShadowMap.value());
  }

  ActionExecutorResult res{};
  res.entitiesToSave.push_back(mEntity);
  return res;
}

bool EntityDeleteDirectionalLight::predicate(WorkspaceState &state,
                                             AssetCache &assetCache) {
  auto &scene = state.scene;

  return scene.entityDatabase.has<DirectionalLight>(mEntity);
}

} // namespace quoll::editor
