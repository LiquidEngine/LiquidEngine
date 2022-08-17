#include "liquid/core/Base.h"
#include "liquid/scene/SceneUpdater.h"

#include "liquid-tests/Testing.h"

class SceneUpdaterTest : public ::testing::Test {
public:
  liquid::EntityDatabase entityDatabase;
  liquid::SceneUpdater sceneUpdater;
};

glm::mat4 getLocalTransform(const liquid::LocalTransformComponent &transform) {
  return glm::translate(glm::mat4(1.0f), transform.localPosition) *
         glm::toMat4(transform.localRotation) *
         glm::scale(glm::mat4(1.0f), transform.localScale);
}

TEST_F(SceneUpdaterTest, SetsLocalTransformToWorldTransformIfNoParent) {
  auto entity = entityDatabase.createEntity();
  liquid::LocalTransformComponent transform{};
  transform.localPosition = glm::vec3(1.0f, 0.5f, 2.5f);
  transform.localRotation = glm::quat(-0.361f, 0.697f, -0.391f, 0.481f);
  transform.localScale = glm::vec3(0.2f, 0.5f, 1.5f);

  entityDatabase.setComponent<liquid::WorldTransformComponent>(entity, {});
  entityDatabase.setComponent(entity, transform);

  sceneUpdater.update(entityDatabase);

  EXPECT_EQ(entityDatabase.getComponent<liquid::WorldTransformComponent>(entity)
                .worldTransform,
            getLocalTransform(transform));
}

TEST_F(SceneUpdaterTest, CalculatesWorldTransformFromParentWorldTransform) {
  // parent
  auto parent = entityDatabase.createEntity();
  liquid::LocalTransformComponent parentTransform{};
  parentTransform.localPosition = glm::vec3(1.0f, 0.5f, 2.5f);
  parentTransform.localRotation = glm::quat(-0.361f, 0.697f, -0.391f, 0.481f);
  parentTransform.localScale = glm::vec3(0.2f, 0.5f, 1.5f);
  entityDatabase.setComponent(parent, parentTransform);
  entityDatabase.setComponent<liquid::WorldTransformComponent>(parent, {});

  // parent -> child1
  auto child1 = entityDatabase.createEntity();
  liquid::LocalTransformComponent child1Transform{};
  child1Transform.localPosition = glm::vec3(1.0f, 0.5f, 2.5f);
  child1Transform.localRotation = glm::quat(-0.361f, 0.697f, -0.391f, 0.481f);
  child1Transform.localScale = glm::vec3(0.2f, 0.5f, 1.5f);
  entityDatabase.setComponent(child1, child1Transform);
  entityDatabase.setComponent<liquid::ParentComponent>(child1, {parent});
  entityDatabase.setComponent<liquid::WorldTransformComponent>(child1, {});

  // parent -> child1 -> child2
  auto child2 = entityDatabase.createEntity();
  liquid::LocalTransformComponent child2Transform{};
  child2Transform.localPosition = glm::vec3(1.0f, 0.5f, 2.5f);
  child2Transform.localRotation = glm::quat(-0.361f, 0.697f, -0.391f, 0.481f);
  child2Transform.localScale = glm::vec3(0.2f, 0.5f, 1.5f);
  entityDatabase.setComponent(child2, child2Transform);
  entityDatabase.setComponent<liquid::ParentComponent>(child2, {child1});
  entityDatabase.setComponent<liquid::WorldTransformComponent>(child2, {});

  sceneUpdater.update(entityDatabase);

  EXPECT_EQ(entityDatabase.getComponent<liquid::WorldTransformComponent>(parent)
                .worldTransform,
            getLocalTransform(parentTransform));

  EXPECT_EQ(entityDatabase.getComponent<liquid::WorldTransformComponent>(child1)
                .worldTransform,
            getLocalTransform(parentTransform) *
                getLocalTransform(child1Transform));

  EXPECT_EQ(entityDatabase.getComponent<liquid::WorldTransformComponent>(child2)
                .worldTransform,
            getLocalTransform(parentTransform) *
                getLocalTransform(child1Transform) *
                getLocalTransform(child2Transform));
}

TEST_F(SceneUpdaterTest, UpdatesCameraBasedOnTransformAndPerspectiveLens) {
  auto entity = entityDatabase.createEntity();

  {
    liquid::LocalTransformComponent transform{};
    transform.localPosition = glm::vec3(1.0f, 0.5f, 2.5f);
    entityDatabase.setComponent(entity, transform);
    entityDatabase.setComponent<liquid::WorldTransformComponent>(entity, {});

    liquid::PerspectiveLensComponent lens{};
    entityDatabase.setComponent(entity, lens);

    liquid::CameraComponent camera{};
    entityDatabase.setComponent(entity, camera);
  }
  sceneUpdater.update(entityDatabase);

  auto &transform =
      entityDatabase.getComponent<liquid::WorldTransformComponent>(entity);
  auto &lens =
      entityDatabase.getComponent<liquid::PerspectiveLensComponent>(entity);
  auto &camera = entityDatabase.getComponent<liquid::CameraComponent>(entity);

  auto expectedPerspective = glm::perspective(
      glm::radians(lens.fovY), lens.aspectRatio, lens.near, lens.far);
  expectedPerspective[1][1] *= -1.0f;

  EXPECT_EQ(camera.viewMatrix, glm::inverse(transform.worldTransform));
  EXPECT_EQ(camera.projectionMatrix, expectedPerspective);
  EXPECT_EQ(camera.projectionViewMatrix,
            camera.projectionMatrix * camera.viewMatrix);
}

TEST_F(SceneUpdaterTest, UpdateDirectionalLightsBasedOnTransforms) {
  auto entity = entityDatabase.createEntity();

  {
    liquid::LocalTransformComponent transform{};
    transform.localRotation = glm::quat(-0.361f, 0.697f, -0.391f, 0.481f);
    entityDatabase.setComponent(entity, transform);
    entityDatabase.setComponent<liquid::WorldTransformComponent>(entity, {});

    liquid::DirectionalLightComponent light{};
    entityDatabase.setComponent(entity, light);
  }
  sceneUpdater.update(entityDatabase);

  auto &transform =
      entityDatabase.getComponent<liquid::WorldTransformComponent>(entity);
  auto &light =
      entityDatabase.getComponent<liquid::DirectionalLightComponent>(entity);

  glm::quat rotation;
  glm::vec3 empty3;
  glm::vec4 empty4;
  glm::vec3 position;

  glm::decompose(transform.worldTransform, empty3, rotation, position, empty3,
                 empty4);

  rotation = glm::conjugate(rotation);
  auto expected =
      glm::normalize(glm::vec3(rotation * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)));

  EXPECT_EQ(light.direction, expected);
}
