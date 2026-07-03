#include "app/iggy3d/creative/Document.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

constexpr cr::CreativeObjectDirtyFlags dirtyFlag(
    cr::CreativeObjectDirtyFlag flag) noexcept {
  return static_cast<cr::CreativeObjectDirtyFlags>(flag);
}

constexpr cr::CreativeObjectDirtyFlags settingsDirtyFlags() noexcept {
  return dirtyFlag(cr::CreativeObjectDirtyFlag::Preview) |
         dirtyFlag(cr::CreativeObjectDirtyFlag::Serialization);
}

bool sameVec3(cr::CreativeVec3 lhs, cr::CreativeVec3 rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool sameGridSize(cr::CreativeGridSize3 lhs, cr::CreativeGridSize3 rhs) {
  return lhs.width == rhs.width && lhs.height == rhs.height &&
         lhs.depth == rhs.depth;
}

bool sameGridSettings(cr::CreativeGridSettings lhs,
                      cr::CreativeGridSettings rhs) {
  return sameVec3(lhs.origin, rhs.origin) &&
         lhs.cellSizeMeters == rhs.cellSizeMeters &&
         sameGridSize(lhs.size, rhs.size);
}

bool sameSnapSettings(cr::CreativeDocumentSnapSettings lhs,
                      cr::CreativeDocumentSnapSettings rhs) {
  return lhs.mode == rhs.mode && lhs.axes == rhs.axes &&
         lhs.stepX == rhs.stepX && lhs.stepY == rhs.stepY &&
         lhs.stepZ == rhs.stepZ && lhs.originX == rhs.originX &&
         lhs.originY == rhs.originY && lhs.originZ == rhs.originZ;
}

bool sameBounds(cr::CreativeBounds lhs, cr::CreativeBounds rhs) {
  return sameVec3(lhs.min, rhs.min) && sameVec3(lhs.max, rhs.max);
}

cr::CreativeDocumentCreateReceipt createRoom(cr::CreativeDocument& document,
                                             std::string_view name = "Room") {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Room;
  request.name = std::string{name};
  return document.createObject(request);
}

cr::CreativeGridSettings authoredGridSettings() {
  cr::CreativeGridSettings settings;
  settings.origin = {1.0, 2.0, 3.0};
  settings.cellSizeMeters = 0.25;
  settings.size = {64, 32, 8};
  return settings;
}

cr::CreativeDocumentSnapSettings authoredSnapSettings() {
  cr::CreativeDocumentSnapSettings settings =
      cr::makeDefaultCreativeDocumentSnapSettings();
  settings.axes = cr::kCreativeDocumentSnapAxisXZ;
  settings.stepX = 0.5;
  settings.stepZ = 2.0;
  settings.originX = -1.0;
  settings.originZ = 4.0;
  return settings;
}

cr::CreativeBounds authoredWorldBounds() {
  return {{-8.0, -1.0, -4.0}, {64.0, 32.0, 8.0}};
}

cr::CreativeObject restoredGroupObject() {
  cr::CreativeObject object;
  object.id = 2;
  object.kind = cr::CreativeObjectKind::Group;
  object.name = "Restored Group";
  object.layerId = 4;
  object.visible = true;
  object.locked = false;
  object.tags = {"container"};
  return object;
}

cr::CreativeObject restoredCrateObject() {
  cr::CreativeObject object;
  object.id = 7;
  object.kind = cr::CreativeObjectKind::Crate;
  object.name = "Restored Crate";
  object.transform.position = {1.0, 2.0, 3.0};
  object.transform.rotation = {0.0, 0.5, 0.0};
  object.transform.scale = {1.0, 2.0, 3.0};
  object.bounds = {{1.0, 2.0, 3.0}, {3.0, 4.0, 5.0}};
  object.layerId = 9;
  object.visible = false;
  object.locked = true;
  object.parentId = 2;
  object.tags = {"crate", "imported"};
  return object;
}

cr::CreativeDocumentRestoreRequest validRestoreRequest() {
  cr::CreativeDocumentRestoreRequest request;
  request.documentId = 9001;
  request.name = "Restored Document";
  request.units = cr::CreativeUnits::Meters;
  request.gridSettings = authoredGridSettings();
  request.snapSettings = authoredSnapSettings();
  request.worldBounds = authoredWorldBounds();
  request.nextObjectId = 42;
  request.objects = {restoredGroupObject(), restoredCrateObject()};
  return request;
}

bool defaultAndCreatedDocumentSettingsAreStable() {
  const cr::CreativeDocument defaultDocument;
  const cr::CreativeDocument createdDocument =
      cr::CreativeDocument::create("Created");
  const cr::CreativeDocumentSnapSettings defaultSnap =
      cr::makeDefaultCreativeDocumentSnapSettings();

  bool ok = true;
  for (const cr::CreativeDocument* document :
       {&defaultDocument, &createdDocument}) {
    ok = ok && expect(document->units() == cr::CreativeUnits::Meters,
                      "default units meters") &&
         expect(document->gridSettings().cellSizeMeters == 1.0,
                "default grid cell size") &&
         expect(sameGridSize(document->gridSettings().size, {0, 0, 0}),
                "default grid zero size") &&
         expect(sameVec3(document->gridSettings().origin, {}),
                "default grid origin zero") &&
         expect(sameSnapSettings(document->documentSnapSettings(), defaultSnap),
                "default document snap settings") &&
         expect(sameBounds(document->worldBounds(), {}),
                "default world bounds zero") &&
         expect(document->nextObjectId() == 1U, "default next object id");
  }
  return ok;
}

bool settingsSettersReviseAndDirtyOnlyOnAcceptedChanges() {
  cr::CreativeDocument document;
  const cr::CreativeGridSettings grid = authoredGridSettings();
  cr::CreativeGridSettings invalidGrid = grid;
  invalidGrid.cellSizeMeters = 0.0;
  const cr::CreativeDocumentSnapSettings snap = authoredSnapSettings();
  cr::CreativeDocumentSnapSettings invalidSnap = snap;
  invalidSnap.stepX = 0.0;
  const cr::CreativeBounds worldBounds = authoredWorldBounds();
  cr::CreativeBounds invalidBounds = worldBounds;
  invalidBounds.max.x = std::numeric_limits<double>::infinity();

  const bool sameUnits = document.setUnits(cr::CreativeUnits::Meters);
  const bool invalidUnits =
      document.setUnits(static_cast<cr::CreativeUnits>(255));
  const bool gridChanged = document.setGridSettings(grid);
  const std::uint64_t revisionAfterGrid = document.revision();
  const bool sameGrid = document.setGridSettings(grid);
  const bool invalidGridRejected = document.setGridSettings(invalidGrid);
  const bool snapChanged = document.setDocumentSnapSettings(snap);
  const std::uint64_t revisionAfterSnap = document.revision();
  const bool sameSnap = document.setDocumentSnapSettings(snap);
  const bool invalidSnapRejected =
      document.setDocumentSnapSettings(invalidSnap);
  const bool boundsChanged = document.setWorldBounds(worldBounds);
  const std::uint64_t revisionAfterBounds = document.revision();
  const bool sameBoundsSet = document.setWorldBounds(worldBounds);
  const bool invalidBoundsRejected = document.setWorldBounds(invalidBounds);

  return expect(!sameUnits, "same units no-change") &&
         expect(!invalidUnits, "invalid units rejected") &&
         expect(gridChanged, "grid settings changed") &&
         expect(revisionAfterGrid == 1U, "grid revision") &&
         expect(sameGridSettings(document.gridSettings(), grid),
                "grid stored") &&
         expect(!sameGrid, "same grid no-change") &&
         expect(!invalidGridRejected, "invalid grid rejected") &&
         expect(snapChanged, "snap settings changed") &&
         expect(revisionAfterSnap == 2U, "snap revision") &&
         expect(sameSnapSettings(document.documentSnapSettings(), snap),
                "snap stored") &&
         expect(!sameSnap, "same snap no-change") &&
         expect(!invalidSnapRejected, "invalid snap rejected") &&
         expect(boundsChanged, "world bounds changed") &&
         expect(revisionAfterBounds == 3U, "bounds revision") &&
         expect(sameBounds(document.worldBounds(), worldBounds),
                "world bounds stored") &&
         expect(!sameBoundsSet, "same bounds no-change") &&
         expect(!invalidBoundsRejected, "invalid bounds rejected") &&
         expect(document.dirtyFlags() == settingsDirtyFlags(),
                "settings dirty flags accumulated") &&
         expect(document.drainDirtyFlags() == settingsDirtyFlags(),
                "settings dirty drain") &&
         expect(document.dirtyFlags() == 0U, "settings dirty drained") &&
         expect(document.revision() == 3U,
                "rejected settings leave revision stable");
}

bool createRemovePreserveSettingsAndNextObjectCursor() {
  cr::CreativeDocument document;
  const cr::CreativeGridSettings grid = authoredGridSettings();
  const cr::CreativeDocumentSnapSettings snap = authoredSnapSettings();
  const cr::CreativeBounds worldBounds = authoredWorldBounds();
  static_cast<void>(document.setGridSettings(grid));
  static_cast<void>(document.setDocumentSnapSettings(snap));
  static_cast<void>(document.setWorldBounds(worldBounds));

  const cr::CreativeDocumentCreateReceipt created =
      createRoom(document, "Created Room");
  const cr::CreativeDocumentRemoveReceipt removed =
      document.removeDocumentObject(created.objectId);

  return expect(created.accepted, "create preserve setup") &&
         expect(created.objectId == 1U, "first object id") &&
         expect(document.nextObjectId() == 2U, "next id after create/remove") &&
         expect(removed.accepted, "remove preserve setup") &&
         expect(sameGridSettings(document.gridSettings(), grid),
                "create remove keeps grid") &&
         expect(sameSnapSettings(document.documentSnapSettings(), snap),
                "create remove keeps snap") &&
         expect(sameBounds(document.worldBounds(), worldBounds),
                "create remove keeps bounds");
}

bool resetRestoresSettingsAndNextObjectCursor() {
  cr::CreativeDocument document;
  static_cast<void>(document.setGridSettings(authoredGridSettings()));
  static_cast<void>(document.setDocumentSnapSettings(authoredSnapSettings()));
  static_cast<void>(document.setWorldBounds(authoredWorldBounds()));
  const cr::CreativeDocumentCreateReceipt created =
      createRoom(document, "Before Reset");

  document.reset();

  return expect(created.accepted, "reset setup create") &&
         expect(document.units() == cr::CreativeUnits::Meters,
                "reset units") &&
         expect(document.gridSettings().cellSizeMeters == 1.0,
                "reset grid cell") &&
         expect(sameGridSize(document.gridSettings().size, {0, 0, 0}),
                "reset grid size") &&
         expect(sameSnapSettings(document.documentSnapSettings(),
                                 cr::makeDefaultCreativeDocumentSnapSettings()),
                "reset snap") &&
         expect(sameBounds(document.worldBounds(), {}), "reset bounds") &&
         expect(document.nextObjectId() == 1U, "reset next id") &&
         expect(document.revision() == 0U, "reset revision") &&
         expect(document.dirtyFlags() == 0U, "reset dirty");
}

bool restoreForLoadReplacesDocumentWithExactState() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Before");
  static_cast<void>(document.assignId(12));
  static_cast<void>(createRoom(document, "Existing"));
  static_cast<void>(document.drainDirtyFlags());

  const cr::CreativeDocumentRestoreRequest request = validRestoreRequest();
  const cr::CreativeDocumentRestoreReceipt receipt =
      document.restoreForLoad(request);
  const cr::CreativeObject* group = document.findObject(2);
  const cr::CreativeObject* crate = document.findObject(7);

  return expect(receipt.requested, "restore requested") &&
         expect(receipt.accepted, "restore accepted") &&
         expect(receipt.changed, "restore changed") &&
         expect(receipt.status == cr::CreativeDocumentRestoreStatus::Restored,
                "restore status") &&
         expect(receipt.message == "document_restored",
                "restore message") &&
         expect(receipt.reasonCode == "document_restored",
                "restore reason") &&
         expect(receipt.documentId == 9001U, "restore receipt id") &&
         expect(receipt.objectCount == 2U, "restore receipt object count") &&
         expect(receipt.nextObjectId == 42U, "restore receipt next id") &&
         expect(document.isValid(), "restored document valid") &&
         expect(document.id() == 9001U, "restored document id") &&
         expect(document.name() == "Restored Document",
                "restored document name") &&
         expect(document.units() == cr::CreativeUnits::Meters,
                "restored units") &&
         expect(sameGridSettings(document.gridSettings(),
                                 request.gridSettings),
                "restored grid settings") &&
         expect(sameSnapSettings(document.documentSnapSettings(),
                                 request.snapSettings),
                "restored snap settings") &&
         expect(sameBounds(document.worldBounds(), request.worldBounds),
                "restored world bounds") &&
         expect(document.objectCount() == 2U, "restored object count") &&
         expect(document.nextObjectId() == 42U,
                "restored exact next object id") &&
         expect(document.revision() == 0U, "restore revision zero") &&
         expect(document.dirtyFlags() == 0U, "restore dirty zero") &&
         expect(document.containsObject(2), "restore index contains group") &&
         expect(document.containsObject(7), "restore index contains crate") &&
         expect(group != nullptr && group->kind == cr::CreativeObjectKind::Group,
                "restored group kind") &&
         expect(group != nullptr && group->name == "Restored Group",
                "restored group name") &&
         expect(crate != nullptr && crate->kind == cr::CreativeObjectKind::Crate,
                "restored crate kind") &&
         expect(crate != nullptr && crate->name == "Restored Crate",
                "restored crate name") &&
         expect(crate != nullptr &&
                    sameVec3(crate->transform.position, {1.0, 2.0, 3.0}),
                "restored transform") &&
         expect(crate != nullptr &&
                    sameBounds(crate->bounds,
                               {{1.0, 2.0, 3.0}, {3.0, 4.0, 5.0}}),
                "restored bounds") &&
         expect(crate != nullptr && crate->layerId == 9U,
                "restored layer") &&
         expect(crate != nullptr && !crate->visible && crate->locked,
                "restored visibility lock") &&
         expect(crate != nullptr && crate->parentId.has_value() &&
                    *crate->parentId == 2U,
                "restored parent") &&
         expect(crate != nullptr && crate->tags.size() == 2U &&
                    crate->tags[0] == "crate" &&
                    crate->tags[1] == "imported",
                "restored tags");
}

