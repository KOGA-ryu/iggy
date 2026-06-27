#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace iggy3d {

enum class PhysicsKernelBenchmarkStatus : std::uint8_t {
  Ready,
  InvalidConfig,
  UnknownKernel,
  KernelFailed,
};

enum class PhysicsKernelBenchmarkKernel : std::uint8_t {
  BroadphaseGrid,
  AabbContact,
  AabbContactSolver,
  KinematicMotor,
  SpatialSurfaceBake,
  PlayerMovePlanner,
};

enum class PhysicsKernelBenchmarkScenario : std::uint8_t {
  TinySeparated,
  DenseOverlap,
  WallSlide,
  GridLineCorridor,
  DenseCluster16,
  CornerSlide,
  RoomFloorWall,
  RoomLongCorridor,
  RoomDenseWalls,
};

struct PhysicsKernelBenchmarkConfig {
  std::uint32_t iterations = 1U;
  bool collectTiming = true;
  float broadphaseCellSizeMeters = 1.0F;
};

struct PhysicsKernelBenchmarkCaseRequest {
  PhysicsKernelBenchmarkKernel kernel =
      PhysicsKernelBenchmarkKernel::BroadphaseGrid;
  PhysicsKernelBenchmarkScenario scenario =
      PhysicsKernelBenchmarkScenario::TinySeparated;
  PhysicsKernelBenchmarkConfig config;
};

struct PhysicsKernelBenchmarkCaseResult {
  bool ok = false;
  PhysicsKernelBenchmarkStatus status =
      PhysicsKernelBenchmarkStatus::InvalidConfig;
  std::string_view reasonCode = "physics_kernel_benchmark_invalid_config";
  std::string_view kernelName = "broadphase_grid";
  std::string_view scenarioName = "tiny_separated";
  std::uint32_t iterations = 0U;
  std::uint64_t elapsedNanoseconds = 0U;
  std::uint64_t colliderCount = 0U;
  std::uint64_t broadphaseCandidatePairCount = 0U;
  std::uint64_t broadphaseTestedPairCount = 0U;
  std::uint64_t broadphaseDuplicatePairRejectedCount = 0U;
  std::uint64_t broadphaseOverlappingPairCount = 0U;
  std::uint64_t contactCount = 0U;
  std::uint64_t solvePlanCount = 0U;
  std::uint64_t positionCorrectionAppliedCount = 0U;
  std::uint64_t velocityImpulseAppliedCount = 0U;
  std::uint64_t frictionImpulseAppliedCount = 0U;
  std::uint64_t kinematicIterationCount = 0U;
  std::uint64_t kinematicHitCount = 0U;
  std::uint64_t surfaceCount = 0U;
  std::uint64_t bakedColliderCount = 0U;
  std::uint64_t skippedSurfaceCount = 0U;
  std::uint64_t playerPlannerHitCount = 0U;
  std::uint64_t playerPlannerIterationCount = 0U;
  float maxPenetrationMeters = 0.0F;
  float totalNormalImpulse = 0.0F;
  float totalFrictionImpulse = 0.0F;
  std::string_view upstreamReasonCode;
};

struct PhysicsKernelBenchmarkSuiteResult {
  bool ok = false;
  PhysicsKernelBenchmarkStatus status =
      PhysicsKernelBenchmarkStatus::InvalidConfig;
  std::string_view reasonCode = "physics_kernel_benchmark_invalid_config";
  std::uint64_t caseCount = 0U;
  std::uint64_t failedCaseCount = 0U;
  std::uint64_t totalElapsedNanoseconds = 0U;
  std::vector<PhysicsKernelBenchmarkCaseResult> cases;
};

std::string_view physicsKernelBenchmarkStatusName(
    PhysicsKernelBenchmarkStatus status);
std::string_view physicsKernelBenchmarkKernelName(
    PhysicsKernelBenchmarkKernel kernel);
std::string_view physicsKernelBenchmarkScenarioName(
    PhysicsKernelBenchmarkScenario scenario);

bool isValidPhysicsKernelBenchmarkConfig(
    const PhysicsKernelBenchmarkConfig& config);
PhysicsKernelBenchmarkCaseResult runPhysicsKernelBenchmarkCase(
    const PhysicsKernelBenchmarkCaseRequest& request);
PhysicsKernelBenchmarkSuiteResult runPhysicsKernelBenchmarkSuite(
    const PhysicsKernelBenchmarkConfig& config = {});

}  // namespace iggy3d
