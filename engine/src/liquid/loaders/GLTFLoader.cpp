#include "liquid/core/Base.h"
#include "liquid/core/EngineGlobals.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_USE_CPP14

#include <json/json.hpp>
#include <stb/stb_image.h>
#include <tinygltf/tiny_gltf.h>

#include "liquid/scene/Vertex.h"
#include "liquid/scene/Mesh.h"
#include "liquid/scene/Skeleton.h"
#include "liquid/renderer/Texture.h"
#include "liquid/renderer/MaterialPBR.h"
#include "GLTFLoader.h"
#include "GLTFError.h"

namespace liquid {

struct BufferMeta {
  tinygltf::Accessor accessor;
  tinygltf::BufferView bufferView;
  const unsigned char *rawData;
};

struct TransformData {
  glm::vec3 localPosition{0.0f};
  glm::quat localRotation{1.0f, 0.0f, 0.0f, 0.0f};
  glm::vec3 localScale{1.0f};

  glm::mat4 localTransform{1.0f};
};

/**
 * @brief Decomposes matrix into TRS values
 *
 * @param matrix Input matrix
 * @param position Output position
 * @param rotation Output rotation
 * @param scale Output scale
 */
static void decomposeMatrix(const glm::mat4 &matrix, glm::vec3 &position,
                            glm::quat &rotation, glm::vec3 &scale) {
  glm::mat4 temp = matrix;
  position = temp[3];

  for (glm::mat4::length_type i = 0; i < 3; ++i) {
    scale[i] = glm::length(temp[i]);
    temp[i] /= scale[i];
  }

  rotation = glm::toQuat(matrix);
}

/**
 * @brief Load transform data
 *
 * Load matrix or each transform
 * attribute based on the given
 * ones
 *
 * @param node
 * @return
 */
TransformData loadTransformData(const tinygltf::Node &node) {
  TransformData data{};

  constexpr size_t TRANSFORM_MATRIX_SIZE = 6;
  constexpr size_t TRANSLATION_MATRIX_SIZE = 3;
  constexpr size_t SCALE_MATRIX_SIZE = 3;
  constexpr size_t ROTATION_MATRIX_SIZE = 4;

  glm::mat4 finalTransform = glm::mat4{1.0f};
  if (node.matrix.size() == TRANSFORM_MATRIX_SIZE) {
    finalTransform = glm::make_mat4(node.matrix.data());
    decomposeMatrix(finalTransform, data.localPosition, data.localRotation,
                    data.localScale);

  } else if (node.matrix.size() > 0) {
    engineLogger.log(Logger::Warning)
        << "Node matrix data must have 16 values. Skipping...";
  } else {
    if (node.translation.size() == TRANSLATION_MATRIX_SIZE) {
      data.localPosition = glm::make_vec3(node.translation.data());
      finalTransform *= glm::translate(glm::mat4{1.0f}, data.localPosition);
    } else if (node.translation.size() > 0) {
      engineLogger.log(Logger::Warning)
          << "Node translation data must have 3 values. Skipping...";
    }

    if (node.rotation.size() == ROTATION_MATRIX_SIZE) {
      data.localRotation = glm::make_quat(node.rotation.data());
      finalTransform *= glm::toMat4(data.localRotation);
    } else if (node.rotation.size() > 0) {
      engineLogger.log(Logger::Warning)
          << "Node rotation data must have 4 values. Skipping...";
    }

    if (node.scale.size() == SCALE_MATRIX_SIZE) {
      data.localScale = glm::make_vec3(node.scale.data());
      finalTransform *= glm::scale(glm::mat4{1.0f}, data.localScale);
    } else if (node.scale.size() > 0) {
      engineLogger.log(Logger::Warning)
          << "Node scale data must have 3 values. Skipping...";
    }
  }

  data.localTransform = finalTransform;

  return data;
}

/**
 * @brief Geta buffer metadata for accessor
 *
 * @param model TinyGLTF model
 * @param accessorIndex Index of buffer accessor
 * @return GLTF buffer metadata
 */
static BufferMeta getBufferMetaForAccessor(const tinygltf::Model &model,
                                           int accessorIndex) {
  auto accessor = model.accessors.at(accessorIndex);
  auto bufferView = model.bufferViews.at(accessor.bufferView);
  const unsigned char *bufferStart =
      &model.buffers.at(bufferView.buffer).data.at(0);
  const unsigned char *bufferOffset =
      bufferStart + accessor.byteOffset + bufferView.byteOffset;

  return {accessor, bufferView, bufferOffset};
}

/**
 * @brief Reads scene data and creates scenes
 *
 * Conforms to on GLTF 2.0 spec
 * https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
 *
 * @param model TinyGLTF model
 * @param meshEntityMap Mesh index entity map
 * @param nodeAnimationMap Node ID animation map
 * @param skeletons Skeleton ID map
 * @param entityContext Entity context
 * @return Scene node
 */
static SceneNode *getScene(const tinygltf::Model &model,
                           const std::map<uint32_t, Entity> &meshEntityMap,
                           const std::map<uint32_t, String> &nodeAnimationMap,
                           const std::map<uint32_t, Skeleton> &skeletons,
                           EntityContext &entityContext) {
  try {
    auto &gltfScene = model.scenes.at(model.defaultScene);
    auto *rootNode =
        new SceneNode(entityContext.createEntity(), {}, nullptr, entityContext);

    std::vector<SceneNode *> nodes(model.nodes.size());

    std::unordered_map<int, bool> jointNodes;
    for (auto &skin : model.skins) {
      for (auto &joint : skin.joints) {
        jointNodes.insert({joint, true});
      }
    }

    std::list<std::pair<int, int>> nodesToProcess;

    for (auto &nodeIndex : gltfScene.nodes) {
      if (jointNodes.find(nodeIndex) == jointNodes.end()) {
        nodesToProcess.push_back(std::make_pair(nodeIndex, -1));
      }
    }

    while (!nodesToProcess.empty()) {
      auto [nodeIndex, parentIndex] = nodesToProcess.front();
      auto &gltfNode = model.nodes.at(nodeIndex);
      nodesToProcess.pop_front();

      Entity entity = gltfNode.mesh >= 0 && meshEntityMap.size() > 0
                          ? meshEntityMap.at(gltfNode.mesh)
                          : entityContext.createEntity();

      String nodeName = String(gltfNode.name);
      String entityName =
          nodeName.length() > 0 ? nodeName : "Entity " + std::to_string(entity);
      entityContext.setComponent<NameComponent>(entity, {entityName});

      auto animation = nodeAnimationMap.find(nodeIndex);
      if (animation != nodeAnimationMap.end()) {
        entityContext.setComponent<AnimationComponent>(entity,
                                                       {animation->second});
      }

      const auto &skeleton = skeletons.find(gltfNode.skin);
      if (skeleton != skeletons.end()) {
        entityContext.setComponent<SkeletonComponent>(entity,
                                                      {skeleton->second});
      }

      TransformComponent transform;

      const auto &data = loadTransformData(gltfNode);

      transform.localPosition = data.localPosition;
      transform.localRotation = data.localRotation;
      transform.localScale = data.localScale;

      if (parentIndex >= 0) {
        nodes.at(nodeIndex) = nodes[parentIndex]->addChild(entity, transform);
      } else {
        nodes.at(nodeIndex) = rootNode->addChild(entity, transform);
      }

      for (auto child : gltfNode.children) {
        if (jointNodes.find(child) == jointNodes.end()) {
          nodesToProcess.push_back(std::make_pair(child, nodeIndex));
        }
      }
    }

    return rootNode;
  } catch (std::out_of_range e) {
    throw GLTFError("Failed to load scene");
  }
}

/**
 * @brief Get skeletons
 *
 * @param model GLTF nodel
 * @return Skeleton skin map
 */
static std::map<uint32_t, Skeleton>
getSkeletons(const tinygltf::Model &model,
             ResourceAllocator *resourceAllocator) {
  std::map<uint32_t, Skeleton> skeletons;

  for (size_t i = 0; i < model.skins.size(); ++i) {
    const auto &skin = model.skins.at(i);

    std::unordered_map<uint32_t, int> jointParents;
    std::unordered_map<uint32_t, uint32_t> normalizedJointMap;

    auto &&ibMeta = getBufferMetaForAccessor(model, skin.inverseBindMatrices);

    LIQUID_ASSERT(ibMeta.accessor.componentType ==
                      TINYGLTF_COMPONENT_TYPE_FLOAT,
                  "Inverse bind matrices accessor must be of type FLOAT");

    LIQUID_ASSERT(ibMeta.accessor.type == TINYGLTF_TYPE_MAT4,
                  "Inverse bind matrices accessor must be of type MAT4");

    LIQUID_ASSERT(ibMeta.accessor.count >= skin.joints.size(),
                  "Inverse bind matrices cannot be less than number of joints");

    std::vector<glm::mat4> inverseBindMatrices(ibMeta.accessor.count);

    {
      const auto *data = reinterpret_cast<const glm::mat4 *>(ibMeta.rawData);
      for (size_t i = 0; i < ibMeta.accessor.count; ++i) {
        inverseBindMatrices.at(i) = data[i];
      }
    }

    for (size_t i = 0; i < skin.joints.size(); ++i) {
      const auto &joint = skin.joints.at(i);
      normalizedJointMap.insert({joint, i});
      jointParents.insert({i, -1});
    }

    for (size_t j = 0; j < model.nodes.size(); ++j) {
      if (normalizedJointMap.find(j) == normalizedJointMap.end()) {
        continue;
      }

      uint32_t nJ = normalizedJointMap.at(j);

      const auto &node = model.nodes.at(j);
      for (auto &child : node.children) {
        if (normalizedJointMap.find(child) == normalizedJointMap.end()) {
          continue;
        }

        uint32_t nChild = normalizedJointMap.at(child);
        jointParents.at(nChild) = static_cast<int>(nJ);
      }
    }

    uint32_t numJoints = static_cast<uint32_t>(skin.joints.size());
    Skeleton skeleton(numJoints, resourceAllocator);

    for (auto &joint : skin.joints) {
      uint32_t nJoint = normalizedJointMap.at(joint);
      int parent = jointParents.at(nJoint);
      if (parent >= 0) {
        const auto &node = model.nodes.at(joint);
        const auto &data = loadTransformData(node);
        skeleton.addJoint(data.localTransform, parent,
                          inverseBindMatrices.at(nJoint), node.name);
      }
    }

    skeleton.update();

    skeletons.insert({i, skeleton});
  }

  return skeletons;
}

/**
 * @brief Load standard mesh attributes
 *
 * Loads position, normal, tangents,
 * and texture coordinates that exist
 * for both mesh and skinned mesh
 *
 * @tparam TMesh Mesh type
 * @param primitive GLTF mesh primitive
 * @param i Mesh index
 * @param p Primitive index
 * @param model GLTF model
 * @param mesh Mesh
 */
template <class TVertex>
std::pair<std::vector<TVertex>, std::vector<uint32_t>>
loadStandardMeshAttributes(const tinygltf::Primitive &primitive, size_t i,
                           size_t p, const tinygltf::Model &model) {
  std::vector<uint32_t> indices;
  std::vector<TVertex> vertices;

  if (primitive.attributes.find("POSITION") == primitive.attributes.end()) {
    engineLogger.log(Logger::Warning)
        << "Mesh #" << i << ", Primitive #" << p
        << " does not have a position attribute. Skipping...";
    return {};
  }

  if (primitive.indices >= 0) {
    auto &&indexMeta = getBufferMetaForAccessor(model, primitive.indices);
    indices.resize(indexMeta.accessor.count);
    if (indexMeta.accessor.componentType ==
            TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT &&
        indexMeta.accessor.type == TINYGLTF_TYPE_SCALAR) {
      const auto *data = reinterpret_cast<const uint32_t *>(indexMeta.rawData);
      for (size_t i = 0; i < indexMeta.accessor.count; ++i) {
        indices[i] = data[i];
      }
    } else if (indexMeta.accessor.componentType ==
                   TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT &&
               indexMeta.accessor.type == TINYGLTF_TYPE_SCALAR) {
      const auto *data = reinterpret_cast<const uint16_t *>(indexMeta.rawData);
      for (size_t i = 0; i < indexMeta.accessor.count; ++i) {
        indices[i] = data[i];
      }
    } else if (indexMeta.accessor.componentType ==
                   TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE &&
               indexMeta.accessor.type == TINYGLTF_TYPE_SCALAR) {
      const auto *data = reinterpret_cast<const uint8_t *>(indexMeta.rawData);
      for (size_t i = 0; i < indexMeta.accessor.count; ++i) {
        indices[i] = data[i];
      }
    } else {
      engineLogger.log(Logger::Warning)
          << "Mesh #" << i << ", Primitive #" << p
          << " has invalid index format. Skipping...";
      return {};
    }
  }

  auto &&positionMeta =
      getBufferMetaForAccessor(model, primitive.attributes.at("POSITION"));

  size_t vertexSize = positionMeta.accessor.count;
  vertices.resize(vertexSize);

  // According to spec, position attribute can only be vec3<float>
  if (positionMeta.accessor.type == TINYGLTF_TYPE_VEC3 &&
      positionMeta.accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
    auto *data = reinterpret_cast<const glm::vec3 *>(positionMeta.rawData);
    for (size_t i = 0; i < vertexSize; ++i) {
      vertices[i].x = data[i].x;
      vertices[i].y = data[i].y;
      vertices[i].z = data[i].z;
    }
  } else {
    engineLogger.log(Logger::Warning)
        << "Mesh #" << i << ", Primitive #0"
        << " has invalid position format. Skipping...";
    return {};
  }

  if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
    auto &&normalMeta =
        getBufferMetaForAccessor(model, primitive.attributes.at("NORMAL"));
    // According to spec, normal attribute can only be vec3<float> and
    // all attributes of a primitive must have the same number of items
    if (normalMeta.accessor.type == TINYGLTF_TYPE_VEC3 &&
        normalMeta.accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT &&
        normalMeta.accessor.count == vertexSize) {
      auto *data = reinterpret_cast<const glm::vec3 *>(normalMeta.rawData);
      for (size_t i = 0; i < vertexSize; ++i) {
        vertices[i].nx = data[i].x;
        vertices[i].ny = data[i].y;
        vertices[i].nz = data[i].z;
      }
    }
  } else {
    engineLogger.log(Logger::Warning)
        << "Calculating flat normals is not supported";
    // TODO: Calculate flat normals
  }

