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
                "wall slide scenario");
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
         expect(suite.caseCount == 5U, "suite case count") &&
         expect(suite.failedCaseCount == 0U, "suite failed count") &&
         expect(suite.totalElapsedNanoseconds == 0U,
                "suite timing disabled") &&
         expect(suite.cases.size() == 5U, "suite vector size") &&
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
                "case 4 scenario");
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
                  contactBenchmarkReportsContactsAndPenetration() &&
                  solverBenchmarkReportsCorrectionAndImpulses() &&
                  kinematicMotorBenchmarkReportsWallHit() &&
                  iterationCountersAccumulateDeterministically() &&
                  suiteRunsDefaultCasesInOrder() &&
                  invalidSuiteConfigRejects();
  if (!ok) {
    return EXIT_FAILURE;
  }
  std::cout << "physics_kernel_benchmark_tests=pass\n";
  return EXIT_SUCCESS;
}
