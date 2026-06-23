#include "content/PackageLoader.hpp"
#include "runtime/save/SaveFileStore.hpp"
#include "runtime/session/Session.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <string_view>

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

bool numericFieldAtLeast(const std::map<std::string, std::string>& fields,
                         const std::string& key,
                         unsigned long minimum) {
  const auto found = fields.find(key);
  if (found == fields.end()) {
    return false;
  }
  char* end = nullptr;
  const unsigned long value = std::strtoul(found->second.c_str(), &end, 10);
  return end != found->second.c_str() && *end == '\0' && value >= minimum;
}

bool writeControlFile(const std::filesystem::path& path,
                      std::string_view selectedAction,
                      bool execute) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << "opening_menu.open=true\n";
  output << "opening_menu.select=" << selectedAction << "\n";
  if (execute) {
    output << "opening_menu.execute=true\n";
  }
  return static_cast<bool>(output);
}

iggy3d::Session makeFixtureSession(const std::filesystem::path& fixture) {
  const iggy3d::PackageLoadResult package = iggy3d::loadPackage({fixture});
  iggy3d::SessionCreateRequest request;
  request.packageId = package.manifest.packageId;
  request.config = package.scenario.config;
  request.seed = package.scenario;
  return iggy3d::Session::create(request).value;
}

iggy3d::SaveAuthoredRoomSection authoredRoomFixture() {
  iggy3d::SaveAuthoredRoomSection authoredRoom;
  authoredRoom.present = true;
  authoredRoom.id = "opening_menu_saved_room";
  iggy3d::SaveAuthoredRoomFloorRecord floor;
  floor.id = "edit_floor_11";
  floor.centerMeters = {3.0F, 0.0F, 3.0F};
  floor.sizeMeters = {2.0F, 0.1F, 2.0F};
  floor.semantics.materialId = "debug_floor";
  floor.semantics.walkable = true;
  floor.semantics.traversalTags = {"walkable"};
  authoredRoom.floors.push_back(floor);
  iggy3d::SaveAuthoredRoomWallRecord wall;
  wall.id = "edit_wall_5";
  wall.startMeters = {2.0F, 0.0F, 2.0F};
  wall.endMeters = {4.0F, 0.0F, 2.0F};
  wall.heightMeters = 1.5F;
  wall.thicknessMeters = 0.2F;
  wall.semantics.materialId = "debug_wall";
  wall.semantics.blocksActor = true;
  wall.semantics.blocksProjectile = true;
  wall.semantics.traversalTags = {"clamber"};
  authoredRoom.walls.push_back(wall);
  return authoredRoom;
}

bool writeAuthoredSaveFile(const std::filesystem::path& fixture,
                           const std::filesystem::path& saveRoot) {
  iggy3d::Session session = makeFixtureSession(fixture);
  iggy3d::SaveAuthoredRoomSection authoredRoom = authoredRoomFixture();
  iggy3d::SaveFileWriteRequest request;
  request.root = saveRoot;
  request.idHint = "save_001";
  request.state = &session.state();
  request.authoredRoom = &authoredRoom;
  return iggy3d::writeSessionSaveFile(request).ok;
}

bool runVisualDemo(const std::filesystem::path& binary,
                   const std::filesystem::path& fixture,
                   const std::filesystem::path& saveRoot,
                   const std::filesystem::path& control,
                   const std::filesystem::path& output) {
  const std::string command =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer null --interactive --frames 1 --opening-menu --save-root " +
      shellQuote(saveRoot) + " --codex-control " + shellQuote(control) +
      " --print-render-receipt > " + shellQuote(output);
  return exitCodeFromSystem(std::system(command.c_str())) == 0;
}

}  // namespace

