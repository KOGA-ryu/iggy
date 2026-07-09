#include "runtime/physics/PhysicsKernelBenchmark.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::PhysicsKernelBenchmarkCaseResult run(
    iggy3d::PhysicsKernelBenchmarkKernel kernel,
    iggy3d::PhysicsKernelBenchmarkScenario scenario,
    iggy3d::PhysicsKernelBenchmarkConfig config = {}) {
  iggy3d::PhysicsKernelBenchmarkCaseRequest request;
  request.kernel = kernel;
  request.scenario = scenario;
  request.config = config;
  return iggy3d::runPhysicsKernelBenchmarkCase(request);
}

bool stableNamesAreLowerSnake() {
  return expect(iggy3d::physicsKernelBenchmarkStatusName(
                    iggy3d::PhysicsKernelBenchmarkStatus::Ready) ==
                    "physics_kernel_benchmark_ready",
                "ready status") &&
         expect(iggy3d::physicsKernelBenchmarkStatusName(
                    iggy3d::PhysicsKernelBenchmarkStatus::InvalidConfig) ==
                    "physics_kernel_benchmark_invalid_config",
                "invalid config status") &&
         expect(iggy3d::physicsKernelBenchmarkStatusName(
                    iggy3d::PhysicsKernelBenchmarkStatus::UnknownKernel) ==
                    "physics_kernel_benchmark_unknown_kernel",
                "unknown kernel status") &&
         expect(iggy3d::physicsKernelBenchmarkStatusName(
                    iggy3d::PhysicsKernelBenchmarkStatus::KernelFailed) ==
                    "physics_kernel_benchmark_kernel_failed",
                "kernel failed status") &&
         expect(iggy3d::physicsKernelBenchmarkKernelName(
                    iggy3d::PhysicsKernelBenchmarkKernel::BroadphaseGrid) ==
                    "broadphase_grid",
                "broadphase name") &&
         expect(iggy3d::physicsKernelBenchmarkKernelName(
                    iggy3d::PhysicsKernelBenchmarkKernel::AabbContact) ==
                    "aabb_contact",
                "contact name") &&
         expect(iggy3d::physicsKernelBenchmarkKernelName(
                    iggy3d::PhysicsKernelBenchmarkKernel::AabbContactSolver) ==
                    "aabb_contact_solver",
                "solver name") &&
         expect(iggy3d::physicsKernelBenchmarkKernelName(
                    iggy3d::PhysicsKernelBenchmarkKernel::KinematicMotor) ==
                    "kinematic_motor",
                "motor name") &&
         expect(iggy3d::physicsKernelBenchmarkKernelName(
                    iggy3d::PhysicsKernelBenchmarkKernel::SpatialSurfaceBake) ==
                    "spatial_surface_bake",
                "spatial surface bake name") &&
         expect(iggy3d::physicsKernelBenchmarkKernelName(
                    iggy3d::PhysicsKernelBenchmarkKernel::PlayerMovePlanner) ==
                    "player_move_planner",
                "player move planner name") &&
         expect(iggy3d::physicsKernelBenchmarkKernelName(
                    iggy3d::PhysicsKernelBenchmarkKernel::AabbRaycastFull) ==
                    "aabb_raycast_full",
                "aabb raycast full name") &&
         expect(iggy3d::physicsKernelBenchmarkKernelName(
                    iggy3d::PhysicsKernelBenchmarkKernel::AabbSegmentAnyHit) ==
                    "aabb_segment_any_hit",
                "aabb segment any hit name") &&
         expect(iggy3d::physicsKernelBenchmarkScenarioName(
                    iggy3d::PhysicsKernelBenchmarkScenario::TinySeparated) ==
                    "tiny_separated",
                "tiny scenario") &&
         expect(iggy3d::physicsKernelBenchmarkScenarioName(
                    iggy3d::PhysicsKernelBenchmarkScenario::DenseOverlap) ==
                    "dense_overlap",
                "dense scenario") &&
         expect(iggy3d::physicsKernelBenchmarkScenarioName(
                    iggy3d::PhysicsKernelBenchmarkScenario::WallSlide) ==
                    "wall_slide",
                "wall slide scenario") &&
         expect(iggy3d::physicsKernelBenchmarkScenarioName(
                    iggy3d::PhysicsKernelBenchmarkScenario::GridLineCorridor) ==
                    "grid_line_corridor",
                "grid line corridor scenario") &&
         expect(iggy3d::physicsKernelBenchmarkScenarioName(
                    iggy3d::PhysicsKernelBenchmarkScenario::DenseCluster16) ==
                    "dense_cluster_16",
                "dense cluster 16 scenario") &&
         expect(iggy3d::physicsKernelBenchmarkScenarioName(
                    iggy3d::PhysicsKernelBenchmarkScenario::CornerSlide) ==
                    "corner_slide",
                "corner slide scenario") &&
         expect(iggy3d::physicsKernelBenchmarkScenarioName(
                    iggy3d::PhysicsKernelBenchmarkScenario::RoomFloorWall) ==
                    "room_floor_wall",
                "room floor wall scenario") &&
         expect(iggy3d::physicsKernelBenchmarkScenarioName(
                    iggy3d::PhysicsKernelBenchmarkScenario::RoomLongCorridor) ==
                    "room_long_corridor",
                "room long corridor scenario") &&
         expect(iggy3d::physicsKernelBenchmarkScenarioName(
                    iggy3d::PhysicsKernelBenchmarkScenario::RoomDenseWalls) ==
                    "room_dense_walls",
                "room dense walls scenario");
}

