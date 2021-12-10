#include "core/Base.h"
#include "core/EngineGlobals.h"
#include "RenderGraph.h"

namespace liquid {

RenderGraph::~RenderGraph() {
  for (auto &x : passes) {
    delete x;
  }
}

void RenderGraph::addPass(RenderGraphPassInterface *pass) {
  passes.push_back(pass);
}

std::vector<RenderGraphPassInterface *> RenderGraph::compile() {
  std::vector<RenderGraphPassInterface *> tempPasses = passes;

  // Validate pass names
  std::set<String> uniquePasses;
  for (auto &x : tempPasses) {
    uniquePasses.insert(x->getName());
  }
  LIQUID_ASSERT(
      uniquePasses.size() == tempPasses.size(),
      "Some of the names in the render graph are used in more than one pass");

  if (uniquePasses.size() != tempPasses.size()) {
    engineLogger.log(Logger::Warning) << "Some of the names in the render "
                                         "graph are used in more than one pass";
  }

  // Create all resources
  for (auto &x : tempPasses) {
    x->build(RenderGraphBuilder(*this, x));
  }

  // Validate that all input attachments exist
  for (auto &x : tempPasses) {
    for (auto &input : x->getInputs()) {
      LIQUID_ASSERT(hasAttachment(input),
                    "An input in " + x->getName()
                        << " does not point to any resource");
      if (!hasAttachment(input)) {
        engineLogger.log(Logger::Fatal) << "An input in " << x->getName()
                                        << "does not point to any resource.";
      }
    }
  }

  // Delete lonely nodes
  for (auto it = tempPasses.begin(); it != tempPasses.end(); ++it) {
    if ((*it)->getInputs().size() == 0 && (*it)->getOutputs().size() == 0) {
      LOG_DEBUG("Lonely pass removed from compilation: " << (*it)->getName());
      tempPasses.erase(it);
      it = tempPasses.begin();
    }
  }

  // Cache reads so we can easily access them
  // for creating the adjacency lsit
  std::unordered_map<GraphResourceId, std::vector<size_t>> passReads;
  for (size_t i = 0; i < tempPasses.size(); ++i) {
    for (auto &resourceId : tempPasses.at(i)->getInputs()) {
      passReads[resourceId].push_back(i);
    }
  }

  // Create adjacency list from inputs and outputs
  // to determine the edges of the graph
  std::vector<std::list<size_t>> adjacencyList;
  std::vector<size_t> stalePasses;
  adjacencyList.resize(tempPasses.size());

  for (size_t i = 0; i < tempPasses.size(); ++i) {
    for (auto &resourceId : tempPasses.at(i)->getOutputs()) {
      if (passReads.find(resourceId) != passReads.end()) {
        for (auto &read : passReads.at(resourceId)) {
          adjacencyList.at(i).push_back(read);
        }
      } else {
        stalePasses.push_back(i);
      }
    }
  }

  // topological sort based on DFS
  std::vector<RenderGraphPassInterface *> sortedPasses;
  sortedPasses.reserve(tempPasses.size());
  std::vector<bool> visited(tempPasses.size(), false);

  for (size_t i = 0; i < tempPasses.size(); ++i) {
    if (!visited.at(i)) {
      topologicalSort(i, visited, adjacencyList, sortedPasses);
    }
  }

  std::reverse(sortedPasses.begin(), sortedPasses.end());

  return sortedPasses;
}

void RenderGraph::topologicalSort(
    size_t index, std::vector<bool> &visited,
    const std::vector<std::list<size_t>> &adjacencyList,
    std::vector<RenderGraphPassInterface *> &output) {
  visited.at(index) = true;

  for (size_t x : adjacencyList.at(index)) {
    if (!visited.at(x)) {
      topologicalSort(x, visited, adjacencyList, output);
    }
  }

  output.push_back(passes.at(index));
}

GraphResourceId RenderGraph::generateNewId() { return lastId++; }

GraphResourceId
RenderGraph::addAttachment(const String &name,
                           const RenderPassAttachment &attachment) {
  auto id = getResourceId(name);
  LIQUID_ASSERT(!hasAttachment(id),
                "Attachment for \"" + name + "\" already exists");
  attachments.insert({id, attachment});
  return id;
}

GraphResourceId RenderGraph::addPipeline(const PipelineDescriptor &descriptor) {
  auto id = generateNewId();
  pipelines.insert({id, descriptor});
  return id;
}

const GraphResourceId RenderGraph::getResourceId(const String &name) {
  if (!hasResourceId(name)) {
    resourceMap[name] = generateNewId();
  }
  return resourceMap.at(name);
}

RenderGraphPassInterface::RenderGraphPassInterface(GraphResourceId renderPass_)
    : renderPass(renderPass_) {}

void RenderGraphPassInterface::addInput(GraphResourceId resourceId) {
  inputs.push_back(resourceId);
}

void RenderGraphPassInterface::addOutput(GraphResourceId resourceId) {
  outputs.push_back(resourceId);
}

void RenderGraphPassInterface::addResource(GraphResourceId resourceId) {
  resources.push_back(resourceId);
}

void RenderGraphPassInterface::setSwapchainRelative(bool swapchainRelative_) {
  swapchainRelative = swapchainRelative_;
}

} // namespace liquid