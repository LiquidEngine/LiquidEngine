#include "quoll/core/Base.h"
#include "quoll/physics/Collidable.h"
#include "EntityCollidableActions.h"

namespace quoll::editor {

namespace {

PhysicsGeometryParams
getDefaultGeometryFromType(const PhysicsGeometryType &type) {
  switch (type) {
  case PhysicsGeometryType::Box:
  default:
    return PhysicsGeometryBox();
  case PhysicsGeometryType::Sphere:
    return PhysicsGeometrySphere();
  case PhysicsGeometryType::Capsule:
    return PhysicsGeometryCapsule();
  case PhysicsGeometryType::Plane:
    return PhysicsGeometryPlane();
  }
}

} // namespace

EntitySetCollidableType::EntitySetCollidableType(Entity entity,
                                                 PhysicsGeometryType type)
    : mEntity(entity), mType(type) {}

ActionExecutorResult
EntitySetCollidableType::onExecute(WorkspaceState &state,
                                   AssetCache &assetCache) {
  auto &scene = state.scene;

  auto &collidable = scene.entityDatabase.get<Collidable>(mEntity);
  mOldCollidable = collidable;

  collidable.geometryDesc.type = mType;
  collidable.geometryDesc.params = getDefaultGeometryFromType(mType);

  scene.entityDatabase.set(mEntity, collidable);

  ActionExecutorResult res{};
  res.entitiesToSave.push_back(mEntity);
  res.addToHistory = true;
  return res;
}

ActionExecutorResult EntitySetCollidableType::onUndo(WorkspaceState &state,
                                                     AssetCache &assetCache) {
  auto &scene = state.scene;

  scene.entityDatabase.set(mEntity, mOldCollidable);

  ActionExecutorResult res{};
  res.entitiesToSave.push_back(mEntity);
  return res;
}

bool EntitySetCollidableType::predicate(WorkspaceState &state,
                                        AssetCache &assetCache) {
  auto &scene = state.scene;

  return scene.entityDatabase.has<Collidable>(mEntity) &&
         scene.entityDatabase.get<Collidable>(mEntity).geometryDesc.type !=
             mType;
}

} // namespace quoll::editor