bool invalidConfigRejects() {
  iggy3d::PhysicsKernelBenchmarkConfig zeroIterations;
  zeroIterations.iterations = 0U;
  iggy3d::PhysicsKernelBenchmarkConfig zeroCell;
  zeroCell.broadphaseCellSizeMeters = 0.0F;
  iggy3d::PhysicsKernelBenchmarkConfig nonfiniteCell;
  nonfiniteCell.broadphaseCellSizeMeters =
      std::numeric_limits<float>::infinity();

  const iggy3d::PhysicsKernelBenchmarkCaseResult zeroIterationResult =
      run(iggy3d::PhysicsKernelBenchmarkKernel::BroadphaseGrid,
          iggy3d::PhysicsKernelBenchmarkScenario::TinySeparated,
          zeroIterations);
  const iggy3d::PhysicsKernelBenchmarkCaseResult zeroCellResult =
      run(iggy3d::PhysicsKernelBenchmarkKernel::BroadphaseGrid,
          iggy3d::PhysicsKernelBenchmarkScenario::TinySeparated, zeroCell);
  const iggy3d::PhysicsKernelBenchmarkCaseResult nonfiniteCellResult =
      run(iggy3d::PhysicsKernelBenchmarkKernel::BroadphaseGrid,
          iggy3d::PhysicsKernelBenchmarkScenario::TinySeparated,
          nonfiniteCell);

  return expect(!iggy3d::isValidPhysicsKernelBenchmarkConfig(
                    zeroIterations),
                "zero iterations invalid") &&
         expect(!zeroIterationResult.ok, "zero iterations rejected") &&
         expect(zeroIterationResult.reasonCode ==
                    "physics_kernel_benchmark_invalid_config",
                "zero iterations reason") &&
         expect(!iggy3d::isValidPhysicsKernelBenchmarkConfig(zeroCell),
                "zero cell invalid") &&
         expect(!zeroCellResult.ok, "zero cell rejected") &&
         expect(zeroCellResult.reasonCode ==
                    "physics_kernel_benchmark_invalid_config",
                "zero cell reason") &&
         expect(!iggy3d::isValidPhysicsKernelBenchmarkConfig(
                    nonfiniteCell),
                "nonfinite cell invalid") &&
         expect(!nonfiniteCellResult.ok, "nonfinite rejected") &&
         expect(nonfiniteCellResult.reasonCode ==
                    "physics_kernel_benchmark_invalid_config",
                "nonfinite reason");
}

