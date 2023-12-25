#pragma once

#include "quoll/asset/AssetRegistry.h"

#include "quoll/core/EntityDeleter.h"
#include "quoll/scene/SceneUpdater.h"
#include "quoll/scene/SkeletonUpdater.h"
#include "quoll/scene/CameraAspectRatioUpdater.h"
#include "quoll/animation/AnimationSystem.h"
#include "quoll/lua-scripting/LuaScriptingSystem.h"
#include "quoll/physics/PhysicsSystem.h"
#include "quoll/audio/AudioSystem.h"
#include "quoll/window/Window.h"
#include "quoll/input/InputMapSystem.h"
#include "quoll/ui/UICanvasUpdater.h"

#include "quoll/editor/editor-scene/EditorCamera.h"
#include "quoll/editor/workspace/WorkspaceState.h"

namespace quoll::editor {

/**
 * @brief Manager editor simulation
 *
 * Provides different updaters for
 * editor simulation
 */
class EditorSimulator {
public:
  /**
   * @brief Create editor simulation
   *
   * @param deviceManager Device manager
   * @param window Window
   * @param assetRegistry Asset registry
   * @param editorCamera Editor camera
   */
  EditorSimulator(InputDeviceManager &deviceManager, Window &window,
                  AssetRegistry &assetRegistry, EditorCamera &editorCamera);

  /**
   * @brief Main update function
   *
   * Uses simulation and editing
   * updates
   *
   * @param dt Time delta
   * @param state Workspace state
   */
  void update(f32 dt, WorkspaceState &state);

  /**
   * @brief Render
   *
   * @param db Entity database
   */
  void render(EntityDatabase &db);

  /**
   * @brief Get physics system
   *
   * @return Physics system
   */
  inline PhysicsSystem &getPhysicsSystem() { return mPhysicsSystem; }

  /**
   * @brief Get camera aspect ratio updater
   *
   * @return Camera aspect ratio updater
   */
  inline CameraAspectRatioUpdater &getCameraAspectRatioUpdater() {
    return mCameraAspectRatioUpdater;
  }

  /**
   * @brief Get UI Canvas updater
   *
   * @return UI Canvas updater
   */
  inline UICanvasUpdater &getUICanvasUpdater() { return mUICanvasUpdater; }

  /**
   * @brief Get editor camera
   *
   * @return Editor camera
   */
  inline EditorCamera &getEditorCamera() { return mEditorCamera; }

  /**
   * @brief Get window
   *
   * @return Window
   */
  inline Window &getWindow() { return mWindow; }

private:
  /**
   * @brief Cleanup simulation database
   *
   * @param simulationDatabase Simulation database
   */
  void cleanupSimulationDatabase(EntityDatabase &simulationDatabase);

  /**
   * @brief Observe changes for simulation database
   *
   * @param simulationDatabase Simulation database
   */
  void observeChanges(EntityDatabase &simulationDatabase);

  /**
   * @brief Editor updater
   *
   * @param dt Time delta
   * @param state Workspace state
   */
  void updateEditor(f32 dt, WorkspaceState &scene);

  /**
   * @brief Simulation updater
   *
   * @param dt Time delta
   * @param state Workspace state
   */
  void updateSimulation(f32 dt, WorkspaceState &scene);

private:
  std::function<void(f32, WorkspaceState &)> mUpdater;

  AssetRegistry &mAssetRegistry;

  EditorCamera &mEditorCamera;
  CameraAspectRatioUpdater mCameraAspectRatioUpdater;
  EntityDeleter mEntityDeleter;
  SkeletonUpdater mSkeletonUpdater;
  SceneUpdater mSceneUpdater;
  AnimationSystem mAnimationSystem;
  LuaScriptingSystem mScriptingSystem;
  PhysicsSystem mPhysicsSystem;
  AudioSystem<DefaultAudioBackend> mAudioSystem;
  InputMapSystem mInputMapSystem;
  Window &mWindow;
  UICanvasUpdater mUICanvasUpdater;

  WorkspaceMode mMode = WorkspaceMode::Edit;
};

} // namespace quoll::editor
