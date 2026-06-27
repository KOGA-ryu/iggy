#include "runtime/physics/PhysicsKernelBenchmark.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

#include "content/assets/RoomAsset.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/physics/PhysicsAabbContact.hpp"
#include "runtime/physics/PhysicsAabbContactSolver.hpp"
#include "runtime/physics/PhysicsBroadphase.hpp"
#include "runtime/physics/PhysicsKinematicMotor.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"
#include "runtime/player/PlayerPhysicsMovePlanner.hpp"

namespace iggy3d {
namespace {

using KernelRun = PhysicsKernelBenchmarkCaseResult (*)(
    const PhysicsKernelBenchmarkConfig&);

struct KernelScenarioHandler {
  PhysicsKernelBenchmarkKernel kernel;
  PhysicsKernelBenchmarkScenario scenario;
  KernelRun run;
};

template <std::size_t Count, typename Enum>
std::string_view enumName(Enum value,
                          const std::array<std::string_view, Count>& names,
                          std::string_view fallback) {
  const auto index = static_cast<std::size_t>(value);
  // branch-gate: BG-1116
  if (index >= names.size()) {
    return fallback;
  }
  return names[index];
}

PhysicsKernelBenchmarkCaseResult caseResult(
    PhysicsKernelBenchmarkStatus status,
    bool ok,
    PhysicsKernelBenchmarkKernel kernel,
    PhysicsKernelBenchmarkScenario scenario) {
  PhysicsKernelBenchmarkCaseResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsKernelBenchmarkStatusName(status);
  result.kernelName = physicsKernelBenchmarkKernelName(kernel);
  result.scenarioName = physicsKernelBenchmarkScenarioName(scenario);
  return result;
}

PhysicsKernelBenchmarkCaseResult readyResult(
    PhysicsKernelBenchmarkKernel kernel,
    PhysicsKernelBenchmarkScenario scenario,
    const PhysicsKernelBenchmarkConfig& config) {
  PhysicsKernelBenchmarkCaseResult result =
      caseResult(PhysicsKernelBenchmarkStatus::Ready, true, kernel, scenario);
  result.iterations = config.iterations;
  return result;
}

PhysicsAabbCollider colliderAt(PhysicsBodyId id,
                               Vec3 centerMeters,
                               Vec3 halfExtentsMeters,
                               bool sensor = false) {
  PhysicsAabbCollider collider;
  collider.bodyId = id;
  collider.worldCenterMeters = centerMeters;
  collider.halfExtentsMeters = halfExtentsMeters;
  collider.bounds = aabbFromCenterExtents(centerMeters, halfExtentsMeters);
  collider.sensor = sensor;
  return collider;
}

std::vector<PhysicsAabbCollider> tinySeparatedColliders() {
  return {
      colliderAt({1U}, {0.0F, 0.0F, 0.0F}, {0.25F, 0.25F, 0.25F}),
      colliderAt({2U}, {4.0F, 0.0F, 0.0F}, {0.25F, 0.25F, 0.25F}),
      colliderAt({3U}, {0.0F, 4.0F, 0.0F}, {0.25F, 0.25F, 0.25F}),
  };
}

std::vector<PhysicsAabbCollider> denseOverlapColliders() {
  return {
      colliderAt({1U}, {0.0F, 0.0F, 0.0F}, {1.25F, 1.25F, 1.25F}),
      colliderAt({2U}, {0.45F, 0.0F, 0.0F}, {1.25F, 1.25F, 1.25F}),
      colliderAt({3U}, {0.15F, 0.35F, 0.0F}, {1.25F, 1.25F, 1.25F}),
  };
}

std::vector<PhysicsAabbCollider> gridLineCorridorColliders() {
  std::vector<PhysicsAabbCollider> colliders;
  colliders.reserve(24U);
  for (std::uint32_t index = 0U; index < 24U; ++index) {
    colliders.push_back(colliderAt({100U + index},
                                   {static_cast<float>(index) * 2.0F, 0.0F,
                                    0.0F},
                                   {0.25F, 0.25F, 0.25F}));
  }
  return colliders;
}

std::vector<PhysicsAabbCollider> denseCluster16Colliders() {
  std::vector<PhysicsAabbCollider> colliders;
  colliders.reserve(16U);
  for (std::uint32_t row = 0U; row < 4U; ++row) {
    for (std::uint32_t column = 0U; column < 4U; ++column) {
      const float x = (static_cast<float>(column) - 1.5F) * 0.25F;
      const float z = (static_cast<float>(row) - 1.5F) * 0.25F;
      colliders.push_back(colliderAt({200U + row * 4U + column},
                                     {x, 0.0F, z},
                                     {0.75F, 0.75F, 0.75F}));
    }
  }
  return colliders;
}

std::vector<PhysicsAabbCollider> wallSlideColliders() {
  return {
      colliderAt({2U}, {1.0F, 0.0F, 0.0F}, {0.10F, 1.0F, 1.0F}),
  };
}

std::vector<PhysicsAabbCollider> cornerSlideColliders() {
  return {
      colliderAt({2U}, {1.0F, 0.0F, 0.0F}, {0.10F, 1.0F, 1.0F}),
      colliderAt({3U}, {0.0F, 0.0F, 1.0F}, {1.0F, 1.0F, 0.10F}),
  };
}

RoomSpatialSurface floorSurface(std::string id,
                                float minX,
                                float maxX,
                                float minZ,
                                float maxZ) {
  RoomSpatialSurface surface;
  surface.id = std::move(id);
  surface.shape = RoomSpatialSurfaceShape::Plane;
  surface.role = RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = {
      {minX, 0.0F, minZ},
      {maxX, 0.0F, minZ},
      {maxX, 0.0F, maxZ},
      {minX, 0.0F, maxZ},
  };
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.runtimeOwnerStableName = surface.id;
  return surface;
}

RoomSpatialSurface wallSurface(std::string id,
                               Vec3 minMeters,
                               Vec3 maxMeters,
                               Vec3 normal) {
  RoomSpatialSurface surface;
  surface.id = std::move(id);
  surface.shape = RoomSpatialSurfaceShape::Box;
  surface.role = RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = {minMeters, maxMeters};
  surface.normal = normal;
  surface.collisionMask = {"actor"};
  surface.blocksActor = true;
  surface.runtimeOwnerStableName = surface.id;
  return surface;
}

RoomAsset roomFloorWallAsset() {
  RoomAsset room;
  room.id = "benchmark_room_floor_wall";
  room.units = "meters";
  room.source = "physics_kernel_benchmark";
  room.spatialSurfaces.push_back(floorSurface("floor", -3.0F, 3.0F,
                                              -3.0F, 3.0F));
  room.spatialSurfaces.push_back(wallSurface(
      "front_wall", {1.10F, 0.0F, -2.0F}, {1.30F, 2.0F, 2.0F},
      {-1.0F, 0.0F, 0.0F}));
  return room;
}

RoomAsset roomLongCorridorAsset() {
  RoomAsset room;
  room.id = "benchmark_room_long_corridor";
  room.units = "meters";
  room.source = "physics_kernel_benchmark";
  room.spatialSurfaces.push_back(floorSurface("corridor_floor", -1.0F,
                                              17.0F, -1.5F, 1.5F));
  for (std::uint32_t segment = 0U; segment < 8U; ++segment) {
    const float minX = static_cast<float>(segment) * 2.0F;
    const float maxX = minX + 1.75F;
    room.spatialSurfaces.push_back(wallSurface(
        "corridor_left_" + std::to_string(segment),
        {minX, 0.0F, -1.45F}, {maxX, 2.0F, -1.25F},
        {0.0F, 0.0F, 1.0F}));
    room.spatialSurfaces.push_back(wallSurface(
        "corridor_right_" + std::to_string(segment),
        {minX, 0.0F, 1.25F}, {maxX, 2.0F, 1.45F},
        {0.0F, 0.0F, -1.0F}));
  }
  return room;
}

RoomAsset roomDenseWallsAsset() {
  RoomAsset room;
  room.id = "benchmark_room_dense_walls";
  room.units = "meters";
  room.source = "physics_kernel_benchmark";
  room.spatialSurfaces.push_back(floorSurface("dense_floor", -2.5F, 2.5F,
                                              -2.5F, 2.5F));
  room.spatialSurfaces.push_back(wallSurface(
      "dense_east_wall", {1.05F, 0.0F, -2.0F}, {1.25F, 2.0F, 2.0F},
      {-1.0F, 0.0F, 0.0F}));
  room.spatialSurfaces.push_back(wallSurface(
      "dense_north_wall", {-2.0F, 0.0F, 1.05F}, {2.0F, 2.0F, 1.25F},
      {0.0F, 0.0F, -1.0F}));
  room.spatialSurfaces.push_back(wallSurface(
      "dense_west_wall", {-1.25F, 0.0F, -2.0F}, {-1.05F, 2.0F, 2.0F},
      {1.0F, 0.0F, 0.0F}));
  room.spatialSurfaces.push_back(wallSurface(
      "dense_south_wall", {-2.0F, 0.0F, -1.25F}, {2.0F, 2.0F, -1.05F},
      {0.0F, 0.0F, 1.0F}));
  room.spatialSurfaces.push_back(wallSurface(
      "dense_inner_pillar", {0.35F, 0.0F, 0.35F}, {0.55F, 1.5F, 0.55F},
      {-1.0F, 0.0F, 0.0F}));
  return room;
}

PhysicsBroadphaseResult runBroadphase(
    const std::vector<PhysicsAabbCollider>& colliders,
    float cellSizeMeters) {
  PhysicsBroadphaseRequest request;
  request.colliders = &colliders;
  request.cellSizeMeters = cellSizeMeters;
  return collectPhysicsBroadphasePairs(request);
}

void copyBroadphaseCounters(PhysicsKernelBenchmarkCaseResult& result,
                            const PhysicsBroadphaseResult& broadphase) {
  result.colliderCount = broadphase.colliderCount;
  result.broadphaseCandidatePairCount += broadphase.candidatePairCount;
  result.broadphaseTestedPairCount += broadphase.testedPairCount;
  result.broadphaseDuplicatePairRejectedCount +=
      broadphase.duplicatePairRejectedCount;
  result.broadphaseOverlappingPairCount += broadphase.overlappingPairCount;
}

bool addContactCounters(PhysicsKernelBenchmarkCaseResult& result,
                        const PhysicsAabbContactResult& contact) {
  // branch-gate: BG-1116
  if (!contact.ok) {
    result.ok = false;
    result.status = PhysicsKernelBenchmarkStatus::KernelFailed;
    result.reasonCode = physicsKernelBenchmarkStatusName(result.status);
    result.upstreamReasonCode = contact.reasonCode;
    return false;
  }
  ++result.contactCount;
  result.maxPenetrationMeters =
      std::max(result.maxPenetrationMeters, contact.contact.penetrationMeters);
  return true;
}

PhysicsBodyView bodyView(PhysicsBodyId id,
                         PhysicsBodyMotionKind motion,
                         Vec3 velocityMetersPerSecond,
                         float inverseMass) {
  PhysicsBodyView body;
  body.id = id;
  body.motion = motion;
  body.positionMeters = {};
  body.velocityMetersPerSecond = velocityMetersPerSecond;
  // branch-gate: BG-1116
  if (inverseMass > 0.0F) {
    body.massKilograms = 1.0F / inverseMass;
  }
  body.inverseMass = inverseMass;
  return body;
}

PhysicsMaterialPairTraits solverMaterialPair() {
  PhysicsMaterialPairTraits pair;
  pair.firstMaterialId = {1U};
  pair.secondMaterialId = {2U};
  pair.staticFriction = 0.50F;
  pair.dynamicFriction = 0.50F;
  pair.restitution = 0.20F;
  pair.dampingMultiplier = 1.0F;
  pair.solveContact = true;
  return pair;
}

void addSolveCounters(PhysicsKernelBenchmarkCaseResult& result,
                      const PhysicsAabbContactSolvePlan& plan) {
  ++result.solvePlanCount;
  // branch-gate: BG-1116
  if (plan.positionCorrectionApplied) {
    ++result.positionCorrectionAppliedCount;
  }
  // branch-gate: BG-1116
  if (plan.velocityImpulseApplied) {
    ++result.velocityImpulseAppliedCount;
  }
  // branch-gate: BG-1116
  if (plan.frictionImpulseApplied) {
    ++result.frictionImpulseAppliedCount;
  }
  result.totalNormalImpulse += plan.normalImpulseMagnitude;
  result.totalFrictionImpulse += plan.frictionImpulseMagnitude;
}

void copySpatialBakeCounters(
    PhysicsKernelBenchmarkCaseResult& result,
    const PhysicsSpatialSurfaceColliderBakeResult& bake) {
  result.surfaceCount = bake.surfaceCount;
  result.bakedColliderCount = bake.colliderCount;
  result.skippedSurfaceCount = bake.skippedSurfaceCount;
  result.colliderCount = bake.colliderCount;
}

void copyPlayerPlannerCounters(
    PhysicsKernelBenchmarkCaseResult& result,
    const PlayerPhysicsMovePlannerResult& plan) {
  result.surfaceCount = plan.bakedSurfaceCount;
  result.bakedColliderCount = plan.bakedColliderCount;
  result.skippedSurfaceCount = plan.skippedSurfaceCount;
  result.colliderCount = plan.bakedColliderCount;
  result.playerPlannerHitCount = plan.hitCount;
  result.playerPlannerIterationCount = plan.iterationCount;
}

PhysicsKernelBenchmarkCaseResult failedSpatialBakeResult(
    PhysicsKernelBenchmarkCaseResult result,
    std::string_view upstreamReasonCode) {
  result.ok = false;
  result.status = PhysicsKernelBenchmarkStatus::KernelFailed;
  result.reasonCode = physicsKernelBenchmarkStatusName(result.status);
  result.upstreamReasonCode = upstreamReasonCode;
  return result;
}

PhysicsSpatialSurfaceColliderBakeResult bakeRoomSurfaces(
    const SpatialSurfaceSet& surfaces) {
  PhysicsSpatialSurfaceColliderBakeRequest request;
  request.surfaces = &surfaces;
  return bakePhysicsAabbCollidersFromSpatialSurfaces(request);
}

PhysicsKernelBenchmarkCaseResult broadphaseTinySeparated(
    const PhysicsKernelBenchmarkConfig& config) {
  PhysicsKernelBenchmarkCaseResult result =
      readyResult(PhysicsKernelBenchmarkKernel::BroadphaseGrid,
                  PhysicsKernelBenchmarkScenario::TinySeparated, config);
  const std::vector<PhysicsAabbCollider> colliders = tinySeparatedColliders();
  const PhysicsBroadphaseResult broadphase =
      runBroadphase(colliders, config.broadphaseCellSizeMeters);
  // branch-gate: BG-1116
  if (!broadphase.ok) {
    result.ok = false;
    result.status = PhysicsKernelBenchmarkStatus::KernelFailed;
    result.reasonCode = physicsKernelBenchmarkStatusName(result.status);
    result.upstreamReasonCode = broadphase.reasonCode;
    return result;
  }
  copyBroadphaseCounters(result, broadphase);
  return result;
}

PhysicsKernelBenchmarkCaseResult broadphaseDenseOverlap(
    const PhysicsKernelBenchmarkConfig& config) {
  PhysicsKernelBenchmarkCaseResult result =
      readyResult(PhysicsKernelBenchmarkKernel::BroadphaseGrid,
                  PhysicsKernelBenchmarkScenario::DenseOverlap, config);
  const std::vector<PhysicsAabbCollider> colliders = denseOverlapColliders();
  const PhysicsBroadphaseResult broadphase =
      runBroadphase(colliders, config.broadphaseCellSizeMeters);
  // branch-gate: BG-1116
  if (!broadphase.ok) {
    result.ok = false;
    result.status = PhysicsKernelBenchmarkStatus::KernelFailed;
    result.reasonCode = physicsKernelBenchmarkStatusName(result.status);
    result.upstreamReasonCode = broadphase.reasonCode;
    return result;
  }
  copyBroadphaseCounters(result, broadphase);
  return result;
}

PhysicsKernelBenchmarkCaseResult contactDenseOverlap(
    const PhysicsKernelBenchmarkConfig& config) {
  PhysicsKernelBenchmarkCaseResult result =
      readyResult(PhysicsKernelBenchmarkKernel::AabbContact,
                  PhysicsKernelBenchmarkScenario::DenseOverlap, config);
  const std::vector<PhysicsAabbCollider> colliders = denseOverlapColliders();
  const PhysicsBroadphaseResult broadphase =
      runBroadphase(colliders, config.broadphaseCellSizeMeters);
  // branch-gate: BG-1116
  if (!broadphase.ok) {
    result.ok = false;
    result.status = PhysicsKernelBenchmarkStatus::KernelFailed;
    result.reasonCode = physicsKernelBenchmarkStatusName(result.status);
    result.upstreamReasonCode = broadphase.reasonCode;
    return result;
  }
  copyBroadphaseCounters(result, broadphase);
  for (const PhysicsBroadphasePair& pair : broadphase.pairs) {
    PhysicsAabbContactRequest request;
    request.colliders = &colliders;
    request.pair = &pair;
    const PhysicsAabbContactResult contact =
        generatePhysicsAabbContact(request);
    // branch-gate: BG-1116
    if (!addContactCounters(result, contact)) {
      return result;
    }
  }
  return result;
}

PhysicsKernelBenchmarkCaseResult broadphaseGridLineCorridor(
    const PhysicsKernelBenchmarkConfig& config) {
  PhysicsKernelBenchmarkCaseResult result =
      readyResult(PhysicsKernelBenchmarkKernel::BroadphaseGrid,
                  PhysicsKernelBenchmarkScenario::GridLineCorridor, config);
  const std::vector<PhysicsAabbCollider> colliders =
      gridLineCorridorColliders();
  const PhysicsBroadphaseResult broadphase =
      runBroadphase(colliders, config.broadphaseCellSizeMeters);
  // branch-gate: BG-1116
  if (!broadphase.ok) {
    result.ok = false;
    result.status = PhysicsKernelBenchmarkStatus::KernelFailed;
    result.reasonCode = physicsKernelBenchmarkStatusName(result.status);
    result.upstreamReasonCode = broadphase.reasonCode;
    return result;
  }
  copyBroadphaseCounters(result, broadphase);
  return result;
}

PhysicsKernelBenchmarkCaseResult broadphaseDenseCluster16(
    const PhysicsKernelBenchmarkConfig& config) {
  PhysicsKernelBenchmarkCaseResult result =
      readyResult(PhysicsKernelBenchmarkKernel::BroadphaseGrid,
                  PhysicsKernelBenchmarkScenario::DenseCluster16, config);
  const std::vector<PhysicsAabbCollider> colliders =
      denseCluster16Colliders();
  const PhysicsBroadphaseResult broadphase =
      runBroadphase(colliders, config.broadphaseCellSizeMeters);
  // branch-gate: BG-1116
  if (!broadphase.ok) {
    result.ok = false;
    result.status = PhysicsKernelBenchmarkStatus::KernelFailed;
    result.reasonCode = physicsKernelBenchmarkStatusName(result.status);
    result.upstreamReasonCode = broadphase.reasonCode;
    return result;
  }
  copyBroadphaseCounters(result, broadphase);
  return result;
}

PhysicsKernelBenchmarkCaseResult contactDenseCluster16(
    const PhysicsKernelBenchmarkConfig& config) {
  PhysicsKernelBenchmarkCaseResult result =
      readyResult(PhysicsKernelBenchmarkKernel::AabbContact,
                  PhysicsKernelBenchmarkScenario::DenseCluster16, config);
  const std::vector<PhysicsAabbCollider> colliders =
      denseCluster16Colliders();
  const PhysicsBroadphaseResult broadphase =
      runBroadphase(colliders, config.broadphaseCellSizeMeters);
  // branch-gate: BG-1116
  if (!broadphase.ok) {
    result.ok = false;
    result.status = PhysicsKernelBenchmarkStatus::KernelFailed;
    result.reasonCode = physicsKernelBenchmarkStatusName(result.status);
    result.upstreamReasonCode = broadphase.reasonCode;
    return result;
  }
  copyBroadphaseCounters(result, broadphase);
  for (const PhysicsBroadphasePair& pair : broadphase.pairs) {
    PhysicsAabbContactRequest request;
    request.colliders = &colliders;
    request.pair = &pair;
    const PhysicsAabbContactResult contact =
        generatePhysicsAabbContact(request);
    // branch-gate: BG-1116
    if (!addContactCounters(result, contact)) {
      return result;
    }
  }
  return result;
}

PhysicsKernelBenchmarkCaseResult solverDenseOverlap(
    const PhysicsKernelBenchmarkConfig& config) {
  PhysicsKernelBenchmarkCaseResult result =
      readyResult(PhysicsKernelBenchmarkKernel::AabbContactSolver,
                  PhysicsKernelBenchmarkScenario::DenseOverlap, config);
  const std::vector<PhysicsAabbCollider> colliders{
      colliderAt({1U}, {0.0F, 0.0F, 0.0F}, {0.50F, 0.50F, 0.50F}),
      colliderAt({2U}, {0.60F, 0.0F, 0.0F}, {0.50F, 0.50F, 0.50F}),
  };
  const PhysicsBroadphaseResult broadphase =
      runBroadphase(colliders, config.broadphaseCellSizeMeters);
  // branch-gate: BG-1116
  if (!broadphase.ok || broadphase.pairs.empty()) {
    result.ok = false;
    result.status = PhysicsKernelBenchmarkStatus::KernelFailed;
    result.reasonCode = physicsKernelBenchmarkStatusName(result.status);
    result.upstreamReasonCode =
        broadphase.ok ? "physics_kernel_benchmark_no_pairs"
                      : broadphase.reasonCode;
    return result;
  }
  copyBroadphaseCounters(result, broadphase);

  const PhysicsBroadphasePair& pair = broadphase.pairs.front();
  PhysicsAabbContactRequest contactRequest;
  contactRequest.colliders = &colliders;
  contactRequest.pair = &pair;
  const PhysicsAabbContactResult contact =
      generatePhysicsAabbContact(contactRequest);
  // branch-gate: BG-1116
  if (!addContactCounters(result, contact)) {
    return result;
  }

  const PhysicsBodyView first =
      bodyView({1U}, PhysicsBodyMotionKind::Dynamic, {2.0F, 0.0F, 1.0F},
               1.0F);
  const PhysicsBodyView second =
      bodyView({2U}, PhysicsBodyMotionKind::Static, {}, 0.0F);
  const PhysicsMaterialPairTraits materialPair = solverMaterialPair();
  PhysicsAabbContactSolveConfig solveConfig;
  solveConfig.penetrationSlopMeters = 0.0F;
  solveConfig.positionCorrectionPercent = 1.0F;
  solveConfig.maxPositionCorrectionMeters = 1.0F;

  PhysicsAabbContactSolveRequest solveRequest;
  solveRequest.contact = &contact.contact;
  solveRequest.firstBody = &first;
  solveRequest.secondBody = &second;
  solveRequest.materialPair = &materialPair;
  solveRequest.config = solveConfig;
  const PhysicsAabbContactSolvePlan plan =
      solvePhysicsAabbContact(solveRequest);
  // branch-gate: BG-1116
  if (!plan.ok) {
    result.ok = false;
    result.status = PhysicsKernelBenchmarkStatus::KernelFailed;
    result.reasonCode = physicsKernelBenchmarkStatusName(result.status);
    result.upstreamReasonCode = plan.reasonCode;
    return result;
  }
  addSolveCounters(result, plan);
  return result;
}

PhysicsKernelBenchmarkCaseResult kinematicMotorWallSlide(
    const PhysicsKernelBenchmarkConfig& config) {
  PhysicsKernelBenchmarkCaseResult result =
      readyResult(PhysicsKernelBenchmarkKernel::KinematicMotor,
                  PhysicsKernelBenchmarkScenario::WallSlide, config);
  const std::vector<PhysicsAabbCollider> colliders = wallSlideColliders();
  const PhysicsAabbCollider motor =
      colliderAt({1U}, {0.0F, 0.0F, 0.0F}, {0.25F, 0.50F, 0.25F});

  PhysicsKinematicMotorConfig motorConfig;
  motorConfig.maxIterations = 3U;
  motorConfig.skinMeters = 0.001F;
  motorConfig.groundProbeDistanceMeters = 0.0F;
  motorConfig.groundSnapDistanceMeters = 0.0F;
  motorConfig.maxMoveDistanceMeters = 5.0F;

  PhysicsKinematicMotorRequest request;
  request.colliders = &colliders;
  request.bodyCollider = &motor;
  request.desiredDisplacementMeters = {1.50F, 0.0F, 0.50F};
  request.config = motorConfig;
  const PhysicsKinematicMotorResult plan =
      planPhysicsKinematicAabbMove(request);
  // branch-gate: BG-1116
  if (!plan.ok) {
    result.ok = false;
    result.status = PhysicsKernelBenchmarkStatus::KernelFailed;
    result.reasonCode = physicsKernelBenchmarkStatusName(result.status);
    result.upstreamReasonCode = plan.reasonCode;
    return result;
  }

  result.colliderCount = colliders.size();
  result.kinematicIterationCount = plan.iterationCount;
  result.kinematicHitCount = plan.hitCount;
  return result;
}

PhysicsKernelBenchmarkCaseResult kinematicMotorCornerSlide(
    const PhysicsKernelBenchmarkConfig& config) {
  PhysicsKernelBenchmarkCaseResult result =
      readyResult(PhysicsKernelBenchmarkKernel::KinematicMotor,
                  PhysicsKernelBenchmarkScenario::CornerSlide, config);
  const std::vector<PhysicsAabbCollider> colliders = cornerSlideColliders();
  const PhysicsAabbCollider motor =
      colliderAt({1U}, {0.0F, 0.0F, 0.0F}, {0.25F, 0.50F, 0.25F});

  PhysicsKinematicMotorConfig motorConfig;
  motorConfig.maxIterations = 4U;
  motorConfig.skinMeters = 0.001F;
  motorConfig.groundProbeDistanceMeters = 0.0F;
  motorConfig.groundSnapDistanceMeters = 0.0F;
  motorConfig.maxMoveDistanceMeters = 5.0F;

  PhysicsKinematicMotorRequest request;
  request.colliders = &colliders;
  request.bodyCollider = &motor;
  request.desiredDisplacementMeters = {1.50F, 0.0F, 1.50F};
  request.config = motorConfig;
  const PhysicsKinematicMotorResult plan =
      planPhysicsKinematicAabbMove(request);
  // branch-gate: BG-1116
  if (!plan.ok) {
    result.ok = false;
    result.status = PhysicsKernelBenchmarkStatus::KernelFailed;
    result.reasonCode = physicsKernelBenchmarkStatusName(result.status);
    result.upstreamReasonCode = plan.reasonCode;
    return result;
  }

  result.colliderCount = colliders.size();
  result.kinematicIterationCount = plan.iterationCount;
  result.kinematicHitCount = plan.hitCount;
  return result;
}

PhysicsKernelBenchmarkCaseResult spatialSurfaceBakeRoomFloorWall(
    const PhysicsKernelBenchmarkConfig& config) {
  PhysicsKernelBenchmarkCaseResult result =
      readyResult(PhysicsKernelBenchmarkKernel::SpatialSurfaceBake,
                  PhysicsKernelBenchmarkScenario::RoomFloorWall, config);
  const SpatialSurfaceSet surfaces =
      buildSpatialSurfaceSet(roomFloorWallAsset());
  const PhysicsSpatialSurfaceColliderBakeResult bake =
      bakeRoomSurfaces(surfaces);
  // branch-gate: BG-1116
  if (!bake.ok) {
    return failedSpatialBakeResult(result, bake.reasonCode);
  }
  copySpatialBakeCounters(result, bake);
  return result;
}

PhysicsKernelBenchmarkCaseResult spatialSurfaceBakeRoomLongCorridor(
    const PhysicsKernelBenchmarkConfig& config) {
  PhysicsKernelBenchmarkCaseResult result =
      readyResult(PhysicsKernelBenchmarkKernel::SpatialSurfaceBake,
                  PhysicsKernelBenchmarkScenario::RoomLongCorridor, config);
  const SpatialSurfaceSet surfaces =
      buildSpatialSurfaceSet(roomLongCorridorAsset());
  const PhysicsSpatialSurfaceColliderBakeResult bake =
      bakeRoomSurfaces(surfaces);
  // branch-gate: BG-1116
  if (!bake.ok) {
    return failedSpatialBakeResult(result, bake.reasonCode);
  }
  copySpatialBakeCounters(result, bake);
  return result;
}

PhysicsKernelBenchmarkCaseResult spatialSurfaceBakeRoomDenseWalls(
    const PhysicsKernelBenchmarkConfig& config) {
  PhysicsKernelBenchmarkCaseResult result =
      readyResult(PhysicsKernelBenchmarkKernel::SpatialSurfaceBake,
                  PhysicsKernelBenchmarkScenario::RoomDenseWalls, config);
  const SpatialSurfaceSet surfaces =
      buildSpatialSurfaceSet(roomDenseWallsAsset());
  const PhysicsSpatialSurfaceColliderBakeResult bake =
      bakeRoomSurfaces(surfaces);
  // branch-gate: BG-1116
  if (!bake.ok) {
    return failedSpatialBakeResult(result, bake.reasonCode);
  }
  copySpatialBakeCounters(result, bake);
  return result;
}

PlayerPhysicsMovePlannerConfig playerBenchmarkPlannerConfig() {
  PlayerPhysicsMovePlannerConfig config;
  config.motor.maxIterations = 4U;
  config.motor.skinMeters = 0.001F;
  config.motor.groundProbeDistanceMeters = 0.0F;
  config.motor.groundSnapDistanceMeters = 0.0F;
  config.motor.maxMoveDistanceMeters = 20.0F;
  return config;
}

PhysicsKernelBenchmarkCaseResult runPlayerMovePlannerBenchmark(
    PhysicsKernelBenchmarkScenario scenario,
    const PhysicsKernelBenchmarkConfig& config,
    const RoomAsset& room,
    Vec3 startCenterMeters,
    Vec3 desiredDisplacementMeters) {
  PhysicsKernelBenchmarkCaseResult result =
      readyResult(PhysicsKernelBenchmarkKernel::PlayerMovePlanner, scenario,
                  config);
  const SpatialSurfaceSet surfaces = buildSpatialSurfaceSet(room);
  PlayerPhysicsMovePlannerRequest request;
  request.collisionSurfaces = &surfaces;
  request.startCenterMeters = startCenterMeters;
  request.desiredDisplacementMeters = desiredDisplacementMeters;
  request.config = playerBenchmarkPlannerConfig();
  const PlayerPhysicsMovePlannerResult plan = planPlayerPhysicsMove(request);
  // branch-gate: BG-1116
  if (!plan.ok) {
    result = failedSpatialBakeResult(result, plan.upstreamReasonCode.empty()
                                                 ? plan.reasonCode
                                                 : plan.upstreamReasonCode);
    copyPlayerPlannerCounters(result, plan);
    return result;
  }
  copyPlayerPlannerCounters(result, plan);
  return result;
}

PhysicsKernelBenchmarkCaseResult playerMovePlannerRoomFloorWall(
    const PhysicsKernelBenchmarkConfig& config) {
  return runPlayerMovePlannerBenchmark(
      PhysicsKernelBenchmarkScenario::RoomFloorWall, config,
      roomFloorWallAsset(), {0.0F, 0.90F, 0.0F}, {1.50F, 0.0F, 0.25F});
}

PhysicsKernelBenchmarkCaseResult playerMovePlannerRoomLongCorridor(
    const PhysicsKernelBenchmarkConfig& config) {
  return runPlayerMovePlannerBenchmark(
      PhysicsKernelBenchmarkScenario::RoomLongCorridor, config,
      roomLongCorridorAsset(), {0.25F, 0.90F, 0.0F}, {6.0F, 0.0F, 0.0F});
}

PhysicsKernelBenchmarkCaseResult playerMovePlannerRoomDenseWalls(
    const PhysicsKernelBenchmarkConfig& config) {
  return runPlayerMovePlannerBenchmark(
      PhysicsKernelBenchmarkScenario::RoomDenseWalls, config,
      roomDenseWallsAsset(), {0.0F, 0.90F, 0.0F}, {1.50F, 0.0F, 1.50F});
}

constexpr std::array<KernelScenarioHandler, 15> kHandlers{{
    {PhysicsKernelBenchmarkKernel::BroadphaseGrid,
     PhysicsKernelBenchmarkScenario::TinySeparated, broadphaseTinySeparated},
    {PhysicsKernelBenchmarkKernel::BroadphaseGrid,
     PhysicsKernelBenchmarkScenario::DenseOverlap, broadphaseDenseOverlap},
    {PhysicsKernelBenchmarkKernel::AabbContact,
     PhysicsKernelBenchmarkScenario::DenseOverlap, contactDenseOverlap},
    {PhysicsKernelBenchmarkKernel::AabbContactSolver,
     PhysicsKernelBenchmarkScenario::DenseOverlap, solverDenseOverlap},
    {PhysicsKernelBenchmarkKernel::KinematicMotor,
     PhysicsKernelBenchmarkScenario::WallSlide, kinematicMotorWallSlide},
    {PhysicsKernelBenchmarkKernel::BroadphaseGrid,
     PhysicsKernelBenchmarkScenario::GridLineCorridor,
     broadphaseGridLineCorridor},
    {PhysicsKernelBenchmarkKernel::BroadphaseGrid,
     PhysicsKernelBenchmarkScenario::DenseCluster16,
     broadphaseDenseCluster16},
    {PhysicsKernelBenchmarkKernel::AabbContact,
     PhysicsKernelBenchmarkScenario::DenseCluster16, contactDenseCluster16},
    {PhysicsKernelBenchmarkKernel::KinematicMotor,
     PhysicsKernelBenchmarkScenario::CornerSlide, kinematicMotorCornerSlide},
    {PhysicsKernelBenchmarkKernel::SpatialSurfaceBake,
     PhysicsKernelBenchmarkScenario::RoomFloorWall,
     spatialSurfaceBakeRoomFloorWall},
    {PhysicsKernelBenchmarkKernel::SpatialSurfaceBake,
     PhysicsKernelBenchmarkScenario::RoomLongCorridor,
     spatialSurfaceBakeRoomLongCorridor},
    {PhysicsKernelBenchmarkKernel::SpatialSurfaceBake,
     PhysicsKernelBenchmarkScenario::RoomDenseWalls,
     spatialSurfaceBakeRoomDenseWalls},
    {PhysicsKernelBenchmarkKernel::PlayerMovePlanner,
     PhysicsKernelBenchmarkScenario::RoomFloorWall,
     playerMovePlannerRoomFloorWall},
    {PhysicsKernelBenchmarkKernel::PlayerMovePlanner,
     PhysicsKernelBenchmarkScenario::RoomLongCorridor,
     playerMovePlannerRoomLongCorridor},
    {PhysicsKernelBenchmarkKernel::PlayerMovePlanner,
     PhysicsKernelBenchmarkScenario::RoomDenseWalls,
     playerMovePlannerRoomDenseWalls},
}};

const KernelScenarioHandler* findHandler(
    PhysicsKernelBenchmarkKernel kernel,
    PhysicsKernelBenchmarkScenario scenario) {
  for (const KernelScenarioHandler& handler : kHandlers) {
    // branch-gate: BG-1116
    if (handler.kernel == kernel && handler.scenario == scenario) {
      return &handler;
    }
  }
  return nullptr;
}

void addIterationCounters(PhysicsKernelBenchmarkCaseResult& total,
                          const PhysicsKernelBenchmarkCaseResult& iteration) {
  total.colliderCount = iteration.colliderCount;
  total.broadphaseCandidatePairCount +=
      iteration.broadphaseCandidatePairCount;
  total.broadphaseTestedPairCount += iteration.broadphaseTestedPairCount;
  total.broadphaseDuplicatePairRejectedCount +=
      iteration.broadphaseDuplicatePairRejectedCount;
  total.broadphaseOverlappingPairCount +=
      iteration.broadphaseOverlappingPairCount;
  total.contactCount += iteration.contactCount;
  total.solvePlanCount += iteration.solvePlanCount;
  total.positionCorrectionAppliedCount +=
      iteration.positionCorrectionAppliedCount;
  total.velocityImpulseAppliedCount += iteration.velocityImpulseAppliedCount;
  total.frictionImpulseAppliedCount += iteration.frictionImpulseAppliedCount;
  total.kinematicIterationCount += iteration.kinematicIterationCount;
  total.kinematicHitCount += iteration.kinematicHitCount;
  total.surfaceCount = iteration.surfaceCount;
  total.bakedColliderCount = iteration.bakedColliderCount;
  total.skippedSurfaceCount = iteration.skippedSurfaceCount;
  total.playerPlannerHitCount += iteration.playerPlannerHitCount;
  total.playerPlannerIterationCount += iteration.playerPlannerIterationCount;
  total.maxPenetrationMeters =
      std::max(total.maxPenetrationMeters, iteration.maxPenetrationMeters);
  total.totalNormalImpulse += iteration.totalNormalImpulse;
  total.totalFrictionImpulse += iteration.totalFrictionImpulse;
}

std::uint64_t elapsedNanoseconds(std::chrono::steady_clock::time_point start,
                                 std::chrono::steady_clock::time_point end) {
  const auto elapsed =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
          .count();
  // branch-gate: BG-1116
  if (elapsed <= 0) {
    return 0U;
  }
  return static_cast<std::uint64_t>(elapsed);
}

}  // namespace

std::string_view physicsKernelBenchmarkStatusName(
    PhysicsKernelBenchmarkStatus status) {
  static constexpr std::array<std::string_view, 4> kNames{
      "physics_kernel_benchmark_ready",
      "physics_kernel_benchmark_invalid_config",
      "physics_kernel_benchmark_unknown_kernel",
      "physics_kernel_benchmark_kernel_failed",
  };
  return enumName(status, kNames, "physics_kernel_benchmark_kernel_failed");
}

std::string_view physicsKernelBenchmarkKernelName(
    PhysicsKernelBenchmarkKernel kernel) {
  static constexpr std::array<std::string_view, 6> kNames{
      "broadphase_grid",
      "aabb_contact",
      "aabb_contact_solver",
      "kinematic_motor",
      "spatial_surface_bake",
      "player_move_planner",
  };
  return enumName(kernel, kNames, "unknown_kernel");
}

std::string_view physicsKernelBenchmarkScenarioName(
    PhysicsKernelBenchmarkScenario scenario) {
  static constexpr std::array<std::string_view, 9> kNames{
      "tiny_separated",
      "dense_overlap",
      "wall_slide",
      "grid_line_corridor",
      "dense_cluster_16",
      "corner_slide",
      "room_floor_wall",
      "room_long_corridor",
      "room_dense_walls",
  };
  return enumName(scenario, kNames, "unknown_scenario");
}

bool isValidPhysicsKernelBenchmarkConfig(
    const PhysicsKernelBenchmarkConfig& config) {
  return config.iterations > 0U &&
         std::isfinite(config.broadphaseCellSizeMeters) &&
         config.broadphaseCellSizeMeters > 0.0F;
}

PhysicsKernelBenchmarkCaseResult runPhysicsKernelBenchmarkCase(
    const PhysicsKernelBenchmarkCaseRequest& request) {
  // branch-gate: BG-1116
  if (!isValidPhysicsKernelBenchmarkConfig(request.config)) {
    return caseResult(PhysicsKernelBenchmarkStatus::InvalidConfig, false,
                      request.kernel, request.scenario);
  }

  const KernelScenarioHandler* handler =
      findHandler(request.kernel, request.scenario);
  // branch-gate: BG-1116
  if (handler == nullptr) {
    PhysicsKernelBenchmarkCaseResult result =
        caseResult(PhysicsKernelBenchmarkStatus::UnknownKernel, false,
                   request.kernel, request.scenario);
    result.iterations = request.config.iterations;
    return result;
  }

  PhysicsKernelBenchmarkCaseResult result =
      readyResult(request.kernel, request.scenario, request.config);
  const auto started = std::chrono::steady_clock::now();
  for (std::uint32_t iteration = 0U; iteration < request.config.iterations;
       ++iteration) {
    const PhysicsKernelBenchmarkCaseResult iterationResult =
        handler->run(request.config);
    // branch-gate: BG-1116
    if (!iterationResult.ok) {
      result.ok = false;
      result.status = PhysicsKernelBenchmarkStatus::KernelFailed;
      result.reasonCode = physicsKernelBenchmarkStatusName(result.status);
      result.upstreamReasonCode = iterationResult.upstreamReasonCode.empty()
                                      ? iterationResult.reasonCode
                                      : iterationResult.upstreamReasonCode;
      break;
    }
    addIterationCounters(result, iterationResult);
  }
  const auto ended = std::chrono::steady_clock::now();
  // branch-gate: BG-1116
  if (request.config.collectTiming) {
    result.elapsedNanoseconds = elapsedNanoseconds(started, ended);
  }
  return result;
}

PhysicsKernelBenchmarkSuiteResult runPhysicsKernelBenchmarkSuite(
    const PhysicsKernelBenchmarkConfig& config) {
  PhysicsKernelBenchmarkSuiteResult suite;
  // branch-gate: BG-1116
  if (!isValidPhysicsKernelBenchmarkConfig(config)) {
    suite.ok = false;
    suite.status = PhysicsKernelBenchmarkStatus::InvalidConfig;
    suite.reasonCode = physicsKernelBenchmarkStatusName(suite.status);
    return suite;
  }

  suite.ok = true;
  suite.status = PhysicsKernelBenchmarkStatus::Ready;
  suite.reasonCode = physicsKernelBenchmarkStatusName(suite.status);
  for (const KernelScenarioHandler& handler : kHandlers) {
    PhysicsKernelBenchmarkCaseRequest request;
    request.kernel = handler.kernel;
    request.scenario = handler.scenario;
    request.config = config;
    PhysicsKernelBenchmarkCaseResult result =
        runPhysicsKernelBenchmarkCase(request);
    // branch-gate: BG-1116
    if (!result.ok) {
      ++suite.failedCaseCount;
      suite.ok = false;
      suite.status = result.status;
      suite.reasonCode = result.reasonCode;
    }
    suite.totalElapsedNanoseconds += result.elapsedNanoseconds;
    suite.cases.push_back(result);
  }
  suite.caseCount = suite.cases.size();
  return suite;
}

}  // namespace iggy3d
