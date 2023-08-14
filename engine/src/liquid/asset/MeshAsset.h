#pragma once

#include "liquid/scene/Vertex.h"
#include "liquid/scene/SkinnedVertex.h"
#include "liquid/rhi/RenderHandle.h"
#include "liquid/rhi/Buffer.h"

#include "Asset.h"

namespace liquid {

/**
 * @brief Base geometry asset data
 *
 * @tparam TVertex Vertex
 */
template <class TVertex> struct BaseGeometryAsset {
  /**
   * List of vertices
   */
  std::vector<TVertex> vertices;

  /**
   * List of indices
   */
  std::vector<uint32_t> indices;
};

/**
 * @brief Mesh asset data
 */
struct MeshAsset {
  /**
   * List of geometries
   */
  std::vector<BaseGeometryAsset<Vertex>> geometries;

  /**
   * @brief Vertex buffer for all geometries
   */
  rhi::Buffer vertexBuffer;

  /**
   * @brief Index buffer for all geometries
   */
  rhi::Buffer indexBuffer;
};

/**
 * @brief Skinned mesh asset data
 */
struct SkinnedMeshAsset {
  /**
   * List of geometries
   */
  std::vector<BaseGeometryAsset<SkinnedVertex>> geometries;

  /**
   * @brief Vertex buffer for all geometries
   */
  rhi::Buffer vertexBuffer;

  /**
   * @brief Index buffer for all geometries
   */
  rhi::Buffer indexBuffer;
};

} // namespace liquid