  if (primitive.attributes.find("TANGENT") != primitive.attributes.end()) {
    auto &&tangentMeta =
        getBufferMetaForAccessor(model, primitive.attributes.at("TANGENT"));
    // According to spec, normal attribute can only be vec4<float> and
    // all attributes of a primitive must have the same number of items
    if (tangentMeta.accessor.type == TINYGLTF_TYPE_VEC4 &&
        tangentMeta.accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT &&
        tangentMeta.accessor.count == vertexSize) {
      auto *data = reinterpret_cast<const glm::vec4 *>(tangentMeta.rawData);
      for (size_t i = 0; i < vertexSize; ++i) {
        vertices[i].tx = data[i].x;
        vertices[i].ty = data[i].y;
        vertices[i].tz = data[i].z;
        vertices[i].tw = data[i].w;
      }
    }
  } else {
    // TODO: Calculate tangents using MikkTSpace algorithms
    engineLogger.log(Logger::Warning)
        << "Tangents will be calculated using derivative functions in "
           "pixel shader. For more accurate results, you need to provide "
           "the tangent attribute when generating GLTF model.";
  }

  if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
    auto &&uvMeta =
        getBufferMetaForAccessor(model, primitive.attributes.at("TEXCOORD_0"));

    // According to spec, UV data is always in vec2 format
    // and all attributes of a primitive must have
    // the same number of items
    if (uvMeta.accessor.type == TINYGLTF_TYPE_VEC2 &&
        uvMeta.accessor.count == vertexSize) {
      // UV coordinates can be float, ubyte, and ushort
      if (uvMeta.accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
        auto *data = reinterpret_cast<const glm::vec2 *>(uvMeta.rawData);
        for (size_t i = 0; i < vertexSize; ++i) {
          vertices[i].u0 = data[i].x;
          vertices[i].v0 = data[i].y;
        }
      } else if (uvMeta.accessor.componentType ==
                     TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE ||
                 uvMeta.accessor.componentType ==
                     TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        // TODO: Convert integer coordinates to float
        engineLogger.log(Logger::Warning)
            << "Integer based texture coordinates are not supported for "
               "TEXCOORD0";
      }
    }
  }

  if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end()) {
    auto &&uvMeta =
        getBufferMetaForAccessor(model, primitive.attributes.at("TEXCOORD_1"));

    // According to spec, UV data is always in vec2 format
    // and all attributes of a primitive must have
    // the same number of items
    if (uvMeta.accessor.type == TINYGLTF_TYPE_VEC2 &&
        uvMeta.accessor.count == vertexSize) {
      // UV coordinates can be float, ubyte, and ushort
      if (uvMeta.accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
        auto *data = reinterpret_cast<const glm::vec2 *>(uvMeta.rawData);
        for (size_t i = 0; i < vertexSize; ++i) {
          vertices[i].u1 = data[i].x;
          vertices[i].v1 = data[i].y;
        }
      } else if (uvMeta.accessor.componentType ==
                     TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE ||
                 uvMeta.accessor.componentType ==
                     TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        // TODO: Convert integer coordinates to float
        engineLogger.log(Logger::Warning)
            << "Integer based texture coordinates are not supported for "
               "TEXCOORD1";
      }
    }
  }

  return {vertices, indices};
}

