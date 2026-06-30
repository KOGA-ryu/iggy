// Regression lock for the main-menu map-deletion live-refresh fix.
//
// The bug: the main-menu "Delete" moved a save file to saveRoot/deleted/ on
// disk but never re-scanned the in-memory `saves` catalog
// (ProductSaveBridgeResult). Continue, the Load list, and the receipt
// save_count all read that one catalog, so the deleted map stayed visible
// until the next app launch. The fix added the in-loop re-scan + selection
// re-clamp to `executeProductSaveSoftDelete` (Operations.cpp), mirroring the
// recover path:
//     saves = scanProductSaves(...);
//     initializeSelectedProductSaveSlot(saves.slots, window);
//
// Why a NEW unit test, when a delete smoke already exists: the --no-window
// smoke drives the real product binary and asserts the *receipt*, which is
// rebuilt from a SEPARATE AppShell re-scan that predates this branch. So
// reverting the in-loop re-scan would NOT fail the smoke -- the smoke cannot
// attribute the live refresh to the executor. This test calls the executor
// DIRECTLY and asserts the `saves` argument it was handed is mutated in
// place. Comment out the `saves = scanProductSaves(...)` line in
// executeProductSaveSoftDelete and this test fails: that is the proof it locks
// the real fix.

#include "app/iggy3d/Operations.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "app/iggy3d/world/PackageSessionSeed.hpp"
#include "app/frontend/FrontendState.hpp"
#include "content/PackageLoader.hpp"
#include "runtime/session/Session.hpp"

#include <filesystem>
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

// The default product world the executor resolves from options (see
// productWorldTemplateFromOptions). Pinning devPackageOverride makes the test
// deterministic and guarantees the saves we seed are *compatible* (and thus
// selectable) with the world the executor re-scans against -- otherwise the
// survivor would scan as disabled and the selection would clamp to "none"
// instead of the surviving id we want to assert on.
const char* kFirstRoomPackage =
    "fixtures/demos/first_room/package.iggy3d.toml";

iggy3d::ProductAppOptions optionsForRoot(const std::filesystem::path& root) {
  iggy3d::ProductAppOptions options;
  options.saveRoot = root;
  options.devPackageOverride = kFirstRoomPackage;
  return options;
}

// Local helper: saveSlotById in Operations.cpp is internal (anonymous
// namespace), so we scan the slots ourselves to check the deleted id is gone.
bool catalogContainsId(const iggy3d::SaveSlotList& slots,
                       std::string_view id) {
  for (const iggy3d::SaveSlotPreview& slot : slots.slots) {
    if (slot.id == id) {
      return true;
    }
  }
  return false;
}

std::filesystem::path freshRoot() {
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() /
      "iggy3d_product_save_delete_executor_tests";
  std::error_code error;
  std::filesystem::remove_all(root, error);
  std::filesystem::create_directories(root, error);
  return root;
}

// Seed a real, compatible save file at `root/<saveId>.iggy3d.save` by creating
// a session from the first_room package and writing it durably -- the same path
// the live app uses. Real files matter so the executor's scanProductSaves and
// softDeleteProductSave (a real filesystem move) operate on real data.
bool seedSave(const std::filesystem::path& root,
              std::string_view saveId,
              std::string_view savedAtUtc) {
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage({kFirstRoomPackage});
  if (package.status != iggy3d::PackageLoadStatus::Ok) {
    return false;
  }
  const iggy3d::ProductPackageSessionSeedResult seed =
      iggy3d::buildProductPackageSessionSeed(package);
  if (!seed.ok) {
    return false;
  }
  iggy3d::SessionCreateRequest create;
  create.packageId = package.manifest.packageId;
  create.seed = seed.seed;
  create.config = seed.seed.config;
  const iggy3d::Result<iggy3d::Session> session =
      iggy3d::Session::create(create);
  if (session.status != iggy3d::ResultStatus::Ok) {
    return false;
  }

  iggy3d::ProductSaveWriteRequest request;
  request.saveRoot = root;
  request.saveIdHint = std::string(saveId);
  request.attemptToken = "attempt_001";
  request.state = &session.value.state();
  request.worldId = "world_0001";
  request.worldTitle = "Delete Executor World";
  request.saveTitle = std::string(saveId);
  request.saveType = "manual";
  request.createdAtUtc = "2026-06-30T00:00:00Z";
  request.savedAtUtc = std::string(savedAtUtc);
  return iggy3d::writeProductSessionSaveDurably(request).ok;
}

