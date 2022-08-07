#include "liquid/core/Base.h"
#include "liquid/imgui/ImguiUtils.h"

#include "EntityPanel.h"

#include "Widgets.h"

namespace liquidator {

/**
 * @brief Imgui text callback user data
 */
struct ImguiInputTextCallbackUserData {
  /**
   * Passed string value ref
   */
  liquid::String &value;
};

/**
 * @brief ImGui input text resize callback
 *
 * @param data Imgui input text callback data
 */
static int InputTextCallback(ImGuiInputTextCallbackData *data) {
  auto *userData =
      static_cast<ImguiInputTextCallbackUserData *>(data->UserData);
  if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
    auto &str = userData->value;
    LIQUID_ASSERT(data->Buf == str.c_str(),
                  "Buffer and string value must point to the same address");
    str.resize(data->BufTextLen);
    data->Buf = str.data();
  }
  return 0;
}

/**
 * @brief Multiline Input text for std::string
 *
 * @param label Label
 * @param value String value
 * @param flags Input text flags
 */
static bool ImguiMultilineInputText(const liquid::String &label,
                                    liquid::String &value, const ImVec2 &size,
                                    ImGuiInputTextFlags flags = 0) {
  LIQUID_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0,
                "Do not back callback resize flag");

  flags |= ImGuiInputTextFlags_CallbackResize;

  ImguiInputTextCallbackUserData userData{
      value,
  };
  return ImGui::InputTextMultiline(label.c_str(), value.data(),
                                   value.capacity() + 1, size, flags,
                                   InputTextCallback, &userData);
}

EntityPanel::EntityPanel(EntityManager &entityManager)
    : mEntityManager(entityManager) {}

void EntityPanel::render(EditorManager &editorManager,
                         liquid::Renderer &renderer,
                         liquid::AssetRegistry &assetRegistry,
                         liquid::PhysicsSystem &physicsSystem) {
  if (widgets::Window::begin("Entity")) {
    if (mEntityManager.getActiveEntityDatabase().hasEntity(mSelectedEntity)) {
      renderName();
      renderTransform();
      renderText(assetRegistry);
      renderMesh(assetRegistry);
      renderLight();
      renderCamera(editorManager);
      renderAnimation(assetRegistry);
      renderSkeleton();
      renderCollidable();
      renderRigidBody();
      renderAudio(assetRegistry);
      renderScripting(assetRegistry);
      renderAddComponent();
      handleDragAndDrop(renderer, assetRegistry);
    }
  }
  widgets::Window::end();
}

void EntityPanel::setSelectedEntity(liquid::Entity entity) {
  mSelectedEntity = entity;
  mName = mEntityManager.getActiveEntityDatabase()
              .getComponent<liquid::NameComponent>(mSelectedEntity)
              .name;
}

void EntityPanel::renderName() {
  if (!mIsNameActivated) {
    mName = mEntityManager.getActiveEntityDatabase()
                .getComponent<liquid::NameComponent>(mSelectedEntity)
                .name;
  }

  if (widgets::Section::begin("Name")) {
    ImGui::InputText(
        "##Input", const_cast<char *>(mName.c_str()), mName.capacity() + 1,
        ImGuiInputTextFlags_CallbackResize,
        [](ImGuiInputTextCallbackData *data) -> int {
          if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
            liquid::String *str = static_cast<liquid::String *>(data->UserData);

            str->resize(data->BufTextLen);
            data->Buf = const_cast<char *>(str->c_str());
          }
          return 0;
        },
        &mName);

    mIsNameActivated = ImGui::IsItemActivated();

    if (ImGui::IsItemDeactivatedAfterEdit()) {
      if (!mName.empty()) {
        mEntityManager.setName(mSelectedEntity, mName);
        mEntityManager.save(mSelectedEntity);
      }

      mName = mEntityManager.getActiveEntityDatabase()
                  .getComponent<liquid::NameComponent>(mSelectedEntity)
                  .name;
    }
  }
  widgets::Section::end();
}

