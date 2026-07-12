#include "app/iggy3d/creative/world/DocumentSection.hpp"
#include "runtime/save/SaveCodec.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <vector>

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
         sameVec3(lhs.rotationEulerRadians, rhs.rotationEulerRadians) &&
         sameVec3(lhs.scale, rhs.scale);
}

bool samePathPoints(std::span<const cr::CreativePathPoint> lhs,
                    std::span<const cr::CreativePathPoint> rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t index = 0; index < lhs.size(); ++index) {
    if (!sameVec3(lhs[index].position, rhs[index].position)) {
      return false;
    }
  }
  return true;
}

bool savePathPointsMatch(
    std::span<const iggy3d::SaveCreativeDocumentVec3Record> lhs,
    std::span<const cr::CreativePathPoint> rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t index = 0; index < lhs.size(); ++index) {
    if (lhs[index].x != rhs[index].position.x ||
        lhs[index].y != rhs[index].position.y ||
        lhs[index].z != rhs[index].position.z) {
      return false;
    }
  }
  return true;
}

std::vector<cr::CreativePathPoint> authoredPathPoints() {
  return {
      cr::CreativePathPoint{{kOneThird, 0.0, kPrecise}},
      cr::CreativePathPoint{{4.0, kOneThird, 6.0}},
      cr::CreativePathPoint{{7.0, 0.0, 8.0}},
  };
}