/**
 * @brief Reads mesh data and creates mesh instances
 *
 * Conforms to on GLTF 2.0 spec
 * https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
 *
 * @param model TinyGLTF model
 * @param materials List of materials
 * @param entityContext Entity context
 * @param renderer Renderer
 * @param defaultMaterial Default material
 * @return Map for mesh index and entity
 */
static std::map<uint32_t, Entity>
getMeshes(const tinygltf::Model &model,
          const std::vector<SharedPtr<Material>> &materials,
          EntityContext &entityContext, VulkanRenderer *renderer,
          const SharedPtr<Material> &defaultMaterial) {
  std::map<uint32_t, Entity> entityMap;

  try {
    for (auto i = 0; i < model.meshes.size(); ++i) {
      const auto &gltfMesh = model.meshes[i];

      // TODO: Support multiple primitives
      if (gltfMesh.primitives.empty()) {
        engineLogger.log(Logger::Warning)
            << "Mesh #" << i << " does not have primitives. Skipping...";
        continue;
      }

      bool isSkinnedMesh = false;
      for (auto &primitive : gltfMesh.primitives) {
        if (primitive.attributes.find("JOINTS_0") !=
            primitive.attributes.end()) {
          isSkinnedMesh = true;
        }
      }

      Mesh mesh;
      SkinnedMesh skinnedMesh;

      for (size_t p = 0; p < gltfMesh.primitives.size(); ++p) {
        const auto &primitive = gltfMesh.primitives.at(p);

        SharedPtr<Material> material = primitive.material >= 0
                                           ? materials.at(primitive.material)
                                           : defaultMaterial;

        if (isSkinnedMesh) {
          auto &&[vertices, indices] =
              loadStandardMeshAttributes<SkinnedMesh::Vertex>(primitive, i, p,
                                                              model);

          if (primitive.attributes.find("JOINTS_0") !=
              primitive.attributes.end()) {

            auto &&jointMeta = getBufferMetaForAccessor(
                model, primitive.attributes.at("JOINTS_0"));

            if (jointMeta.accessor.type != TINYGLTF_TYPE_VEC4) {
              engineLogger.log(Logger::Warning)
                  << "Mesh #" << i
                  << " JOINTS_0 is not in VEC4 format. Skipping...";
            } else if (jointMeta.accessor.componentType ==
                           TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE &&
                       jointMeta.accessor.type == TINYGLTF_TYPE_VEC4) {
              const auto *data =
                  reinterpret_cast<const glm::u8vec4 *>(jointMeta.rawData);

              for (size_t i = 0; i < jointMeta.accessor.count; ++i) {
                vertices.at(i).j0 = data[i].x;
                vertices.at(i).j1 = data[i].y;
                vertices.at(i).j2 = data[i].z;
                vertices.at(i).j3 = data[i].w;
              }
            } else if (jointMeta.accessor.componentType ==
                       TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
              const auto *data =
                  reinterpret_cast<const glm::u16vec4 *>(jointMeta.rawData);
              for (size_t i = 0; i < jointMeta.accessor.count; ++i) {
                vertices.at(i).j0 = data[i].x;
                vertices.at(i).j1 = data[i].y;
                vertices.at(i).j2 = data[i].z;
                vertices.at(i).j3 = data[i].w;
              }
            }
          }

          if (primitive.attributes.find("WEIGHTS_0") !=
              primitive.attributes.end()) {
            auto &&weightMeta = getBufferMetaForAccessor(
                model, primitive.attributes.at("WEIGHTS_0"));
            if (weightMeta.accessor.componentType ==
                TINYGLTF_COMPONENT_TYPE_FLOAT) {
              const auto *data =
                  reinterpret_cast<const glm::vec4 *>(weightMeta.rawData);

              for (size_t i = 0; i < weightMeta.accessor.count; ++i) {
                vertices.at(i).w0 = data[i].x;
                vertices.at(i).w1 = data[i].y;
                vertices.at(i).w2 = data[i].z;
                vertices.at(i).w3 = data[i].w;
              }
            }
          }

          if (vertices.size() > 0) {
            skinnedMesh.addGeometry({vertices, indices, material});
          }
        } else {
          const auto &[vertices, indices] =
              loadStandardMeshAttributes<Mesh::Vertex>(primitive, i, p, model);
          if (vertices.size() > 0) {
            mesh.addGeometry({vertices, indices, material});
          }
        }
      }

      auto entity = entityContext.createEntity();
      if (isSkinnedMesh) {
        entityContext.setComponent<SkinnedMeshComponent>(
            entity, {std::make_shared<MeshInstance<SkinnedMesh>>(
                        skinnedMesh, renderer->getResourceAllocator())});

      } else {
        entityContext.setComponent<MeshComponent>(
            entity, {std::make_shared<MeshInstance<Mesh>>(
                        mesh, renderer->getResourceAllocator())});
      }

      entityMap.insert({i, entity});
    }
    return entityMap;
  } catch (std::out_of_range e) {
    throw GLTFError("Failed load meshes");
  }
}