void EntityPanel::renderLight() {
  if (!mEntityManager.getActiveEntityDatabase()
           .hasComponent<liquid::DirectionalLightComponent>(mSelectedEntity)) {
    return;
  }

  if (widgets::Section::begin("Light")) {
    auto &component =
        mEntityManager.getActiveEntityDatabase()
            .getComponent<liquid::DirectionalLightComponent>(mSelectedEntity);

    ImGui::Text("Type");
    if (ImGui::BeginCombo("###LightType", "Directional", 0)) {
      if (ImGui::Selectable("Directional")) {
      }
      ImGui::EndCombo();
    }

    ImGui::Text("Direction");
    ImGui::Text("%.3f %.3f %.3f", component.direction.x, component.direction.y,
                component.direction.z);

    ImGui::Text("Color");
    if (liquid::imgui::inputColor("###InputColor", component.color)) {
      mEntityManager.save(mSelectedEntity);
    }

    ImGui::Text("Intensity");
    if (liquid::imgui::input("###InputIntensity", component.intensity)) {
      mEntityManager.save(mSelectedEntity);
    }
  }
  widgets::Section::end();
}

void EntityPanel::renderCamera(EditorManager &editorManager) {
  if (!mEntityManager.getActiveEntityDatabase()
           .hasComponent<liquid::PerspectiveLensComponent>(mSelectedEntity)) {
    return;
  }

  if (widgets::Section::begin("Camera")) {
    auto &component =
        mEntityManager.getActiveEntityDatabase()
            .getComponent<liquid::PerspectiveLensComponent>(mSelectedEntity);

    ImGui::Text("FOV");
    ImGui::InputFloat("###InputFOV", &component.fovY);
    if (component.fovY < 0.0f) {
      component.fovY = 0.0f;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      mEntityManager.save(mSelectedEntity);
    }

    ImGui::Text("Near");
    ImGui::InputFloat("###InputNear", &component.near);
    if (component.near < 0.0f) {
      component.near = 0.0f;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      mEntityManager.save(mSelectedEntity);
    }

    ImGui::Text("Far");
    ImGui::InputFloat("###InputFar", &component.far);
    if (component.far < 0.0f) {
      component.far = 0.0f;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      mEntityManager.save(mSelectedEntity);
    }

    ImGui::Text("Aspect Ratio");
    static constexpr float MinCustomAspectRatio = 0.01f;
    static constexpr float MaxCustomAspectRatio = 1000.0f;

    bool hasViewportAspectRatio =
        mEntityManager.getActiveEntityDatabase()
            .hasComponent<liquid::AutoAspectRatioComponent>(mSelectedEntity);

    if (ImGui::BeginCombo("###AspectRatioType",
                          hasViewportAspectRatio ? "Viewport ratio" : "Custom",
                          0)) {

      if (ImGui::Selectable("Viewport ratio")) {
        mEntityManager.getActiveEntityDatabase()
            .setComponent<liquid::AutoAspectRatioComponent>(mSelectedEntity,
                                                            {});
        mEntityManager.save(mSelectedEntity);
      }

      if (ImGui::Selectable("Custom")) {
        mEntityManager.getActiveEntityDatabase()
            .deleteComponent<liquid::AutoAspectRatioComponent>(mSelectedEntity);
        mEntityManager.save(mSelectedEntity);
      }

      ImGui::EndCombo();
    }

    if (!hasViewportAspectRatio) {
      ImGui::Text("Custom aspect ratio");
      if (ImGui::DragFloat("###CustomAspectRatio", &component.aspectRatio,
                           MinCustomAspectRatio, MinCustomAspectRatio,
                           MaxCustomAspectRatio, "%.2f")) {
      }

      if (ImGui::IsItemDeactivatedAfterEdit()) {
        mEntityManager.save(mSelectedEntity);
      }
    }

    if (!editorManager.isUsingCamera(mSelectedEntity) &&
        ImGui::Button("Set as active camera")) {
      editorManager.setActiveCamera(mSelectedEntity);
    }
  }
  widgets::Section::end();
}

