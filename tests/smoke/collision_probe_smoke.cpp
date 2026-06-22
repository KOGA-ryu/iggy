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
  const std::filesystem::path moveOutput = "/tmp/iggy3d_collision_probe_move.out";
  const std::filesystem::path projectileOutput = "/tmp/iggy3d_collision_probe_projectile.out";

  std::map<std::string, std::string> suite;
  std::map<std::string, std::string> list;
  std::map<std::string, std::string> segment;
  std::map<std::string, std::string> move;
  std::map<std::string, std::string> projectile;

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

  const bool movePassed =
      runProbe(binary,
               "--package " + shellQuote(fixture) +
                   " --query move --units feet --start 10,0.05,9 --intent 0,0,-1 --seconds 0.5",
               moveOutput, move) &&
      hasField(move, "result", "pass") &&
      hasField(move, "movement_accepted", "true") &&
      hasField(move, "movement_policy_band", "flat") &&
      hasField(move, "ground_snap_applied", "false");

  const bool projectilePassed =
      runProbe(binary,
               "--package " + shellQuote(fixture) +
                   " --query projectile --units feet --start 4,1,14 --intent 0,0,4 --seconds 1 --gravity 0",
               projectileOutput, projectile) &&
      hasField(projectile, "result", "pass") &&
      hasField(projectile, "projectile_status", "impact") &&
      hasField(projectile, "projectile_impact", "true") &&
      hasField(projectile, "projectile_hit_surface_id",
               "spawn_crate_projectile_blocker");

  const bool passed = suitePassed && listPassed && segmentPassed && movePassed &&
                      projectilePassed;
#else
  const bool suitePassed = false;
  const bool listPassed = false;
  const bool segmentPassed = false;
  const bool movePassed = false;
  const bool projectilePassed = false;
  const bool passed = false;
#endif

  std::cout << "smoke=collision_probe\n";
  std::cout << "suite_passed=" << (suitePassed ? "true" : "false") << "\n";
  std::cout << "list_passed=" << (listPassed ? "true" : "false") << "\n";
  std::cout << "segment_passed=" << (segmentPassed ? "true" : "false") << "\n";
  std::cout << "move_passed=" << (movePassed ? "true" : "false") << "\n";
  std::cout << "projectile_passed=" << (projectilePassed ? "true" : "false") << "\n";
  std::cout << "result=" << (passed ? "pass" : "fail") << "\n";
  std::cout << "reason_code="
            << (passed ? "collision_probe_smoke_pass" : "collision_probe_smoke_failed")
            << "\n";
  return passed ? 0 : 1;
}
