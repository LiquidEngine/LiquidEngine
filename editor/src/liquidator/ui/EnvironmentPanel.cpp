#include "liquid/core/Base.h"
#include "EnvironmentPanel.h"

#include "liquid/imgui/ImguiUtils.h"
#include "FontAwesome.h"
#include "Widgets.h"

#include "liquidator/actions/SceneActions.h"

namespace liquid::editor {

static String getSkyboxTypeLabel(Scene &scene) {
  if (!scene.entityDatabase.has<EnvironmentSkybox>(scene.environment)) {
    return "None";
  }

  auto type =
      scene.entityDatabase.get<EnvironmentSkybox>(scene.environment).type;

  if (type == EnvironmentSkyboxType::Color) {
    return "Color";
  }

  if (type == EnvironmentSkyboxType::Texture) {
    return "Texture";
  }

  return "None";
}

static void dndEnvironmentAsset(widgets::Section &section,
                                ActionExecutor &actionExecutor) {
  static constexpr float DropBorderWidth = 3.5f;
  auto &g = *ImGui::GetCurrentContext();

  ImVec2 dropMin(section.getClipRect().Min.x + DropBorderWidth,
                 g.LastItemData.Rect.Min.y + DropBorderWidth);
  ImVec2 dropMax(section.getClipRect().Max.x - DropBorderWidth,
                 g.LastItemData.Rect.Max.y - DropBorderWidth);
  if (ImGui::BeginDragDropTargetCustom(ImRect(dropMin, dropMax),
                                       g.LastItemData.ID)) {
    if (auto *payload = ImGui::AcceptDragDropPayload(
            getAssetTypeString(AssetType::Environment).c_str())) {
      auto asset = *static_cast<EnvironmentAssetHandle *>(payload->Data);
      actionExecutor.execute(std::make_unique<SceneSetSkyboxTexture>(asset));
    }
  }
}

void EnvironmentPanel::render(WorkspaceState &state,
                              ActionExecutor &actionExecutor) {
  auto &scene = state.mode == WorkspaceMode::Simulation ? state.simulationScene
                                                        : state.scene;
  if (auto _ = widgets::Window("Environment")) {
    renderSkyboxSection(scene, state.assetRegistry, actionExecutor);
    renderLightingSection(scene, state.assetRegistry, actionExecutor);
  }
}

void EnvironmentPanel::renderSkyboxSection(Scene &scene,
                                           AssetRegistry &assetRegistry,
                                           ActionExecutor &actionExecutor) {
  auto &textures = assetRegistry.getTextures();
  auto &environments = assetRegistry.getEnvironments();

  if (auto section = widgets::Section("Skybox")) {
    float width = section.getClipRect().GetWidth();
    const float height = width * 0.5f;

    ImGui::Text("Type");
    if (ImGui::BeginCombo("###SkyboxType", getSkyboxTypeLabel(scene).c_str())) {
      if (ImGui::Selectable("None")) {
        actionExecutor.execute(std::make_unique<SceneRemoveSkybox>());
      } else if (ImGui::Selectable("Color")) {
        actionExecutor.execute(std::make_unique<SceneSetSkyboxColor>(
            glm::vec4{0.0f, 0.0f, 0.0f, 1.0f}));
      } else if (ImGui::Selectable("Texture")) {
        actionExecutor.execute(std::make_unique<SceneSetSkyboxTexture>(
            EnvironmentAssetHandle::Invalid));
      }

      ImGui::EndCombo();
    }

    if (!scene.entityDatabase.has<EnvironmentSkybox>(scene.environment)) {
      return;
    }

    const auto &skybox =
        scene.entityDatabase.get<EnvironmentSkybox>(scene.environment);

    if (skybox.type == EnvironmentSkyboxType::Color) {
      auto &color =
          scene.entityDatabase.get<EnvironmentSkybox>(scene.environment).color;

      widgets::InputColor("Color", color);

      if (ImGui::IsItemDeactivatedAfterEdit()) {
        actionExecutor.execute(std::make_unique<SceneSetSkyboxColor>(color));
      }
    } else if (skybox.type == EnvironmentSkyboxType::Texture) {
      if (environments.hasAsset(skybox.texture)) {
        const auto &envAsset = environments.getAsset(skybox.texture);

        imgui::image(envAsset.preview, ImVec2(width, height), ImVec2(0, 0),
                     ImVec2(1, 1), ImGui::GetID("environment-texture-drop"));

        dndEnvironmentAsset(section, actionExecutor);

        if (ImGui::Button(fa::Times)) {
          actionExecutor.execute(std::make_unique<SceneSetSkyboxTexture>(
              EnvironmentAssetHandle::Invalid));
        }

      } else {
        ImGui::Button("Drag environment asset here", ImVec2(width, height));
        dndEnvironmentAsset(section, actionExecutor);
      }
    }
  }
}

void EnvironmentPanel::renderLightingSection(Scene &scene,
                                             AssetRegistry &assetRegistry,
                                             ActionExecutor &actionExecutor) {
  auto &textures = assetRegistry.getTextures();
  auto &environments = assetRegistry.getEnvironments();

  if (auto _ = widgets::Section("Lighting")) {
    String sourceName = "None";
    if (scene.entityDatabase.has<EnvironmentLightingSkyboxSource>(
            scene.environment)) {
      sourceName = "Skybox";
    }

    ImGui::Text("Source");
    if (ImGui::BeginCombo("###LightingSource", sourceName.c_str())) {
      if (ImGui::Selectable("None")) {
        actionExecutor.execute(std::make_unique<SceneRemoveLighting>());
      }

      if (ImGui::Selectable("Use skybox")) {
        actionExecutor.execute(
            std::make_unique<SceneSetSkyboxLightingSource>());
      }

      ImGui::EndCombo();
    }
  }
}

} // namespace liquid::editor