void EntityPanel::renderTransform() {
  if (!mEntityManager.getActiveEntityDatabase()
           .hasComponent<liquid::LocalTransformComponent>(mSelectedEntity) ||
      !mEntityManager.getActiveEntityDatabase()
           .hasComponent<liquid::WorldTransformComponent>(mSelectedEntity)) {
    return;
  }

  if (widgets::Section::begin("Transform")) {
    auto &component =
        mEntityManager.getActiveEntityDatabase()
            .getComponent<liquid::LocalTransformComponent>(mSelectedEntity);
    auto &world =
        mEntityManager.getActiveEntityDatabase()
            .getComponent<liquid::WorldTransformComponent>(mSelectedEntity);

    ImGui::Text("Position");

    if (liquid::imgui::input("###InputTransformPosition",
                             component.localPosition)) {
      mEntityManager.save(mSelectedEntity);
    }

    ImGui::Text("Rotation");
    const auto &euler = glm::eulerAngles(component.localRotation);

    std::array<float, glm::vec3::length()> imguiRotation{euler.x, euler.y,
                                                         euler.z};
    if (ImGui::InputFloat3("###InputTransformRotation", imguiRotation.data())) {
      component.localRotation = glm::quat(glm::vec3(
          imguiRotation.at(0), imguiRotation.at(1), imguiRotation.at(2)));
    }

    if (ImGui::IsItemDeactivatedAfterEdit()) {
      mEntityManager.save(mSelectedEntity);
    }

    ImGui::Text("Scale");
    if (liquid::imgui::input("###InputTransformScale", component.localScale)) {
      mEntityManager.save(mSelectedEntity);
    }

    ImGui::Text("World Transform");
    if (ImGui::BeginTable("table-transformWorld", 4,
                          ImGuiTableFlags_Borders |
                              ImGuiTableColumnFlags_WidthStretch |
                              ImGuiTableFlags_RowBg)) {

      for (glm::mat4::length_type i = 0; i < 4; ++i) {
        liquid::imgui::renderRow(
            world.worldTransform[i].x, world.worldTransform[i].y,
            world.worldTransform[i].z, world.worldTransform[i].w);
      }

      ImGui::EndTable();
    }
  }
  widgets::Section::end();
}

void EntityPanel::renderMesh(liquid::AssetRegistry &assetRegistry) {
  if (mEntityManager.getActiveEntityDatabase()
          .hasComponent<liquid::MeshComponent>(mSelectedEntity)) {
    if (widgets::Section::begin("Mesh")) {
      auto handle = mEntityManager.getActiveEntityDatabase()
                        .getComponent<liquid::MeshComponent>(mSelectedEntity)
                        .handle;

      const auto &asset = assetRegistry.getMeshes().getAsset(handle);

      if (ImGui::BeginTable("table-mesh", 2,
                            ImGuiTableFlags_Borders |
                                ImGuiTableColumnFlags_WidthStretch |
                                ImGuiTableFlags_RowBg)) {

        liquid::imgui::renderRow("Name", asset.name);
        liquid::imgui::renderRow(
            "Geometries", static_cast<uint32_t>(asset.data.geometries.size()));
        ImGui::EndTable();
      }
    }
    widgets::Section::end();
  }

  if (mEntityManager.getActiveEntityDatabase()
          .hasComponent<liquid::SkinnedMeshComponent>(mSelectedEntity)) {
    if (widgets::Section::begin("Mesh")) {
      auto handle =
          mEntityManager.getActiveEntityDatabase()
              .getComponent<liquid::SkinnedMeshComponent>(mSelectedEntity)
              .handle;

      const auto &asset = assetRegistry.getSkinnedMeshes().getAsset(handle);

      if (ImGui::BeginTable("table-mesh", 2,
                            ImGuiTableFlags_Borders |
                                ImGuiTableColumnFlags_WidthStretch |
                                ImGuiTableFlags_RowBg)) {

        liquid::imgui::renderRow("Name", asset.name);
        liquid::imgui::renderRow(
            "Geometries", static_cast<uint32_t>(asset.data.geometries.size()));
        ImGui::EndTable();
      }
    }
    widgets::Section::end();
  }
}

void EntityPanel::renderSkeleton() {
  if (!mEntityManager.getActiveEntityDatabase()
           .hasComponent<liquid::SkeletonComponent>(mSelectedEntity)) {
    return;
  }

  if (widgets::Section::begin("Skeleton")) {
    bool showBones =
        mEntityManager.getActiveEntityDatabase()
            .hasComponent<liquid::SkeletonDebugComponent>(mSelectedEntity);

    if (ImGui::Checkbox("Show bones", &showBones)) {
      mEntityManager.toggleSkeletonDebugForEntity(mSelectedEntity);
    }
  }
  widgets::Section::end();
}

