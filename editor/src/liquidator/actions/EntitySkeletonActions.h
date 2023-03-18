#pragma once

#include "liquidator/actions/Action.h"

namespace liquid::editor {

/**
 * @brief Toggle debug bones for skeleton entity actions
 */
class EntityToggleSkeletonDebugBones : public Action {
public:
  /**
   * @brief Create action
   *
   * @param entity Entity
   */
  EntityToggleSkeletonDebugBones(Entity entity);

  /**
   * @brief Action executor
   *
   * @param state Workspace state
   * @return Executor result
   */
  ActionExecutorResult onExecute(WorkspaceState &state) override;

  /**
   * @brief Action predicate
   *
   * @param state Workspace state
   * @retval true Predicate is true
   * @retval false Predicate is false
   */
  bool predicate(WorkspaceState &state) override;

private:
  Entity mEntity;
};

/**
 * @brief Set skeleton for entity action
 */
class EntitySetSkeleton : public Action {
public:
  /**
   * @brief Create action
   *
   * @param entity Entity
   * @param handle Skeleton asset handle
   */
  EntitySetSkeleton(Entity entity, SkeletonAssetHandle handle);

  /**
   * @brief Action executor
   *
   * @param state Workspace state
   * @return Executor result
   */
  ActionExecutorResult onExecute(WorkspaceState &state) override;

  /**
   * @brief Action predicate
   *
   * @param state Workspace state
   * @retval true Predicate is true
   * @retval false Predicate is false
   */
  bool predicate(WorkspaceState &state) override;

private:
  Entity mEntity;
  SkeletonAssetHandle mHandle;
};

} // namespace liquid::editor