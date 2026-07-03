#include "app/iggy3d/save/CreativeDocumentSection.hpp"
#include "runtime/save/SaveCodec.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

constexpr double kOneThird = 1.0 / 3.0;
constexpr double kPrecise = 0.1234567890123;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
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

bool sameTransform(cr::CreativeTransform lhs, cr::CreativeTransform rhs) {
  return sameVec3(lhs.position, rhs.position) &&
         sameVec3(lhs.rotation, rhs.rotation) &&
         sameVec3(lhs.scale, rhs.scale);
}

cr::CreativeGridSettings authoredGridSettings() {
  cr::CreativeGridSettings settings;
  settings.origin = {kOneThird, kPrecise, -7.25};
  settings.cellSizeMeters = kOneThird;
  settings.size = {64, 32, 8};
  return settings;
}

cr::CreativeDocumentSnapSettings authoredSnapSettings() {
  cr::CreativeDocumentSnapSettings settings =
      cr::makeDefaultCreativeDocumentSnapSettings();
  settings.mode = cr::CreativeDocumentSnapMode::Grid;
  settings.axes = cr::kCreativeDocumentSnapAxisXZ;
  settings.stepX = kPrecise;
  settings.stepY = 2.5;
  settings.stepZ = kOneThird;
  settings.originX = -1.25;
  settings.originY = kOneThird;
  settings.originZ = 4.75;
  return settings;
}

cr::CreativeBounds authoredWorldBounds() {
  return {{-8.0, -1.0, -4.0}, {64.0, 32.0, 8.0}};
}

cr::CreativeObject restoredGroupObject() {
  cr::CreativeObject object;
  object.id = 2;
  object.kind = cr::CreativeObjectKind::Group;
  object.name = "Root Group";
  object.layerId = 4;
  object.visible = true;
  object.locked = false;
  object.tags = {"container", "roundtrip"};
  return object;
}

cr::CreativeObject restoredCrateObject() {
  cr::CreativeObject object;
  object.id = 7;
  object.kind = cr::CreativeObjectKind::Crate;
  object.name = "Precise Crate";
  object.transform.position = {kOneThird, 2.0, kPrecise};
  object.transform.rotation = {0.0, kPrecise, 1.5};
  object.transform.scale = {1.0, 2.0, 3.0};
  object.bounds = {{0.0, -0.25, kOneThird}, {10.5, 4.25, 7.75}};
  object.layerId = 9;
  object.visible = false;
  object.locked = true;
  object.parentId = 2;
  object.tags = {"crate", "imported"};
  return object;
}

cr::CreativeDocumentRestoreRequest authoredRestoreRequest() {
  cr::CreativeDocumentRestoreRequest request;
  request.documentId = 9001;
  request.name = "Round Trip Creative";
  request.units = cr::CreativeUnits::Meters;
  request.gridSettings = authoredGridSettings();
  request.snapSettings = authoredSnapSettings();
  request.worldBounds = authoredWorldBounds();
  request.nextObjectId = 100;
  request.objects = {restoredGroupObject(), restoredCrateObject()};
  return request;
}

cr::CreativeDocument authoredDocument() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Before");
  const cr::CreativeDocumentRestoreReceipt restored =
      document.restoreForLoad(authoredRestoreRequest());
  if (!restored.accepted) {
    std::cerr << "FAIL: authored document restore setup\n";
  }
  return document;
}

iggy3d::SaveEnvelope minimalEnvelope() {
  iggy3d::SaveEnvelope envelope;
  envelope.metadata.savedStateHash = 0;
  envelope.metadata.savedStateHashHex = "0000000000000000";
  return envelope;
}

bool sectionObjectMatches(const iggy3d::SaveCreativeDocumentObjectRecord& save,
                          const cr::CreativeObject& object) {
  return save.id == object.id &&
         save.kind == std::string{cr::toString(object.kind)} &&
         save.name == object.name &&
         save.transform.position.x == object.transform.position.x &&
         save.transform.position.y == object.transform.position.y &&
         save.transform.position.z == object.transform.position.z &&
         save.transform.rotation.x == object.transform.rotation.x &&
         save.transform.rotation.y == object.transform.rotation.y &&
         save.transform.rotation.z == object.transform.rotation.z &&
         save.transform.scale.x == object.transform.scale.x &&
         save.transform.scale.y == object.transform.scale.y &&
         save.transform.scale.z == object.transform.scale.z &&
         save.bounds.min.x == object.bounds.min.x &&
         save.bounds.min.y == object.bounds.min.y &&
         save.bounds.min.z == object.bounds.min.z &&
         save.bounds.max.x == object.bounds.max.x &&
         save.bounds.max.y == object.bounds.max.y &&
         save.bounds.max.z == object.bounds.max.z &&
         save.layerId == object.layerId && save.visible == object.visible &&
         save.locked == object.locked &&
         save.hasParent == object.parentId.has_value() &&
         save.parentId == object.parentId.value_or(cr::kInvalidObjectId) &&
         save.tags == object.tags;
}