/**
 * @brief Geta buffer metadata for accessor
 *
 * @param model TinyGLTF model
 * @param accessorIndex Index of buffer accessor
 * @param renderer Renderer
 * @return GLTF buffer metadata
 */
static std::vector<SharedPtr<Material>>
getMaterials(const tinygltf::Model &model, VulkanRenderer *renderer) {
  try {
    std::vector<SharedPtr<Texture>> textures;
    std::vector<SharedPtr<Material>> materials;

    for (auto &gltfTexture : model.textures) {
      // TODO: Support creating different samplers
      auto &image = model.images.at(gltfTexture.source);

      TextureData imageData;
      imageData.height = image.height;
      imageData.width = image.width;
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
      imageData.data = const_cast<unsigned char *>(image.image.data());

      textures.push_back(
          renderer->getResourceAllocator()->createTexture2D(imageData));
    }

    for (auto &gltfMaterial : model.materials) {
      std::vector<std::pair<String, Property>> materialProperties;

      MaterialPBR::Properties properties{};

      if (gltfMaterial.pbrMetallicRoughness.baseColorTexture.index >= 0) {
        properties.baseColorTexture = textures.at(
            gltfMaterial.pbrMetallicRoughness.baseColorTexture.index);
      }
      properties.baseColorTextureCoord =
          gltfMaterial.pbrMetallicRoughness.baseColorTexture.texCoord;
      auto &colorFactor = gltfMaterial.pbrMetallicRoughness.baseColorFactor;
      properties.baseColorFactor = glm::vec4{colorFactor[0], colorFactor[1],
                                             colorFactor[2], colorFactor[3]};

      if (gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index >=
          0) {
        properties.metallicRoughnessTexture = textures.at(
            gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index);
      }
      properties.metallicRoughnessTextureCoord =
          gltfMaterial.pbrMetallicRoughness.baseColorTexture.texCoord;
      properties.metallicFactor =
          static_cast<float>(gltfMaterial.pbrMetallicRoughness.metallicFactor);
      properties.roughnessFactor =
          static_cast<float>(gltfMaterial.pbrMetallicRoughness.roughnessFactor);

      if (gltfMaterial.normalTexture.index >= 0) {
        properties.normalTexture =
            textures.at(gltfMaterial.normalTexture.index);
      }
      properties.normalTextureCoord = gltfMaterial.normalTexture.texCoord;
      properties.normalScale =
          static_cast<float>(gltfMaterial.normalTexture.scale);

      if (gltfMaterial.occlusionTexture.index >= 0) {
        properties.occlusionTexture =
            textures.at(gltfMaterial.occlusionTexture.index);
      }
      properties.occlusionTextureCoord = gltfMaterial.occlusionTexture.texCoord;
      properties.occlusionStrength =
          static_cast<float>(gltfMaterial.occlusionTexture.strength);

      if (gltfMaterial.emissiveTexture.index >= 0) {
        properties.emissiveTexture =
            textures.at(gltfMaterial.emissiveTexture.index);
      }
      properties.emissiveTextureCoord = gltfMaterial.emissiveTexture.texCoord;
      auto &emissiveFactor = gltfMaterial.emissiveFactor;
      properties.emissiveFactor =
          glm::vec3{emissiveFactor[0], emissiveFactor[1], emissiveFactor[2]};

      CullMode cullMode =
          gltfMaterial.doubleSided ? CullMode::None : CullMode::Back;
      materials.push_back(renderer->createMaterialPBR(properties, cullMode));
    }

    return materials;
  } catch (std::out_of_range e) {
    throw GLTFError("Failed load textures and materials");
  }
}

