#pragma once

#include "quoll/lua-scripting/LuaUserTypeBase.h"
#include "UIComponents.h"

namespace quoll {

/**
 * @brief Lua table for ui canvas
 */
class UICanvasLuaTable {
public:
  /**
   * @brief Create ui canvas lua table
   *
   * @param entity Entity
   * @param scriptGlobals Script globals
   */
  UICanvasLuaTable(Entity entity, ScriptGlobals scriptGlobals);

  /**
   * @brief Render to ui canvas
   *
   * @param view UI view
   */
  void render(UIView view);

  /**
   * @brief Create user type
   *
   * @param usertype User type
   * @param state Sol state
   */
  static void create(sol::usertype<UICanvasLuaTable> usertype,
                     sol::state_view state);

  /**
   * @brief Get component name in scripts
   *
   * @return Component name
   */
  static const String getName() { return "uiCanvas"; }

private:
  Entity mEntity;
  ScriptGlobals mScriptGlobals;
};

} // namespace quoll
