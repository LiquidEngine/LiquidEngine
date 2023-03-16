#pragma once

#include "liquidator/project/Project.h"
#include "liquidator/state/WorkspaceState.h"

#include "EditorCamera.h"
#include "EntityManager.h"

namespace liquid::editor {

enum class EnvironmentLightingSource { None = 0, Skybox = 1 };

/**
 * @brief Editor manager
 *
 * Manages, saves, and loads the
 * scene with editor settings
 */
class EditorManager {
public:
  /**
   * @brief Create editor manager
   *
   * @param editorCamera Editor camera
   * @param entityManager Entity manager
   * @param project Project
   */
  EditorManager(EditorCamera &editorCamera, EntityManager &entityManager,
                const Project &project);

  EditorManager(const EditorManager &) = delete;
  EditorManager(EditorManager &&) = delete;
  EditorManager &operator=(const EditorManager &) = delete;
  EditorManager &operator=(EditorManager &&) = delete;
  ~EditorManager() = default;

  /**
   * @brief Save workspace state to a file
   *
   * @param state Workspace state
   * @param path Path to workspace state file
   */
  void saveWorkspaceState(WorkspaceState &state,
                          const std::filesystem::path &path);

  /**
   * @brief Load workspace state from file
   *
   * @param path Path to workspace state file
   * @param state Workspace state
   */
  void loadWorkspaceState(const std::filesystem::path &path,
                          WorkspaceState &state);

  /**
   * @brief Get editor camera
   *
   * @return Editor camera
   */
  inline EditorCamera &getEditorCamera() { return mEditorCamera; }

  /**
   * @brief Creates a new scene and sets it as active
   */
  void createNewScene();

  /**
   * @brief Load or create scene
   */
  void loadOrCreateScene();

private:
  EditorCamera &mEditorCamera;
  std::filesystem::path mScenePath;
  EntityManager &mEntityManager;
  Project mProject;
};

} // namespace liquid::editor
