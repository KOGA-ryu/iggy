#include "runtime/physics/PhysicsKernelBenchmarkJson.hpp"

#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

namespace {

enum class CliMode {
  Suite,
  Case,
};

struct NameKernel {
  std::string_view name;
  iggy3d::PhysicsKernelBenchmarkKernel kernel;
};

struct NameScenario {
  std::string_view name;
  iggy3d::PhysicsKernelBenchmarkScenario scenario;
};

struct CliConfig {
  CliMode mode = CliMode::Suite;
  bool modeExplicit = false;
  bool help = false;
  bool hasKernel = false;
  bool hasScenario = false;
  iggy3d::PhysicsKernelBenchmarkKernel kernel =
      iggy3d::PhysicsKernelBenchmarkKernel::BroadphaseGrid;
  iggy3d::PhysicsKernelBenchmarkScenario scenario =
      iggy3d::PhysicsKernelBenchmarkScenario::TinySeparated;
  iggy3d::PhysicsKernelBenchmarkConfig benchmark;
  std::filesystem::path outputPath;
};

constexpr NameKernel kKernelNames[] = {
    {"broadphase_grid", iggy3d::PhysicsKernelBenchmarkKernel::BroadphaseGrid},
    {"aabb_contact", iggy3d::PhysicsKernelBenchmarkKernel::AabbContact},
    {"aabb_contact_solver",
     iggy3d::PhysicsKernelBenchmarkKernel::AabbContactSolver},
    {"kinematic_motor", iggy3d::PhysicsKernelBenchmarkKernel::KinematicMotor},
};

constexpr NameScenario kScenarioNames[] = {
    {"tiny_separated", iggy3d::PhysicsKernelBenchmarkScenario::TinySeparated},
    {"dense_overlap", iggy3d::PhysicsKernelBenchmarkScenario::DenseOverlap},
    {"wall_slide", iggy3d::PhysicsKernelBenchmarkScenario::WallSlide},
    {"grid_line_corridor",
     iggy3d::PhysicsKernelBenchmarkScenario::GridLineCorridor},
    {"dense_cluster_16",
     iggy3d::PhysicsKernelBenchmarkScenario::DenseCluster16},
    {"corner_slide", iggy3d::PhysicsKernelBenchmarkScenario::CornerSlide},
};

void printUsage(std::ostream& out) {
  out << "usage: iggy3d_physics_kernel_bench [--suite]\n"
      << "       iggy3d_physics_kernel_bench --case --kernel <name> "
         "--scenario <name> [options]\n"
      << "\n"
      << "options:\n"
      << "  --suite\n"
      << "  --case\n"
      << "  --kernel <broadphase_grid|aabb_contact|aabb_contact_solver|"
         "kinematic_motor>\n"
      << "  --scenario <tiny_separated|dense_overlap|wall_slide|"
         "grid_line_corridor|dense_cluster_16|corner_slide>\n"
      << "  --iterations <positive_uint>\n"
      << "  --cell-size <positive_float>\n"
      << "  --no-timing\n"
      << "  --output <path>\n"
      << "  --help\n";
}

bool needsValue(int argc, const char* const* argv, int index) {
  return index + 1 >= argc ||
         std::string_view(argv[index + 1]).starts_with("--");
}

bool parseKernel(std::string_view text,
                 iggy3d::PhysicsKernelBenchmarkKernel& kernel) {
  for (const NameKernel& entry : kKernelNames) {
    // branch-gate: BG-1118
    if (entry.name == text) {
      kernel = entry.kernel;
      return true;
    }
  }
  return false;
}

bool parseScenario(std::string_view text,
                   iggy3d::PhysicsKernelBenchmarkScenario& scenario) {
  for (const NameScenario& entry : kScenarioNames) {
    // branch-gate: BG-1118
    if (entry.name == text) {
      scenario = entry.scenario;
      return true;
    }
  }
  return false;
}

bool parsePositiveUint(std::string_view text, std::uint32_t& value) {
  std::uint32_t parsed = 0U;
  const char* begin = text.data();
  const char* end = text.data() + text.size();
  const auto result = std::from_chars(begin, end, parsed);
  // branch-gate: BG-1118
  if (result.ec != std::errc{} || result.ptr != end || parsed == 0U) {
    return false;
  }
  value = parsed;
  return true;
}

bool parsePositiveFloat(std::string_view text, float& value) {
  std::string owned{text};
  char* end = nullptr;
  errno = 0;
  const float parsed = std::strtof(owned.c_str(), &end);
  // branch-gate: BG-1118
  if (errno != 0 || end == owned.c_str() || *end != '\0' ||
      !std::isfinite(parsed) || parsed <= 0.0F) {
    return false;
  }
  value = parsed;
  return true;
}

bool selectMode(CliConfig& config, CliMode mode, std::string& diagnostic) {
  // branch-gate: BG-1118
  if (config.modeExplicit && config.mode != mode) {
    diagnostic = "conflicting mode";
    return false;
  }
  config.mode = mode;
  config.modeExplicit = true;
  return true;
}

bool parseArgs(int argc,
               const char* const* argv,
               CliConfig& config,
               std::string& diagnostic) {
  for (int index = 1; index < argc; ++index) {
    const std::string_view arg = argv[index];
    // branch-gate: BG-1118
    if (arg == "--help" || arg == "-h") {
      config.help = true;
    // branch-gate: BG-1118
    } else if (arg == "--suite") {
      // branch-gate: BG-1118
      if (!selectMode(config, CliMode::Suite, diagnostic)) {
        return false;
      }
    // branch-gate: BG-1118
    } else if (arg == "--case") {
      // branch-gate: BG-1118
      if (!selectMode(config, CliMode::Case, diagnostic)) {
        return false;
      }
    // branch-gate: BG-1118
    } else if (arg == "--kernel") {
      // branch-gate: BG-1118
      if (needsValue(argc, argv, index)) {
        diagnostic = "missing kernel";
        return false;
      }
      ++index;
      // branch-gate: BG-1118
      if (!parseKernel(argv[index], config.kernel)) {
        diagnostic = "unknown kernel";
        return false;
      }
      config.hasKernel = true;
    // branch-gate: BG-1118
    } else if (arg == "--scenario") {
      // branch-gate: BG-1118
      if (needsValue(argc, argv, index)) {
        diagnostic = "missing scenario";
        return false;
      }
      ++index;
      // branch-gate: BG-1118
      if (!parseScenario(argv[index], config.scenario)) {
        diagnostic = "unknown scenario";
        return false;
      }
      config.hasScenario = true;
    // branch-gate: BG-1118
    } else if (arg == "--iterations") {
      // branch-gate: BG-1118
      if (needsValue(argc, argv, index)) {
        diagnostic = "missing iterations";
        return false;
      }
      ++index;
      // branch-gate: BG-1118
      if (!parsePositiveUint(argv[index], config.benchmark.iterations)) {
        diagnostic = "invalid iterations";
        return false;
      }
    // branch-gate: BG-1118
    } else if (arg == "--cell-size") {
      // branch-gate: BG-1118
      if (needsValue(argc, argv, index)) {
        diagnostic = "missing cell size";
        return false;
      }
      ++index;
      // branch-gate: BG-1118
      if (!parsePositiveFloat(argv[index],
                              config.benchmark.broadphaseCellSizeMeters)) {
        diagnostic = "invalid cell size";
        return false;
      }
    // branch-gate: BG-1118
    } else if (arg == "--no-timing") {
      config.benchmark.collectTiming = false;
    // branch-gate: BG-1118
    } else if (arg == "--output") {
      // branch-gate: BG-1118
      if (needsValue(argc, argv, index)) {
        diagnostic = "missing output path";
        return false;
      }
      config.outputPath = argv[++index];
    } else {
      diagnostic = "unknown option";
      return false;
    }
  }

  // branch-gate: BG-1118
  if (config.mode == CliMode::Case && !config.hasKernel) {
    diagnostic = "missing kernel";
    return false;
  }
  // branch-gate: BG-1118
  if (config.mode == CliMode::Case && !config.hasScenario) {
    diagnostic = "missing scenario";
    return false;
  }
  return true;
}

bool writeOutput(const std::filesystem::path& outputPath,
                 const std::string& json,
                 std::string& diagnostic) {
  // branch-gate: BG-1118
  if (outputPath.empty()) {
    std::cout << json;
    return true;
  }
  std::ofstream out(outputPath);
  // branch-gate: BG-1118
  if (!out) {
    diagnostic = "output write failed";
    return false;
  }
  out << json;
  return static_cast<bool>(out);
}

int runBench(const CliConfig& config) {
  std::string json;
  bool ok = false;
  // branch-gate: BG-1118
  if (config.mode == CliMode::Case) {
    iggy3d::PhysicsKernelBenchmarkCaseRequest request;
    request.kernel = config.kernel;
    request.scenario = config.scenario;
    request.config = config.benchmark;
    const iggy3d::PhysicsKernelBenchmarkCaseResult result =
        iggy3d::runPhysicsKernelBenchmarkCase(request);
    ok = result.ok;
    json = iggy3d::physicsKernelBenchmarkCaseToJson(result);
  } else {
    const iggy3d::PhysicsKernelBenchmarkSuiteResult result =
        iggy3d::runPhysicsKernelBenchmarkSuite(config.benchmark);
    ok = result.ok;
    json = iggy3d::physicsKernelBenchmarkSuiteToJson(result);
  }

  std::string diagnostic;
  // branch-gate: BG-1118
  if (!writeOutput(config.outputPath, json, diagnostic)) {
    std::cerr << diagnostic << '\n';
    return 2;
  }
  // branch-gate: BG-1118
  if (ok) {
    return 0;
  }
  return 1;
}

}  // namespace

int main(int argc, const char* const* argv) {
  CliConfig config;
  std::string diagnostic;
  // branch-gate: BG-1118
  if (!parseArgs(argc, argv, config, diagnostic)) {
    std::cerr << diagnostic << '\n';
    printUsage(std::cerr);
    return 2;
  }
  // branch-gate: BG-1118
  if (config.help) {
    printUsage(std::cout);
    return 0;
  }
  return runBench(config);
}