/**
 * @brief Read animation dataa and return them for usage
 *
 * Conforms to on GLTF 2.0 spec
 * https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
 *
 * @param model TinyGLTF model
 * @return Map for animation index and Animation
 */
static std::map<uint32_t, String>
getAnimations(const tinygltf::Model &model, AnimationSystem &animationSystem) {
  std::map<uint32_t, String> animations;
  for (size_t i = 0; i < model.animations.size(); ++i) {
    const auto &gltfAnimation = model.animations.at(i);

    struct SamplerInfo {
      std::vector<float> times;
      std::vector<glm::vec4> values;
      KeyframeSequenceInterpolation interpolation =
          KeyframeSequenceInterpolation::Linear;
    };

    std::vector<SamplerInfo> samplers(gltfAnimation.samplers.size());

    float maxTime = 0.0f;

    for (size_t i = 0; i < gltfAnimation.samplers.size(); ++i) {
      const auto &sampler = gltfAnimation.samplers.at(i);
      const auto &input = getBufferMetaForAccessor(model, sampler.input);
      const auto &output = getBufferMetaForAccessor(model, sampler.output);

      LIQUID_ASSERT(input.accessor.type == TINYGLTF_TYPE_SCALAR,
                    "Animation time accessor must be in SCALAR format");

      LIQUID_ASSERT(input.accessor.componentType ==
                        TINYGLTF_COMPONENT_TYPE_FLOAT,
                    "Animation time accessor component type must be FLOAT");

      LIQUID_ASSERT(
          input.accessor.count == output.accessor.count,
          "Sampler input and output must have the same number of items");

      LIQUID_ASSERT(output.accessor.componentType ==
                        TINYGLTF_COMPONENT_TYPE_FLOAT,
                    "Animation output accessor component type must be FLOAT");

      std::vector<float> &times = samplers.at(i).times;
      times.resize(input.accessor.count);

      std::vector<glm::vec4> &values = samplers.at(i).values;
      values.resize(output.accessor.count);

      if (sampler.interpolation == "LINEAR") {
        samplers.at(i).interpolation = KeyframeSequenceInterpolation::Linear;
      } else if (sampler.interpolation == "STEP") {
        samplers.at(i).interpolation = KeyframeSequenceInterpolation::Step;
      }

      float max = 0.0f;

      {
        const float *inputData = reinterpret_cast<const float *>(input.rawData);
        for (size_t i = 0; i < input.accessor.count; ++i) {
          times.at(i) = inputData[i];
          max = std::max(max, inputData[i]);
        }
      }

      maxTime = std::max(max, maxTime);

      // Normalize the time
      {
        for (size_t i = 0; i < times.size(); ++i) {
          times.at(i) = times.at(i) / max;
        }
      }

      if (output.accessor.type == TINYGLTF_TYPE_VEC3) {
        const glm::vec3 *outputData =
            reinterpret_cast<const glm::vec3 *>(output.rawData);
        for (size_t i = 0; i < output.accessor.count; ++i) {
          values.at(i) = glm::vec4(outputData[i], 0.0f);
        }
      } else if (output.accessor.type == TINYGLTF_TYPE_VEC4) {
        const glm::vec4 *outputData =
            reinterpret_cast<const glm::vec4 *>(output.rawData);
        for (size_t i = 0; i < output.accessor.count; ++i) {
          values.at(i) = outputData[i];
        }
      } else if (output.accessor.type == TINYGLTF_TYPE_SCALAR) {
        const float *outputData =
            reinterpret_cast<const float *>(output.rawData);
        for (size_t i = 0; i < output.accessor.count; ++i) {
          values.at(i) = glm::vec4(outputData[i], 0, 0, 0);
        }
      }
    }

    Animation animation(gltfAnimation.name, maxTime);
    int32_t targetNode = -1;

    for (const auto &channel : gltfAnimation.channels) {
      const auto &sampler = samplers.at(channel.sampler);

      if (channel.target_node == -1) {
        // Ignore channel if target node is not specified
        continue;
      }

      if (targetNode == -1) {
        targetNode = channel.target_node;
      } else {
        LIQUID_ASSERT(
            targetNode == channel.target_node,
            "All channels in animation must point to the same target node");
      }

      KeyframeSequenceTarget target = KeyframeSequenceTarget::Position;
      if (channel.target_path == "rotation") {
        target = KeyframeSequenceTarget::Rotation;
      } else if (channel.target_path == "scale") {
        target = KeyframeSequenceTarget::Scale;
      } else if (channel.target_path == "position") {
        target = KeyframeSequenceTarget::Position;
      }

      KeyframeSequence sequence(target, sampler.interpolation);
      for (size_t i = 0; i < sampler.times.size(); ++i) {
        sequence.addKeyframe(sampler.times.at(i), sampler.values.at(i));
      }
      animation.addKeyframeSequence(sequence);
    }

    LIQUID_ASSERT(targetNode >= 0, "Animation must have a target node");

    animations.insert({targetNode, animation.getName()});
    animationSystem.addAnimation(animation);
  }
  return animations;
}