bool timingDisabledReturnsZeroElapsed() {
  iggy3d::PhysicsKernelBenchmarkConfig config;
  config.collectTiming = false;

  const iggy3d::PhysicsKernelBenchmarkCaseResult result =
      run(iggy3d::PhysicsKernelBenchmarkKernel::BroadphaseGrid,
          iggy3d::PhysicsKernelBenchmarkScenario::TinySeparated, config);

  return expect(result.ok, "timing disabled ok") &&
         expect(result.elapsedNanoseconds == 0U, "elapsed zero") &&
         expect(result.iterations == 1U, "iteration count");
}

bool unknownKernelScenarioComboRejects() {
  const iggy3d::PhysicsKernelBenchmarkCaseResult result =
      run(iggy3d::PhysicsKernelBenchmarkKernel::KinematicMotor,
          iggy3d::PhysicsKernelBenchmarkScenario::DenseOverlap);

  return expect(!result.ok, "unknown combo rejected") &&
         expect(result.reasonCode ==
                    "physics_kernel_benchmark_unknown_kernel",
                "unknown combo reason") &&
         expect(result.kernelName == "kinematic_motor",
                "unknown combo kernel name") &&
         expect(result.scenarioName == "dense_overlap",
                "unknown combo scenario name");
}

bool broadphaseSeparatedReportsZeroOverlaps() {
  const iggy3d::PhysicsKernelBenchmarkCaseResult result =
      run(iggy3d::PhysicsKernelBenchmarkKernel::BroadphaseGrid,
          iggy3d::PhysicsKernelBenchmarkScenario::TinySeparated);

  return expect(result.ok, "separated ok") &&
         expect(result.colliderCount == 3U, "separated collider count") &&
         expect(result.broadphaseCandidatePairCount == 0U,
                "separated candidate count") &&
         expect(result.broadphaseTestedPairCount == 0U,
                "separated tested count") &&
         expect(result.broadphaseOverlappingPairCount == 0U,
                "separated overlap count") &&
         expect(result.contactCount == 0U, "separated contact count");
}

bool broadphaseDenseOverlapReportsPressure() {
  const iggy3d::PhysicsKernelBenchmarkCaseResult result =
      run(iggy3d::PhysicsKernelBenchmarkKernel::BroadphaseGrid,
          iggy3d::PhysicsKernelBenchmarkScenario::DenseOverlap);

  return expect(result.ok, "dense ok") &&
         expect(result.colliderCount == 3U, "dense collider count") &&
         expect(result.broadphaseCandidatePairCount > 0U,
                "dense candidates") &&
         expect(result.broadphaseTestedPairCount > 0U, "dense tested") &&
         expect(result.broadphaseDuplicatePairRejectedCount > 0U,
                "dense duplicates") &&
         expect(result.broadphaseOverlappingPairCount > 0U,
                "dense overlaps");
}

bool broadphaseGridLineCorridorReportsManyLowOverlapColliders() {
  const iggy3d::PhysicsKernelBenchmarkCaseResult tiny =
      run(iggy3d::PhysicsKernelBenchmarkKernel::BroadphaseGrid,
          iggy3d::PhysicsKernelBenchmarkScenario::TinySeparated);
  const iggy3d::PhysicsKernelBenchmarkCaseResult corridor =
      run(iggy3d::PhysicsKernelBenchmarkKernel::BroadphaseGrid,
          iggy3d::PhysicsKernelBenchmarkScenario::GridLineCorridor);

  return expect(tiny.ok, "tiny ok for corridor comparison") &&
         expect(corridor.ok, "corridor ok") &&
         expect(corridor.colliderCount > tiny.colliderCount,
                "corridor has more colliders than tiny") &&
         expect(corridor.broadphaseOverlappingPairCount == 0U,
                "corridor has zero overlaps") &&
         expect(corridor.contactCount == 0U, "corridor contact count");
}

