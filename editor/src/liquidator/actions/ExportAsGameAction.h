#pragma once

#include "liquidator/actions/Action.h"

namespace liquid::editor {

/**
 * @brief Export as game action
 */
class ExportAsGameAction : public Action {
public:
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
};

} // namespace liquid::editor