bool documentObjectMatches(const cr::CreativeObject& lhs,
                           const cr::CreativeObject& rhs) {
  return lhs.id == rhs.id && lhs.kind == rhs.kind && lhs.name == rhs.name &&
         sameTransform(lhs.transform, rhs.transform) &&
         sameBounds(lhs.bounds, rhs.bounds) && lhs.layerId == rhs.layerId &&
         lhs.visible == rhs.visible && lhs.locked == rhs.locked &&
         lhs.parentId == rhs.parentId && lhs.tags == rhs.tags;
}

bool buildSectionCopiesDocumentExactly() {
  const cr::CreativeDocument document = authoredDocument();
  const cr::CreativeDocumentRestoreRequest original = authoredRestoreRequest();
  const cr::CreativeObject& group = original.objects[0];
  const cr::CreativeObject& crate = original.objects[1];
  const iggy3d::ProductCreativeDocumentSectionBuildResult result =
      iggy3d::buildSaveCreativeDocumentSection(document);
  const iggy3d::SaveCreativeDocumentSection& section = result.section;

  return expect(result.receipt.requested, "build receipt requested") &&
         expect(result.receipt.accepted, "build receipt accepted") &&
         expect(result.receipt.changed, "build receipt changed") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::Converted,
                "build receipt converted") &&
         expect(result.receipt.documentId == 9001U,
                "build receipt document id") &&
         expect(result.receipt.objectCount == 2U,
                "build receipt object count") &&
         expect(result.receipt.nextObjectId == 100U,
                "build receipt next id") &&
         expect(result.receipt.reasonCode ==
                    "creative_document_section_converted",
                "build receipt reason") &&
         expect(section.present, "section present") &&
         expect(section.version == 1U, "section version") &&
         expect(section.documentId == document.id(), "section document id") &&
         expect(section.name == document.name(), "section name") &&
         expect(section.units == "Meters", "section units") &&
         expect(section.gridOrigin.x == document.gridSettings().origin.x &&
                    section.gridOrigin.y == document.gridSettings().origin.y &&
                    section.gridOrigin.z == document.gridSettings().origin.z,
                "section grid origin") &&
         expect(section.cellSizeMeters ==
                    document.gridSettings().cellSizeMeters,
                "section grid cell size") &&
         expect(section.gridWidth ==
                    static_cast<std::uint32_t>(
                        document.gridSettings().size.width) &&
                    section.gridHeight ==
                        static_cast<std::uint32_t>(
                            document.gridSettings().size.height) &&
                    section.gridDepth ==
                        static_cast<std::uint32_t>(
                            document.gridSettings().size.depth),
                "section grid dimensions") &&
         expect(section.snapMode == "Grid", "section snap mode") &&
         expect(section.snapAxes == document.documentSnapSettings().axes,
                "section snap axes") &&
         expect(section.snapStepX == document.documentSnapSettings().stepX &&
                    section.snapStepY == document.documentSnapSettings().stepY &&
                    section.snapStepZ == document.documentSnapSettings().stepZ,
                "section snap steps") &&
         expect(section.worldBounds.min.x == document.worldBounds().min.x &&
                    section.worldBounds.max.z == document.worldBounds().max.z,
                "section world bounds") &&
         expect(section.nextObjectId == document.nextObjectId(),
                "section next id") &&
         expect(section.objects.size() == 2U, "section object count") &&
         expect(sectionObjectMatches(section.objects[0], group),
                "section group object") &&
         expect(sectionObjectMatches(section.objects[1], crate),
                "section crate object");
}