void EntityPanel::renderAnimation(liquid::AssetRegistry &assetRegistry) {
  if (!mEntityManager.getActiveEntityDatabase()
           .hasComponent<liquid::AnimatorComponent>(mSelectedEntity)) {
    return;
  }

  if (widgets::Section::begin("Animation")) {
    auto &component =
        mEntityManager.getActiveEntityDatabase()
            .getComponent<liquid::AnimatorComponent>(mSelectedEntity);

    const auto &animations = assetRegistry.getAnimations().getAssets();

    const auto &currentAnimation =
        animations.at(component.animations.at(component.currentAnimation));

    if (ImGui::BeginCombo("###SelectAnimation", currentAnimation.name.c_str(),
                          0)) {
      for (size_t i = 0; i < component.animations.size(); ++i) {
        bool selectable = component.currentAnimation == i;

        const auto &animationName =
            animations.at(component.animations.at(i)).name;

        if (ImGui::Selectable(animationName.c_str(), &selectable)) {
          component.currentAnimation = static_cast<uint32_t>(i);
        }
      }
      ImGui::EndCombo();
    }

    if (mEntityManager.isUsingSimulationDatabase()) {
      ImGui::Text("Time");

      float animationTime =
          component.normalizedTime * currentAnimation.data.time;
      if (ImGui::SliderFloat("###AnimationTime", &animationTime, 0.0f,
                             currentAnimation.data.time)) {
        component.normalizedTime = animationTime / currentAnimation.data.time;
      }

      ImGui::Checkbox("Loop", &component.loop);

      if (!component.playing) {
        if (ImGui::Button("Play")) {
          component.playing = true;
        }
      } else {
        if (ImGui::Button("Pause")) {
          component.playing = false;
        }
      }

      ImGui::SameLine();

      if (ImGui::Button("Reset")) {
        component.normalizedTime = 0.0f;
      }
    }
  }
  widgets::Section::end();
}

/**
 * @brief Get geometry name
 *
 * @param type Geometry type
 * @return String name for geometry
 */
static liquid::String getGeometryName(const liquid::PhysicsGeometryType &type) {
  switch (type) {
  case liquid::PhysicsGeometryType::Box:
    return "Box";
  case liquid::PhysicsGeometryType::Sphere:
    return "Sphere";
  case liquid::PhysicsGeometryType::Capsule:
    return "Capsule";
  case liquid::PhysicsGeometryType::Plane:
    return "Plane";
  default:
    return "Unknown";
  }
}

/**
 * @brief Get defaulty geometry from type
 *
 * @param type Geometry type
 * @return Default geometry parameters
 */
static liquid::PhysicsGeometryParams
getDefaultGeometryFromType(const liquid::PhysicsGeometryType &type) {
  switch (type) {
  case liquid::PhysicsGeometryType::Box:
  default:
    return liquid::PhysicsGeometryBox();
  case liquid::PhysicsGeometryType::Sphere:
    return liquid::PhysicsGeometrySphere();
  case liquid::PhysicsGeometryType::Capsule:
    return liquid::PhysicsGeometryCapsule();
  case liquid::PhysicsGeometryType::Plane:
    return liquid::PhysicsGeometryPlane();
  }
}

