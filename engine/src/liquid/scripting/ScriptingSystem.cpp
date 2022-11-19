#include "liquid/core/Base.h"
#include "ScriptingSystem.h"
#include "LuaTable.h"
#include "EntityDecorator.h"

#include "liquid/core/EngineGlobals.h"

namespace liquid {

ScriptingSystem::ScriptingSystem(EventSystem &eventSystem,
                                 AssetRegistry &assetRegistry)
    : mEventSystem(eventSystem), mAssetRegistry(assetRegistry) {}

void ScriptingSystem::start(EntityDatabase &entityDatabase) {
  LIQUID_PROFILE_EVENT("ScriptingSystem::start");
  EntityDecorator entityDecorator;
  entityDatabase.iterateEntities<Script>([this, &entityDatabase,
                                          &entityDecorator](auto entity,
                                                            Script &component) {
    if (component.started) {
      return;
    }

    component.started = true;

    if (component.scope.getLuaState()) {
      mLuaInterpreter.destroyScope(component.scope);
    }
    component.scope = mLuaInterpreter.createScope();

    entityDecorator.attachToScope(component.scope, entity, entityDatabase);

    auto &script = mAssetRegistry.getLuaScripts().getAsset(component.handle);
    mLuaInterpreter.evaluate(script.data.bytes, component.scope);

    createScriptingData(component, entity);

    component.scope.luaGetGlobal("start");
    component.scope.call(0);
  });
}

void ScriptingSystem::update(float dt, EntityDatabase &entityDatabase) {
  LIQUID_PROFILE_EVENT("ScriptingSystem::update");

  entityDatabase.iterateEntities<Script, Delete>(
      [this, &entityDatabase](auto entity, Script &scripting, auto &) {
        destroyScriptingData(scripting);
        entityDatabase.remove<Script>(entity);
      });

  entityDatabase.iterateEntities<Script>(
      [this, &dt](auto entity, Script &component) {
        component.scope.luaGetGlobal("update");
        component.scope.set(dt);
        component.scope.call(1);
      });
}

void ScriptingSystem::cleanup(EntityDatabase &entityDatabase) {
  entityDatabase.iterateEntities<Script>(
      [this](auto entity, Script &scripting) {
        destroyScriptingData(scripting);
      });

  entityDatabase.destroyComponents<Script>();
}

void ScriptingSystem::createScriptingData(Script &component, Entity entity) {
  if (component.scope.hasFunction("on_collision_start")) {
    component.onCollisionStart = mEventSystem.observe(
        CollisionEvent::CollisionStarted,
        [this, &component, entity](const CollisionObject &data) {
          if (data.a == entity || data.b == entity) {
            component.scope.luaGetGlobal("on_collision_start");
            Entity target = data.a == entity ? data.b : data.a;
            auto table = component.scope.createTable(1);
            table.set("target", target);

            component.scope.call(1);
          }
        });
  }

  if (component.scope.hasFunction("on_collision_end")) {
    component.onCollisionEnd = mEventSystem.observe(
        CollisionEvent::CollisionEnded,
        [this, &component, entity](const CollisionObject &data) {
          if (data.a == entity || data.b == entity) {
            component.scope.luaGetGlobal("on_collision_end");
            Entity target = data.a == entity ? data.b : data.a;
            auto table = component.scope.createTable(1);
            table.set("target", target);

            component.scope.call(1);
          }
        });
  }

  if (component.scope.hasFunction("on_key_press")) {
    component.onKeyPress = mEventSystem.observe(
        KeyboardEvent::Pressed, [this, &component](const auto &data) {
          component.scope.luaGetGlobal("on_key_press");

          auto table = component.scope.createTable(1);
          table.set("key", data.key);

          component.scope.call(1);
        });
  }

  if (component.scope.hasFunction("on_key_release")) {
    component.onKeyRelease = mEventSystem.observe(
        KeyboardEvent::Released, [this, &component](const auto &data) {
          component.scope.luaGetGlobal("on_key_release");
          auto table = component.scope.createTable(1);
          table.set("key", data.key);

          component.scope.call(1);
        });
  }
}

void ScriptingSystem::destroyScriptingData(Script &component) {
  mLuaInterpreter.destroyScope(component.scope);

  if (component.onCollisionStart != EventObserverMax) {
    mEventSystem.removeObserver(CollisionEvent::CollisionStarted,
                                component.onCollisionStart);
  }

  if (component.onCollisionEnd != EventObserverMax) {
    mEventSystem.removeObserver(CollisionEvent::CollisionEnded,
                                component.onCollisionEnd);
  }

  if (component.onKeyPress != EventObserverMax) {
    mEventSystem.removeObserver(KeyboardEvent::Pressed, component.onKeyPress);
  }

  if (component.onKeyRelease != EventObserverMax) {
    mEventSystem.removeObserver(KeyboardEvent::Released,
                                component.onKeyRelease);
  }
}

} // namespace liquid