int main() {
#if defined(IGGY3D_VISUAL_DEMO_PATH)
  constexpr bool visualBuilt = true;
#else
  constexpr bool visualBuilt = false;
#endif
  bool hudPassed = false;
  bool createPassed = false;
  bool saveExitPassed = false;
  bool loadPassed = false;
  bool deletePassed = false;
  bool authoredLoadPassed = false;
#if defined(IGGY3D_VISUAL_DEMO_PATH)
  const std::filesystem::path binary{IGGY3D_VISUAL_DEMO_PATH};
  const std::filesystem::path fixture =
      std::filesystem::current_path() / "fixtures/demos/movement_playground/package.iggy3d.toml";
  const std::filesystem::path saveRoot =
      std::filesystem::temp_directory_path() / "iggy3d_opening_menu_saves";
  const std::filesystem::path authoredSaveRoot =
      std::filesystem::temp_directory_path() / "iggy3d_opening_menu_authored_saves";
  const std::filesystem::path hudControl = "/tmp/iggy3d_opening_menu_hud_control.in";
  const std::filesystem::path hudOutput = "/tmp/iggy3d_opening_menu_hud.out";
  const std::filesystem::path createControl = "/tmp/iggy3d_opening_menu_create_control.in";
  const std::filesystem::path createOutput = "/tmp/iggy3d_opening_menu_create.out";
  const std::filesystem::path saveExitControl = "/tmp/iggy3d_opening_menu_save_exit_control.in";
  const std::filesystem::path saveExitOutput = "/tmp/iggy3d_opening_menu_save_exit.out";
  const std::filesystem::path loadControl = "/tmp/iggy3d_opening_menu_load_control.in";
  const std::filesystem::path loadOutput = "/tmp/iggy3d_opening_menu_load.out";
  const std::filesystem::path deleteControl = "/tmp/iggy3d_opening_menu_delete_control.in";
  const std::filesystem::path deleteOutput = "/tmp/iggy3d_opening_menu_delete.out";
  const std::filesystem::path authoredLoadControl =
      "/tmp/iggy3d_opening_menu_authored_load_control.in";
  const std::filesystem::path authoredLoadOutput =
      "/tmp/iggy3d_opening_menu_authored_load.out";
  std::error_code error;
  std::filesystem::remove_all(saveRoot, error);
  std::filesystem::remove_all(authoredSaveRoot, error);

  std::map<std::string, std::string> fields;
  hudPassed = std::filesystem::exists(binary) &&
              writeControlFile(hudControl, "existing_saves", false) &&
              runVisualDemo(binary, fixture, saveRoot, hudControl, hudOutput) &&
              parseReceiptFile(hudOutput, fields) && hasField(fields, "result", "pass") &&
              hasField(fields, "backend", "null") &&
              hasField(fields, "opening_menu_enabled", "true") &&
              hasField(fields, "opening_menu_open", "true") &&
              hasField(fields, "opening_menu_hud_visible", "true") &&
              numericFieldAtLeast(fields, "opening_menu_hud_line_count", 8UL) &&
              hasField(fields, "opening_menu_selected_action", "existing_saves") &&
              hasField(fields, "opening_menu_save_file_count", "0") &&
              hasField(fields, "opening_menu_selected_save_id", "none");

  createPassed = writeControlFile(createControl, "new_world", true) &&
                 runVisualDemo(binary, fixture, saveRoot, createControl, createOutput) &&
                 parseReceiptFile(createOutput, fields) &&
                 hasField(fields, "result", "pass") &&
                 hasField(fields, "opening_menu_open", "false") &&
                 hasField(fields, "opening_menu_last_action", "new_world") &&
                 hasField(fields, "opening_menu_status", "save_file_created") &&
                 hasField(fields, "opening_menu_created_save", "true") &&
                 hasField(fields, "opening_menu_save_file_count", "1") &&
                 hasField(fields, "opening_menu_selected_save_id", "save_001") &&
                 std::filesystem::exists(saveRoot / "save_001.iggy3d.save");

  saveExitPassed = writeControlFile(saveExitControl, "save_and_exit", true) &&
                   runVisualDemo(binary, fixture, saveRoot, saveExitControl, saveExitOutput) &&
                   parseReceiptFile(saveExitOutput, fields) &&
                   hasField(fields, "result", "pass") &&
                   hasField(fields, "opening_menu_open", "false") &&
                   hasField(fields, "opening_menu_last_action", "save_and_exit") &&
                   hasField(fields, "opening_menu_status", "save_file_written_for_exit") &&
                   hasField(fields, "opening_menu_saved_and_exit", "true") &&
                   hasField(fields, "opening_menu_exit_requested", "true") &&
                   hasField(fields, "opening_menu_selected_save_id", "save_001") &&
                   std::filesystem::exists(saveRoot / "save_001.iggy3d.save");

  loadPassed = writeControlFile(loadControl, "existing_saves", true) &&
               runVisualDemo(binary, fixture, saveRoot, loadControl, loadOutput) &&
               parseReceiptFile(loadOutput, fields) &&
               hasField(fields, "result", "pass") &&
               hasField(fields, "opening_menu_last_action", "existing_saves") &&
               hasField(fields, "opening_menu_status", "save_file_loaded") &&
               hasField(fields, "opening_menu_loaded_save", "true") &&
               hasField(fields, "opening_menu_selected_save_id", "save_001");

  deletePassed = writeControlFile(deleteControl, "delete_selected", true) &&
                 runVisualDemo(binary, fixture, saveRoot, deleteControl, deleteOutput) &&
                 parseReceiptFile(deleteOutput, fields) &&
                 hasField(fields, "result", "pass") &&
                 hasField(fields, "opening_menu_last_action", "delete_selected") &&
                 hasField(fields, "opening_menu_status", "save_file_deleted") &&
                 hasField(fields, "opening_menu_deleted_save", "true") &&
                 hasField(fields, "opening_menu_save_file_count", "0") &&
                 !std::filesystem::exists(saveRoot / "save_001.iggy3d.save");

  authoredLoadPassed = writeAuthoredSaveFile(fixture, authoredSaveRoot) &&
                       writeControlFile(authoredLoadControl, "existing_saves", true) &&
                       runVisualDemo(binary, fixture, authoredSaveRoot, authoredLoadControl,
                                     authoredLoadOutput) &&
                       parseReceiptFile(authoredLoadOutput, fields) &&
                       hasField(fields, "result", "pass") &&
                       hasField(fields, "opening_menu_status", "save_file_loaded") &&
                       hasField(fields, "opening_menu_loaded_save", "true") &&
                       hasField(fields, "editor_last_command", "load_room") &&
                       hasField(fields, "editor_last_status", "authored_room_loaded") &&
                       hasField(fields, "editor_floor_count", "1") &&
                       hasField(fields, "editor_wall_count", "1") &&
                       numericFieldAtLeast(fields, "editor_runtime_surface_count", 2UL);
#endif

  const bool passed =
      hudPassed && createPassed && saveExitPassed && loadPassed && deletePassed &&
      authoredLoadPassed;
  std::cout << "smoke=package_visual_opening_menu\n";
  std::cout << "backend=" << (visualBuilt ? "null" : "unavailable") << "\n";
  std::cout << "opening_menu_hud=" << (hudPassed ? "true" : "false") << "\n";
  std::cout << "opening_menu_create=" << (createPassed ? "true" : "false") << "\n";
  std::cout << "opening_menu_save_exit=" << (saveExitPassed ? "true" : "false") << "\n";
  std::cout << "opening_menu_load=" << (loadPassed ? "true" : "false") << "\n";
  std::cout << "opening_menu_delete=" << (deletePassed ? "true" : "false") << "\n";
  std::cout << "opening_menu_authored_load=" << (authoredLoadPassed ? "true" : "false")
            << "\n";
  std::cout << "result=" << (passed ? "pass" : (visualBuilt ? "fail" : "skip")) << "\n";
  std::cout << "reason_code=" << (passed ? "visual_opening_menu_pass"
                                         : (visualBuilt ? "visual_opening_menu_failed"
                                                        : "visual_demo_unavailable"))
            << "\n";
  if (passed) {
    return 0;
  }
  return visualBuilt ? 1 : 77;
}