bool restoreForLoadRejectsBadInputsWithoutMutation() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Original");
  static_cast<void>(document.assignId(33));
  const cr::CreativeDocumentCreateReceipt original =
      createRoom(document, "Original Room");
  const cr::CreativeDocumentId originalId = document.id();
  const std::string_view originalName = document.name();
  const std::uint64_t originalRevision = document.revision();
  const cr::CreativeObjectDirtyFlags originalDirty = document.dirtyFlags();
  const cr::CreativeObjectId originalNextObjectId = document.nextObjectId();

  auto unchanged = [&]() {
    return document.id() == originalId && document.name() == originalName &&
           document.revision() == originalRevision &&
           document.dirtyFlags() == originalDirty &&
           document.objectCount() == 1U &&
           document.nextObjectId() == originalNextObjectId &&
           document.findObject(original.objectId) != nullptr;
  };

  cr::CreativeDocumentRestoreRequest invalidDocId = validRestoreRequest();
  invalidDocId.documentId = cr::kInvalidDocumentId;
  const cr::CreativeDocumentRestoreReceipt invalidDocIdReceipt =
      document.restoreForLoad(invalidDocId);

  cr::CreativeDocumentRestoreRequest invalidObject = validRestoreRequest();
  invalidObject.objects.front().id = cr::kInvalidObjectId;
  const cr::CreativeDocumentRestoreReceipt invalidObjectReceipt =
      document.restoreForLoad(invalidObject);

  cr::CreativeDocumentRestoreRequest duplicate = validRestoreRequest();
  duplicate.objects[1].id = duplicate.objects[0].id;
  const cr::CreativeDocumentRestoreReceipt duplicateReceipt =
      document.restoreForLoad(duplicate);

  cr::CreativeDocumentRestoreRequest badNext = validRestoreRequest();
  badNext.nextObjectId = 7;
  const cr::CreativeDocumentRestoreReceipt badNextReceipt =
      document.restoreForLoad(badNext);

  cr::CreativeDocumentRestoreRequest badGrid = validRestoreRequest();
  badGrid.gridSettings.cellSizeMeters = 0.0;
  const cr::CreativeDocumentRestoreReceipt badGridReceipt =
      document.restoreForLoad(badGrid);

  cr::CreativeDocumentRestoreRequest badSnap = validRestoreRequest();
  badSnap.snapSettings.stepX = 0.0;
  const cr::CreativeDocumentRestoreReceipt badSnapReceipt =
      document.restoreForLoad(badSnap);

  return expect(original.accepted, "reject setup create accepted") &&
         expect(!invalidDocIdReceipt.accepted,
                "invalid document id rejected") &&
         expect(invalidDocIdReceipt.status ==
                    cr::CreativeDocumentRestoreStatus::InvalidDocumentId,
                "invalid document id status") &&
         expect(invalidDocIdReceipt.reasonCode == "invalid_document_id",
                "invalid document id reason") &&
         expect(unchanged(), "invalid document id leaves unchanged") &&
         expect(!invalidObjectReceipt.accepted, "invalid object rejected") &&
         expect(invalidObjectReceipt.status ==
                    cr::CreativeDocumentRestoreStatus::InvalidObject,
                "invalid object status") &&
         expect(invalidObjectReceipt.reasonCode == "invalid_object",
                "invalid object reason") &&
         expect(unchanged(), "invalid object leaves unchanged") &&
         expect(!duplicateReceipt.accepted, "duplicate id rejected") &&
         expect(duplicateReceipt.status ==
                    cr::CreativeDocumentRestoreStatus::DuplicateObjectId,
                "duplicate id status") &&
         expect(duplicateReceipt.reasonCode == "duplicate_object_id",
                "duplicate id reason") &&
         expect(unchanged(), "duplicate leaves unchanged") &&
         expect(!badNextReceipt.accepted, "bad next id rejected") &&
         expect(badNextReceipt.status ==
                    cr::CreativeDocumentRestoreStatus::InvalidNextObjectId,
                "bad next id status") &&
         expect(badNextReceipt.reasonCode == "invalid_next_object_id",
                "bad next id reason") &&
         expect(unchanged(), "bad next leaves unchanged") &&
         expect(!badGridReceipt.accepted, "bad grid rejected") &&
         expect(badGridReceipt.status ==
                    cr::CreativeDocumentRestoreStatus::InvalidSettings,
                "bad grid status") &&
         expect(badGridReceipt.reasonCode == "invalid_grid_settings",
                "bad grid reason") &&
         expect(unchanged(), "bad grid leaves unchanged") &&
         expect(!badSnapReceipt.accepted, "bad snap rejected") &&
         expect(badSnapReceipt.status ==
                    cr::CreativeDocumentRestoreStatus::InvalidSettings,
                "bad snap status") &&
         expect(badSnapReceipt.reasonCode ==
                    "invalid_document_snap_settings",
                "bad snap reason") &&
         expect(unchanged(), "bad snap leaves unchanged");
}

