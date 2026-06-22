#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/wait.h>
#endif

namespace {

std::string shellQuote(const std::filesystem::path& path) {
  std::string value = path.string();
  std::string quoted = "'";
  for (const char character : value) {
    if (character == '\'') {
      quoted += "'\\''";
    } else {
      quoted.push_back(character);
    }
  }
  quoted += "'";
  return quoted;
}

int exitCodeFromSystem(int status) {
  if (status == -1) {
    return 1;
  }
#if defined(__unix__) || defined(__APPLE__)
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  return 1;
#else
  return status;
#endif
}

bool parseReceiptFile(const std::filesystem::path& path,
                      std::map<std::string, std::string>& fields) {
  fields.clear();
  std::ifstream input(path);
  if (!input) {
    return false;
  }
  std::string line;
  while (std::getline(input, line)) {
    const std::size_t equals = line.find('=');
    if (equals == std::string::npos || equals == 0U) {
      return false;
    }
    if (!fields.emplace(line.substr(0, equals), line.substr(equals + 1U)).second) {
      return false;
    }
  }
  return true;
}

bool hasField(const std::map<std::string, std::string>& fields,
              const std::string& key,
              const std::string& value) {
  const auto found = fields.find(key);
  return found != fields.end() && found->second == value;
}

bool runProbe(const std::filesystem::path& binary,
              const std::string& args,
              const std::filesystem::path& output,
              std::map<std::string, std::string>& fields) {
  const std::string command = shellQuote(binary) + " " + args + " > " + shellQuote(output);
  const int exitCode = exitCodeFromSystem(std::system(command.c_str()));
  return exitCode == 0 && parseReceiptFile(output, fields);
}

}  // namespace

int main() {
#if defined(IGGY3D_COLLISION_PROBE_PATH)
  const std::filesystem::path binary{IGGY3D_COLLISION_PROBE_PATH};
  const std::filesystem::path root = std::filesystem::current_path();
  const std::filesystem::path fixture =
      root / "fixtures/demos/first_room/package.iggy3d.toml";
  const std::filesystem::path suiteOutput = "/tmp/iggy3d_collision_probe_suite.out";
  const std::filesystem::path listOutput = "/tmp/iggy3d_collision_probe_list.out";
  const std::filesystem::path segmentOutput = "/tmp/iggy3d_collision_probe_segment.out";

  std::map<std::string, std::string> suite;
  std::map<std::string, std::string> list;
  std::map<std::string, std::string> segment;

  const bool suitePassed =
      std::filesystem::exists(binary) &&
      runProbe(binary, "--package " + shellQuote(fixture) + " --query suite", suiteOutput, suite) &&
      hasField(suite, "result", "pass") &&
      hasField(suite, "surface_height_sampled", "true") &&
      hasField(suite, "surface_normal_valid", "true") &&
      hasField(suite, "actor_blocker_hit", "true") &&
      hasField(suite, "opening_blocks_actor", "false") &&
      hasField(suite, "projectile_blocker_hit", "true") &&
      hasField(suite, "query_result_order", "stable");

  const bool listPassed =
      runProbe(binary, "--package " + shellQuote(fixture) + " --list-surfaces", listOutput, list) &&
      hasField(list, "result", "pass") &&
      hasField(list, "surface_count", "4") &&
      hasField(list, "surface_0_id", "spawn_floor_walkable") &&
      hasField(list, "surface_1_id", "north_wall_actor_blocker") &&
      hasField(list, "surface_2_id", "spawn_crate_projectile_blocker") &&
      hasField(list, "surface_3_id", "east_opening_non_blocker");

  const bool segmentPassed =
      runProbe(binary,
               "--package " + shellQuote(fixture) +
                   " --query segment --kind actor --start 3.048,1,2 --end 3.048,1,-1",
               segmentOutput, segment) &&
      hasField(segment, "result", "pass") &&
      hasField(segment, "status", "hit") &&
      hasField(segment, "surface_id", "north_wall_actor_blocker") &&
      hasField(segment, "role", "blocker");

  const bool passed = suitePassed && listPassed && segmentPassed;
#else
  const bool suitePassed = false;
  const bool listPassed = false;
  const bool segmentPassed = false;
  const bool passed = false;
#endif

  std::cout << "smoke=collision_probe\n";
  std::cout << "suite_passed=" << (suitePassed ? "true" : "false") << "\n";
  std::cout << "list_passed=" << (listPassed ? "true" : "false") << "\n";
  std::cout << "segment_passed=" << (segmentPassed ? "true" : "false") << "\n";
  std::cout << "result=" << (passed ? "pass" : "fail") << "\n";
  std::cout << "reason_code="
            << (passed ? "collision_probe_smoke_pass" : "collision_probe_smoke_failed")
            << "\n";
  return passed ? 0 : 1;
}
