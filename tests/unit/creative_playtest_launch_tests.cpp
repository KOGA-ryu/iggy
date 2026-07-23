// Playtest launch proof (C3): the pure plan builder, the snapshot slot, and
// the validate->snapshot->plan orchestration -- all headless, no spawn.

#include "EditorControls.hpp"
#include "EditorPlaytestLaunch.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "app/iggy3d/creative/world/MapTemplate.hpp"
#include "app/iggy3d/creative/world/WorldService.hpp"
#include "content/assets/StaticMeshAsset.hpp"

namespace {

namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

std::filesystem::path scratchRoot() {
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() / "iggy3d_playtest_launch_tests";
  std::error_code error;
  std::filesystem::remove_all(root, error);
  std::filesystem::create_directories(root, error);
  return root;
}

std::string readFile(const std::filesystem::path& path) {
  std::ifstream stream(path, std::ios::binary);
  std::ostringstream out;
  out << stream.rdbuf();
  return out.str();
}

// ---- launch plan ---------------------------------------------------------

bool planJoinsPathAndOrdersArgv() {
  const app::PlaytestLaunchPlan plan = app::buildPlaytestLaunchPlan(
      "/opt/iggy3d/bin", "/home/user/.iggy3d/creative_standalone/playtest",
      "snapshot");
  return expect(plan.valid, "plan builds") &&
         expect(plan.binaryPath ==
                    std::filesystem::path("/opt/iggy3d/bin/i3dp"),
                "binary path joined") &&
         expect(plan.argv.size() == 5U, "argv has five entries") &&
         expect(plan.argv[0] == "/opt/iggy3d/bin/i3dp" &&
                    plan.argv[1] == "--save-root" &&
                    plan.argv[2] ==
                        "/home/user/.iggy3d/creative_standalone/playtest" &&
                    plan.argv[3] == "--load" && plan.argv[4] == "snapshot",
                "argv order matches i3dp usage");
}

bool planRefusesBadInputs() {
  return expect(!app::buildPlaytestLaunchPlan("", "/root", "snapshot").valid,
                "empty base path refused") &&
         expect(!app::buildPlaytestLaunchPlan("/bin", "", "snapshot").valid,
                "empty save root refused") &&
         expect(!app::buildPlaytestLaunchPlan("/bin", "/root", "").valid,
                "empty id refused") &&
         expect(!app::buildPlaytestLaunchPlan("/bin", "/root", "bad id").valid,
                "id with space refused") &&
         expect(!app::buildPlaytestLaunchPlan("/bin", "/root", "../up").valid,
                "id with traversal refused") &&
         expect(app::buildPlaytestLaunchPlan("/bin", "/root", "A-b_9").valid,
                "well-formed id accepted");
}

// ---- snapshot slot -------------------------------------------------------

bool snapshotWritesOverwritesAndRoundTrips() {
  const std::filesystem::path root = scratchRoot();
  const cr::CreativeMapTemplateResult map =
      cr::buildCreativeMapTemplate(cr::kMapDemoTemplateId, 1U);
  if (!expect(map.accepted, "map template builds")) {
    return false;
  }
  const app::PlaytestSnapshotResult first =
      app::writePlaytestSnapshot(map.document, root);
  if (!expect(first.ok, "snapshot writes") ||
      !expect(first.path ==
                  root / "playtest" / "snapshot.iggy3d.save",
              "snapshot lands in the playtest subdirectory")) {
    return false;
  }
  const std::string firstBytes = readFile(first.path);
  if (!expect(!firstBytes.empty(), "snapshot file non-empty")) {
    return false;
  }
  // Re-Play overwrites the same slot.
  const app::PlaytestSnapshotResult second =
      app::writePlaytestSnapshot(map.document, root);
  if (!expect(second.ok && second.path == first.path,
              "re-play overwrites the same slot")) {
    return false;
  }
  // Round trip through the real codec: the snapshot loads and the document
  // it re-encodes to is byte-identical (save codec oracle pattern).
  const iggy3d::CreativeWorldOpenResult opened =
      iggy3d::openCreativeWorld({root / "playtest", "snapshot"});
  if (!expect(opened.accepted &&
                  opened.document.objectCount() == map.document.objectCount(),
              "snapshot opens with full object count")) {
    return false;
  }
  cr::CreativeDocument reopened = opened.document;
  iggy3d::CreativeWorldSaveRequest resaveRequest;
  resaveRequest.saveRoot = root / "resave";
  std::error_code error;
  std::filesystem::create_directories(resaveRequest.saveRoot, error);
  resaveRequest.saveId = "snapshot";
  resaveRequest.document = &reopened;
  resaveRequest.worldTitle = "playtest";
  resaveRequest.saveTitle = "playtest snapshot";
  const iggy3d::CreativeWorldSaveResult resaved =
      iggy3d::saveCreativeWorld(resaveRequest);
  if (!expect(resaved.accepted && resaved.saved, "snapshot re-saves")) {
    return false;
  }
  const std::string resavedBytes =
      readFile(resaveRequest.saveRoot / "snapshot.iggy3d.save");
  return expect(resavedBytes == firstBytes,
                "snapshot round-trips byte-identically");
}

bool editorScansDoNotSeeSnapshot() {
  const std::filesystem::path root = scratchRoot();
  const cr::CreativeMapTemplateResult map =
      cr::buildCreativeMapTemplate(cr::kMapDemoTemplateId, 1U);
  const app::PlaytestSnapshotResult snapshot =
      app::writePlaytestSnapshot(map.document, root);
  if (!expect(snapshot.ok, "snapshot writes for scan test")) {
    return false;
  }
  // The editor's whole-root scan (listSaveFiles / product save sweep) is
  // non-recursive: the playtest subdirectory stays invisible.
  const auto files = iggy3d::listSaveFiles(root);
  return expect(files.empty(),
                "editor save scan does not see the playtest snapshot");
}

// ---- orchestration -------------------------------------------------------

bool invalidDocumentIsRefusedWithoutSideEffects() {
  const std::filesystem::path root = scratchRoot();
  // An empty document has no spawn point and fails play validation.
  cr::CreativeDocument empty = cr::CreativeDocument::create("Empty");
  (void)empty.assignId(1);
  iggy3d::StaticMeshAssetCatalog catalog;
  const app::PlaytestLaunchPreparation preparation =
      app::preparePlaytestLaunch(empty, &catalog, root, "/opt/bin");
  return expect(!preparation.accepted, "invalid document refused") &&
         expect(preparation.reasonCode.rfind("playtest_refused_", 0) == 0,
                "refusal carries the validation status") &&
         expect(!std::filesystem::exists(root / "playtest" /
                                         "snapshot.iggy3d.save"),
                "no snapshot written on refusal") &&
         expect(!preparation.plan.valid, "no plan built on refusal");
}

bool validDocumentBuildsSnapshotAndPlan() {
  const std::filesystem::path root = scratchRoot();
  const cr::CreativeMapTemplateResult map =
      cr::buildCreativeMapTemplate(cr::kMapDemoTemplateId, 1U);
  const iggy3d::StaticMeshAssetCatalog catalog =
      iggy3d::discoverStaticMeshAssetCatalog("assets/creative");
  const app::PlaytestLaunchPreparation preparation =
      app::preparePlaytestLaunch(map.document, &catalog, root,
                                 "/opt/iggy3d/bin");
  return expect(preparation.accepted, "valid document accepted") &&
         expect(preparation.snapshot.ok &&
                    std::filesystem::exists(preparation.snapshot.path),
                "snapshot written") &&
         expect(preparation.plan.valid &&
                    preparation.plan.argv.size() == 5U &&
                    preparation.plan.argv[2] ==
                        (root / "playtest").generic_string() &&
                    preparation.plan.argv[4] == "snapshot",
                "plan points i3dp at the snapshot");
}

// ---- fullscreen/resolution slice ----------------------------------------

bool resolutionParsing() {
  return expect(app::parsePlaytestResolution("1920x1080").valid &&
                    app::parsePlaytestResolution("1920x1080").width == 1920U,
                "well-formed WxH parses") &&
         expect(app::parsePlaytestResolution("640x360").valid,
                "lower bound accepted") &&
         expect(app::parsePlaytestResolution("16384x16384").valid,
                "upper bound accepted") &&
         expect(!app::parsePlaytestResolution("639x360").valid,
                "below minimum width refused") &&
         expect(!app::parsePlaytestResolution("640x359").valid,
                "below minimum height refused") &&
         expect(!app::parsePlaytestResolution("16385x1080").valid,
                "above maximum refused") &&
         expect(!app::parsePlaytestResolution("banana").valid &&
                    !app::parsePlaytestResolution("1920x").valid &&
                    !app::parsePlaytestResolution("x1080").valid &&
                    !app::parsePlaytestResolution("1920X1080").valid,
                "malformed forms refused");
}

bool planWindowPassThrough() {
  app::PlaytestWindowPreferences none;
  const app::PlaytestLaunchPlan bare = app::buildPlaytestLaunchPlan(
      "/bin", "/root/playtest", "snapshot", &none);
  app::PlaytestWindowPreferences windowed;
  windowed.present = true;
  windowed.width = 1920U;
  windowed.height = 1080U;
  const app::PlaytestLaunchPlan sized = app::buildPlaytestLaunchPlan(
      "/bin", "/root/playtest", "snapshot", &windowed);
  app::PlaytestWindowPreferences both = windowed;
  both.fullscreen = true;
  const app::PlaytestLaunchPlan fullscreen = app::buildPlaytestLaunchPlan(
      "/bin", "/root/playtest", "snapshot", &both);
  return expect(bare.argv.size() == 5U,
                "absent section: argv byte-identical to before") &&
         expect(sized.argv.size() == 7U &&
                    sized.argv[5] == "--resolution" &&
                    sized.argv[6] == "1920x1080",
                "width/height appends --resolution") &&
         expect(fullscreen.argv.size() == 6U &&
                    fullscreen.argv[5] == "--fullscreen",
                "fullscreen wins when both are configured");
}

bool profileSectionAdditiveCompat() {
  namespace cr = iggy3d::creative;
  const std::filesystem::path root = scratchRoot();
  const std::filesystem::path path = root / "creative_controls_v1.cfg";
  const cr::CreativeControlProfile defaults =
      cr::makeDefaultCreativeControlProfile();
  // 1. Legacy save (no section) loads unchanged -- and reports no section.
  if (!expect(app::saveCreativeEditorControlProfile(defaults, path).status ==
                  app::CreativeEditorControlPersistenceStatus::Saved,
              "legacy save writes")) {
    return false;
  }
  cr::CreativeControlProfile loaded;
  app::PlaytestWindowPreferences preferences;
  if (!expect(app::loadCreativeEditorControlProfile(loaded, path,
                                                    &preferences).status ==
                      app::CreativeEditorControlPersistenceStatus::Loaded &&
                  !preferences.present,
              "existing profiles parse unchanged, section absent")) {
    return false;
  }
  // 2. Ace's hand edit: append the section; both fields load.
  {
    std::ofstream append(path, std::ios::app);
    append << "[playtest]\nfullscreen = true\nwidth = 1920\n"
              "height = 1080\n";
  }
  app::PlaytestWindowPreferences edited;
  if (!expect(app::loadCreativeEditorControlProfile(loaded, path, &edited)
                      .status ==
                      app::CreativeEditorControlPersistenceStatus::Loaded &&
                  edited.present && edited.fullscreen &&
                  edited.width == 1920U && edited.height == 1080U,
              "hand-added [playtest] section loads")) {
    return false;
  }
  // 3. A control save preserves the section (Ace's edit survives).
  if (!expect(app::saveCreativeEditorControlProfile(defaults, path, &edited)
                      .status ==
                  app::CreativeEditorControlPersistenceStatus::Saved,
              "save with preferences writes")) {
    return false;
  }
  app::PlaytestWindowPreferences roundTripped;
  const bool reload =
      app::loadCreativeEditorControlProfile(loaded, path, &roundTripped)
              .status ==
          app::CreativeEditorControlPersistenceStatus::Loaded &&
      roundTripped.present && roundTripped.fullscreen &&
      roundTripped.width == 1920U && roundTripped.height == 1080U;
  // 4. Partial section (fullscreen only) is valid.
  {
    std::ofstream rewrite(path, std::ios::trunc);
    std::string text;
    static_cast<void>(
        app::serializeCreativeEditorControlProfile(defaults, text));
    rewrite << text << "[playtest]\nfullscreen = false\n";
  }
  app::PlaytestWindowPreferences partial;
  const bool partialOk =
      app::loadCreativeEditorControlProfile(loaded, path, &partial).status ==
          app::CreativeEditorControlPersistenceStatus::Loaded &&
      partial.present && !partial.fullscreen && partial.width == 0U;
  return expect(reload, "control save round-trips the section") &&
         expect(partialOk, "partial section (fullscreen only) loads");
}

}  // namespace

int main() {
  const bool ok = planJoinsPathAndOrdersArgv() &&
                  planRefusesBadInputs() &&
                  snapshotWritesOverwritesAndRoundTrips() &&
                  editorScansDoNotSeeSnapshot() &&
                  invalidDocumentIsRefusedWithoutSideEffects() &&
                  validDocumentBuildsSnapshotAndPlan() &&
                  resolutionParsing() && planWindowPassThrough() &&
                  profileSectionAdditiveCompat();
  if (ok) {
    std::cout << "creative_playtest_launch_tests passed\n";
  }
  return ok ? 0 : 1;
}
