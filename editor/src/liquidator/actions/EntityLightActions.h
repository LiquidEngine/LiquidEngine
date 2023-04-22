#pragma once

#include "liquidator/actions/Action.h"
#include "EntityDefaultDeleteAction.h"

namespace liquid::editor {

using EntityCreateDirectionalLight =
    EntityDefaultCreateComponent<DirectionalLight>;

using EntitySetDirectionalLight =
    EntityDefaultUpdateComponent<DirectionalLight>;

using EntityEnableCascadedShadowMap =
    EntityDefaultCreateComponent<CascadedShadowMap>;

using EntityDisableCascadedShadowMap =
    EntityDefaultDeleteAction<CascadedShadowMap>;

using EntitySetCascadedShadowMap =
    EntityDefaultUpdateComponent<CascadedShadowMap>;

/**
 * @brief Delete directional light from entity action
 */
class EntityDeleteDirectionalLight : public Action {
public:
  /**
   * @brief Create action
   *
   * @param entity Entity
   */
  EntityDeleteDirectionalLight(Entity entity);

  /**
   * @brief Action executor
   *
   * @param state Workspace state
   * @return Executor result
   */
  ActionExecutorResult onExecute(WorkspaceState &state) override;

  /**
   * @brief Action undo
   *
   * @param state Workspace state
   * @return Executor result
   */
  ActionExecutorResult onUndo(WorkspaceState &state) override;

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
  DirectionalLight mOldDirectionalLight;
  std::optional<CascadedShadowMap> mOldCascadedShadowMap;
};

using EntityCreatePointLight = EntityDefaultCreateComponent<PointLight>;

using EntitySetPointLight = EntityDefaultUpdateComponent<PointLight>;

using EntityDeletePointLight = EntityDefaultDeleteAction<PointLight>;

} // namespace liquid::editor
