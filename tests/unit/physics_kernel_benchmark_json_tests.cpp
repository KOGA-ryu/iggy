#include "runtime/physics/PhysicsKernelBenchmarkJson.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::PhysicsKernelBenchmarkCaseResult representativeCase() {
  iggy3d::PhysicsKernelBenchmarkCaseResult result;
  result.ok = true;
  result.status = iggy3d::PhysicsKernelBenchmarkStatus::Ready;
  result.reasonCode = "physics_kernel_benchmark_ready";
  result.kernelName = "broadphase_grid";
  result.scenarioName = "dense_overlap";
  result.iterations = 3U;
  result.elapsedNanoseconds = 0U;
  result.colliderCount = 4U;
  result.broadphaseCandidatePairCount = 12U;
  result.broadphaseTestedPairCount = 5U;
  result.broadphaseDuplicatePairRejectedCount = 7U;
  result.broadphaseOverlappingPairCount = 3U;
  result.contactCount = 2U;
  result.solvePlanCount = 1U;
  result.positionCorrectionAppliedCount = 1U;
  result.velocityImpulseAppliedCount = 1U;
  result.frictionImpulseAppliedCount = 1U;
  result.kinematicIterationCount = 0U;
  result.kinematicHitCount = 0U;
  result.maxPenetrationMeters = 1.25F;
  result.totalNormalImpulse = 2.5F;
  result.totalFrictionImpulse = 0.125F;
  return result;
}

iggy3d::PhysicsKernelBenchmarkCaseResult failedCase(
    std::string_view upstream) {
  iggy3d::PhysicsKernelBenchmarkCaseResult result;
  result.ok = false;
  result.status = iggy3d::PhysicsKernelBenchmarkStatus::KernelFailed;
  result.reasonCode = "physics_kernel_benchmark_kernel_failed";
  result.kernelName = "aabb_contact_solver";
  result.scenarioName = "dense_overlap";
  result.iterations = 1U;
  result.elapsedNanoseconds = 9U;
  result.upstreamReasonCode = upstream;
  return result;
}

bool exactCaseJsonMatchesStableShape() {
  const iggy3d::PhysicsKernelBenchmarkCaseResult result =
      representativeCase();
  const std::string json =
      iggy3d::physicsKernelBenchmarkCaseToJson(result);
  const std::string expected = R"json({
  "schema": "iggy3d.physics_kernel_benchmark.case.v1",
  "ok": true,
  "status": "physics_kernel_benchmark_ready",
  "reason_code": "physics_kernel_benchmark_ready",
  "kernel": "broadphase_grid",
  "scenario": "dense_overlap",
  "iterations": 3,
  "elapsed_nanoseconds": 0,
  "counters": {
    "collider_count": 4,
    "broadphase_candidate_pair_count": 12,
    "broadphase_tested_pair_count": 5,
    "broadphase_duplicate_pair_rejected_count": 7,
    "broadphase_overlapping_pair_count": 3,
    "contact_count": 2,
    "solve_plan_count": 1,
    "position_correction_applied_count": 1,
    "velocity_impulse_applied_count": 1,
    "friction_impulse_applied_count": 1,
    "kinematic_iteration_count": 0,
    "kinematic_hit_count": 0
  },
  "extrema": {
    "max_penetration_meters": 1.25,
    "total_normal_impulse": 2.5,
    "total_friction_impulse": 0.125
  },
  "upstream_reason_code": ""
}
)json";

  return expect(json == expected, "case json exact shape");
}