bool broadphaseDenseCluster16ReportsLargerPressure() {
  const iggy3d::PhysicsKernelBenchmarkCaseResult dense =
      run(iggy3d::PhysicsKernelBenchmarkKernel::BroadphaseGrid,
          iggy3d::PhysicsKernelBenchmarkScenario::DenseOverlap);
  const iggy3d::PhysicsKernelBenchmarkCaseResult cluster =
      run(iggy3d::PhysicsKernelBenchmarkKernel::BroadphaseGrid,
          iggy3d::PhysicsKernelBenchmarkScenario::DenseCluster16);

  return expect(dense.ok, "dense ok for cluster comparison") &&
         expect(cluster.ok, "cluster ok") &&
         expect(cluster.colliderCount == 16U, "cluster collider count") &&
         expect(cluster.broadphaseCandidatePairCount >
                    dense.broadphaseCandidatePairCount,
                "cluster candidate pressure") &&
         expect(cluster.broadphaseTestedPairCount >
                    dense.broadphaseTestedPairCount,
                "cluster tested pressure") &&
         expect(cluster.broadphaseDuplicatePairRejectedCount >
                    dense.broadphaseDuplicatePairRejectedCount,
                "cluster duplicate pressure") &&
         expect(cluster.broadphaseOverlappingPairCount >
                    dense.broadphaseOverlappingPairCount,
                "cluster overlap pressure");
}

bool contactBenchmarkReportsContactsAndPenetration() {
  const iggy3d::PhysicsKernelBenchmarkCaseResult result =
      run(iggy3d::PhysicsKernelBenchmarkKernel::AabbContact,
          iggy3d::PhysicsKernelBenchmarkScenario::DenseOverlap);

  return expect(result.ok, "contact ok") &&
         expect(result.contactCount > 0U, "contact count") &&
         expect(result.maxPenetrationMeters > 0.0F,
                "contact penetration") &&
         expect(result.broadphaseOverlappingPairCount > 0U,
                "contact broadphase pairs");
}

bool contactDenseCluster16ReportsMoreContacts() {
  const iggy3d::PhysicsKernelBenchmarkCaseResult dense =
      run(iggy3d::PhysicsKernelBenchmarkKernel::AabbContact,
          iggy3d::PhysicsKernelBenchmarkScenario::DenseOverlap);
  const iggy3d::PhysicsKernelBenchmarkCaseResult cluster =
      run(iggy3d::PhysicsKernelBenchmarkKernel::AabbContact,
          iggy3d::PhysicsKernelBenchmarkScenario::DenseCluster16);

  return expect(dense.ok, "dense contact ok for cluster comparison") &&
         expect(cluster.ok, "cluster contact ok") &&
         expect(cluster.colliderCount == 16U, "cluster contact colliders") &&
         expect(cluster.contactCount > dense.contactCount,
                "cluster contact count") &&
         expect(cluster.maxPenetrationMeters > 0.0F,
                "cluster max penetration");
}

bool solverBenchmarkReportsCorrectionAndImpulses() {
  const iggy3d::PhysicsKernelBenchmarkCaseResult result =
      run(iggy3d::PhysicsKernelBenchmarkKernel::AabbContactSolver,
          iggy3d::PhysicsKernelBenchmarkScenario::DenseOverlap);

  return expect(result.ok, "solver ok") &&
         expect(result.contactCount > 0U, "solver contact count") &&
         expect(result.solvePlanCount > 0U, "solver plan count") &&
         expect(result.positionCorrectionAppliedCount > 0U,
                "solver position corrections") &&
         expect(result.velocityImpulseAppliedCount > 0U,
                "solver velocity impulses") &&
         expect(result.frictionImpulseAppliedCount > 0U,
                "solver friction impulses") &&
         expect(result.totalNormalImpulse > 0.0F,
                "solver normal impulse") &&
         expect(result.totalFrictionImpulse > 0.0F,
                "solver friction impulse");
}

