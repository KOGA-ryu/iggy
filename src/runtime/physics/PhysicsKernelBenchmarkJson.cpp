#include "runtime/physics/PhysicsKernelBenchmarkJson.hpp"

#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

namespace iggy3d {
namespace {

void appendIndent(std::string& out, int depth) {
  for (int index = 0; index < depth; ++index) {
    out += "  ";
  }
}

void appendEscapedString(std::string& out, std::string_view value) {
  out.push_back('"');
  for (const char raw : value) {
    const auto ch = static_cast<unsigned char>(raw);
    // branch-gate: BG-1117
    switch (ch) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\b':
        out += "\\b";
        break;
      case '\f':
        out += "\\f";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        // branch-gate: BG-1117
        if (ch < 0x20U) {
          std::ostringstream escaped;
          escaped << "\\u" << std::hex << std::setw(4)
                  << std::setfill('0') << static_cast<int>(ch);
          out += escaped.str();
        } else {
          out.push_back(static_cast<char>(ch));
        }
        break;
    }
  }
  out.push_back('"');
}

std::string floatText(float value) {
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(6) << value;
  std::string text = stream.str();
  // branch-gate: BG-1117
  if (text == "-0.000000") {
    return "0.0";
  }
  while (text.size() > 2U && text.back() == '0' &&
         text[text.size() - 2U] != '.') {
    text.pop_back();
  }
  return text;
}

void appendKey(std::string& out, int depth, std::string_view key) {
  appendIndent(out, depth);
  appendEscapedString(out, key);
  out += ": ";
}

void appendStringField(std::string& out,
                       int depth,
                       std::string_view key,
                       std::string_view value,
                       bool comma = true) {
  appendKey(out, depth, key);
  appendEscapedString(out, value);
  // branch-gate: BG-1117
  if (comma) {
    out.push_back(',');
  }
  out.push_back('\n');
}

void appendBoolField(std::string& out,
                     int depth,
                     std::string_view key,
                     bool value,
                     bool comma = true) {
  appendKey(out, depth, key);
  // branch-gate: BG-1117
  if (value) {
    out += "true";
  } else {
    out += "false";
  }
  // branch-gate: BG-1117
  if (comma) {
    out.push_back(',');
  }
  out.push_back('\n');
}

void appendUintField(std::string& out,
                     int depth,
                     std::string_view key,
                     std::uint64_t value,
                     bool comma = true) {
  appendKey(out, depth, key);
  out += std::to_string(value);
  // branch-gate: BG-1117
  if (comma) {
    out.push_back(',');
  }
  out.push_back('\n');
}

void appendFloatField(std::string& out,
                      int depth,
                      std::string_view key,
                      float value,
                      bool comma = true) {
  appendKey(out, depth, key);
  out += floatText(value);
  // branch-gate: BG-1117
  if (comma) {
    out.push_back(',');
  }
  out.push_back('\n');
}

void appendCountersObject(std::string& out,
                          const PhysicsKernelBenchmarkCaseResult& result,
                          int depth) {
  appendKey(out, depth, "counters");
  out += "{\n";
  appendUintField(out, depth + 1, "collider_count", result.colliderCount);
  appendUintField(out, depth + 1, "broadphase_candidate_pair_count",
                  result.broadphaseCandidatePairCount);
  appendUintField(out, depth + 1, "broadphase_tested_pair_count",
                  result.broadphaseTestedPairCount);
  appendUintField(out, depth + 1,
                  "broadphase_duplicate_pair_rejected_count",
                  result.broadphaseDuplicatePairRejectedCount);
  appendUintField(out, depth + 1, "broadphase_overlapping_pair_count",
                  result.broadphaseOverlappingPairCount);
  appendUintField(out, depth + 1, "contact_count", result.contactCount);
  appendUintField(out, depth + 1, "solve_plan_count",
                  result.solvePlanCount);
  appendUintField(out, depth + 1, "position_correction_applied_count",
                  result.positionCorrectionAppliedCount);
  appendUintField(out, depth + 1, "velocity_impulse_applied_count",
                  result.velocityImpulseAppliedCount);
  appendUintField(out, depth + 1, "friction_impulse_applied_count",
                  result.frictionImpulseAppliedCount);
  appendUintField(out, depth + 1, "kinematic_iteration_count",
                  result.kinematicIterationCount);
  appendUintField(out, depth + 1, "kinematic_hit_count",
                  result.kinematicHitCount);
  appendUintField(out, depth + 1, "surface_count", result.surfaceCount);
  appendUintField(out, depth + 1, "baked_collider_count",
                  result.bakedColliderCount);
  appendUintField(out, depth + 1, "skipped_surface_count",
                  result.skippedSurfaceCount);
  appendUintField(out, depth + 1, "player_planner_hit_count",
                  result.playerPlannerHitCount);
  appendUintField(out, depth + 1, "player_planner_iteration_count",
                  result.playerPlannerIterationCount, false);
  appendIndent(out, depth);
  out += "},\n";
}

void appendExtremaObject(std::string& out,
                         const PhysicsKernelBenchmarkCaseResult& result,
                         int depth) {
  appendKey(out, depth, "extrema");
  out += "{\n";
  appendFloatField(out, depth + 1, "max_penetration_meters",
                   result.maxPenetrationMeters);
  appendFloatField(out, depth + 1, "total_normal_impulse",
                   result.totalNormalImpulse);
  appendFloatField(out, depth + 1, "total_friction_impulse",
                   result.totalFrictionImpulse, false);
  appendIndent(out, depth);
  out += "},\n";
}

void appendCaseObject(std::string& out,
                      const PhysicsKernelBenchmarkCaseResult& result,
                      int depth) {
  appendIndent(out, depth);
  out += "{\n";
  appendStringField(out, depth + 1, "schema",
                    "iggy3d.physics_kernel_benchmark.case.v1");
  appendBoolField(out, depth + 1, "ok", result.ok);
  appendStringField(out, depth + 1, "status",
                    physicsKernelBenchmarkStatusName(result.status));
  appendStringField(out, depth + 1, "reason_code", result.reasonCode);
  appendStringField(out, depth + 1, "kernel", result.kernelName);
  appendStringField(out, depth + 1, "scenario", result.scenarioName);
  appendUintField(out, depth + 1, "iterations", result.iterations);
  appendUintField(out, depth + 1, "elapsed_nanoseconds",
                  result.elapsedNanoseconds);
  appendCountersObject(out, result, depth + 1);
  appendExtremaObject(out, result, depth + 1);
  appendStringField(out, depth + 1, "upstream_reason_code",
                    result.upstreamReasonCode, false);
  appendIndent(out, depth);
  out.push_back('}');
}

}  // namespace

