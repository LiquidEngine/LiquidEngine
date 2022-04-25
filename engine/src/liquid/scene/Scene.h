#pragma once

#include "Camera.h"
#include "Light.h"
#include "liquid/entity/Entity.h"
#include "liquid/entity/EntityContext.h"

namespace liquid {

class Scene;

class SceneNode {
public:
  /**
   * @brief Create scene node
   *
   * @param entity Entity
   * @param transform Transform component
   * @param parent Parent node
   * @param context Entity context
   */
  SceneNode(Entity entity, const TransformComponent &transform,
            SceneNode *parent, EntityContext &context);

  /**
   * @brief Destroys scene node and its children
   */
  ~SceneNode();

  SceneNode(const SceneNode &rhs) = delete;
  SceneNode(SceneNode &&rhs) = delete;
  SceneNode &operator=(const SceneNode &rhs) = delete;
  SceneNode &operator=(SceneNode &&rhs) = delete;

  /**
   * @brief Updates children
   */
  void update();

  /**
   * @brief Adds child node with entity
   *
   * @param entity Entity
   * @param transform Transform component
   * @return Pointer to newly created scene node
   */
  SceneNode *addChild(Entity entity, const TransformComponent &component = {});

  /*
   * @brief Add child node
   *
   * @param node Child node
   */
  void addChild(SceneNode *node);

  /**
   * @brief Delete child node
   *
   * @param node Child node
   */
  void removeChild(SceneNode *node);

  /**
   * @brief Set entity
   *
   * @param entity Entity
   */
  void setEntity(Entity entity);

  /**
   * @brief Get transform
   *
   * @return Transform component
   */
  inline TransformComponent &getTransform() {
    return mEntityContext.getComponent<TransformComponent>(mEntity);
  }

  /**
   * @brief Gets world transform
   *
   * @return World transform matrix
   */
  inline const glm::mat4 &getWorldTransform() const {
    return mEntityContext.getComponent<TransformComponent>(mEntity)
        .worldTransform;
  }

  /**
   * @brief Get entity
   *
   * @return Entity
   */
  inline Entity getEntity() const { return mEntity; }

  /**
   * @brief Gets children
   *
   * @return List of children
   */
  inline const std::vector<SceneNode *> &getChildren() const {
    return mChildren;
  }

  /**
   * @brief Gets parent node
   *
   * @return Parent node
   */
  inline SceneNode *getParent() { return mParent; }

private:
  Entity mEntity = std::numeric_limits<Entity>::max();

  SceneNode *mParent = nullptr;
  std::vector<SceneNode *> mChildren;

  EntityContext &mEntityContext;
};

class Scene {
public:
  /**
   * @brief Creates root scene
   *
   * @param entityContext Entity context
   */
  Scene(EntityContext &entityContext);

  /**
   * @brief Destroys root scene
   */
  ~Scene();

  Scene(const Scene &rhs) = delete;
  Scene(Scene &&rhs) = delete;
  Scene &operator=(const Scene &rhs) = delete;
  Scene &operator=(Scene &&rhs) = delete;

  /**
   * @brief Updates scene
   */
  void update();

  /**
   * @brief Get root node
   *
   * @return Root scene node
   */
  inline SceneNode *getRootNode() { return mRootNode; }

  /**
   * @brief Get entity context
   *
   * @return Entity context
   */
  inline EntityContext &getEntityContext() { return mEntityContext; }

private:
  SceneNode *mRootNode = nullptr;
  EntityContext &mEntityContext;
};

} // namespace liquid