bool encodeDecodeAndRestoreRoundTripsDocument() {
  const cr::CreativeDocument document = authoredDocument();
  const iggy3d::ProductCreativeDocumentSectionBuildResult built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  envelope.creativeDocument = built.section;

  const iggy3d::SaveEncodeResult encoded = iggy3d::encodeSaveEnvelope(envelope);
  const bool encodedOmitsRevision =
      encoded.encodedText.find("creativeDocument.revision") ==
      std::string::npos;
  const bool encodedOmitsDirtyFlags =
      encoded.encodedText.find("creativeDocument.dirty") == std::string::npos;
  const bool preciseDoubleSurvivedText =
      encoded.encodedText.find("0.333\n") == std::string::npos &&
      encoded.encodedText.find("0.123\n") == std::string::npos;
  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::ProductCreativeDocumentSectionRestoreResult restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(
          decoded.envelope.creativeDocument);

  const cr::CreativeDocument& restoredDocument = restored.document;
  const cr::CreativeObject* restoredGroup = restoredDocument.findObject(2);
  const cr::CreativeObject* restoredCrate = restoredDocument.findObject(7);
  const cr::CreativeDocumentRestoreRequest original =
      authoredRestoreRequest();

  return expect(built.receipt.accepted, "round trip build accepted") &&
         expect(encoded.status == iggy3d::SaveCodecStatus::Ok,
                "round trip encode ok") &&
         expect(encodedOmitsRevision, "encoded omits creative revision") &&
         expect(encodedOmitsDirtyFlags, "encoded omits dirty flags") &&
         expect(preciseDoubleSurvivedText,
                "creative doubles not three-decimal formatted") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok,
                "round trip decode ok") &&
         expect(decoded.envelope.creativeDocument.present,
                "decoded creative section present") &&
         expect(restored.receipt.accepted, "restore receipt accepted") &&
         expect(restored.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::Converted,
                "restore receipt converted") &&
         expect(restored.receipt.documentId == original.documentId,
                "restore receipt id") &&
         expect(restored.receipt.objectCount == original.objects.size(),
                "restore receipt object count") &&
         expect(restored.receipt.nextObjectId == original.nextObjectId,
                "restore receipt next id") &&
         expect(restoredDocument.id() == document.id(), "restored id") &&
         expect(restoredDocument.name() == document.name(), "restored name") &&
         expect(restoredDocument.units() == cr::CreativeUnits::Meters,
                "restored units") &&
         expect(sameGridSettings(restoredDocument.gridSettings(),
                                 document.gridSettings()),
                "restored grid") &&
         expect(sameSnapSettings(restoredDocument.documentSnapSettings(),
                                 document.documentSnapSettings()),
                "restored snap") &&
         expect(sameBounds(restoredDocument.worldBounds(),
                           document.worldBounds()),
                "restored world bounds") &&
         expect(restoredDocument.nextObjectId() == 100U,
                "restored exact next id") &&
         expect(restoredDocument.revision() == 0U,
                "restored revision zero") &&
         expect(restoredDocument.dirtyFlags() == 0U,
                "restored dirty zero") &&
         expect(restoredDocument.objectCount() == 2U,
                "restored object count") &&
         expect(restoredGroup != nullptr &&
                    documentObjectMatches(*restoredGroup, original.objects[0]),
                "restored group") &&
         expect(restoredCrate != nullptr &&
                    documentObjectMatches(*restoredCrate, original.objects[1]),
                "restored crate") &&
         expect(restoredCrate != nullptr &&
                    restoredCrate->transform.position.x == kOneThird &&
                    restoredCrate->transform.position.z == kPrecise &&
                    restoredDocument.gridSettings().cellSizeMeters ==
                        kOneThird &&
                    restoredDocument.documentSnapSettings().stepX == kPrecise,
                "precise doubles restored exactly");
}

bool buildRejectsDocumentWithInvalidId() {
  const cr::CreativeDocument document = cr::CreativeDocument::create("No Id");
  const iggy3d::ProductCreativeDocumentSectionBuildResult result =
      iggy3d::buildSaveCreativeDocumentSection(document);

  return expect(!result.section.present, "invalid id section absent") &&
         expect(result.receipt.requested, "invalid id requested") &&
         expect(!result.receipt.accepted, "invalid id not accepted") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::
                        InvalidDocumentId,
                "invalid id status") &&
         expect(result.receipt.reasonCode == "invalid_document_id",
                "invalid id reason");
}

iggy3d::SaveCreativeDocumentSection validSection() {
  return iggy3d::buildSaveCreativeDocumentSection(authoredDocument()).section;
}