GLTFLoader::GLTFLoader(EntityContext &entityContext_, VulkanRenderer *renderer_,
                       AnimationSystem &animationSystem_)
    : entityContext(entityContext_), renderer(renderer_),
      animationSystem(animationSystem_),
      defaultMaterial(renderer->createMaterialPBR({}, CullMode::Back)) {}

SceneNode *GLTFLoader::loadFromFile(const String &filename) {
  tinygltf::TinyGLTF loader;
  tinygltf::Model model;
  String error, warning;

  bool ret = loader.LoadASCIIFromFile(&model, &error, &warning, filename);

  if (!warning.empty()) {
    engineLogger.log(Logger::Warning) << warning;
  }

  if (!error.empty()) {
    throw GLTFError(error);
  }

  if (!ret) {
    throw GLTFError("Failed to parse GLTF file");
  }

  auto &&skeletons = getSkeletons(model, renderer->getResourceAllocator());
  auto &&animations = getAnimations(model, animationSystem);
  auto &&materials = getMaterials(model, renderer);
  auto &&meshes =
      getMeshes(model, materials, entityContext, renderer, defaultMaterial);
  auto *scene = getScene(model, meshes, animations, skeletons, entityContext);

  LOG_DEBUG("[GLTF] Loaded GLTF scene from " << filename);

  return scene;
}

} // namespace liquid