bool kinematicMotorBenchmarkReportsWallHit() {
  const iggy3d::PhysicsKernelBenchmarkCaseResult result =
      run(iggy3d::PhysicsKernelBenchmarkKernel::KinematicMotor,
          iggy3d::PhysicsKernelBenchmarkScenario::WallSlide);

  return expect(result.ok, "motor ok") &&
         expect(result.colliderCount == 1U, "motor collider count") &&
         expect(result.kinematicIterationCount > 0U,
                "motor iterations") &&
         expect(result.kinematicHitCount > 0U, "motor hits");
}

bool kinematicMotorCornerSlideReportsHits() {
  const iggy3d::PhysicsKernelBenchmarkCaseResult result =
      run(iggy3d::PhysicsKernelBenchmarkKernel::KinematicMotor,
          iggy3d::PhysicsKernelBenchmarkScenario::CornerSlide);

  return expect(result.ok, "corner motor ok") &&
         expect(result.colliderCount == 2U, "corner collider count") &&
         expect(result.kinematicIterationCount > 0U,
                "corner iterations") &&
         expect(result.kinematicHitCount > 0U, "corner hits");
}

bool spatialSurfaceBakeRoomFloorWallReportsBakeCounters() {
  const iggy3d::PhysicsKernelBenchmarkCaseResult result =
      run(iggy3d::PhysicsKernelBenchmarkKernel::SpatialSurfaceBake,
          iggy3d::PhysicsKernelBenchmarkScenario::RoomFloorWall);

  return expect(result.ok, "room floor wall bake ok") &&
         expect(result.surfaceCount > 0U, "room floor wall surfaces") &&
         expect(result.bakedColliderCount > 0U,
                "room floor wall baked colliders") &&
         expect(result.colliderCount == result.bakedColliderCount,
                "room floor wall collider mirror") &&
         expect(result.playerPlannerHitCount == 0U,
                "room floor wall no planner hits");
}

bool spatialSurfaceBakeRoomLongCorridorReportsMorePressure() {
  const iggy3d::PhysicsKernelBenchmarkCaseResult floorWall =
      run(iggy3d::PhysicsKernelBenchmarkKernel::SpatialSurfaceBake,
          iggy3d::PhysicsKernelBenchmarkScenario::RoomFloorWall);
  const iggy3d::PhysicsKernelBenchmarkCaseResult corridor =
      run(iggy3d::PhysicsKernelBenchmarkKernel::SpatialSurfaceBake,
          iggy3d::PhysicsKernelBenchmarkScenario::RoomLongCorridor);

  return expect(floorWall.ok, "floor wall bake ok for corridor comparison") &&
         expect(corridor.ok, "corridor bake ok") &&
         expect(corridor.surfaceCount > floorWall.surfaceCount,
                "corridor has more surfaces") &&
         expect(corridor.bakedColliderCount > floorWall.bakedColliderCount,
                "corridor has more baked colliders") &&
         expect(corridor.colliderCount == corridor.bakedColliderCount,
                "corridor collider mirror");
}

bool spatialSurfaceBakeRoomDenseWallsReportsMoreWallPressure() {
  const iggy3d::PhysicsKernelBenchmarkCaseResult floorWall =
      run(iggy3d::PhysicsKernelBenchmarkKernel::SpatialSurfaceBake,
          iggy3d::PhysicsKernelBenchmarkScenario::RoomFloorWall);
  const iggy3d::PhysicsKernelBenchmarkCaseResult dense =
      run(iggy3d::PhysicsKernelBenchmarkKernel::SpatialSurfaceBake,
          iggy3d::PhysicsKernelBenchmarkScenario::RoomDenseWalls);

  return expect(floorWall.ok, "floor wall bake ok for dense comparison") &&
         expect(dense.ok, "dense wall bake ok") &&
         expect(dense.surfaceCount > floorWall.surfaceCount,
                "dense walls has more surfaces") &&
         expect(dense.bakedColliderCount > floorWall.bakedColliderCount,
                "dense walls has more baked colliders") &&
         expect(dense.colliderCount == dense.bakedColliderCount,
                "dense walls collider mirror");
}