// The core regression: delete the SELECTED save of a two-save catalog and prove
// the executor mutates the catalog (`saves`) and the window selection in place.
// Pre-fix, `saves` was const and never re-scanned, so it would still show two
// maps and the stale selection after this call.
bool deleteSelectedShrinksCatalogAndReclampsSelectionInPlace() {
  const std::filesystem::path root = freshRoot();
  const iggy3d::ProductAppOptions options = optionsForRoot(root);

  // save_002 is the newer map and the one the operator selected to delete.
  if (!expect(seedSave(root, "save_001", "2026-06-30T01:00:00Z"),
              "seed save_001") ||
      !expect(seedSave(root, "save_002", "2026-06-30T02:00:00Z"),
              "seed save_002")) {
    return false;
  }

  iggy3d::ProductSaveBridgeResult saves = iggy3d::scanProductSaves(
      root, "iggy3d.first_room", "first_room.runtime_loop");
  if (!expect(saves.slots.slots.size() == 2U, "two saves before delete") ||
      !expect(saves.slots.compatibleCount == 2U,
              "both compatible before delete")) {
    return false;
  }

  iggy3d::ProductAppWindowState window;
  iggy3d::FrontendState frontend;
  // The operator selected save_002 and confirmed delete of save_002.
  iggy3d::selectProductSaveSlotById(saves.slots, "save_002", window);
  window.saveDeleteCandidateId = "save_002";
  if (!expect(window.selectedProductSaveId == "save_002",
              "selection starts on save_002")) {
    return false;
  }

  iggy3d::executeProductSaveSoftDelete(options, saves, window, frontend);

  // The catalog argument was mutated IN PLACE: count drops by one and the
  // deleted id is gone from saves.slots. This is exactly what the in-loop
  // re-scan produces -- without it `saves` would still hold both maps.
  const bool deletedGone = !catalogContainsId(saves.slots, "save_002");
  // The selection re-clamps onto the SURVIVING map, never the deleted one and
  // never a stale value left over from before the delete.
  return expect(window.saveDeleteExecuted, "delete executed") &&
         expect(window.saveDeleteStatus == "product_save_soft_deleted",
                "delete status soft deleted") &&
         expect(saves.slots.slots.size() == 1U,
                "catalog shrinks to one map in place") &&
         expect(saves.slots.compatibleCount == 1U,
                "compatible count drops in place") &&
         expect(deletedGone, "deleted id removed from saves.slots") &&
         expect(saves.slots.slots.front().id == "save_001",
                "surviving map is save_001") &&
         expect(window.selectedProductSaveId == "save_001",
                "selection re-clamps to surviving map") &&
         expect(window.selectedProductSaveId != "save_002",
                "selection is not the deleted map") &&
         expect(window.selectedProductSaveEnabled,
                "re-clamped selection is selectable") &&
         expect(window.selectedProductSaveStatus == "selected",
                "re-clamped selection status selected") &&
         // The filesystem move really happened and is isolated to this root.
         expect(!std::filesystem::exists(root / "save_002.iggy3d.save"),
                "deleted file left the active root") &&
         expect(std::filesystem::exists(root / "deleted" /
                                        "save_002.iggy3d.save"),
                "deleted file moved to deleted/") &&
         expect(std::filesystem::exists(root / "save_001.iggy3d.save"),
                "survivor file untouched in active root");
}

// Deleting the only map empties the catalog in place: the count reaches zero
// and the selection clamps to "none"/"empty" rather than the stale single id.
bool deleteLastSaveEmptiesCatalogInPlace() {
  const std::filesystem::path root = freshRoot();
  const iggy3d::ProductAppOptions options = optionsForRoot(root);
  if (!expect(seedSave(root, "save_001", "2026-06-30T01:00:00Z"),
              "seed lone save_001")) {
    return false;
  }

  iggy3d::ProductSaveBridgeResult saves = iggy3d::scanProductSaves(
      root, "iggy3d.first_room", "first_room.runtime_loop");
  if (!expect(saves.slots.slots.size() == 1U, "one save before delete")) {
    return false;
  }

  iggy3d::ProductAppWindowState window;
  iggy3d::FrontendState frontend;
  iggy3d::selectProductSaveSlotById(saves.slots, "save_001", window);
  window.saveDeleteCandidateId = "save_001";

  iggy3d::executeProductSaveSoftDelete(options, saves, window, frontend);

  return expect(window.saveDeleteExecuted, "lone delete executed") &&
         expect(saves.slots.slots.empty(), "catalog empties in place") &&
         expect(saves.slots.compatibleCount == 0U,
                "compatible count reaches zero in place") &&
         expect(window.selectedProductSaveId == "none",
                "selection clamps to none") &&
         expect(!window.selectedProductSaveEnabled,
                "no selection enabled when empty") &&
         expect(window.selectedProductSaveStatus == "empty",
                "selection status empty") &&
         expect(!std::filesystem::exists(root / "save_001.iggy3d.save"),
                "lone deleted file left the active root") &&
         expect(std::filesystem::exists(root / "deleted" /
                                        "save_001.iggy3d.save"),
                "lone deleted file moved to deleted/");
}

}  // namespace

int main() {
  const bool ok = deleteSelectedShrinksCatalogAndReclampsSelectionInPlace() &&
                  deleteLastSaveEmptiesCatalogInPlace();
  if (!ok) {
    return 1;
  }
  std::cout << "product_save_delete_executor_tests passed\n";
  return 0;
}
