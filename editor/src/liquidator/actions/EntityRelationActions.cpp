#include "liquid/core/Base.h"
#include "EntityRelationActions.h"

namespace liquid::editor {

EntitySetParent::EntitySetParent(Entity entity, Entity parent)
    : mEntity(entity), mParent(parent) {}

ActionExecutorResult EntitySetParent::onExecute(WorkspaceState &state) {
  auto &scene = state.mode == WorkspaceMode::Simulation ? state.simulationScene
                                                        : state.scene;
  auto &db = scene.entityDatabase;

  if (db.has<Parent>(mEntity)) {
    mPreviousParent = db.get<Parent>(mEntity).parent;

    LIQUID_ASSERT(db.has<Children>(mPreviousParent),
                  "Parent entity must have children");
    auto &children = db.get<Children>(mPreviousParent).children;

    auto it = std::find(children.begin(), children.end(), mEntity);

    LIQUID_ASSERT(it != children.end(),
                  "Entity must exist in children of parent");

    if (children.size() == 1) {
      db.remove<Children>(mPreviousParent);
    } else {
      children.erase(it);
    }
  } else {
    mPreviousParent = Entity::Null;
  }

  db.set<Parent>(mEntity, {mParent});
  if (db.has<Children>(mParent)) {
    db.get<Children>(mParent).children.push_back(mEntity);
  } else {
    db.set<Children>(mParent, {{mEntity}});
  }

  ActionExecutorResult res;
  res.addToHistory = true;

  res.entitiesToSave = {mEntity, mParent};
  if (mPreviousParent != Entity::Null) {
    res.entitiesToSave.push_back(mPreviousParent);
  }

  return res;
}

ActionExecutorResult EntitySetParent::onUndo(WorkspaceState &state) {
  auto &scene = state.mode == WorkspaceMode::Simulation ? state.simulationScene
                                                        : state.scene;
  auto &db = scene.entityDatabase;

  if (mPreviousParent != Entity::Null) {
    if (db.has<Children>(mPreviousParent)) {
      db.get<Children>(mPreviousParent).children.push_back(mEntity);
    } else {
      db.set<Children>(mPreviousParent, {{mEntity}});
    }

    db.set<Parent>(mEntity, {mPreviousParent});
  } else {
    db.remove<Parent>(mEntity);
  }

  LIQUID_ASSERT(db.has<Children>(mParent), "Entity parent has no children");
  auto &children = db.get<Children>(mParent).children;
  auto it = std::find(children.begin(), children.end(), mEntity);

  LIQUID_ASSERT(it != children.end(),
                "Entity must exist in children of parent");

  if (children.size() == 1) {
    db.remove<Children>(mParent);
  } else {
    children.erase(it);
  }

  ActionExecutorResult res;

  res.entitiesToSave = {mEntity, mParent};
  if (mPreviousParent != Entity::Null) {
    res.entitiesToSave.push_back(mPreviousParent);
  }

  return res;
}

bool EntitySetParent::predicate(WorkspaceState &state) {
  auto &scene = state.mode == WorkspaceMode::Simulation ? state.simulationScene
                                                        : state.scene;
  auto &db = scene.entityDatabase;

  // Parent does not exist
  if (!db.exists(mParent)) {
    return false;
  }

  // Parent is already a parent of entity
  if (db.has<Parent>(mEntity) && db.get<Parent>(mEntity).parent == mParent) {
    return false;
  }

  auto parent = mParent;
  bool parentIsNotDescendant = parent != mEntity;
  while (parentIsNotDescendant && db.has<Parent>(parent)) {
    auto p = db.get<Parent>(parent).parent;
    parentIsNotDescendant = p != mEntity;
    parent = p;
  }

  return parentIsNotDescendant;
}

} // namespace liquid::editor