bool restoreRejectsMissingSection() {
  const iggy3d::ProductCreativeDocumentSectionRestoreResult result =
      iggy3d::restoreCreativeDocumentFromSaveSection({});

  return expect(!result.receipt.accepted, "missing section rejected") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::
                        MissingSection,
                "missing section status") &&
         expect(result.receipt.reasonCode ==
                    "missing_creative_document_section",
                "missing section reason") &&
         expect(result.document.objectCount() == 0U,
                "missing section returns empty document");
}

bool restoreRejectsInvalidObjectKind() {
  iggy3d::SaveCreativeDocumentSection section = validSection();
  section.objects[1].kind = "DefinitelyNotAKind";
  const iggy3d::ProductCreativeDocumentSectionRestoreResult result =
      iggy3d::restoreCreativeDocumentFromSaveSection(section);

  return expect(!result.receipt.accepted, "invalid kind rejected") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::
                        InvalidObjectKind,
                "invalid kind status") &&
         expect(result.receipt.reasonCode == "invalid_object_kind",
                "invalid kind reason") &&
         expect(result.document.objectCount() == 0U,
                "invalid kind returns empty document");
}

bool restoreRejectsDuplicateObjectIds() {
  iggy3d::SaveCreativeDocumentSection section = validSection();
  section.objects[1].id = section.objects[0].id;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult result =
      iggy3d::restoreCreativeDocumentFromSaveSection(section);

  return expect(!result.receipt.accepted, "duplicate id rejected") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::
                        DuplicateObjectId,
                "duplicate id status") &&
         expect(result.receipt.reasonCode == "duplicate_object_id",
                "duplicate id reason");
}

bool restoreRejectsBadNextObjectId() {
  iggy3d::SaveCreativeDocumentSection section = validSection();
  section.nextObjectId = section.objects[1].id;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult result =
      iggy3d::restoreCreativeDocumentFromSaveSection(section);

  return expect(!result.receipt.accepted, "bad next id rejected") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::
                        InvalidNextObjectId,
                "bad next id status") &&
         expect(result.receipt.reasonCode == "invalid_next_object_id",
                "bad next id reason");
}

bool restoreRejectsInvalidSnapModeAndAxes() {
  iggy3d::SaveCreativeDocumentSection badMode = validSection();
  badMode.snapMode = "Magnetic";
  const iggy3d::ProductCreativeDocumentSectionRestoreResult modeResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(badMode);

  iggy3d::SaveCreativeDocumentSection badAxes = validSection();
  badAxes.snapAxes =
      static_cast<std::uint32_t>(
          std::numeric_limits<cr::CreativeDocumentSnapAxisMask>::max()) +
      1U;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult axesResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(badAxes);

  iggy3d::SaveCreativeDocumentSection badSettings = validSection();
  badSettings.snapStepX = 0.0;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult settingsResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(badSettings);

  return expect(!modeResult.receipt.accepted,
                "invalid snap mode rejected") &&
         expect(modeResult.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidSnap,
                "invalid snap mode status") &&
         expect(modeResult.receipt.reasonCode ==
                    "invalid_document_snap_settings",
                "invalid snap mode reason") &&
         expect(!axesResult.receipt.accepted,
                "invalid snap axes rejected") &&
         expect(axesResult.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidSnap,
                "invalid snap axes status") &&
         expect(settingsResult.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidSnap,
                "invalid snap settings status");
}

bool restoreRejectsTooLargeGridDimension() {
  iggy3d::SaveCreativeDocumentSection section = validSection();
  section.gridWidth =
      static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()) +
      1U;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult result =
      iggy3d::restoreCreativeDocumentFromSaveSection(section);

  return expect(!result.receipt.accepted, "too large grid rejected") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidGrid,
                "too large grid status") &&
         expect(result.receipt.reasonCode == "invalid_grid_settings",
                "too large grid reason");
}

}  // namespace

int main() {
  bool ok = true;
  ok = buildSectionCopiesDocumentExactly() && ok;
  ok = encodeDecodeAndRestoreRoundTripsDocument() && ok;
  ok = buildRejectsDocumentWithInvalidId() && ok;
  ok = restoreRejectsMissingSection() && ok;
  ok = restoreRejectsInvalidObjectKind() && ok;
  ok = restoreRejectsDuplicateObjectIds() && ok;
  ok = restoreRejectsBadNextObjectId() && ok;
  ok = restoreRejectsInvalidSnapModeAndAxes() && ok;
  ok = restoreRejectsTooLargeGridDimension() && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