std::vector<cr::CreativePathPoint> authoredLineEndpoints() {
  return {
      cr::CreativePathPoint{{kPrecise, 0.0, kOneThird}},
      cr::CreativePathPoint{{4.0, 0.0, 6.0}},
  };
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

cr::CreativeObject restoredWallObject() {
  cr::CreativeObject object;
  object.id = 7;
  object.kind = cr::CreativeObjectKind::Wall;
  object.name = "Precise Wall";
  object.transform.position = {kOneThird, 2.0, kPrecise};
  object.transform.rotationEulerRadians = {0.0, kPrecise, 1.5};
  object.transform.scale = {1.0, 2.0, 3.0};
  object.bounds = {{0.0, -0.25, kOneThird}, {10.5, 4.25, 7.75}};
  object.layerId = 9;
  object.visible = false;
  object.locked = true;
  object.parentId = 2;
  object.tags = {"wall", "imported"};
  return object;
}

cr::CreativeObject restoredPatrolRouteObject() {
  cr::CreativeObject object;
  object.id = 11;
  object.kind = cr::CreativeObjectKind::PatrolRoute;
  object.name = "Precise Route";
  object.layerId = 5;
  object.visible = true;
  object.locked = false;
  object.tags = {"route", "patrol"};
  object.pathPoints = authoredPathPoints();
  return object;
}

cr::CreativeObject restoredNavLinkObject() {
  cr::CreativeObject object;
  object.id = 12;
  object.kind = cr::CreativeObjectKind::NavLink;
  object.name = "Precise Link";
  object.layerId = 6;
  object.visible = true;
  object.locked = false;
  object.tags = {"link", "navigation"};
  object.pathPoints = authoredLineEndpoints();
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
  request.objects = {restoredGroupObject(), restoredWallObject()};
  return request;
}

cr::CreativeDocumentRestoreRequest authoredPathRestoreRequest() {
  cr::CreativeDocumentRestoreRequest request;
  request.documentId = 9002;
  request.name = "Path Creative";
  request.units = cr::CreativeUnits::Meters;
  request.gridSettings = authoredGridSettings();
  request.snapSettings = authoredSnapSettings();
  request.worldBounds = authoredWorldBounds();
  request.nextObjectId = 25;
  request.objects = {restoredPatrolRouteObject(), restoredNavLinkObject()};
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

cr::CreativeDocument authoredPathDocument() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Before Path");
  const cr::CreativeDocumentRestoreReceipt restored =
      document.restoreForLoad(authoredPathRestoreRequest());
  if (!restored.accepted) {
    std::cerr << "FAIL: authored path document restore setup\n";
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
         save.kind == std::string{cr::serializedObjectKindId(object.kind)} &&
         save.name == object.name &&
         save.transform.position.x == object.transform.position.x &&
         save.transform.position.y == object.transform.position.y &&
         save.transform.position.z == object.transform.position.z &&
         save.transform.rotation.x == object.transform.rotationEulerRadians.x &&
         save.transform.rotation.y == object.transform.rotationEulerRadians.y &&
         save.transform.rotation.z == object.transform.rotationEulerRadians.z &&
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
         save.tags == object.tags &&
         savePathPointsMatch(save.pathPoints, object.pathPoints);
}

bool documentObjectMatches(const cr::CreativeObject& lhs,
                           const cr::CreativeObject& rhs) {
  return lhs.id == rhs.id && lhs.kind == rhs.kind && lhs.name == rhs.name &&
         sameTransform(lhs.transform, rhs.transform) &&
         sameBounds(lhs.bounds, rhs.bounds) && lhs.layerId == rhs.layerId &&
         lhs.visible == rhs.visible && lhs.locked == rhs.locked &&
         lhs.parentId == rhs.parentId && lhs.tags == rhs.tags &&
         samePathPoints(lhs.pathPoints, rhs.pathPoints);
}

bool buildSectionCopiesDocumentExactly() {
  const cr::CreativeDocument document = authoredDocument();
  const cr::CreativeDocumentRestoreRequest original = authoredRestoreRequest();
  const cr::CreativeObject& group = original.objects[0];
  const cr::CreativeObject& wall = original.objects[1];
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
         expect(section.version == iggy3d::kSaveCreativeDocumentSectionVersion,
                "section version") &&
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
         expect(sectionObjectMatches(section.objects[1], wall),
                "section wall object");
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
  const cr::CreativeObject* restoredWall = restoredDocument.findObject(7);
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
         expect(restoredWall != nullptr &&
                    documentObjectMatches(*restoredWall, original.objects[1]),
                "restored wall") &&
         expect(restoredWall != nullptr &&
                    restoredWall->transform.position.x == kOneThird &&
                    restoredWall->transform.position.z == kPrecise &&
                    restoredDocument.gridSettings().cellSizeMeters ==
                        kOneThird &&
                    restoredDocument.documentSnapSettings().stepX == kPrecise,
                "precise doubles restored exactly");
}

bool buildSectionCopiesPathPoints() {
  const cr::CreativeDocument document = authoredPathDocument();
  const cr::CreativeDocumentRestoreRequest original =
      authoredPathRestoreRequest();
  const cr::CreativeObject& route = original.objects[0];
  const cr::CreativeObject& link = original.objects[1];
  const iggy3d::ProductCreativeDocumentSectionBuildResult result =
      iggy3d::buildSaveCreativeDocumentSection(document);
  const iggy3d::SaveCreativeDocumentSection& section = result.section;

  return expect(result.receipt.accepted, "path build accepted") &&
         expect(section.objects.size() == 2U, "path build object count") &&
         expect(section.objects[0].kind == "PatrolRoute",
                "path build kind") &&
         expect(section.objects[0].pathPoints.size() == 3U,
                "path build path count") &&
         expect(savePathPointsMatch(section.objects[0].pathPoints,
                                    route.pathPoints),
                "path build points exact") &&
         expect(section.objects[1].kind == "NavLink",
                "line endpoint build kind") &&
         expect(section.objects[1].pathPoints.size() == 2U,
                "line endpoint build path count") &&
         expect(savePathPointsMatch(section.objects[1].pathPoints,
                                    link.pathPoints),
                "line endpoint build points exact");
}

bool restoreSectionRestoresPathPoints() {
  const cr::CreativeDocument document = authoredPathDocument();
  const iggy3d::SaveCreativeDocumentSection section =
      iggy3d::buildSaveCreativeDocumentSection(document).section;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult result =
      iggy3d::restoreCreativeDocumentFromSaveSection(section);
  const cr::CreativeObject* route = result.document.findObject(11);
  const cr::CreativeObject* link = result.document.findObject(12);

  return expect(result.receipt.accepted, "path restore accepted") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::Converted,
                "path restore converted") &&
         expect(result.document.id() == 9002U, "path restore document id") &&
         expect(result.document.objectCount() == 2U,
                "path restore object count") &&
         expect(result.document.nextObjectId() == 25U,
                "path restore next object id") &&
         expect(result.document.revision() == 0U,
                "path restore revision zero") &&
         expect(result.document.dirtyFlags() == 0U,
                "path restore dirty zero") &&
         expect(route != nullptr, "path restore route findable") &&
         expect(route != nullptr &&
                    samePathPoints(route->pathPoints, authoredPathPoints()),
                "path restore points exact") &&
         expect(link != nullptr, "line endpoint restore link findable") &&
         expect(link != nullptr &&
                    samePathPoints(link->pathPoints,
                                   authoredLineEndpoints()),
                "line endpoint restore points exact");
}

bool encodeDecodeAndRestoreRoundTripsPathAndLineEndpointPayloads() {
  const cr::CreativeDocument document = authoredPathDocument();
  const iggy3d::ProductCreativeDocumentSectionBuildResult built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  envelope.creativeDocument = built.section;

  const iggy3d::SaveEncodeResult encoded = iggy3d::encodeSaveEnvelope(envelope);
  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::ProductCreativeDocumentSectionRestoreResult restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(
          decoded.envelope.creativeDocument);
  const cr::CreativeObject* route = restored.document.findObject(11);
  const cr::CreativeObject* link = restored.document.findObject(12);

  return expect(built.receipt.accepted,
                "path line endpoint codec build accepted") &&
         expect(encoded.status == iggy3d::SaveCodecStatus::Ok,
                "path line endpoint codec encode ok") &&
         expect(encoded.encodedText.find(
                    "creativeDocument.object.1.kind=NavLink\n") !=
                    std::string::npos,
                "line endpoint kind encoded") &&
         expect(encoded.encodedText.find(
                    "creativeDocument.object.1.pathPoint.count=2\n") !=
                    std::string::npos,
                "line endpoint point count encoded") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok,
                "path line endpoint codec decode ok") &&
         expect(restored.receipt.accepted,
                "path line endpoint codec restore accepted") &&
         expect(restored.document.objectCount() == 2U,
                "path line endpoint codec object count") &&
         expect(route != nullptr, "path line endpoint codec route findable") &&
         expect(route != nullptr &&
                    samePathPoints(route->pathPoints, authoredPathPoints()),
                "path line endpoint codec route points") &&
         expect(link != nullptr, "path line endpoint codec link findable") &&
         expect(link != nullptr &&
                    samePathPoints(link->pathPoints,
                                   authoredLineEndpoints()),
                "path line endpoint codec link endpoints");
}