bool playerMovePlannerRoomFloorWallReportsPlannerCounters() {
  const iggy3d::PhysicsKernelBenchmarkCaseResult result =
      run(iggy3d::PhysicsKernelBenchmarkKernel::PlayerMovePlanner,
          iggy3d::PhysicsKernelBenchmarkScenario::RoomFloorWall);

  return expect(result.ok, "player floor wall ok") &&
         expect(result.bakedColliderCount > 0U,
                "player floor wall baked colliders") &&
         expect(result.colliderCount == result.bakedColliderCount,
                "player floor wall collider mirror") &&
         expect(result.playerPlannerIterationCount > 0U,
                "player floor wall iterations") &&
         expect(result.playerPlannerHitCount > 0U,
                "player floor wall hit count");
}

bool playerMovePlannerRoomDenseWallsReportsPressure() {
  const iggy3d::PhysicsKernelBenchmarkCaseResult floorWall =
      run(iggy3d::PhysicsKernelBenchmarkKernel::PlayerMovePlanner,
          iggy3d::PhysicsKernelBenchmarkScenario::RoomFloorWall);
  const iggy3d::PhysicsKernelBenchmarkCaseResult dense =
      run(iggy3d::PhysicsKernelBenchmarkKernel::PlayerMovePlanner,
          iggy3d::PhysicsKernelBenchmarkScenario::RoomDenseWalls);

  return expect(floorWall.ok, "player floor wall ok for dense comparison") &&
         expect(dense.ok, "player dense wall ok") &&
         expect(dense.bakedColliderCount > floorWall.bakedColliderCount,
                "player dense wall baked colliders") &&
         expect(dense.playerPlannerIterationCount > 0U,
                "player dense wall iterations") &&
         expect(dense.playerPlannerHitCount >= floorWall.playerPlannerHitCount,
                "player dense wall hit pressure");
}

bool aabbSegmentAnyHitBenchmarkComparesFullRaycastPath() {
  iggy3d::PhysicsKernelBenchmarkConfig config;
  config.collectTiming = false;

  const iggy3d::PhysicsKernelBenchmarkCaseResult fullRaycast =
      run(iggy3d::PhysicsKernelBenchmarkKernel::AabbRaycastFull,
          iggy3d::PhysicsKernelBenchmarkScenario::GridLineCorridor, config);
  const iggy3d::PhysicsKernelBenchmarkCaseResult anyHit =
      run(iggy3d::PhysicsKernelBenchmarkKernel::AabbSegmentAnyHit,
          iggy3d::PhysicsKernelBenchmarkScenario::GridLineCorridor, config);

  return expect(fullRaycast.ok, "full raycast benchmark ok") &&
         expect(anyHit.ok, "segment any-hit benchmark ok") &&
         expect(fullRaycast.colliderCount == 24U,
                "full raycast benchmark collider count") &&
         expect(anyHit.colliderCount == fullRaycast.colliderCount,
                "segment any-hit same collider count") &&
         expect(fullRaycast.kinematicIterationCount ==
                    fullRaycast.colliderCount,
                "full raycast tests every collider") &&
         expect(anyHit.kinematicIterationCount == 0U,
                "segment any-hit exposes no tested count") &&
         expect(fullRaycast.kinematicHitCount == fullRaycast.colliderCount,
                "full raycast reports every hit") &&
         expect(anyHit.kinematicHitCount == 1U,
                "segment any-hit reports one hit") &&
         expect(fullRaycast.kinematicHitCount > anyHit.kinematicHitCount,
                "segment any-hit avoids full hit collection") &&
         expect(fullRaycast.elapsedNanoseconds == 0U,
                "full raycast timing disabled") &&
         expect(anyHit.elapsedNanoseconds == 0U,
                "segment any-hit timing disabled");
}

