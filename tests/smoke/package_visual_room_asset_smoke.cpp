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

bool integerFieldGreaterThanZero(const std::map<std::string, std::string>& fields,
                                 const std::string& key) {
  const auto found = fields.find(key);
  if (found == fields.end() || found->second.empty()) {
    return false;
  }
  unsigned long long value = 0ULL;
  for (const char character : found->second) {
    if (character < '0' || character > '9') {
      return false;
    }
    value = value * 10ULL + static_cast<unsigned long long>(character - '0');
  }
  return value > 0ULL;
}

}  // namespace

int main() {
#if defined(IGGY3D_VISUAL_DEMO_PATH)
  const std::filesystem::path binary{IGGY3D_VISUAL_DEMO_PATH};
  const std::filesystem::path fixture =
      std::filesystem::current_path() / "fixtures/demos/first_room/package.iggy3d.toml";
  const std::filesystem::path output = "/tmp/iggy3d_package_visual_room_asset.out";
  const std::string command =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer vulkan --frames 3 --window --print-render-receipt > " +
      shellQuote(output);

  const int exitCode =
      std::filesystem::exists(binary) ? exitCodeFromSystem(std::system(command.c_str())) : 1;
  std::map<std::string, std::string> fields;
  const bool receiptValid = parseReceiptFile(output, fields);
  const bool skipped = exitCode == 77 && receiptValid && hasField(fields, "result", "skip");
  const bool passed =
      exitCode == 0 && receiptValid && hasField(fields, "backend", "vulkan") &&
      hasField(fields, "rendering_path", "package_room_meshes") &&
      hasField(fields, "record_mode", "room_mesh_draws") &&
      hasField(fields, "room_asset_loaded", "true") &&
      hasField(fields, "room_asset_id", "spawn_corridor") &&
      hasField(fields, "source_toml",
               "/Users/kogaryu/edi/artifacts/provingground/provingground.map.toml") &&
      hasField(fields, "source_subset", "spawn_room_corridor_stub") &&
      integerFieldGreaterThanZero(fields, "room_static_mesh_count") &&
      integerFieldGreaterThanZero(fields, "room_material_count") &&
      integerFieldGreaterThanZero(fields, "room_anchor_count") &&
      integerFieldGreaterThanZero(fields, "mesh_draw_count") &&
      integerFieldGreaterThanZero(fields, "indexed_draw_count") &&
      hasField(fields, "vertex_buffer_uploaded", "true") &&
      hasField(fields, "index_buffer_uploaded", "true") &&
      integerFieldGreaterThanZero(fields, "draw_count") &&
      hasField(fields, "depth_enabled", "true") &&
      hasField(fields, "camera_projection", "perspective") &&
      hasField(fields, "projection_application", "single") &&
      hasField(fields, "floor_visible", "true") &&
      hasField(fields, "wall_visible", "true") &&
      hasField(fields, "opening_visible", "true") &&
      hasField(fields, "prop_visible", "true") &&
      hasField(fields, "key_marker_visible", "true") &&
      hasField(fields, "dummy_marker_visible", "true") &&
      hasField(fields, "first_room_visible", "true") &&
      hasField(fields, "result", "pass");
#else
  const int exitCode = 77;
  const bool receiptValid = false;
  const bool skipped = true;
  const bool passed = false;
#endif

  std::cout << "smoke=package_visual_room_asset\n";
  std::cout << "receipt_valid=" << (receiptValid ? "true" : "false") << "\n";
  std::cout << "actual_exit_code=" << exitCode << "\n";
  std::cout << "result=" << (passed ? "pass" : (skipped ? "skip" : "fail")) << "\n";
  std::cout << "reason_code="
            << (passed ? "package_visual_room_asset_pass"
                       : (skipped ? "package_visual_room_asset_skip"
                                  : "package_visual_room_asset_failed"))
            << "\n";
  if (passed) {
    return 0;
  }
  return skipped ? 77 : 1;
}
