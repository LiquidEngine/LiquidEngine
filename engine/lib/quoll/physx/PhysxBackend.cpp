#include "quoll/core/Base.h"
#include "quoll/core/Engine.h"
#include "quoll/core/Profiler.h"
#include "quoll/physics/Collidable.h"
#include "quoll/physics/Force.h"
#include "quoll/physics/Impulse.h"
#include "quoll/physics/RigidBody.h"
#include "quoll/physics/RigidBodyClear.h"
#include "quoll/physics/Torque.h"
#include "quoll/scene/LocalTransform.h"
#include "quoll/scene/Parent.h"
#include "quoll/scene/WorldTransform.h"
#include "quoll/skeleton/JointAttachment.h"
#include "quoll/skeleton/Skeleton.h"
#include "quoll/system/SystemView.h"
#include "PhysxBackend.h"
#include "PhysxMapping.h"
#include "PhysxQueryFilterCallback.h"

#ifdef QUOLL_PROFILER
static const bool RECORD_MEMORY_ALLOCATIONS = true;
#else
static const bool RECORD_MEMORY_ALLOCATIONS = false;
#endif

using namespace physx;

namespace quoll {

namespace {

PxTransform getShapeLocalTransform(const glm::vec3 &center,
                                   PhysicsGeometryType type) {
  if (type == PhysicsGeometryType::Capsule) {
    return PxTransform(PhysxMapping::getPhysxVec3(center),
                       PxQuat(PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f)));
  }

  return PxTransform(PhysxMapping::getPhysxVec3(center));
}

void updateShapeWithGeometryData(const PhysicsGeometryDesc &geometryDesc,
                                 PxShape *shape,
                                 const glm::mat4 &worldTransform) {
  glm::vec3 scale;
  glm::quat rotation;
  glm::vec3 translation;
  glm::vec3 skew;
  glm::vec4 perspective;
  glm::decompose(worldTransform, scale, rotation, translation, skew,
                 perspective);

  scale = glm::abs(scale);

  if (geometryDesc.type == PhysicsGeometryType::Sphere) {
    const auto &[radius] = std::get<PhysicsGeometrySphere>(geometryDesc.params);
    const PxSphereGeometry geometry(
        std::max(scale.x, std::max(scale.y, scale.z)) * radius);
    shape->setGeometry(geometry);
  } else if (geometryDesc.type == PhysicsGeometryType::Box) {
    const auto &[halfExtents] =
        std::get<PhysicsGeometryBox>(geometryDesc.params);
    const PxBoxGeometry geometry(scale.x * halfExtents.x,
                                 scale.y * halfExtents.y,
                                 scale.z * halfExtents.z);
    shape->setGeometry(geometry);
  } else if (geometryDesc.type == PhysicsGeometryType::Capsule) {
    const auto &[radius, halfHeight] =
        std::get<PhysicsGeometryCapsule>(geometryDesc.params);
    const PxCapsuleGeometry geometry(scale.x * radius, scale.y * halfHeight);
    shape->setGeometry(geometry);

    const PxTransform relativePose(PxQuat(PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f)));
    shape->setLocalPose(relativePose);
  } else if (geometryDesc.type == PhysicsGeometryType::Plane) {
    shape->setGeometry(PxPlaneGeometry());
  }
}

PxFilterFlags physxFilterAllCollisionShader(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags &pairFlags, const void *constantBlock,
    PxU32 constantBlockSize) {

  if (PxFilterObjectIsTrigger(attributes0) ||
      PxFilterObjectIsTrigger(attributes1)) {
    pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
    return PxFilterFlag::eDEFAULT;
  }

  pairFlags = PxPairFlag::eCONTACT_DEFAULT | PxPairFlag::eNOTIFY_TOUCH_FOUND |
              PxPairFlag::eNOTIFY_TOUCH_LOST;

  return PxFilterFlag::eDEFAULT;
}

} // namespace