std::string physicsKernelBenchmarkCaseToJson(
    const PhysicsKernelBenchmarkCaseResult& result) {
  std::string out;
  appendCaseObject(out, result, 0);
  out.push_back('\n');
  return out;
}

std::string physicsKernelBenchmarkSuiteToJson(
    const PhysicsKernelBenchmarkSuiteResult& result) {
  std::string out;
  out += "{\n";
  appendStringField(out, 1, "schema",
                    "iggy3d.physics_kernel_benchmark.suite.v1");
  appendBoolField(out, 1, "ok", result.ok);
  appendStringField(out, 1, "status",
                    physicsKernelBenchmarkStatusName(result.status));
  appendStringField(out, 1, "reason_code", result.reasonCode);
  appendUintField(out, 1, "case_count", result.caseCount);
  appendUintField(out, 1, "failed_case_count", result.failedCaseCount);
  appendUintField(out, 1, "total_elapsed_nanoseconds",
                  result.totalElapsedNanoseconds);
  appendKey(out, 1, "cases");
  out += "[\n";
  for (std::size_t index = 0U; index < result.cases.size(); ++index) {
    appendCaseObject(out, result.cases[index], 2);
    // branch-gate: BG-1117
    if (index + 1U < result.cases.size()) {
      out.push_back(',');
    }
    out.push_back('\n');
  }
  appendIndent(out, 1);
  out += "]\n";
  out += "}\n";
  return out;
}

std::string physicsKernelBenchmarkSuiteToJson(
    const PhysicsKernelBenchmarkConfig& config) {
  return physicsKernelBenchmarkSuiteToJson(
      runPhysicsKernelBenchmarkSuite(config));
}

}  // namespace iggy3d