bool restoreRejectsInvalidPathPayloads() {
  const iggy3d::SaveCreativeDocumentSection valid =
      iggy3d::buildSaveCreativeDocumentSection(authoredPathDocument()).section;

  iggy3d::SaveCreativeDocumentSection missing = valid;
  missing.objects[0].pathPoints.clear();
  const iggy3d::ProductCreativeDocumentSectionRestoreResult missingResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(missing);

  iggy3d::SaveCreativeDocumentSection onePoint = valid;
  onePoint.objects[0].pathPoints.resize(1U);
  const iggy3d::ProductCreativeDocumentSectionRestoreResult onePointResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(onePoint);

  iggy3d::SaveCreativeDocumentSection nonFinite = valid;
  nonFinite.objects[0].pathPoints[1].x =
      std::numeric_limits<double>::quiet_NaN();
  const iggy3d::ProductCreativeDocumentSectionRestoreResult nonFiniteResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(nonFinite);

  return expect(!missingResult.receipt.accepted,
                "missing path points rejected") &&
         expect(missingResult.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidObject,
                "missing path points status") &&
         expect(missingResult.receipt.reasonCode == "invalid_path_points",
                "missing path points reason") &&
         expect(missingResult.document.objectCount() == 0U,
                "missing path returns empty document") &&
         expect(!onePointResult.receipt.accepted,
                "one path point rejected") &&
         expect(onePointResult.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidObject,
                "one path point status") &&
         expect(onePointResult.receipt.reasonCode == "invalid_path_points",
                "one path point reason") &&
         expect(!nonFiniteResult.receipt.accepted,
                "nonfinite path point rejected") &&
         expect(nonFiniteResult.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidObject,
                "nonfinite path point status") &&
         expect(nonFiniteResult.receipt.reasonCode == "invalid_path_points",
                "nonfinite path point reason");
}

