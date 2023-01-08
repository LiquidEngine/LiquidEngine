#include "liquid/core/Base.h"
#include "liquid/renderer/Renderer.h"
#include "liquid/renderer/Presenter.h"
#include "liquid/asset/AssetRegistry.h"
#include "liquid/loop/MainLoop.h"
#include "liquid/profiler/FPSCounter.h"
#include "liquid/profiler/ImguiDebugLayer.h"
#include "liquid/imgui/ImguiUtils.h"

#include "liquidator/editor-scene/EditorCamera.h"
#include "liquidator/ui/Theme.h"
#include "liquidator/ui/FontAwesome.h"
#include "liquidator/ui/Widgets.h"
#include "liquidator/ui/StyleStack.h"

#include "ProjectSelectorScreen.h"

namespace liquidator {

ProjectSelectorScreen::ProjectSelectorScreen(liquid::Window &window,
                                             liquid::EventSystem &eventSystem,
                                             liquid::rhi::RenderDevice *device)
    : mWindow(window), mEventSystem(eventSystem), mDevice(device) {}

std::optional<Project> ProjectSelectorScreen::start() {
  liquid::EntityDatabase entityDatabase;
  liquid::AssetRegistry assetRegistry;
  liquid::ShaderLibrary shaderLibrary;
  liquid::RenderGraphEvaluator graphEvaluator(mDevice);
  liquid::RenderStorage renderStorage(mDevice);

  liquid::ImguiRenderer imguiRenderer(mWindow, shaderLibrary, renderStorage,
                                      mDevice);
  liquid::Presenter presenter(shaderLibrary, mDevice);

  liquidator::ProjectManager projectManager;

  liquid::FPSCounter fpsCounter;
  liquid::MainLoop mainLoop(mWindow, fpsCounter);
  liquidator::EditorCamera editorCamera(entityDatabase, mEventSystem, mWindow);
  std::optional<liquidator::Project> project;

  presenter.updateFramebuffers(mDevice->getSwapchain());

  editorCamera.reset();

  Theme::apply();

  imguiRenderer.setClearColor(Theme::getColor(ThemeColor::BackgroundColor));
  imguiRenderer.buildFonts();

  liquid::RenderGraph graph("Main");
  auto imguiPassData = imguiRenderer.attach(graph);

  graph.setFramebufferExtent(mWindow.getFramebufferSize());

  auto resizeHandler = mWindow.addResizeHandler(
      [&graph, this, &presenter](auto width, auto height) {
        graph.setFramebufferExtent({width, height});
      });

  mainLoop.setUpdateFn([&project, this](float dt) {
    mEventSystem.poll();
    return !project.has_value();
  });

  liquid::ImguiDebugLayer debugLayer(mDevice->getDeviceInformation(),
                                     mDevice->getDeviceStats(), fpsCounter);

  mainLoop.setRenderFn([&imguiRenderer, &graphEvaluator, &editorCamera, &graph,
                        &imguiPassData, &project, &projectManager,
                        &entityDatabase, &presenter, &debugLayer,
                        this]() mutable {
    auto &imgui = imguiRenderer;

    imgui.beginRendering();

    ImGui::BeginMainMenuBar();
    debugLayer.renderMenu();
    ImGui::EndMainMenuBar();
    debugLayer.render();

    static constexpr ImVec2 CenterWindowPivot(0.5f, 0.5f);
    static constexpr float ActionButtonWidth = 240.0f;
    static constexpr float ActionButtonHeight = 40.0f;
    static constexpr float WindowPadding = 20.0f;
    static constexpr ImVec2 ActionButtonSize{ActionButtonWidth,
                                             ActionButtonHeight};

    const auto &fbSize = glm::vec2(mWindow.getFramebufferSize());
    const auto actionBarPos =
        ImVec2(fbSize.x - ActionButtonWidth - WindowPadding,
               fbSize.y * 0.5f - WindowPadding);

    ImGui::SetNextWindowPos(actionBarPos, 0, CenterWindowPivot);

    if (ImGui::Begin("Liquidator", nullptr,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoTitleBar)) {
      StyleStack styles;
      styles.pushStyle(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));

      static const auto CreateProjectLabel =
          liquid::String(fa::FolderPlus) + "  Create project";
      if (ImGui::Button(CreateProjectLabel.c_str(), ActionButtonSize)) {
        if (projectManager.createProjectInPath()) {
          project = projectManager.getProject();
        }
      }

      static const auto OpenProjectLabel =
          liquid::String(fa::FolderOpen) + "  Open project";
      if (ImGui::Button(OpenProjectLabel.c_str(), ActionButtonSize)) {
        if (projectManager.openProjectInPath()) {
          project = projectManager.getProject();
        }
      }
    }
    ImGui::End();

    imgui.endRendering();

    const auto &renderFrame = mDevice->beginFrame();

    if (renderFrame.frameIndex < std::numeric_limits<uint32_t>::max()) {
      imgui.updateFrameData(renderFrame.frameIndex);
      graph.compile(mDevice);
      graphEvaluator.build(graph);
      graphEvaluator.execute(renderFrame.commandList, graph,
                             renderFrame.frameIndex);

      presenter.present(renderFrame.commandList, imguiPassData.imguiColor,
                        renderFrame.swapchainImageIndex);
      mDevice->endFrame(renderFrame);
    } else {
      presenter.updateFramebuffers(mDevice->getSwapchain());
    }
  });

  mainLoop.run();

  mWindow.removeResizeHandler(resizeHandler);
  mDevice->waitForIdle();

  return project;
}

} // namespace liquidator
