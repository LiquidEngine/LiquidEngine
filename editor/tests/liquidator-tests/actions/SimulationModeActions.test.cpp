#include "liquid/core/Base.h"
#include "liquidator/actions/SimulationModeActions.h"

#include "liquidator-tests/Testing.h"

using WM = liquid::editor::WorkspaceMode;

class SimulationModeActionsTestBase : public ::testing::Test {
public:
  liquid::AssetRegistry registry;
  liquid::editor::WorkspaceState state{{}, registry};
};

using StartSimulationModeActionTest = SimulationModeActionsTestBase;

TEST_F(StartSimulationModeActionTest, ExecutorSetsWorkspaceModeToSimulation) {
  state.mode = WM::Edit;

  liquid::editor::StartSimulationModeAction action;
  action.onExecute(state);
  EXPECT_EQ(state.mode, WM::Simulation);
}

TEST_F(StartSimulationModeActionTest,
       ExecutorDuplicatesSceneToSimulationScene) {
  state.mode = WM::Edit;
  state.scene.environment = liquid::Entity{12};
  state.scene.activeCamera = liquid::Entity{14};
  state.scene.dummyCamera = liquid::Entity{15};
  auto entity = state.scene.entityDatabase.create();

  EXPECT_EQ(state.simulationScene.environment, liquid::Entity::Null);
  EXPECT_EQ(state.simulationScene.activeCamera, liquid::Entity::Null);
  EXPECT_EQ(state.simulationScene.dummyCamera, liquid::Entity::Null);
  EXPECT_FALSE(state.simulationScene.entityDatabase.exists(entity));

  liquid::editor::StartSimulationModeAction action;
  action.onExecute(state);

  EXPECT_EQ(state.simulationScene.environment, state.scene.environment);
  EXPECT_EQ(state.simulationScene.activeCamera, state.scene.activeCamera);
  EXPECT_EQ(state.simulationScene.dummyCamera, state.scene.dummyCamera);
  EXPECT_TRUE(state.simulationScene.entityDatabase.exists(entity));
}

TEST_F(StartSimulationModeActionTest,
       PredicateReturnsTrueIfWorkspaceModeIsEdit) {
  state.mode = WM::Edit;

  liquid::editor::StartSimulationModeAction action;
  EXPECT_TRUE(action.predicate(state));
}

TEST_F(StartSimulationModeActionTest,
       PredicateReturnsFalseIfWorkspaceModeIsSimulation) {
  state.mode = WM::Simulation;

  liquid::editor::StartSimulationModeAction action;
  EXPECT_FALSE(action.predicate(state));
}

using StopSimulationModeActionTest = SimulationModeActionsTestBase;

TEST_F(StopSimulationModeActionTest, ExecutorSetsWorkspaceModeToSimulation) {
  state.mode = WM::Simulation;

  liquid::editor::StopSimulationModeAction action;
  action.onExecute(state);
  EXPECT_EQ(state.mode, WM::Edit);
}

TEST_F(StopSimulationModeActionTest,
       PredicateReturnsTrueIfWorkspaceModeIsSimulation) {
  state.mode = WM::Simulation;

  liquid::editor::StopSimulationModeAction action;
  EXPECT_TRUE(action.predicate(state));
}

TEST_F(StopSimulationModeActionTest,
       PredicateReturnsFalseIfWorkspaceModeIsSimulation) {
  state.mode = WM::Edit;

  liquid::editor::StopSimulationModeAction action;
  EXPECT_FALSE(action.predicate(state));
}