bool restoreRejectsInvalidLineEndpointPayloads() {
  const iggy3d::SaveCreativeDocumentSection valid =
      iggy3d::buildSaveCreativeDocumentSection(authoredPathDocument()).section;

  iggy3d::SaveCreativeDocumentSection tooMany = valid;
  tooMany.objects[1].pathPoints.push_back({7.0, 0.0, 8.0});
  const iggy3d::ProductCreativeDocumentSectionRestoreResult tooManyResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(tooMany);

  iggy3d::SaveCreativeDocumentSection nonFinite = valid;
  nonFinite.objects[1].pathPoints[0].x =
      std::numeric_limits<double>::quiet_NaN();
  const iggy3d::ProductCreativeDocumentSectionRestoreResult nonFiniteResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(nonFinite);

  return expect(!tooManyResult.receipt.accepted,
                "too many line endpoints rejected") &&
         expect(tooManyResult.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidObject,
                "too many line endpoints status") &&
         expect(tooManyResult.receipt.reasonCode == "invalid_line_endpoints",
                "too many line endpoints reason") &&
         expect(!nonFiniteResult.receipt.accepted,
                "nonfinite line endpoint rejected") &&
         expect(nonFiniteResult.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidObject,
                "nonfinite line endpoint status") &&
         expect(nonFiniteResult.receipt.reasonCode == "invalid_line_endpoints",
                "nonfinite line endpoint reason");
}

bool restoreRejectsNonPathObjectCarryingPathPoints() {
  iggy3d::SaveCreativeDocumentSection section =
      iggy3d::buildSaveCreativeDocumentSection(authoredDocument()).section;
  if (section.objects.size() < 2U) {
    return expect(false, "nonpath path setup object count");
  }
  section.objects[1].pathPoints = {
      {1.0, 0.0, 2.0},
      {3.0, 0.0, 4.0},
  };
  const iggy3d::ProductCreativeDocumentSectionRestoreResult result =
      iggy3d::restoreCreativeDocumentFromSaveSection(section);

  return expect(!result.receipt.accepted,
                "nonpath path points rejected") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidObject,
                "nonpath path points status") &&
         expect(result.receipt.reasonCode == "path_unsupported",
                "nonpath path points reason") &&
         expect(result.document.objectCount() == 0U,
                "nonpath path returns empty document");
}

bool restoreRejectsUnsupportedParentPayload() {
  iggy3d::SaveCreativeDocumentSection section =
      iggy3d::buildSaveCreativeDocumentSection(authoredDocument()).section;
  if (section.objects.size() < 2U) {
    return expect(false, "unsupported parent setup object count");
  }
  section.objects[1].kind = "Crate";
  section.objects[1].name = "Unsupported Parented Crate";

  const iggy3d::ProductCreativeDocumentSectionRestoreResult result =
      iggy3d::restoreCreativeDocumentFromSaveSection(section);

  return expect(!result.receipt.accepted,
                "unsupported parent restore rejected") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidObject,
                "unsupported parent restore status") &&
         expect(result.receipt.reasonCode == "parent_unsupported",
                "unsupported parent restore reason") &&
         expect(result.document.objectCount() == 0U,
                "unsupported parent returns empty document");
}

