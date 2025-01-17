#include "quoll/core/Base.h"
#include "quoll/core/Engine.h"
#include "quoll/asset/FileTracker.h"
#include "quoll/entity/EntityDatabase.h"
#include "quoll/imgui/ImguiRenderer.h"
#include "quoll/imgui/ImguiUtils.h"
#include "quoll/logger/StreamTransport.h"
#include "quoll/loop/MainEngineModules.h"
#include "quoll/loop/MainLoop.h"
#include "quoll/profiler/FPSCounter.h"
#include "quoll/profiler/ImguiDebugLayer.h"
#include "quoll/profiler/MetricsCollector.h"
#include "quoll/profiler/PerformanceDebugPanel.h"
#include "quoll/renderer/Presenter.h"
#include "quoll/renderer/RenderStorage.h"
#include "quoll/renderer/Renderer.h"
#include "quoll/renderer/RendererAssetRegistry.h"
#include "quoll/renderer/SceneRenderer.h"
#include "quoll/renderer/StandardPushConstants.h"
#include "quoll/ui/UICanvasUpdater.h"
#include "quoll/editor/asset/AssetManager.h"
#include "quoll/editor/core/LogMemoryStorage.h"
#include "quoll/editor/scene/SceneEditorWorkspace.h"
#include "quoll/editor/scene/core/EditorCamera.h"
#include "quoll/editor/ui/AssetLoadStatusDialog.h"
#include "quoll/editor/ui/FontAwesome.h"
#include "quoll/editor/ui/LogViewer.h"
#include "quoll/editor/ui/MainMenuBar.h"
#include "quoll/editor/ui/StyleStack.h"
#include "quoll/editor/ui/Theme.h"
#include "quoll/editor/ui/Widgets.h"
#include "quoll/editor/workspace/WorkspaceManager.h"
#include "quoll/editor/workspace/WorkspaceTabs.h"
#include "EditorWindow.h"
#include "ImGuizmo.h"

