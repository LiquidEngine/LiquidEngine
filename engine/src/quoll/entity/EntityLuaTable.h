#pragma once

#include "quoll/entity/Entity.h"

#include "quoll/lua-scripting/LuaUserTypeBase.h"
#include "quoll/physics/RigidBodyLuaTable.h"
#include "quoll/physics/CollidableLuaTable.h"
#include "quoll/audio/AudioLuaTable.h"
#include "quoll/scene/TransformLuaTable.h"
#include "quoll/scene/PerspectiveLensLuaTable.h"
#include "quoll/text/TextLuaTable.h"
#include "quoll/animation/AnimatorLuaTable.h"
#include "quoll/input/InputMapLuaTable.h"
#include "quoll/ui/UICanvasLuaTable.h"
#include "quoll/lua-scripting/ScriptLuaTable.h"

namespace quoll {

/**
 * @brief Entity table
 */
class EntityLuaTable {
public:
  /**
   * @brief Create entity table
   *
   * @param entity Entity
   * @param scriptGlobals Script globals
   */
  EntityLuaTable(Entity entity, ScriptGlobals scriptGlobals);

  /**
   * @brief Create interface
   *
   * @param state Sol state
   */
  static void create(sol::state_view state);

  /**
   * @brief Get entity
   *
   * @return Entity
   */
  inline Entity getEntity() { return mEntity; }

  /**
   * @brief Get script globals
   *
   * @return Script globals;
   */
  inline ScriptGlobals &getScriptGlobals() { return mScriptGlobals; }

  /**
   * @brief Equality operator
   *
   * @param rhs Other entity lua table
   * @retval true Entities are equal
   * @retval false Entities are not equal
   */
  inline bool operator==(const EntityLuaTable &rhs) const {
    return mEntity == rhs.mEntity;
  }

private:
  Entity mEntity;
  ScriptGlobals mScriptGlobals;

  TransformLuaTable mTransform;
  PerspectiveLensLuaTable mPerspectiveLens;
  RigidBodyLuaTable mRigidBody;
  CollidableLuaTable mCollidable;
  AudioLuaTable mAudio;
  TextLuaTable mText;
  AnimatorLuaTable mAnimator;
  InputMapLuaTable mInputMap;
  UICanvasLuaTable mUICanvas;
  ScriptLuaTable mScript;
};

} // namespace quoll