bool restoreRejectsMissingParentPayload() {
  iggy3d::SaveCreativeDocumentSection section =
      iggy3d::buildSaveCreativeDocumentSection(authoredDocument()).section;
  if (section.objects.size() < 2U) {
    return expect(false, "missing parent setup object count");
  }
  section.objects[1].parentId = 404;

  const iggy3d::ProductCreativeDocumentSectionRestoreResult result =
      iggy3d::restoreCreativeDocumentFromSaveSection(section);

  return expect(!result.receipt.accepted,
                "missing parent restore rejected") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidObject,
                "missing parent restore status") &&
         expect(result.receipt.reasonCode == "missing_parent",
                "missing parent restore reason") &&
         expect(result.document.objectCount() == 0U,
                "missing parent returns empty document");
}

bool restoreRejectsUnsupportedParentOwnerPayload() {
  iggy3d::SaveCreativeDocumentSection section =
      iggy3d::buildSaveCreativeDocumentSection(authoredDocument()).section;
  if (section.objects.size() < 2U) {
    return expect(false, "unsupported owner setup object count");
  }

  iggy3d::SaveCreativeDocumentObjectRecord crateParent = section.objects[1];
  crateParent.id = 6;
  crateParent.kind = "Crate";
  crateParent.name = "Unsupported Owner Crate";
  crateParent.hasParent = false;
  crateParent.parentId = cr::kInvalidObjectId;

  iggy3d::SaveCreativeDocumentObjectRecord wallChild = section.objects[1];
  wallChild.parentId = crateParent.id;
  section.objects = {section.objects[0], crateParent, wallChild};

  const iggy3d::ProductCreativeDocumentSectionRestoreResult result =
      iggy3d::restoreCreativeDocumentFromSaveSection(section);

  return expect(!result.receipt.accepted,
                "unsupported owner restore rejected") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidObject,
                "unsupported owner restore status") &&
         expect(result.receipt.reasonCode == "parent_owner_unsupported",
                "unsupported owner restore reason") &&
         expect(result.document.objectCount() == 0U,
                "unsupported owner returns empty document");
}

bool restoreRejectsParentCyclePayload() {
  iggy3d::SaveCreativeDocumentSection section =
      iggy3d::buildSaveCreativeDocumentSection(authoredDocument()).section;
  if (section.objects.size() < 2U) {
    return expect(false, "parent cycle setup object count");
  }

  section.objects[0].kind = "Group";
  section.objects[0].hasParent = true;
  section.objects[0].parentId = section.objects[1].id;
  section.objects[1].kind = "Group";
  section.objects[1].name = "Cycled Group";
  section.objects[1].hasParent = true;
  section.objects[1].parentId = section.objects[0].id;

  const iggy3d::ProductCreativeDocumentSectionRestoreResult result =
      iggy3d::restoreCreativeDocumentFromSaveSection(section);

  return expect(!result.receipt.accepted, "parent cycle restore rejected") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidObject,
                "parent cycle restore status") &&
         expect(result.receipt.reasonCode == "parent_cycle",
                "parent cycle restore reason") &&
         expect(result.document.objectCount() == 0U,
                "parent cycle returns empty document");
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
  if (section.objects.size() < 2U) {
    return expect(false, "invalid kind setup object count");
  }
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

bool restoreRejectsDisplayNameAsObjectKind() {
  iggy3d::SaveCreativeDocumentSection section = validSection();
  if (section.objects.size() < 2U) {
    return expect(false, "display name kind setup object count");
  }
  section.objects[1].kind = "Moving Platform";
  const iggy3d::ProductCreativeDocumentSectionRestoreResult result =
      iggy3d::restoreCreativeDocumentFromSaveSection(section);

  return expect(!result.receipt.accepted, "display name kind rejected") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::
                        InvalidObjectKind,
                "display name kind status") &&
         expect(result.receipt.reasonCode == "invalid_object_kind",
                "display name kind reason") &&
         expect(result.document.objectCount() == 0U,
                "display name kind returns empty document");
}