PhysxBackend::PhysxBackend() : mSimulationEventCallback(mSignals) {
  static constexpr glm::vec3 Gravity(0.0f, -9.8f, 0.0f);
  static u32 NumThreads = 1;

  mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, mDefaultAllocator,
                                   mDefaultErrorCallback);
  mDebugPanel.create(mFoundation);

  mPhysics =
      PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, PxTolerancesScale(),
                      RECORD_MEMORY_ALLOCATIONS, mDebugPanel.getPvd());

  mDispatcher = PxDefaultCpuDispatcherCreate(NumThreads);

  PxSceneDesc sceneDesc(mPhysics->getTolerancesScale());
  sceneDesc.cpuDispatcher = mDispatcher;
  sceneDesc.filterShader = physxFilterAllCollisionShader;
  sceneDesc.gravity = PxVec3(Gravity.x, Gravity.y, Gravity.z);
  sceneDesc.flags = PxSceneFlag::eENABLE_ACTIVE_ACTORS;
  sceneDesc.simulationEventCallback = &mSimulationEventCallback;
  sceneDesc.staticKineFilteringMode = PxPairFilteringMode::eKEEP;
  sceneDesc.kineKineFilteringMode = PxPairFilteringMode::eKEEP;
  mScene = mPhysics->createScene(sceneDesc);

  auto *pvdClient = mScene->getScenePvdClient();
  if (pvdClient) {
    pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
    pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
    pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
  }

  Engine::getLogger().info()
      << "Physx engine v" << PX_PHYSICS_VERSION_MAJOR << "."
      << PX_PHYSICS_VERSION_MINOR << "." << PX_PHYSICS_VERSION_BUGFIX
      << " initialized with " << NumThreads << " CPU thread"
      << (NumThreads > 1 ? "s" : "");
}

PhysxBackend::~PhysxBackend() {
  mScene->release();
  mDispatcher->release();
  mPhysics->release();
  mDebugPanel.release();
  mFoundation->release();
}

void PhysxBackend::update(f32 dt, SystemView &view) {
  QUOLL_PROFILE_EVENT("PhysicsSystem::update");

  synchronizeComponents(view);

  mScene->simulate(dt);
  mScene->fetchResults(true);

  synchronizeTransforms(view);
}

void PhysxBackend::cleanup(SystemView &view) {
  auto &entityDatabase = view.scene->entityDatabase;
  for (auto [entity, physx] : entityDatabase.view<PhysxInstance>()) {
    if (physx.rigidStatic) {
      if (physx.shape) {
        physx.rigidStatic->detachShape(*physx.shape);
      }
      mScene->removeActor(*physx.rigidStatic);
      physx.rigidStatic->release();
    }

    if (physx.rigidDynamic) {
      if (physx.shape) {
        physx.rigidDynamic->detachShape(*physx.shape);
      }
      mScene->removeActor(*physx.rigidDynamic);
      physx.rigidDynamic->release();
    }
  }

  entityDatabase.destroyComponents<PhysxInstance>();
  view.physx.instanceRemoveObserver.clear();
}

void PhysxBackend::createSystemViewData(SystemView &view) {
  auto &entityDatabase = view.scene->entityDatabase;

  view.physx.instanceRemoveObserver =
      entityDatabase.observeRemove<PhysxInstance>();
}

bool PhysxBackend::sweep(EntityDatabase &entityDatabase, Entity entity,
                         const glm::vec3 &direction, f32 maxDistance,
                         CollisionHit &hit) {
  QuollAssert(entityDatabase.has<PhysxInstance>(entity),
              "Physx instance not found");

  const auto &physx = entityDatabase.get<PhysxInstance>(entity);
  const auto &transform = entityDatabase.get<WorldTransform>(entity);
  const auto &collidable = entityDatabase.get<Collidable>(entity);

  PxSweepBuffer buffer;

  const PxQueryFilterData filterData(
      PxQueryFlag::eDYNAMIC | PxQueryFlag::eSTATIC | PxQueryFlag::ePREFILTER);
  PhysxQueryFilterCallback filterCallback(physx.shape);

  const bool result =
      mScene->sweep(physx.shape->getGeometry().any(),
                    PhysxMapping::getPhysxTransform(transform.worldTransform) *
                        physx.shape->getLocalPose(),
                    PhysxMapping::getPhysxVec3(direction), maxDistance, buffer,
                    PxHitFlag::eDEFAULT, filterData, &filterCallback);

  if (result) {
    const auto &bh = buffer.getAnyHit(0);
    hit.normal = PhysxMapping::getVec3(bh.normal);
    hit.distance = bh.distance;
    hit.entity =
        static_cast<Entity>(reinterpret_cast<uptr>(bh.actor->userData));
  }

  return result;
}