namespace quoll::editor {

EditorWindow::EditorWindow(Window &window, InputDeviceManager &deviceManager,
                           rhi::RenderDevice *device)
    : mDeviceManager(deviceManager), mWindow(window), mDevice(device) {}

void EditorWindow::start(const Project &rawProject) {
  auto project = rawProject;

  LogMemoryStorage userLogStorage;
  Engine::getUserLogger().setTransport(userLogStorage.createTransport());

  FPSCounter fpsCounter;

  quoll::MetricsCollector metricsCollector;

  RenderStorage renderStorage(mDevice, metricsCollector);
  RendererAssetRegistry rendererAssetRegistry(renderStorage);

  quoll::RendererOptions initialOptions{};
  initialOptions.framebufferSize = mWindow.getFramebufferSize();
  Renderer renderer(renderStorage, initialOptions);

  AssetManager assetManager(project.assetsPath, project.assetsCachePath,
                            renderStorage, true, true);

  ImguiRenderer imguiRenderer(mWindow, renderStorage, rendererAssetRegistry);

  Presenter presenter(renderStorage);

  presenter.updateFramebuffers(mDevice->getSwapchain());

  auto res = assetManager.syncAssets();
  AssetLoadStatusDialog loadStatusDialog("Loaded with warnings");

  if (res.hasWarnings()) {
    for (const auto &warning : res.warnings()) {
      Engine::getUserLogger().warning() << warning;
    }

    loadStatusDialog.setMessages(res.warnings());
    loadStatusDialog.show();
  }

  auto sceneUuid = assetManager.findRootAssetUuid(project.assetsPath /
                                                  "scenes" / "main.scene");
  project.startingScene = sceneUuid;

  auto sceneAsset = assetManager.getCache().request<SceneAsset>(sceneUuid);
  QuollAssert(sceneAsset, "Scene asset does not exist");
  if (!sceneAsset) {
    return;
  }

  Theme::apply();
  imguiRenderer.setClearColor(Theme::getClearColor());
  imguiRenderer.buildFonts();

  FileTracker tracker(project.assetsPath);
  tracker.trackForChanges();

  EditorCamera editorCamera(mWindow);

  MainLoop mainLoop(mWindow, fpsCounter);

  IconRegistry::loadIcons(renderStorage,
                          std::filesystem::current_path() / "assets" / "icons");

  SceneRenderer sceneRenderer(assetManager.getAssetRegistry(), renderStorage,
                              rendererAssetRegistry);
  EditorRenderer editorRenderer(assetManager.getAssetRegistry(), renderStorage,
                                rendererAssetRegistry);

  renderer.setGraphBuilder([&](auto &graph, const auto &options) {
    auto scenePassGroup = sceneRenderer.attach(graph, options);
    auto imguiPassGroup = imguiRenderer.attach(graph, options);
    imguiPassGroup.pass.read(scenePassGroup.finalColor);
    editorRenderer.attach(graph, scenePassGroup, options);
    sceneRenderer.attachText(graph, scenePassGroup);

    return RendererTextures{imguiPassGroup.imguiColor,
                            scenePassGroup.finalColor};
  });

  MousePickingGraph mousePicking(sceneRenderer.getFrameData(), renderStorage,
                                 rendererAssetRegistry);

  mousePicking.setFramebufferSize(mWindow.getFramebufferSize());

  mWindow.getSignals().onFramebufferResize().connect(
      [&](auto width, auto height) {
        renderer.setFramebufferSize({width, height});
        mousePicking.setFramebufferSize({width, height});
        presenter.enqueueFramebufferUpdate();
      });

  MainEngineModules engineModules(mDeviceManager, mWindow,
                                  assetManager.getCache());

  debug::PerformanceDebugPanel performanceDebugPanel(mDevice, metricsCollector,
                                                     fpsCounter);

  ImguiDebugLayer debugLayer(
      {renderer.getDebugPanel(), &performanceDebugPanel,
       assetManager.getCache().getDebugPanel(),
       engineModules.getPhysicsSystem().getDebugPanel()});

  // Workspace manager
  WorkspaceManager workspaceManager;
  workspaceManager.add(new SceneEditorWorkspace(
      project, assetManager, sceneAsset,
      project.assetsPath / "scenes" / "main.scene", renderer, sceneRenderer,
      editorRenderer, mousePicking, engineModules, editorCamera,
      workspaceManager));

  mWindow.getSignals().onKeyPress().connect([&](const auto &data) {
    workspaceManager.getCurrentWorkspace()->processShortcuts(data.key,
                                                             data.mods);
  });

  mWindow.getSignals().onFocus().connect([&tracker, &loadStatusDialog,
                                          &assetManager, &renderer,
                                          &workspaceManager](bool focused) {
    if (!focused)
      return;

    const auto &changes = tracker.trackForChanges();
    std::vector<String> messages;
    for (auto &change : changes) {
      auto res = assetManager.loadSourceIfChanged(change.path);
      if (!res) {
        messages.push_back(res.error());

        Engine::getUserLogger().error() << res.error().message();
      } else {
        messages.insert(messages.end(), res.warnings().begin(),
                        res.warnings().end());

        for (const auto &warning : res.warnings()) {
          Engine::getUserLogger().warning() << warning;
        }
      }
    }

    if (changes.size() > 0) {
      workspaceManager.getCurrentWorkspace()->reload();
    }

    if (!messages.empty()) {
      loadStatusDialog.setMessages(messages);
      loadStatusDialog.show();
    }
  });

  mainLoop.setPrepareFn([&workspaceManager]() {
    workspaceManager.getCurrentWorkspace()->prepare();
  });

  mainLoop.setFixedUpdateFn([&workspaceManager](f32 dt) {
    workspaceManager.getCurrentWorkspace()->fixedUpdate(dt);
  });

  mainLoop.setUpdateFn([&workspaceManager](f32 dt) {
    workspaceManager.getCurrentWorkspace()->update(dt);
  });

  LogViewer logViewer;
  mainLoop.setRenderFn([&]() {
    if (presenter.requiresFramebufferUpdate()) {
      mDevice->recreateSwapchain();
      presenter.updateFramebuffers(mDevice->getSwapchain());
      return;
    }

    renderer.rebuildIfSettingsChanged();

    // TODO: Why is -2.0f needed here
    static const f32 IconSize = ImGui::GetFrameHeight() - 2.0f;

    imguiRenderer.beginRendering();
    ImGuizmo::BeginFrame();

    auto *workspace = workspaceManager.getCurrentWorkspace();
    workspace->render();

    if (auto _ = MainMenuBar()) {
      debugLayer.renderMenu();

      static constexpr f32 SpaceBetweenMainMenuAndTabBar = 20.0f;
      ImGui::Dummy(ImVec2(SpaceBetweenMainMenuAndTabBar, 0.0));
      WorkspaceTabs::render(workspaceManager);
    }

    debugLayer.render();

    logViewer.render(userLogStorage);

    StatusBar::render(editorCamera);

    loadStatusDialog.render();

    imguiRenderer.endRendering();

    const auto &renderFrame = mDevice->beginFrame();

    if (renderFrame.frameIndex < std::numeric_limits<u32>::max()) {
      imguiRenderer.updateFrameData(renderFrame.frameIndex);
      workspace->updateFrameData(renderFrame.commandList,
                                 renderFrame.frameIndex);

      renderer.execute(renderFrame.commandList, renderFrame.frameIndex);

      presenter.present(renderFrame.commandList, renderer.getFinalTexture(),
                        renderFrame.swapchainImageIndex);

      mDevice->endFrame(renderFrame);

      metricsCollector.getResults(mDevice);
    } else {
      presenter.updateFramebuffers(mDevice->getSwapchain());
    }
  });

  mainLoop.setStatsFn([this, &metricsCollector](u32 frames) {
    metricsCollector.markForCollection();
  });

  mWindow.maximize();
  mainLoop.run();
  Engine::resetLoggers();

  mDevice->waitForIdle();
  assetManager.getCache().waitForIdle();
}

} // namespace quoll::editor