bool restoreRejectsDuplicateObjectIds() {
  iggy3d::SaveCreativeDocumentSection section = validSection();
  if (section.objects.size() < 2U) {
    return expect(false, "duplicate id setup object count");
  }
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
  if (section.objects.size() < 2U) {
    return expect(false, "bad next setup object count");
  }
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

bool voxelChunksEncodeDecodeAndRestore() {
  cr::CreativeDocument document = authoredDocument();
  const std::array edits{
      cr::CreativeVoxelEdit{{-1, 2, 3}, cr::CreativeObjectKind::Wall},
      cr::CreativeVoxelEdit{{16, 2, 3}, cr::CreativeObjectKind::Floor},
      cr::CreativeVoxelEdit{{17, 2, 3}, cr::CreativeObjectKind::Crate},
  };
  const cr::CreativeVoxelMutationReceipt mutation =
      document.applyVoxelEdits(edits);
  const iggy3d::ProductCreativeDocumentSectionBuildResult built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  envelope.creativeDocument = built.section;
  const iggy3d::SaveEncodeResult encoded =
      iggy3d::encodeSaveEnvelope(envelope);
  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::ProductCreativeDocumentSectionRestoreResult restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(
          decoded.envelope.creativeDocument);

  return expect(mutation.accepted && mutation.changed,
                "voxel save setup applies") &&
         expect(built.receipt.accepted &&
                    built.section.version ==
                        iggy3d::kSaveCreativeDocumentSectionVersion &&
                    built.section.voxelChunks.size() == 2U,
                "save section stores sparse chunks") &&
         expect(encoded.status == iggy3d::SaveCodecStatus::Ok &&
                    encoded.encodedText.find(
                        "creativeDocument.voxelChunk.count=2\n") !=
                        std::string::npos,
                "codec writes voxel chunk key") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok &&
                    decoded.envelope.creativeDocument.voxelChunks.size() == 2U,
                "codec decodes voxel chunks") &&
         expect(restored.receipt.accepted &&
                    restored.document.voxelField().occupiedCellCount() == 3U,
                "restore owns three voxel cells") &&
         expect(restored.document.voxelField().materialAt({-1, 2, 3}) ==
                    cr::CreativeObjectKind::Wall &&
                    restored.document.voxelField().materialAt({16, 2, 3}) ==
                        cr::CreativeObjectKind::Floor &&
                    restored.document.voxelField().materialAt({17, 2, 3}) ==
                        cr::CreativeObjectKind::Crate,
                "restored voxel coordinates and materials match");
}

bool terrainControlsEncodeDecodeAndRestore() {
  cr::CreativeDocument document = authoredDocument();
  const std::array edits{
      cr::CreativeTerrainControlEdit{
          cr::CreativeTerrainEditKind::Upsert, {{-4, 7}, 5, 3}},
      cr::CreativeTerrainControlEdit{
          cr::CreativeTerrainEditKind::Upsert, {{8, -2}, 12, 6}},
  };
  const cr::CreativeTerrainMutationReceipt mutation =
      document.applyTerrainControlEdits(edits);
  const iggy3d::ProductCreativeDocumentSectionBuildResult built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  envelope.creativeDocument = built.section;
  const iggy3d::SaveEncodeResult encoded =
      iggy3d::encodeSaveEnvelope(envelope);
  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::ProductCreativeDocumentSectionRestoreResult restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(
          decoded.envelope.creativeDocument);
  const cr::CreativeTerrainControlPoint* first =
      restored.document.terrainField().controlAt({-4, 7});
  const cr::CreativeTerrainControlPoint* second =
      restored.document.terrainField().controlAt({8, -2});

  return expect(mutation.accepted && mutation.changed,
                "terrain save setup applies") &&
         expect(built.receipt.accepted &&
                    built.receipt.terrainControlCount == 2U &&
                    built.section.terrainControls.size() == 2U,
                "save section stores terrain controls") &&
         expect(encoded.status == iggy3d::SaveCodecStatus::Ok &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainControl.count=2\n") !=
                        std::string::npos,
                "codec writes terrain control block") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok &&
                    decoded.envelope.creativeDocument.terrainControls.size() ==
                        2U,
                "codec decodes terrain controls") &&
         expect(restored.receipt.accepted && first != nullptr &&
                    first->heightCells == 5U && first->radiusCells == 3U &&
                    second != nullptr && second->heightCells == 12U &&
                    second->radiusCells == 6U,
                "restore preserves terrain coordinates and settings");
}