bool iterationCountersAccumulateDeterministically() {
  iggy3d::PhysicsKernelBenchmarkConfig config;
  config.iterations = 2U;
  config.collectTiming = false;

  const iggy3d::PhysicsKernelBenchmarkCaseResult one =
      run(iggy3d::PhysicsKernelBenchmarkKernel::BroadphaseGrid,
          iggy3d::PhysicsKernelBenchmarkScenario::DenseOverlap,
          iggy3d::PhysicsKernelBenchmarkConfig{});
  const iggy3d::PhysicsKernelBenchmarkCaseResult two =
      run(iggy3d::PhysicsKernelBenchmarkKernel::BroadphaseGrid,
          iggy3d::PhysicsKernelBenchmarkScenario::DenseOverlap, config);

  return expect(one.ok, "one iteration ok") &&
         expect(two.ok, "two iterations ok") &&
         expect(two.iterations == 2U, "two iteration field") &&
         expect(two.colliderCount == one.colliderCount,
                "collider count remains scenario size") &&
         expect(two.broadphaseCandidatePairCount ==
                    one.broadphaseCandidatePairCount * 2U,
                "candidate count accumulates") &&
         expect(two.broadphaseTestedPairCount ==
                    one.broadphaseTestedPairCount * 2U,
                "tested count accumulates") &&
         expect(two.elapsedNanoseconds == 0U,
                "two iteration timing disabled");
}

bool suiteRunsDefaultCasesInOrder() {
  iggy3d::PhysicsKernelBenchmarkConfig config;
  config.collectTiming = false;

  const iggy3d::PhysicsKernelBenchmarkSuiteResult suite =
      iggy3d::runPhysicsKernelBenchmarkSuite(config);

  return expect(suite.ok, "suite ok") &&
         expect(suite.reasonCode == "physics_kernel_benchmark_ready",
                "suite reason") &&
         expect(suite.caseCount == 15U, "suite case count") &&
         expect(suite.failedCaseCount == 0U, "suite failed count") &&
         expect(suite.totalElapsedNanoseconds == 0U,
                "suite timing disabled") &&
         expect(suite.cases.size() == 15U, "suite vector size") &&
         expect(suite.cases[0].kernelName == "broadphase_grid",
                "case 0 kernel") &&
         expect(suite.cases[0].scenarioName == "tiny_separated",
                "case 0 scenario") &&
         expect(suite.cases[1].kernelName == "broadphase_grid",
                "case 1 kernel") &&
         expect(suite.cases[1].scenarioName == "dense_overlap",
                "case 1 scenario") &&
         expect(suite.cases[2].kernelName == "aabb_contact",
                "case 2 kernel") &&
         expect(suite.cases[2].scenarioName == "dense_overlap",
                "case 2 scenario") &&
         expect(suite.cases[3].kernelName == "aabb_contact_solver",
                "case 3 kernel") &&
         expect(suite.cases[3].scenarioName == "dense_overlap",
                "case 3 scenario") &&
         expect(suite.cases[4].kernelName == "kinematic_motor",
                "case 4 kernel") &&
         expect(suite.cases[4].scenarioName == "wall_slide",
                "case 4 scenario") &&
         expect(suite.cases[5].kernelName == "broadphase_grid",
                "case 5 kernel") &&
         expect(suite.cases[5].scenarioName == "grid_line_corridor",
                "case 5 scenario") &&
         expect(suite.cases[6].kernelName == "broadphase_grid",
                "case 6 kernel") &&
         expect(suite.cases[6].scenarioName == "dense_cluster_16",
                "case 6 scenario") &&
         expect(suite.cases[7].kernelName == "aabb_contact",
                "case 7 kernel") &&
         expect(suite.cases[7].scenarioName == "dense_cluster_16",
                "case 7 scenario") &&
         expect(suite.cases[8].kernelName == "kinematic_motor",
                "case 8 kernel") &&
         expect(suite.cases[8].scenarioName == "corner_slide",
                "case 8 scenario") &&
         expect(suite.cases[9].kernelName == "spatial_surface_bake",
                "case 9 kernel") &&
         expect(suite.cases[9].scenarioName == "room_floor_wall",
                "case 9 scenario") &&
         expect(suite.cases[10].kernelName == "spatial_surface_bake",
                "case 10 kernel") &&
         expect(suite.cases[10].scenarioName == "room_long_corridor",
                "case 10 scenario") &&
         expect(suite.cases[11].kernelName == "spatial_surface_bake",
                "case 11 kernel") &&
         expect(suite.cases[11].scenarioName == "room_dense_walls",
                "case 11 scenario") &&
         expect(suite.cases[12].kernelName == "player_move_planner",
                "case 12 kernel") &&
         expect(suite.cases[12].scenarioName == "room_floor_wall",
                "case 12 scenario") &&
         expect(suite.cases[13].kernelName == "player_move_planner",
                "case 13 kernel") &&
         expect(suite.cases[13].scenarioName == "room_long_corridor",
                "case 13 scenario") &&
         expect(suite.cases[14].kernelName == "player_move_planner",
                "case 14 kernel") &&
         expect(suite.cases[14].scenarioName == "room_dense_walls",
                "case 14 scenario");
}