void EntityPanel::renderCollidable() {
  if (!mEntityManager.getActiveEntityDatabase()
           .hasComponent<liquid::CollidableComponent>(mSelectedEntity)) {
    return;
  }

  if (widgets::Section::begin("Collidable")) {
    std::array<liquid::PhysicsGeometryType, sizeof(liquid::PhysicsGeometryType)>
        types{
            liquid::PhysicsGeometryType::Box,
            liquid::PhysicsGeometryType::Sphere,
            liquid::PhysicsGeometryType::Capsule,
            liquid::PhysicsGeometryType::Plane,
        };

    auto &collidable =
        mEntityManager.getActiveEntityDatabase()
            .getComponent<liquid::CollidableComponent>(mSelectedEntity);

    if (ImGui::BeginCombo(
            "###SelectGeometryType",
            getGeometryName(collidable.geometryDesc.type).c_str())) {

      for (auto type : types) {
        if (type != collidable.geometryDesc.type &&
            ImGui::Selectable(getGeometryName(type).c_str())) {
          collidable.geometryDesc.type = type;
          collidable.geometryDesc.params = getDefaultGeometryFromType(type);
        }
      }
      ImGui::EndCombo();
    }

    if (collidable.geometryDesc.type == liquid::PhysicsGeometryType::Box) {
      auto &box =
          std::get<liquid::PhysicsGeometryBox>(collidable.geometryDesc.params);
      std::array<float, 3> extents{box.halfExtents.x, box.halfExtents.y,
                                   box.halfExtents.z};
      ImGui::Text("Half extents");
      if (ImGui::InputFloat3("###HalfExtents", extents.data())) {
        box.halfExtents.x = extents.at(0);
        box.halfExtents.y = extents.at(1);
        box.halfExtents.z = extents.at(2);
      }
    } else if (collidable.geometryDesc.type ==
               liquid::PhysicsGeometryType::Sphere) {
      auto &sphere = std::get<liquid::PhysicsGeometrySphere>(
          collidable.geometryDesc.params);
      ImGui::Text("Radius");
      ImGui::InputFloat("###Radius", &sphere.radius);
    } else if (collidable.geometryDesc.type ==
               liquid::PhysicsGeometryType::Capsule) {
      auto &capsule = std::get<liquid::PhysicsGeometryCapsule>(
          collidable.geometryDesc.params);
      ImGui::Text("Radius");
      ImGui::InputFloat("###Radius", &capsule.radius);

      ImGui::Text("Half height");
      ImGui::InputFloat("###HalfHeight", &capsule.halfHeight);
    }
  }
  widgets::Section::end();
}

void EntityPanel::renderRigidBody() {
  if (!mEntityManager.getActiveEntityDatabase()
           .hasComponent<liquid::RigidBodyComponent>(mSelectedEntity)) {
    return;
  }

  if (widgets::Section::begin("Rigid body")) {
    auto &rigidBody =
        mEntityManager.getActiveEntityDatabase()
            .getComponent<liquid::RigidBodyComponent>(mSelectedEntity);

    ImGui::Text("Mass");
    ImGui::InputFloat("###Mass", &rigidBody.dynamicDesc.mass);

    std::array<float, 3> inertia{rigidBody.dynamicDesc.inertia.x,
                                 rigidBody.dynamicDesc.inertia.y,
                                 rigidBody.dynamicDesc.inertia.z};

    if (ImGui::InputFloat3("###Inertia", inertia.data())) {
      rigidBody.dynamicDesc.inertia.x = inertia.at(0);
      rigidBody.dynamicDesc.inertia.y = inertia.at(1);
      rigidBody.dynamicDesc.inertia.z = inertia.at(2);
    }

    if (rigidBody.actor) {
      rigidBody.actor->getLinearVelocity();
      if (ImGui::BeginTable("TableRigidBodyDetails", 2,
                            ImGuiTableFlags_Borders |
                                ImGuiTableColumnFlags_WidthStretch |
                                ImGuiTableFlags_RowBg)) {
        auto *actor = rigidBody.actor;

        const auto &pose = actor->getGlobalPose();
        const auto &cmass = actor->getCMassLocalPose();
        const auto &invInertia = actor->getMassSpaceInvInertiaTensor();
        const auto &linearVelocity = actor->getLinearVelocity();
        const auto &angularVelocity = actor->getAngularVelocity();

        liquid::imgui::renderRow("Pose position",
                                 glm::vec3(pose.p.x, pose.p.y, pose.p.y));
        liquid::imgui::renderRow(
            "Pose rotation",
            glm::quat(cmass.q.w, cmass.q.x, cmass.q.y, cmass.q.z));
        liquid::imgui::renderRow("CMass position",
                                 glm::vec3(cmass.p.x, cmass.p.y, cmass.p.y));
        liquid::imgui::renderRow(
            "CMass rotation",
            glm::quat(cmass.q.w, cmass.q.x, cmass.q.y, cmass.q.z));
        liquid::imgui::renderRow(
            "Inverse inertia tensor position",
            glm::vec3(invInertia.x, invInertia.y, invInertia.y));
        liquid::imgui::renderRow("Linear damping",
                                 static_cast<float>(actor->getLinearDamping()));
        liquid::imgui::renderRow(
            "Angular damping", static_cast<float>(actor->getAngularDamping()));
        liquid::imgui::renderRow(
            "Linear velocity",
            glm::vec3(linearVelocity.x, linearVelocity.y, linearVelocity.z));
        liquid::imgui::renderRow(
            "Angular velocity",
            glm::vec3(angularVelocity.x, angularVelocity.y, angularVelocity.z));

        ImGui::EndTable();
      }
    }
  }
  widgets::Section::end();
}