bool restoreForLoadPreservesObjectIdGapsAndCursor() {
  cr::CreativeDocument document;
  cr::CreativeDocumentRestoreRequest request = validRestoreRequest();
  request.objects.clear();
  request.objects.push_back(restoredCrateObject());
  request.objects.front().id = 4;
  request.nextObjectId = 100;

  const cr::CreativeDocumentRestoreReceipt receipt =
      document.restoreForLoad(request);
  const cr::CreativeObjectId nextObjectIdAfterRestore =
      document.nextObjectId();
  const cr::CreativeDocumentCreateReceipt created =
      createRoom(document, "After Restore");

  return expect(receipt.accepted, "gap restore accepted") &&
         expect(nextObjectIdAfterRestore == 100U, "gap next id exact") &&
         expect(created.accepted, "post restore create accepted") &&
         expect(created.objectId == 100U, "post restore uses exact cursor") &&
         expect(document.nextObjectId() == 101U,
                "post restore cursor increments");
}

}  // namespace

int main() {
  const bool ok = defaultAndCreatedDocumentSettingsAreStable() &&
                  settingsSettersReviseAndDirtyOnlyOnAcceptedChanges() &&
                  createRemovePreserveSettingsAndNextObjectCursor() &&
                  resetRestoresSettingsAndNextObjectCursor() &&
                  restoreForLoadReplacesDocumentWithExactState() &&
                  restoreForLoadRejectsBadInputsWithoutMutation() &&
                  restoreForLoadPreservesObjectIdGapsAndCursor();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