void PhysxBackend::synchronizeComponents(SystemView &view) {
  QUOLL_PROFILE_EVENT("PhysicsSystem::synchronizeEntitiesWithPhysx");

  {
    QUOLL_PROFILE_EVENT("Cleanup dangling physx objects in scene");
    for (auto [entity, physx] : view.physx.instanceRemoveObserver) {
      if (physx.rigidDynamic) {
        mScene->removeActor(*physx.rigidDynamic);
        physx.rigidDynamic->release();
      }

      if (physx.rigidStatic) {
        mScene->removeActor(*physx.rigidStatic);
        physx.rigidStatic->release();
      }

      if (physx.material) {
        physx.material->release();
      }
    }

    view.physx.instanceRemoveObserver.clear();
  }

  {
    QUOLL_PROFILE_EVENT("Synchronize collidable components");
    auto &entityDatabase = view.scene->entityDatabase;
    for (auto [entity, collidable, world] :
         entityDatabase.view<Collidable, WorldTransform>()) {
      if (!entityDatabase.has<PhysxInstance>(entity)) {
        entityDatabase.set<PhysxInstance>(entity, {});
      }
      auto &physx = entityDatabase.get<PhysxInstance>(entity);

      // Create or set material
      if (!physx.material) {
        physx.material =
            mPhysics->createMaterial(collidable.materialDesc.staticFriction,
                                     collidable.materialDesc.dynamicFriction,
                                     collidable.materialDesc.restitution);
      } else {
        physx.material->setRestitution(collidable.materialDesc.restitution);
        physx.material->setStaticFriction(
            collidable.materialDesc.staticFriction);
        physx.material->setDynamicFriction(
            collidable.materialDesc.dynamicFriction);
      }

      // Create or set shape
      if (!physx.shape) {
        physx.shape = createShape(entity, collidable.geometryDesc,
                                  *physx.material, world.worldTransform);
        physx.material->release();
      } else if (PhysxMapping::getPhysxGeometryType(
                     collidable.geometryDesc.type) ==
                 physx.shape->getGeometryType()) {
        updateShapeWithGeometryData(collidable.geometryDesc, physx.shape,
                                    world.worldTransform);
      } else {
        auto *newShape = createShape(entity, collidable.geometryDesc,
                                     *physx.material, world.worldTransform);

        if (entityDatabase.has<RigidBody>(entity)) {
          physx.rigidDynamic->detachShape(*physx.shape);
          physx.rigidDynamic->attachShape(*newShape);
        } else {
          physx.rigidStatic->detachShape(*physx.shape);
          physx.rigidStatic->attachShape(*newShape);
        }

        physx.shape->release();
        physx.shape = newShape;
      }

      if (physx.useShapeInSimulation != collidable.useInSimulation) {
        physx.shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE,
                             collidable.useInSimulation);
      }
      physx.useShapeInSimulation = collidable.useInSimulation;

      if (physx.useShapeInQueries != collidable.useInQueries) {
        physx.shape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE,
                             collidable.useInQueries);
      }

      physx.useShapeInQueries = collidable.useInQueries;
      physx.shape->setLocalPose(getShapeLocalTransform(
          collidable.geometryDesc.center, collidable.geometryDesc.type));

      // Create rigid static if no rigid body
      if (!entityDatabase.has<RigidBody>(entity) && !physx.rigidStatic) {
        physx.rigidStatic = mPhysics->createRigidStatic(
            PhysxMapping::getPhysxTransform(world.worldTransform));
        physx.rigidStatic->attachShape(*physx.shape);
        physx.rigidStatic->userData =
            reinterpret_cast<void *>(static_cast<uptr>(entity));

        mScene->addActor(*physx.rigidStatic);
      } else if (physx.rigidStatic) {
        // Update transform of rigid static if exists
        physx.rigidStatic->setGlobalPose(
            PhysxMapping::getPhysxTransform(world.worldTransform));
      }
    }
  }

  {
    QUOLL_PROFILE_EVENT("Synchronize rigid body components");
    auto &entityDatabase = view.scene->entityDatabase;
    for (auto [entity, rigidBody, world] :
         entityDatabase.view<RigidBody, WorldTransform>()) {
      if (!entityDatabase.has<PhysxInstance>(entity)) {
        entityDatabase.set<PhysxInstance>(entity, {});
      }

      auto &physx = entityDatabase.get<PhysxInstance>(entity);

      if (!physx.rigidDynamic) {
        physx.rigidDynamic = mPhysics->createRigidDynamic(
            PhysxMapping::getPhysxTransform(world.worldTransform));
        physx.rigidDynamic->userData =
            reinterpret_cast<void *>(static_cast<uptr>(entity));

        mScene->addActor(*physx.rigidDynamic);

        // Remove rigid static if exists
        if (physx.rigidStatic) {
          physx.rigidStatic->detachShape(*physx.shape);
          mScene->removeActor(*physx.rigidStatic, false);
          physx.rigidStatic->release();
          physx.rigidStatic = nullptr;
        }
      }

      if (physx.shape && physx.rigidDynamic->getNbShapes() == 0) {
        physx.rigidDynamic->attachShape(*physx.shape);
      }

      physx.rigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC,
                                           rigidBody.type ==
                                               RigidBodyType::Kinematic);
      physx.rigidDynamic->setActorFlag(PxActorFlag::eDISABLE_GRAVITY,
                                       !rigidBody.dynamicDesc.applyGravity);
      if (rigidBody.type == RigidBodyType::Kinematic) {
        physx.rigidDynamic->setKinematicTarget(
            PhysxMapping::getPhysxTransform(world.worldTransform));
      } else {
        physx.rigidDynamic->setGlobalPose(
            PhysxMapping::getPhysxTransform(world.worldTransform));
      }

      physx.rigidDynamic->setMass(rigidBody.dynamicDesc.mass);
      physx.rigidDynamic->setMassSpaceInertiaTensor(
          {rigidBody.dynamicDesc.inertia.x, rigidBody.dynamicDesc.inertia.y,
           rigidBody.dynamicDesc.inertia.z});
    };
  }

  {
    QUOLL_PROFILE_EVENT("Clear rigid body velocities");
    auto &entityDatabase = view.scene->entityDatabase;
    for (auto [entity, _, _2, physx] :
         entityDatabase.view<RigidBodyClear, RigidBody, PhysxInstance>()) {
      physx.rigidDynamic->setLinearVelocity(PxVec3(0.0f));
      physx.rigidDynamic->setAngularVelocity(PxVec3(0.0f));
    }

    entityDatabase.destroyComponents<RigidBodyClear>();
  }

  {
    QUOLL_PROFILE_EVENT("Apply forces");
    auto &entityDatabase = view.scene->entityDatabase;
    for (auto [entity, force, _, physx] :
         entityDatabase.view<Force, RigidBody, PhysxInstance>()) {
      physx.rigidDynamic->addForce(
          PxVec3(force.force.x, force.force.y, force.force.z),
          physx::PxForceMode::eFORCE);
    };

    entityDatabase.destroyComponents<Force>();
  }

  {
    QUOLL_PROFILE_EVENT("Apply impulses");
    auto &entityDatabase = view.scene->entityDatabase;
    for (auto [entity, impulse, _, physx] :
         entityDatabase.view<Impulse, RigidBody, PhysxInstance>()) {
      physx.rigidDynamic->addForce(
          PxVec3(impulse.impulse.x, impulse.impulse.y, impulse.impulse.z),
          physx::PxForceMode::eIMPULSE);
    };

    entityDatabase.destroyComponents<Impulse>();
  }

  {
    QUOLL_PROFILE_EVENT("Apply torques");
    auto &entityDatabase = view.scene->entityDatabase;
    for (auto [entity, torque, _, physx] :
         entityDatabase.view<Torque, RigidBody, PhysxInstance>()) {
      physx.rigidDynamic->addTorque(
          PxVec3(torque.torque.x, torque.torque.y, torque.torque.z));
    };

    entityDatabase.destroyComponents<Torque>();
  }
}