void EntityPanel::renderText(liquid::AssetRegistry &assetRegistry) {
  if (!mEntityManager.getActiveEntityDatabase()
           .hasComponent<liquid::TextComponent>(mSelectedEntity)) {
    return;
  }

  if (widgets::Section::begin("Text")) {
    auto &text = mEntityManager.getActiveEntityDatabase()
                     .getComponent<liquid::TextComponent>(mSelectedEntity);

    const auto &fonts = assetRegistry.getFonts().getAssets();

    static constexpr float ContentInputHeight = 100.0f;

    ImGui::Text("Content");
    if (ImguiMultilineInputText(
            "###InputContent", text.text,
            ImVec2(ImGui::GetWindowWidth(), ContentInputHeight), 0)) {
      mEntityManager.save(mSelectedEntity);
    }

    ImGui::Text("Line height");
    if (liquid::imgui::input("###InputLineHeight", text.lineHeight)) {
      mEntityManager.save(mSelectedEntity);
    }

    ImGui::Text("Select font");
    if (ImGui::BeginCombo("###SelectFont", fonts.at(text.font).name.c_str(),
                          0)) {
      for (const auto &[handle, data] : fonts) {
        bool selectable = handle == text.font;

        const auto &fontName = data.name;

        if (ImGui::Selectable(fontName.c_str(), &selectable)) {
          text.font = handle;
          mEntityManager.save(mSelectedEntity);
        }
      }
      ImGui::EndCombo();
    }
  }

  widgets::Section::end();
}

void EntityPanel::renderAudio(liquid::AssetRegistry &assetRegistry) {
  if (!mEntityManager.getActiveEntityDatabase()
           .hasComponent<liquid::AudioSourceComponent>(mSelectedEntity)) {
    return;
  }

  if (widgets::Section::begin("Audio")) {
    const auto &audio =
        mEntityManager.getActiveEntityDatabase()
            .getComponent<liquid::AudioSourceComponent>(mSelectedEntity);

    const auto &asset = assetRegistry.getAudios().getAsset(audio.source);

    ImGui::Text("Name: %s", asset.name.c_str());
  }

  widgets::Section::end();
}

void EntityPanel::renderScripting(liquid::AssetRegistry &assetRegistry) {
  if (!mEntityManager.getActiveEntityDatabase()
           .hasComponent<liquid::ScriptingComponent>(mSelectedEntity)) {
    return;
  }

  if (widgets::Section::begin("Scripts")) {
    const auto &scripting =
        mEntityManager.getActiveEntityDatabase()
            .getComponent<liquid::ScriptingComponent>(mSelectedEntity);

    const auto &asset =
        assetRegistry.getLuaScripts().getAsset(scripting.handle);

    ImGui::Text("Name: %s", asset.name.c_str());
  }

  widgets::Section::end();
}

