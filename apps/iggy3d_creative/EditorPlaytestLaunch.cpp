#include "EditorPlaytestLaunch.hpp"

#include <system_error>
#include <utility>

#include <SDL3/SDL.h>

#include "app/iggy3d/creative/play/PlayPreparation.hpp"
#include "app/iggy3d/creative/world/WorldService.hpp"
#include "runtime/save/SaveFileStore.hpp"

namespace iggy3d_creative_app {
namespace {

[[nodiscard]] bool validLaunchSaveId(const std::string& saveId) {
  if (saveId.empty()) {
    return false;
  }
  for (const char value : saveId) {
    const bool ok = (value >= 'A' && value <= 'Z') ||
                    (value >= 'a' && value <= 'z') ||
                    (value >= '0' && value <= '9') || value == '_' ||
                    value == '-';
    if (!ok) {
      return false;
    }
  }
  return true;
}

}  // namespace

PlaytestResolution parsePlaytestResolution(std::string_view argument) {
  PlaytestResolution resolution;
  const std::size_t separator = argument.find('x');
  if (separator == std::string_view::npos || separator == 0U ||
      separator + 1U >= argument.size()) {
    return resolution;
  }
  const auto parseDimension = [](std::string_view digits,
                                 std::uint32_t& out) {
    if (digits.empty() || digits.size() > 5U) {
      return false;
    }
    std::uint32_t value = 0U;
    for (const char c : digits) {
      if (c < '0' || c > '9') {
        return false;
      }
      value = value * 10U + static_cast<std::uint32_t>(c - '0');
    }
    out = value;
    return true;
  };
  std::uint32_t width = 0U;
  std::uint32_t height = 0U;
  if (!parseDimension(argument.substr(0, separator), width) ||
      !parseDimension(argument.substr(separator + 1U), height) ||
      width < kPlaytestMinWindowWidth || height < kPlaytestMinWindowHeight ||
      width > kPlaytestMaxWindowDimension ||
      height > kPlaytestMaxWindowDimension) {
    return resolution;
  }
  resolution.valid = true;
  resolution.width = width;
  resolution.height = height;
  return resolution;
}

PlaytestLaunchPlan buildPlaytestLaunchPlan(
    const std::filesystem::path& basePath,
    const std::filesystem::path& saveRoot,
    const std::string& saveId,
    const PlaytestWindowPreferences* windowPreferences) {
  PlaytestLaunchPlan plan;
  if (basePath.empty()) {
    plan.reasonCode = "playtest_launch_missing_base_path";
    return plan;
  }
  if (saveRoot.empty()) {
    plan.reasonCode = "playtest_launch_missing_save_root";
    return plan;
  }
  if (!validLaunchSaveId(saveId)) {
    plan.reasonCode = "playtest_launch_invalid_save_id";
    return plan;
  }
  plan.binaryPath = basePath / "i3dp";
  plan.argv = {plan.binaryPath.generic_string(),
               "--save-root", saveRoot.generic_string(),
               "--load", saveId};
  if (windowPreferences != nullptr && windowPreferences->present) {
    // Decided precedence: fullscreen wins when both are configured.
    if (windowPreferences->fullscreen) {
      plan.argv.emplace_back("--fullscreen");
    } else if (windowPreferences->width != 0U &&
               windowPreferences->height != 0U) {
      plan.argv.emplace_back("--resolution");
      plan.argv.emplace_back(std::to_string(windowPreferences->width) + "x" +
                             std::to_string(windowPreferences->height));
    }
  }
  plan.valid = true;
  plan.reasonCode = "playtest_launch_plan_ready";
  return plan;
}

PlaytestSnapshotResult writePlaytestSnapshot(
    const iggy3d::creative::CreativeDocument& document,
    const std::filesystem::path& editorSaveRoot) {
  PlaytestSnapshotResult result;
  if (editorSaveRoot.empty()) {
    result.reasonCode = "playtest_snapshot_missing_save_root";
    return result;
  }
  const std::filesystem::path snapshotRoot =
      editorSaveRoot / std::string(kPlaytestSnapshotDirName);
  std::error_code error;
  std::filesystem::create_directories(snapshotRoot, error);
  if (error) {
    result.reasonCode = "playtest_snapshot_directory_failed";
    return result;
  }
  // Copy: saveCreativeWorld drains through a mutable pointer; the editor's
  // live document must stay untouched.
  iggy3d::creative::CreativeDocument documentCopy = document;
  iggy3d::CreativeWorldSaveRequest request;
  request.saveRoot = snapshotRoot;
  request.saveId = std::string(kPlaytestSnapshotSaveId);
  request.document = &documentCopy;
  request.worldTitle = "playtest";
  request.saveTitle = "playtest snapshot";
  const iggy3d::CreativeWorldSaveResult saved =
      iggy3d::saveCreativeWorld(request);
  if (!saved.accepted || !saved.saved) {
    result.reasonCode = saved.reasonCode.empty()
                            ? "playtest_snapshot_save_failed"
                            : saved.reasonCode;
    return result;
  }
  result.ok = true;
  result.reasonCode = "playtest_snapshot_written";
  result.path = iggy3d::saveFilePathForId(
      snapshotRoot, std::string(kPlaytestSnapshotSaveId));
  return result;
}

PlaytestLaunchPreparation preparePlaytestLaunch(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog* staticMeshAssetCatalog,
    const std::filesystem::path& editorSaveRoot,
    const std::filesystem::path& basePath) {
  PlaytestLaunchPreparation preparation;
  // VALIDATION gate: the same preparation Play mode used embedded. An
  // invalid document is refused here -- no snapshot, no plan, no spawn.
  iggy3d::creative::CreativePlayPreparationRequest validationRequest;
  validationRequest.document = &document;
  validationRequest.staticMeshAssetCatalog = staticMeshAssetCatalog;
  const iggy3d::creative::CreativePlayPreparationResult validation =
      iggy3d::creative::prepareCreativePlay(validationRequest);
  if (!validation.accepted) {
    preparation.reasonCode =
        std::string("playtest_refused_") +
        std::string(toString(validation.status));
    return preparation;
  }
  preparation.snapshot = writePlaytestSnapshot(document, editorSaveRoot);
  if (!preparation.snapshot.ok) {
    preparation.reasonCode = preparation.snapshot.reasonCode;
    return preparation;
  }
  preparation.plan = buildPlaytestLaunchPlan(
      basePath, editorSaveRoot / std::string(kPlaytestSnapshotDirName),
      std::string(kPlaytestSnapshotSaveId));
  if (!preparation.plan.valid) {
    preparation.reasonCode = preparation.plan.reasonCode;
    return preparation;
  }
  preparation.accepted = true;
  preparation.reasonCode = "playtest_launch_ready";
  return preparation;
}

bool spawnPlaytestProcess(const PlaytestLaunchPlan& plan,
                          std::string& reasonCode) {
  if (!plan.valid) {
    reasonCode = plan.reasonCode;
    return false;
  }
  std::vector<const char*> argv;
  argv.reserve(plan.argv.size() + 1U);
  for (const std::string& argument : plan.argv) {
    argv.push_back(argument.c_str());
  }
  argv.push_back(nullptr);
  SDL_Process* process =
      SDL_CreateProcess(argv.data(), /*pipe_stdio=*/false);
  if (process == nullptr) {
    reasonCode = std::string("playtest_spawn_failed: ") + SDL_GetError();
    return false;
  }
  // Fire-and-forget this slice: the child owns its window and lifetime.
  SDL_DestroyProcess(process);
  reasonCode = "playtest_spawned";
  return true;
}

}  // namespace iggy3d_creative_app