void PhysxBackend::synchronizeTransforms(SystemView &view) {
  QUOLL_PROFILE_EVENT("PhysicsSystem::synchronizeTransforms");
  auto &entityDatabase = view.scene->entityDatabase;
  u32 count = 0;
  auto **actors = mScene->getActiveActors(count);

  {
    QUOLL_PROFILE_EVENT("Synchronize world transforms");
    // calculate world transforms first so that local
    // transforms are calculated from the most recent
    // values
    for (u32 i = 0; i < count; ++i) {
      if (actors[i]->getType() != PxActorType::eRIGID_DYNAMIC) {
        continue;
      }
      auto *actor = static_cast<PxRigidActor *>(actors[i]);
      const auto &globalTransform = actor->getGlobalPose();

      const Entity entity =
          static_cast<Entity>(reinterpret_cast<uptr>(actor->userData));

      const glm::vec3 position(globalTransform.p.x, globalTransform.p.y,
                               globalTransform.p.z);
      const glm::quat rotation(globalTransform.q.w, globalTransform.q.x,
                               globalTransform.q.y, globalTransform.q.z);

      if (entityDatabase.has<WorldTransform>(entity)) {
        auto &world = entityDatabase.get<WorldTransform>(entity);

        glm::quat emptyQuat;
        glm::vec3 scale;
        glm::vec3 empty3;
        glm::vec4 empty4;

        glm::decompose(world.worldTransform, scale, emptyQuat, empty3, empty3,
                       empty4);

        world.worldTransform = glm::translate(glm::mat4{1.0f}, position) *
                               glm::toMat4(rotation) *
                               glm::scale(glm::mat4{1.0f}, scale);
      }
    }
  }

  {
    QUOLL_PROFILE_EVENT("Synchronize local transforms");
    // Calculate local transforms from parent world transforms
    for (u32 i = 0; i < count; ++i) {
      if (actors[i]->getType() != PxActorType::eRIGID_DYNAMIC) {
        continue;
      }
      auto *actor = static_cast<PxRigidActor *>(actors[i]);
      const auto &globalTransform = actor->getGlobalPose();

      const Entity entity =
          static_cast<Entity>(reinterpret_cast<uptr>(actor->userData));

      const glm::vec3 position(globalTransform.p.x, globalTransform.p.y,
                               globalTransform.p.z);
      const glm::quat rotation(globalTransform.q.w, globalTransform.q.x,
                               globalTransform.q.y, globalTransform.q.z);

      if (entityDatabase.has<LocalTransform>(entity)) {
        auto &transform = entityDatabase.get<LocalTransform>(entity);

        if (entityDatabase.has<Parent>(entity)) {
          auto parent = entityDatabase.get<Parent>(entity).parent;
          const auto &parentTransform =
              entityDatabase.get<WorldTransform>(parent);

          glm::mat4 invParentTransform{1.0f};
          i16 jointId = -1;
          if (entityDatabase.has<JointAttachment>(entity) &&
              entityDatabase.has<Skeleton>(parent)) {
            jointId = entityDatabase.get<JointAttachment>(entity).joint;

            const auto &jointTransform =
                entityDatabase.get<Skeleton>(parent).jointWorldTransforms.at(
                    jointId);

            invParentTransform =
                glm::inverse(parentTransform.worldTransform * jointTransform);
          } else {
            invParentTransform = glm::inverse(parentTransform.worldTransform);
          }

          transform.localPosition =
              glm::vec3(invParentTransform * glm::vec4(position, 1.0));
          transform.localRotation =
              glm::toQuat(invParentTransform * glm::toMat4(rotation));
        } else {
          transform.localPosition = position;
          transform.localRotation = rotation;
        }
      }
    }
  }
}

