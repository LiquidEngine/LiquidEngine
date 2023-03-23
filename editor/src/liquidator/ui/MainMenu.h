#pragma once

#include "liquidator/actions/ActionExecutor.h"
#include "Shortcut.h"

namespace liquid::editor {

/**
 * @brief Main menu item
 *
 * Represents a menu item that can
 * either be clicked or is a parent
 */
class MainMenuItem {
public:
  /**
   * @brief Create main menu item
   *
   * @param parent Parent menu item
   * @param label Menu item label
   * @param action Menu item action
   * @param shortcut Shortcut
   */
  MainMenuItem(MainMenuItem *parent, String label, Action *action,
               Shortcut shortcut);

  /**
   * @brief Begin submenu
   *
   * @param label Menu item label
   * @return Newly created item
   */
  MainMenuItem &begin(String label);

  /**
   * @brief End submenu
   *
   * @return Parent menu item or itself
   */
  MainMenuItem &end();

  /**
   * @brief Add actionable menu item
   *
   * @param label Menu item label
   * @param action Menu item action
   * @param shortcut Shortcut
   * @return This menu item
   */
  MainMenuItem &add(String label, Action *action, Shortcut shortcut = {});

  /**
   * @brief Get children
   *
   * @return Menu item children
   */
  inline const std::vector<MainMenuItem> &getChildren() const {
    return mChildren;
  }

  /**
   * @brief Render menu item
   *
   * @param actionExecutor Action executor
   */
  void render(ActionExecutor &actionExecutor) const;

private:
  std::unique_ptr<Action> mAction;
  String mLabel;
  Shortcut mShortcut;
  std::vector<MainMenuItem> mChildren;

  MainMenuItem *mParent = nullptr;
};

/**
 * @brief Main menu
 */
class MainMenu : public MainMenuItem {
public:
  /**
   * @brief Create main menu
   */
  MainMenu();

  /**
   * @brief Render main menu
   *
   * @param actionExecutor Action executor
   */
  void render(ActionExecutor &actionExecutor);
};

} // namespace liquid::editor