bool terrainMaterialsEncodeDecodeAndRestore() {
  cr::CreativeDocument document = authoredDocument();
  const std::array edits{
      cr::CreativeTerrainMaterialEdit{
          cr::CreativeTerrainMaterialEditKind::Set, {-4, 7},
          cr::CreativeTerrainMaterial::Stone},
      cr::CreativeTerrainMaterialEdit{
          cr::CreativeTerrainMaterialEditKind::Set, {8, -2},
          cr::CreativeTerrainMaterial::Sand},
  };
  const cr::CreativeTerrainMaterialMutationReceipt mutation =
      document.applyTerrainMaterialEdits(edits);
  const iggy3d::ProductCreativeDocumentSectionBuildResult built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  envelope.creativeDocument = built.section;
  const iggy3d::SaveEncodeResult encoded =
      iggy3d::encodeSaveEnvelope(envelope);
  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::ProductCreativeDocumentSectionRestoreResult restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(
          decoded.envelope.creativeDocument);

  return expect(mutation.accepted && mutation.changed,
                "terrain material save setup applies") &&
         expect(built.receipt.accepted &&
                    built.receipt.terrainMaterialOverrideCount == 2U &&
                    built.section.terrainMaterials.size() == 2U,
                "save section stores terrain material overrides") &&
         expect(encoded.status == iggy3d::SaveCodecStatus::Ok &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainMaterial.count=2\n") !=
                        std::string::npos,
                "codec writes terrain material block") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok &&
                    decoded.envelope.creativeDocument.terrainMaterials.size() ==
                        2U,
                "codec decodes terrain material overrides") &&
         expect(restored.receipt.accepted &&
                    restored.document.terrainMaterialField().materialAt(
                        {-4, 7}) == cr::CreativeTerrainMaterial::Stone &&
                    restored.document.terrainMaterialField().materialAt(
                        {8, -2}) == cr::CreativeTerrainMaterial::Sand &&
                    restored.document.terrainMaterialField().materialAt(
                        {0, 0}) == cr::CreativeTerrainMaterial::Grass,
                "restore preserves overrides and sparse grass default");
}

}  // namespace

int main() {
  bool ok = true;
  ok = buildSectionCopiesDocumentExactly() && ok;
  ok = encodeDecodeAndRestoreRoundTripsDocument() && ok;
  ok = buildSectionCopiesPathPoints() && ok;
  ok = restoreSectionRestoresPathPoints() && ok;
  ok = encodeDecodeAndRestoreRoundTripsPathAndLineEndpointPayloads() && ok;
  ok = restoreRejectsInvalidPathPayloads() && ok;
  ok = restoreRejectsInvalidLineEndpointPayloads() && ok;
  ok = restoreRejectsNonPathObjectCarryingPathPoints() && ok;
  ok = restoreRejectsUnsupportedParentPayload() && ok;
  ok = restoreRejectsMissingParentPayload() && ok;
  ok = restoreRejectsUnsupportedParentOwnerPayload() && ok;
  ok = restoreRejectsParentCyclePayload() && ok;
  ok = buildRejectsDocumentWithInvalidId() && ok;
  ok = restoreRejectsMissingSection() && ok;
  ok = restoreRejectsInvalidObjectKind() && ok;
  ok = restoreRejectsDisplayNameAsObjectKind() && ok;
  ok = restoreRejectsDuplicateObjectIds() && ok;
  ok = restoreRejectsBadNextObjectId() && ok;
  ok = restoreRejectsInvalidSnapModeAndAxes() && ok;
  ok = restoreRejectsTooLargeGridDimension() && ok;
  ok = voxelChunksEncodeDecodeAndRestore() && ok;
  ok = terrainControlsEncodeDecodeAndRestore() && ok;
  ok = terrainMaterialsEncodeDecodeAndRestore() && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