void EntityPanel::renderAddComponent() {
  if (!mEntityManager.getActiveEntityDatabase().hasEntity(mSelectedEntity)) {
    return;
  }

  bool hasAllComponents =
      mEntityManager.getActiveEntityDatabase()
          .hasComponent<liquid::LocalTransformComponent>(mSelectedEntity) &&
      mEntityManager.getActiveEntityDatabase()
          .hasComponent<liquid::RigidBodyComponent>(mSelectedEntity) &&
      mEntityManager.getActiveEntityDatabase()
          .hasComponent<liquid::CollidableComponent>(mSelectedEntity) &&
      mEntityManager.getActiveEntityDatabase()
          .hasComponent<liquid::DirectionalLightComponent>(mSelectedEntity) &&
      mEntityManager.getActiveEntityDatabase()
          .hasComponent<liquid::PerspectiveLensComponent>(mSelectedEntity);

  if (hasAllComponents)
    return;

  if (ImGui::Button("Add component")) {
    ImGui::OpenPopup("AddComponentPopup");
  }

  if (ImGui::BeginPopup("AddComponentPopup")) {
    if (!mEntityManager.getActiveEntityDatabase()
             .hasComponent<liquid::LocalTransformComponent>(mSelectedEntity) &&
        ImGui::Selectable("Transform")) {
      mEntityManager.getActiveEntityDatabase()
          .setComponent<liquid::LocalTransformComponent>(mSelectedEntity, {});
      mEntityManager.save(mSelectedEntity);
    }

    if (!mEntityManager.getActiveEntityDatabase()
             .hasComponent<liquid::RigidBodyComponent>(mSelectedEntity) &&
        ImGui::Selectable("Rigid body")) {
      mEntityManager.getActiveEntityDatabase()
          .setComponent<liquid::RigidBodyComponent>(mSelectedEntity, {});
      mEntityManager.save(mSelectedEntity);
    }

    if (!mEntityManager.getActiveEntityDatabase()
             .hasComponent<liquid::CollidableComponent>(mSelectedEntity) &&
        ImGui::Selectable("Collidable")) {
      static constexpr glm::vec3 DefaultValue(0.5f);

      mEntityManager.getActiveEntityDatabase()
          .setComponent<liquid::CollidableComponent>(
              mSelectedEntity, {liquid::PhysicsGeometryType::Box,
                                liquid::PhysicsGeometryBox{DefaultValue}});
      mEntityManager.save(mSelectedEntity);
    }

    if (!mEntityManager.getActiveEntityDatabase()
             .hasComponent<liquid::DirectionalLightComponent>(
                 mSelectedEntity) &&
        ImGui::Selectable("Light")) {
      mEntityManager.getActiveEntityDatabase()
          .setComponent<liquid::DirectionalLightComponent>(mSelectedEntity, {});
      mEntityManager.save(mSelectedEntity);
    }

    if (!mEntityManager.getActiveEntityDatabase()
             .hasComponent<liquid::PerspectiveLensComponent>(mSelectedEntity) &&
        ImGui::Selectable("Camera")) {
      mEntityManager.setCamera(mSelectedEntity, {}, true);
      mEntityManager.save(mSelectedEntity);
    }

    ImGui::EndPopup();
  }
}

void EntityPanel::handleDragAndDrop(liquid::Renderer &renderer,
                                    liquid::AssetRegistry &assetRegistry) {
  static constexpr float HALF = 0.5f;
  auto width = ImGui::GetWindowContentRegionWidth();

  ImGui::Button("Drag asset here", ImVec2(width, width * HALF));

  if (ImGui::BeginDragDropTarget()) {
    if (auto *payload = ImGui::AcceptDragDropPayload(
            liquid::getAssetTypeString(liquid::AssetType::Mesh).c_str())) {
      auto asset = *static_cast<liquid::MeshAssetHandle *>(payload->Data);

      mEntityManager.setMesh(mSelectedEntity, asset);
      mEntityManager.save(mSelectedEntity);
    }

    if (auto *payload = ImGui::AcceptDragDropPayload(
            liquid::getAssetTypeString(liquid::AssetType::SkinnedMesh)
                .c_str())) {
      auto asset =
          *static_cast<liquid::SkinnedMeshAssetHandle *>(payload->Data);
      mEntityManager.setSkinnedMesh(mSelectedEntity, asset);
      mEntityManager.save(mSelectedEntity);
    }

    if (auto *payload = ImGui::AcceptDragDropPayload(
            liquid::getAssetTypeString(liquid::AssetType::Skeleton).c_str())) {
      auto asset = *static_cast<liquid::SkeletonAssetHandle *>(payload->Data);

      mEntityManager.setSkeletonForEntity(mSelectedEntity, asset);
      mEntityManager.save(mSelectedEntity);
    }

    if (auto *payload = ImGui::AcceptDragDropPayload(
            liquid::getAssetTypeString(liquid::AssetType::Audio).c_str())) {
      auto asset = *static_cast<liquid::AudioAssetHandle *>(payload->Data);

      mEntityManager.setAudio(mSelectedEntity, asset);
      mEntityManager.save(mSelectedEntity);
    }

    if (auto *payload = ImGui::AcceptDragDropPayload(
            liquid::getAssetTypeString(liquid::AssetType::LuaScript).c_str())) {
      auto asset = *static_cast<liquid::LuaScriptAssetHandle *>(payload->Data);

      mEntityManager.setScript(mSelectedEntity, asset);
      mEntityManager.save(mSelectedEntity);
    }

    ImGui::EndDragDropTarget();
  }
}

} // namespace liquidator
