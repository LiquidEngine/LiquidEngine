#pragma once

#include "quoll/asset/AssetCache.h"
#include "quoll/lua-scripting/LuaScriptAsset.h"
#include "quoll/lua-scripting/LuaScriptingSystem.h"
#include "quoll/physics/PhysicsSystem.h"
#include "quoll/system/SystemView.h"
#include "quoll/window/WindowSignals.h"
#include "quoll-tests/Testing.h"
#include "quoll-tests/test-utils/AssetCacheUtils.h"
#include "TestPhysicsBackend.h"

class LuaScriptingInterfaceTestBase : public ::testing::Test {
  static const quoll::String ScriptName;

public:
  LuaScriptingInterfaceTestBase(const quoll::String &scriptName = ScriptName);

  sol::state_view start(quoll::Entity entity);

  sol::state_view call(quoll::Entity entity, const quoll::String &functionName);

  quoll::AssetRef<quoll::LuaScriptAsset> loadScript(quoll::String scriptName);

  void SetUp() override;

  void TearDown() override;

  template <typename TAssetData>
  quoll::AssetRef<TAssetData> createAsset(TAssetData data = {}) {
    return createAssetInCache(assetCache, data);
  }

protected:
  quoll::AssetCache assetCache;
  quoll::Scene scene;
  quoll::EntityDatabase &entityDatabase = scene.entityDatabase;
  quoll::SystemView view{&scene};
  quoll::LuaScriptingSystem scriptingSystem;
  TestPhysicsBackend *physicsBackend = new TestPhysicsBackend;
  quoll::PhysicsSystem physicsSystem;
  quoll::WindowSignals windowSignals;
  quoll::String mScriptName;
};