bool exactSuiteJsonMatchesStableShapeAndEscapesStrings() {
  const std::string upstream = "bad \"quote\" \\ path\n\t";
  iggy3d::PhysicsKernelBenchmarkSuiteResult suite;
  suite.ok = false;
  suite.status = iggy3d::PhysicsKernelBenchmarkStatus::KernelFailed;
  suite.reasonCode = "physics_kernel_benchmark_kernel_failed";
  suite.caseCount = 2U;
  suite.failedCaseCount = 1U;
  suite.totalElapsedNanoseconds = 9U;
  suite.cases.push_back(representativeCase());
  suite.cases.push_back(failedCase(upstream));

  const std::string json =
      iggy3d::physicsKernelBenchmarkSuiteToJson(suite);
  const std::string expected = R"json({
  "schema": "iggy3d.physics_kernel_benchmark.suite.v1",
  "ok": false,
  "status": "physics_kernel_benchmark_kernel_failed",
  "reason_code": "physics_kernel_benchmark_kernel_failed",
  "case_count": 2,
  "failed_case_count": 1,
  "total_elapsed_nanoseconds": 9,
  "cases": [
    {
      "schema": "iggy3d.physics_kernel_benchmark.case.v1",
      "ok": true,
      "status": "physics_kernel_benchmark_ready",
      "reason_code": "physics_kernel_benchmark_ready",
      "kernel": "broadphase_grid",
      "scenario": "dense_overlap",
      "iterations": 3,
      "elapsed_nanoseconds": 0,
      "counters": {
        "collider_count": 4,
        "broadphase_candidate_pair_count": 12,
        "broadphase_tested_pair_count": 5,
        "broadphase_duplicate_pair_rejected_count": 7,
        "broadphase_overlapping_pair_count": 3,
        "contact_count": 2,
        "solve_plan_count": 1,
        "position_correction_applied_count": 1,
        "velocity_impulse_applied_count": 1,
        "friction_impulse_applied_count": 1,
        "kinematic_iteration_count": 0,
        "kinematic_hit_count": 0
      },
      "extrema": {
        "max_penetration_meters": 1.25,
        "total_normal_impulse": 2.5,
        "total_friction_impulse": 0.125
      },
      "upstream_reason_code": ""
    },
    {
      "schema": "iggy3d.physics_kernel_benchmark.case.v1",
      "ok": false,
      "status": "physics_kernel_benchmark_kernel_failed",
      "reason_code": "physics_kernel_benchmark_kernel_failed",
      "kernel": "aabb_contact_solver",
      "scenario": "dense_overlap",
      "iterations": 1,
      "elapsed_nanoseconds": 9,
      "counters": {
        "collider_count": 0,
        "broadphase_candidate_pair_count": 0,
        "broadphase_tested_pair_count": 0,
        "broadphase_duplicate_pair_rejected_count": 0,
        "broadphase_overlapping_pair_count": 0,
        "contact_count": 0,
        "solve_plan_count": 0,
        "position_correction_applied_count": 0,
        "velocity_impulse_applied_count": 0,
        "friction_impulse_applied_count": 0,
        "kinematic_iteration_count": 0,
        "kinematic_hit_count": 0
      },
      "extrema": {
        "max_penetration_meters": 0.0,
        "total_normal_impulse": 0.0,
        "total_friction_impulse": 0.0
      },
      "upstream_reason_code": "bad \"quote\" \\ path\n\t"
    }
  ]
}
)json";

  return expect(json == expected, "suite json exact shape and escapes");
}

bool controlCharactersAreEscaped() {
  const std::string upstream =
      std::string("control") + static_cast<char>(1) + "end";
  const iggy3d::PhysicsKernelBenchmarkCaseResult result =
      failedCase(upstream);
  const std::string json =
      iggy3d::physicsKernelBenchmarkCaseToJson(result);

  return expect(json.find("control\\u0001end") != std::string::npos,
                "control char escaped");
}

bool realSuiteSerializesWithExpectedSchemaStrings() {
  iggy3d::PhysicsKernelBenchmarkConfig config;
  config.collectTiming = false;

  const std::string json =
      iggy3d::physicsKernelBenchmarkSuiteToJson(config);

  return expect(json.find(
                    "\"schema\": "
                    "\"iggy3d.physics_kernel_benchmark.suite.v1\"") !=
                    std::string::npos,
                "suite schema present") &&
         expect(json.find("\"case_count\": 5") != std::string::npos,
                "case count present") &&
         expect(json.find("\"kernel\": \"broadphase_grid\"") !=
                    std::string::npos,
                "broadphase case present") &&
         expect(json.find("\"kernel\": \"kinematic_motor\"") !=
                    std::string::npos,
                "motor case present") &&
         expect(json.find("\"elapsed_nanoseconds\": 0") !=
                    std::string::npos,
                "timing disabled present");
}

bool deterministicAndDoesNotMutateSource() {
  iggy3d::PhysicsKernelBenchmarkCaseResult result = representativeCase();
  const iggy3d::PhysicsKernelBenchmarkCaseResult before = result;

  const std::string first =
      iggy3d::physicsKernelBenchmarkCaseToJson(result);
  const std::string second =
      iggy3d::physicsKernelBenchmarkCaseToJson(result);

  return expect(first == second, "deterministic output") &&
         expect(result.ok == before.ok, "ok not mutated") &&
         expect(result.status == before.status, "status not mutated") &&
         expect(result.reasonCode == before.reasonCode,
                "reason not mutated") &&
         expect(result.colliderCount == before.colliderCount,
                "counter not mutated") &&
         expect(result.totalNormalImpulse == before.totalNormalImpulse,
                "float not mutated");
}

}  // namespace

int main() {
  const bool ok = exactCaseJsonMatchesStableShape() &&
                  exactSuiteJsonMatchesStableShapeAndEscapesStrings() &&
                  controlCharactersAreEscaped() &&
                  realSuiteSerializesWithExpectedSchemaStrings() &&
                  deterministicAndDoesNotMutateSource();
  if (!ok) {
    return EXIT_FAILURE;
  }
  std::cout << "physics_kernel_benchmark_json_tests=pass\n";
  return EXIT_SUCCESS;
}
