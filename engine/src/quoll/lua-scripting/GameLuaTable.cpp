#include "quoll/core/Base.h"
#include "quoll/entity/EntityQueryLuaTable.h"
#include "quoll/entity/EntitySpawnerLuaTable.h"
#include "quoll/logger/UserLoggerLuaTable.h"
#include "quoll/ui/UILuaTable.h"

#include "GameLuaTable.h"

namespace quoll {

GameLuaTable::GameLuaTable(Entity entity, ScriptGlobals scriptGlobals)
    : mEntity(entity), mScriptGlobals(scriptGlobals) {}

sol::object GameLuaTable::get(String name) {
  auto &script = mScriptGlobals.entityDatabase.get<LuaScript>(mEntity);

  sol::state_view state(script.state);
  if (name == "EntityQuery") {
    EntityQueryLuaTable::create(state);
    return sol::make_object(state, EntityQueryLuaTable(mScriptGlobals));
  }

  if (name == "EntitySpawner") {
    EntitySpawnerLuaTable::create(state);
    return sol::make_object(state, EntitySpawnerLuaTable(mScriptGlobals));
  }

  if (name == "UI") {
    return sol::make_object(state, UILuaTable::create(state));
  }

  if (name == "Logger") {
    return UserLoggerLuaTable::create(state);
  }

  return sol::make_object(state, sol::nil);
}

lua::ScriptSignalView GameLuaTable::onUpdate() {
  auto &script = mScriptGlobals.entityDatabase.get<LuaScript>(mEntity);
  return lua::ScriptSignalView(mScriptGlobals.scriptLoop.getUpdateSignal(),
                               script);
}

void GameLuaTable::create(sol::state_view state) {
  auto usertype = state.new_usertype<GameLuaTable>("Game", sol::no_constructor);
  usertype["on_update"] = sol::property(&GameLuaTable::onUpdate);
  usertype["get"] = &GameLuaTable::get;
}

} // namespace quoll