bool invalidSuiteConfigRejects() {
  iggy3d::PhysicsKernelBenchmarkConfig config;
  config.iterations = 0U;

  const iggy3d::PhysicsKernelBenchmarkSuiteResult suite =
      iggy3d::runPhysicsKernelBenchmarkSuite(config);

  return expect(!suite.ok, "invalid suite rejected") &&
         expect(suite.reasonCode ==
                    "physics_kernel_benchmark_invalid_config",
                "invalid suite reason") &&
         expect(suite.caseCount == 0U, "invalid suite case count") &&
         expect(suite.cases.empty(), "invalid suite cases empty");
}

}  // namespace

int main() {
  const bool ok = stableNamesAreLowerSnake() && invalidConfigRejects() &&
                  timingDisabledReturnsZeroElapsed() &&
                  unknownKernelScenarioComboRejects() &&
                  broadphaseSeparatedReportsZeroOverlaps() &&
                  broadphaseDenseOverlapReportsPressure() &&
                  broadphaseGridLineCorridorReportsManyLowOverlapColliders() &&
                  broadphaseDenseCluster16ReportsLargerPressure() &&
                  contactBenchmarkReportsContactsAndPenetration() &&
                  contactDenseCluster16ReportsMoreContacts() &&
                  solverBenchmarkReportsCorrectionAndImpulses() &&
                  kinematicMotorBenchmarkReportsWallHit() &&
                  kinematicMotorCornerSlideReportsHits() &&
                  spatialSurfaceBakeRoomFloorWallReportsBakeCounters() &&
                  spatialSurfaceBakeRoomLongCorridorReportsMorePressure() &&
                  spatialSurfaceBakeRoomDenseWallsReportsMoreWallPressure() &&
                  playerMovePlannerRoomFloorWallReportsPlannerCounters() &&
                  playerMovePlannerRoomDenseWallsReportsPressure() &&
                  aabbSegmentAnyHitBenchmarkComparesFullRaycastPath() &&
                  iterationCountersAccumulateDeterministically() &&
                  suiteRunsDefaultCasesInOrder() &&
                  invalidSuiteConfigRejects();
  if (!ok) {
    return EXIT_FAILURE;
  }
  std::cout << "physics_kernel_benchmark_tests=pass\n";
  return EXIT_SUCCESS;
}