PxShape *PhysxBackend::createShape(Entity entity,
                                   const PhysicsGeometryDesc &geometryDesc,
                                   PxMaterial &material,
                                   const glm::mat4 &worldTransform) {
  glm::vec3 scale;
  glm::quat rotation;
  glm::vec3 translation;
  glm::vec3 skew;
  glm::vec4 perspective;
  glm::decompose(worldTransform, scale, rotation, translation, skew,
                 perspective);

  scale = glm::abs(scale);

  PxShape *shape = nullptr;
  if (geometryDesc.type == PhysicsGeometryType::Sphere) {
    const auto &[radius] = std::get<PhysicsGeometrySphere>(geometryDesc.params);
    const PxSphereGeometry geometry(
        std::max(scale.x, std::max(scale.y, scale.z)) * radius);
    shape = mPhysics->createShape(geometry, material, true);
  } else if (geometryDesc.type == PhysicsGeometryType::Box) {
    const auto &[halfExtents] =
        std::get<PhysicsGeometryBox>(geometryDesc.params);
    const PxBoxGeometry geometry(scale.x * halfExtents.x,
                                 scale.y * halfExtents.y,
                                 scale.z * halfExtents.z);
    shape = mPhysics->createShape(geometry, material, true);
  } else if (geometryDesc.type == PhysicsGeometryType::Capsule) {
    const auto &[radius, halfHeight] =
        std::get<PhysicsGeometryCapsule>(geometryDesc.params);
    const PxCapsuleGeometry geometry(std::max(scale.x, scale.y) * radius,
                                     scale.y * halfHeight);
    shape = mPhysics->createShape(geometry, material, true);

    const PxTransform relativePose(
        PhysxMapping::getPhysxVec3(geometryDesc.center),
        PxQuat(PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f)));
    shape->setLocalPose(relativePose);
  } else if (geometryDesc.type == PhysicsGeometryType::Plane) {
    shape = mPhysics->createShape(PxPlaneGeometry(), material, true);
  }

  PxFilterData filterData{};
  filterData.word0 = static_cast<PxU32>(entity);
  shape->setSimulationFilterData(filterData);

  return shape;
}

} // namespace quoll
