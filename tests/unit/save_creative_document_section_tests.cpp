#include "runtime/save/SaveCodec.hpp"
#include "runtime/save/SaveCompatibility.hpp"

#include <iostream>
#include <string>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

std::string replaceFirst(std::string text,
                         std::string_view value,
                         std::string_view replacement) {
  const std::size_t pos = text.find(value);
  if (pos != std::string::npos) {
    text.replace(pos, value.size(), replacement);
  }
  return text;
}

std::string replaceValueForKey(std::string text,
                               std::string_view key,
                               std::string_view replacement) {
  const std::string needle = std::string{key} + "=";
  const std::size_t pos = text.find(needle);
  if (pos == std::string::npos) {
    return text;
  }
  const std::size_t valueBegin = pos + needle.size();
  const std::size_t valueEnd = text.find('\n', valueBegin);
  text.replace(valueBegin,
               valueEnd == std::string::npos ? std::string::npos
                                              : valueEnd - valueBegin,
               replacement);
  return text;
}

std::string eraseLineForKey(std::string text, std::string_view key) {
  const std::string needle = std::string{key} + "=";
  const std::size_t pos = text.find(needle);
  if (pos == std::string::npos) {
    return text;
  }
  const std::size_t lineEnd = text.find('\n', pos);
  text.erase(pos,
             lineEnd == std::string::npos ? std::string::npos
                                          : lineEnd + 1U - pos);
  return text;
}

iggy3d::SaveEnvelope minimalEnvelope() {
  iggy3d::SaveEnvelope envelope;
  envelope.metadata.savedStateHash = 0;
  envelope.metadata.savedStateHashHex = "0000000000000000";
  return envelope;
}

bool defaultEnvelopeOmitsCreativeDocumentSection() {
  const iggy3d::SaveEnvelope envelope = minimalEnvelope();
  const iggy3d::SaveEncodeResult encoded = iggy3d::encodeSaveEnvelope(envelope);
  const iggy3d::SaveDecodeResult decoded = iggy3d::decodeSaveEnvelope(encoded.encodedText);

  return expect(encoded.status == iggy3d::SaveCodecStatus::Ok, "default encode ok") &&
         expect(encoded.encodedText.find("metadata.schemaVersion=3\n") !=
                    std::string::npos,
                "schema v3 metadata encoded") &&
         expect(encoded.encodedText.find("creativeDocument.present=") ==
                    std::string::npos,
                "creative document absent by default") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok, "default decode ok") &&
         expect(!decoded.envelope.creativeDocument.present,
                "creative document defaults absent after decode");
}

bool creativeDocumentSectionRoundTripsThroughSaveCodec() {
  constexpr double kOneThird = 1.0 / 3.0;
  constexpr double kPrecise = 0.1234567890123;

  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  envelope.authoredRoom.present = true;
  envelope.authoredRoom.id = "authored_before_creative";

  iggy3d::SaveCreativeDocumentSection& section = envelope.creativeDocument;
  section.present = true;
  section.version = iggy3d::kSaveCreativeDocumentSectionVersion;
  section.documentId = 77;
  section.name = "Creative Save = Alpha";
  section.units = "Meters";
  section.gridOrigin = {kOneThird, kPrecise, -7.25};
  section.cellSizeMeters = kOneThird;
  section.gridWidth = 64;
  section.gridHeight = 32;
  section.gridDepth = 8;
  section.snapMode = "Grid";
  section.snapAxes = 7;
  section.snapStepX = kPrecise;
  section.snapStepY = 2.5;
  section.snapStepZ = kOneThird;
  section.snapOriginX = -1.25;
  section.snapOriginY = kOneThird;
  section.snapOriginZ = 4.75;
  section.worldBounds.min = {-8.0, -1.0, -4.0};
  section.worldBounds.max = {64.0, 32.0, 8.0};
  section.nextObjectId = 43;

  iggy3d::SaveCreativeDocumentObjectRecord object;
  object.id = 42;
  object.kind = "PatrolRoute";
  object.name = "Visible Route";
  object.assetId = "boulder_01";
  object.transform.position = {kOneThird, 2.0, kPrecise};
  object.transform.rotation = {0.0, kPrecise, 1.5};
  object.transform.scale = {1.0, 2.0, 3.0};
  object.bounds.min = {0.0, 0.0, 0.0};
  object.bounds.max = {10.5, 4.25, 7.75};
  object.layerId = 3;
  object.visible = false;
  object.locked = true;
  object.hasParent = true;
  object.parentId = 7;
  object.attachmentSocket = "door_frame";
  object.tags = {"boss=room", "line\nbreak"};
  object.pathPoints = {
      {kOneThird, 0.0, kPrecise, 0.0, 0.75},
      {kPrecise, kOneThird, 4.75, 1.25, 2.25},
      {-7.25, 2.5, kOneThird, 0.5, 1.0},
  };
  section.objects.push_back(object);
  section.logicLinks.push_back({7U, 42U, "Open"});

  const iggy3d::SaveEncodeResult encoded = iggy3d::encodeSaveEnvelope(envelope);
  const std::size_t authoredPos =
      encoded.encodedText.find("authoredRoom.marker.count=0\n");
  const std::size_t creativePos =
      encoded.encodedText.find("creativeDocument.present=true\n");
  const std::size_t playersPos = encoded.encodedText.find("players.slot.count=0\n");
  const iggy3d::SaveDecodeResult decoded = iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::SaveCreativeDocumentSection& decodedSection =
      decoded.envelope.creativeDocument;
  const iggy3d::SaveCreativeDocumentObjectRecord* decodedObject =
      decodedSection.objects.empty() ? nullptr : &decodedSection.objects.front();

  bool ok = expect(encoded.status == iggy3d::SaveCodecStatus::Ok,
                   "creative encode ok") &&
            expect(authoredPos != std::string::npos &&
                       creativePos != std::string::npos &&
                       playersPos != std::string::npos &&
                       authoredPos < creativePos && creativePos < playersPos,
                   "creative section ordered after authored room before players") &&
            expect(encoded.encodedText.find("creativeDocument.documentId=77\n") !=
                       std::string::npos,
                   "document id encoded") &&
            expect(encoded.encodedText.find("creativeDocument.name=Creative Save %3D Alpha\n") !=
                       std::string::npos,
                   "document name escaped") &&
            expect(encoded.encodedText.find("creativeDocument.grid.origin=") !=
                       std::string::npos,
                   "grid origin encoded") &&
            expect(encoded.encodedText.find("creativeDocument.grid.cellSizeMeters=0.333\n") ==
                       std::string::npos,
                   "creative double not fixed-three truncated") &&
            expect(encoded.encodedText.find("creativeDocument.object.0.kind=PatrolRoute\n") !=
                       std::string::npos,
                   "object kind encoded") &&
            expect(encoded.encodedText.find(
                       "creativeDocument.object.0.assetId=boulder_01\n") !=
                       std::string::npos,
                   "object asset id encoded") &&
            expect(encoded.encodedText.find("creativeDocument.object.0.hasParent=true\n") !=
                       std::string::npos,
                   "object parent presence encoded") &&
            expect(encoded.encodedText.find("creativeDocument.object.0.tag.count=2\n") !=
                       std::string::npos,
                   "object tag count encoded") &&
            expect(encoded.encodedText.find(
                       "creativeDocument.object.0.pathPoint.count=3\n") !=
                       std::string::npos,
                   "object path point count encoded") &&
            expect(encoded.encodedText.find(
                       "creativeDocument.object.0.pathPoint.1.position=") !=
                       std::string::npos,
                   "object path point position encoded") &&
            expect(encoded.encodedText.find(
                       "creativeDocument.object.0.pathPoint.1.dwellSeconds=1.25\n") !=
                       std::string::npos,
                   "object path point dwell encoded") &&
            expect(encoded.encodedText.find(
                       "creativeDocument.object.0.pathPoint.1."
                       "outgoingSpeedMultiplier=2.25\n") !=
                       std::string::npos,
                   "object path point segment speed encoded") &&
            expect(encoded.encodedText.find(
                       "creativeDocument.logicLink.0.action=Open\n") !=
                       std::string::npos,
                   "logic link action encoded") &&
            expect(decoded.status == iggy3d::SaveCodecStatus::Ok, "creative decode ok") &&
            expect(decodedSection.present, "creative present decoded") &&
            expect(decodedSection.documentId == 77U, "document id decoded") &&
            expect(decodedSection.name == "Creative Save = Alpha", "document name decoded") &&
            expect(decodedSection.units == "Meters", "units decoded") &&
            expect(decodedSection.gridOrigin.x == kOneThird &&
                       decodedSection.gridOrigin.y == kPrecise &&
                       decodedSection.gridOrigin.z == -7.25,
                   "grid origin round trips exactly") &&
            expect(decodedSection.cellSizeMeters == kOneThird,
                   "cell size round trips exactly") &&
            expect(decodedSection.gridWidth == 64U && decodedSection.gridHeight == 32U &&
                       decodedSection.gridDepth == 8U,
                   "grid dimensions decoded") &&
            expect(decodedSection.snapMode == "Grid" && decodedSection.snapAxes == 7U,
                   "snap mode axes decoded") &&
            expect(decodedSection.snapStepX == kPrecise &&
                       decodedSection.snapStepY == 2.5 &&
                       decodedSection.snapStepZ == kOneThird,
                   "snap steps decoded") &&
            expect(decodedSection.snapOriginX == -1.25 &&
                       decodedSection.snapOriginY == kOneThird &&
                       decodedSection.snapOriginZ == 4.75,
                   "snap origins decoded") &&
            expect(decodedSection.worldBounds.min.x == -8.0 &&
                       decodedSection.worldBounds.max.y == 32.0,
                   "world bounds decoded") &&
            expect(decodedSection.nextObjectId == 43U, "next object id decoded") &&
            expect(decodedSection.objects.size() == 1U, "object count decoded") &&
            expect(decodedSection.logicLinks.size() == 1U &&
                       decodedSection.logicLinks[0].sourceObjectId == 7U &&
                       decodedSection.logicLinks[0].targetObjectId == 42U &&
                       decodedSection.logicLinks[0].action == "Open",
                   "logic link decoded");

  ok = ok && expect(decodedObject != nullptr && decodedObject->id == 42U,
                    "object id decoded") &&
       expect(decodedObject != nullptr && decodedObject->kind == "PatrolRoute",
              "object kind decoded") &&
       expect(decodedObject != nullptr && decodedObject->name == "Visible Route",
              "object name decoded") &&
       expect(decodedObject != nullptr && decodedObject->assetId == "boulder_01",
              "object asset id decoded") &&
       expect(decodedObject != nullptr &&
                  decodedObject->transform.position.x == kOneThird &&
                  decodedObject->transform.position.z == kPrecise,
              "object transform position decoded") &&
       expect(decodedObject != nullptr &&
                  decodedObject->transform.rotation.y == kPrecise &&
                  decodedObject->transform.scale.z == 3.0,
              "object transform rotation scale decoded") &&
       expect(decodedObject != nullptr && decodedObject->bounds.max.x == 10.5 &&
                  decodedObject->bounds.max.z == 7.75,
              "object bounds decoded") &&
       expect(decodedObject != nullptr && decodedObject->layerId == 3U,
              "object layer decoded") &&
       expect(decodedObject != nullptr && !decodedObject->visible && decodedObject->locked,
              "object visible locked decoded") &&
       expect(decodedObject != nullptr && decodedObject->hasParent &&
                  decodedObject->parentId == 7U,
              "object parent decoded") &&
       expect(decodedObject != nullptr &&
                  decodedObject->attachmentSocket == "door_frame",
              "object attachment socket decoded") &&
       expect(decodedObject != nullptr && decodedObject->tags.size() == 2U &&
                  decodedObject->tags[0] == "boss=room" &&
                  decodedObject->tags[1] == "line\nbreak",
              "object tags decoded") &&
       expect(decodedObject != nullptr && decodedObject->pathPoints.size() == 3U,
              "object path point count decoded") &&
       expect(decodedObject != nullptr &&
                  decodedObject->pathPoints[0].x == kOneThird &&
                  decodedObject->pathPoints[0].z == kPrecise &&
                  decodedObject->pathPoints[1].x == kPrecise &&
                  decodedObject->pathPoints[1].y == kOneThird &&
                  decodedObject->pathPoints[1].dwellSeconds == 1.25 &&
                  decodedObject->pathPoints[1].outgoingSpeedMultiplier == 2.25 &&
                  decodedObject->pathPoints[2].x == -7.25 &&
                  decodedObject->pathPoints[2].z == kOneThird &&
                  decodedObject->pathPoints[2].dwellSeconds == 0.5 &&
                  decodedObject->pathPoints[2].outgoingSpeedMultiplier == 1.0,
              "object path points decoded exactly");

  std::string version6Text = encoded.encodedText;
  const std::string currentVersionLine =
      "creativeDocument.version=" +
      std::to_string(iggy3d::kSaveCreativeDocumentSectionVersion) + "\n";
  if (const std::size_t versionPosition =
          version6Text.find(currentVersionLine);
      versionPosition != std::string::npos) {
    version6Text.replace(versionPosition, currentVersionLine.size(),
                         "creativeDocument.version=6\n");
  }
  const std::string linkBlock =
      "creativeDocument.logicLink.count=1\n"
      "creativeDocument.logicLink.0.sourceObjectId=7\n"
      "creativeDocument.logicLink.0.targetObjectId=42\n"
      "creativeDocument.logicLink.0.action=Open\n";
  if (const std::size_t linkPosition = version6Text.find(linkBlock);
      linkPosition != std::string::npos) {
    version6Text.erase(linkPosition, linkBlock.size());
  }
  const iggy3d::SaveDecodeResult version6Decoded =
      iggy3d::decodeSaveEnvelope(version6Text);
  ok = expect(version6Decoded.status == iggy3d::SaveCodecStatus::Ok &&
                  version6Decoded.envelope.creativeDocument.version == 6U &&
                  version6Decoded.envelope.creativeDocument.objects.size() ==
                      1U &&
                  version6Decoded.envelope.creativeDocument.objects[0]
                          .assetId == "boulder_01" &&
                  version6Decoded.envelope.creativeDocument.logicLinks.empty(),
              "version 6 document without link block remains readable") &&
       ok;

  std::string version5Text = version6Text;
  if (const std::size_t versionPosition =
          version5Text.find("creativeDocument.version=6\n");
      versionPosition != std::string::npos) {
    version5Text.replace(versionPosition,
                         std::string("creativeDocument.version=6\n").size(),
                         "creativeDocument.version=5\n");
  }
  const std::string assetLine =
      "creativeDocument.object.0.assetId=boulder_01\n";
  if (const std::size_t assetPosition = version5Text.find(assetLine);
      assetPosition != std::string::npos) {
    version5Text.erase(assetPosition, assetLine.size());
  }
  const iggy3d::SaveDecodeResult version5Decoded =
      iggy3d::decodeSaveEnvelope(version5Text);
  ok = expect(version5Decoded.status == iggy3d::SaveCodecStatus::Ok &&
                  version5Decoded.envelope.creativeDocument.version == 5U &&
                  version5Decoded.envelope.creativeDocument.objects.size() ==
                      1U &&
                  version5Decoded.envelope.creativeDocument.objects[0]
                      .assetId.empty(),
              "version 5 object without asset id remains readable") &&
       ok;
  return ok;
}

bool version7MovingPlatformDefaultsRemainReadable() {
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  iggy3d::SaveCreativeDocumentSection& section = envelope.creativeDocument;
  section.present = true;
  section.documentId = 91U;
  section.name = "Legacy Moving Platform";
  section.gridWidth = 8U;
  section.gridHeight = 8U;
  section.gridDepth = 8U;
  section.worldBounds.max = {8.0, 8.0, 8.0};
  section.nextObjectId = 2U;

  iggy3d::SaveCreativeDocumentObjectRecord object;
  object.id = 1U;
  object.kind = "MovingPlatform";
  object.name = "Legacy Lift";
  object.bounds.max = {2.0, 0.25, 2.0};
  object.pathPoints = {{1.0, 0.125, 1.0}, {1.0, 3.125, 1.0}};
  object.movingPlatformSpeedMetersPerSecond = 4.0;
  object.movingPlatformTraversalMode = "Loop";
  object.movingPlatformStartsActive = false;
  section.objects.push_back(object);

  const iggy3d::SaveEncodeResult encoded =
      iggy3d::encodeSaveEnvelope(envelope);
  std::string version7Text = replaceValueForKey(
      encoded.encodedText, "creativeDocument.version", "7");
  version7Text = eraseLineForKey(
      version7Text,
      "creativeDocument.object.0.movingPlatform.speedMetersPerSecond");
  version7Text = eraseLineForKey(
      version7Text,
      "creativeDocument.object.0.movingPlatform.traversalMode");
  version7Text = eraseLineForKey(
      version7Text,
      "creativeDocument.object.0.movingPlatform.startsActive");

  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(version7Text);
  const iggy3d::SaveCreativeDocumentObjectRecord* decodedObject =
      decoded.envelope.creativeDocument.objects.empty()
          ? nullptr
          : &decoded.envelope.creativeDocument.objects.front();

  return expect(encoded.status == iggy3d::SaveCodecStatus::Ok,
                "legacy moving platform setup encode ok") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok,
                "version 7 moving platform decode ok") &&
         expect(decoded.envelope.creativeDocument.version == 7U,
                "version 7 moving platform version retained") &&
         expect(decodedObject != nullptr &&
                    decodedObject->movingPlatformSpeedMetersPerSecond == 1.5 &&
                    decodedObject->movingPlatformTraversalMode == "PingPong" &&
                    decodedObject->movingPlatformStartsActive,
                "version 7 moving platform receives safe defaults");
}

bool version8WaypointDwellDefaultsRemainReadable() {
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  iggy3d::SaveCreativeDocumentSection& section = envelope.creativeDocument;
  section.present = true;
  section.documentId = 92U;
  section.name = "Legacy Waypoint Dwell";
  section.gridWidth = 8U;
  section.gridHeight = 8U;
  section.gridDepth = 8U;
  section.worldBounds.max = {8.0, 8.0, 8.0};
  section.nextObjectId = 2U;

  iggy3d::SaveCreativeDocumentObjectRecord object;
  object.id = 1U;
  object.kind = "MovingPlatform";
  object.name = "Legacy Lift";
  object.bounds.max = {2.0, 0.25, 2.0};
  object.pathPoints = {{1.0, 0.125, 1.0, 0.0},
                       {1.0, 3.125, 1.0, 0.0}};
  section.objects.push_back(object);

  const iggy3d::SaveEncodeResult encoded =
      iggy3d::encodeSaveEnvelope(envelope);
  std::string version8Text = replaceValueForKey(
      encoded.encodedText, "creativeDocument.version", "8");
  version8Text = eraseLineForKey(
      version8Text,
      "creativeDocument.object.0.pathPoint.0.dwellSeconds");
  version8Text = eraseLineForKey(
      version8Text,
      "creativeDocument.object.0.pathPoint.1.dwellSeconds");

  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(version8Text);
  const iggy3d::SaveCreativeDocumentObjectRecord* decodedObject =
      decoded.envelope.creativeDocument.objects.empty()
          ? nullptr
          : &decoded.envelope.creativeDocument.objects.front();
  return expect(encoded.status == iggy3d::SaveCodecStatus::Ok,
                "version 8 dwell setup encode ok") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok,
                "version 8 without dwell remains readable") &&
         expect(decodedObject != nullptr &&
                    decodedObject->pathPoints.size() == 2U &&
                    decodedObject->pathPoints[0].dwellSeconds == 0.0 &&
                    decodedObject->pathPoints[1].dwellSeconds == 0.0,
                "version 8 waypoints receive zero dwell defaults");
}

bool version9SegmentSpeedDefaultsRemainReadable() {
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  iggy3d::SaveCreativeDocumentSection& section = envelope.creativeDocument;
  section.present = true;
  section.documentId = 93U;
  section.name = "Legacy Segment Speed";
  section.gridWidth = 8U;
  section.gridHeight = 8U;
  section.gridDepth = 8U;
  section.worldBounds.max = {8.0, 8.0, 8.0};
  section.nextObjectId = 2U;

  iggy3d::SaveCreativeDocumentObjectRecord object;
  object.id = 1U;
  object.kind = "MovingPlatform";
  object.name = "Legacy Lift";
  object.bounds.max = {2.0, 0.25, 2.0};
  object.pathPoints = {{1.0, 0.125, 1.0, 0.0, 1.0},
                       {1.0, 3.125, 1.0, 0.5, 1.0}};
  section.objects.push_back(object);

  const iggy3d::SaveEncodeResult encoded =
      iggy3d::encodeSaveEnvelope(envelope);
  std::string version9Text = replaceValueForKey(
      encoded.encodedText, "creativeDocument.version", "9");
  version9Text = eraseLineForKey(
      version9Text,
      "creativeDocument.object.0.pathPoint.0.outgoingSpeedMultiplier");
  version9Text = eraseLineForKey(
      version9Text,
      "creativeDocument.object.0.pathPoint.1.outgoingSpeedMultiplier");

  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(version9Text);
  const iggy3d::SaveCreativeDocumentObjectRecord* decodedObject =
      decoded.envelope.creativeDocument.objects.empty()
          ? nullptr
          : &decoded.envelope.creativeDocument.objects.front();
  return expect(encoded.status == iggy3d::SaveCodecStatus::Ok,
                "version 9 segment speed setup encode ok") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok,
                "version 9 without segment speed remains readable") &&
         expect(decodedObject != nullptr &&
                    decodedObject->pathPoints.size() == 2U &&
                    decodedObject->pathPoints[0].outgoingSpeedMultiplier == 1.0 &&
                    decodedObject->pathPoints[1].outgoingSpeedMultiplier == 1.0,
                "version 9 route segments receive 1x speed defaults");
}

bool malformedCreativeDocumentPathPointKeysReject() {
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  iggy3d::SaveCreativeDocumentSection& section = envelope.creativeDocument;
  section.present = true;
  section.documentId = 77;
  section.name = "Malformed Path";
  section.units = "Meters";
  section.nextObjectId = 43;

  iggy3d::SaveCreativeDocumentObjectRecord object;
  object.id = 42;
  object.kind = "PatrolRoute";
  object.name = "Route";
  object.pathPoints = {
      {1.0 / 3.0, 0.0, 0.1234567890123},
      {2.0, 0.0, 3.0},
  };
  section.objects.push_back(object);

  const iggy3d::SaveEncodeResult encoded = iggy3d::encodeSaveEnvelope(envelope);
  const iggy3d::SaveDecodeResult invalidCount =
      iggy3d::decodeSaveEnvelope(replaceValueForKey(
          encoded.encodedText,
          "creativeDocument.object.0.pathPoint.count",
          "not_a_number"));
  const iggy3d::SaveDecodeResult invalidPosition =
      iggy3d::decodeSaveEnvelope(replaceValueForKey(
          encoded.encodedText,
          "creativeDocument.object.0.pathPoint.1.position",
          "nan,0,0"));
  const iggy3d::SaveDecodeResult missingIndexedPosition =
      iggy3d::decodeSaveEnvelope(eraseLineForKey(
          encoded.encodedText,
          "creativeDocument.object.0.pathPoint.1.position"));
  const iggy3d::SaveDecodeResult invalidDwell =
      iggy3d::decodeSaveEnvelope(replaceValueForKey(
          encoded.encodedText,
          "creativeDocument.object.0.pathPoint.1.dwellSeconds", "nan"));
  const iggy3d::SaveDecodeResult missingDwell =
      iggy3d::decodeSaveEnvelope(eraseLineForKey(
          encoded.encodedText,
          "creativeDocument.object.0.pathPoint.1.dwellSeconds"));
  const iggy3d::SaveDecodeResult invalidSpeed =
      iggy3d::decodeSaveEnvelope(replaceValueForKey(
          encoded.encodedText,
          "creativeDocument.object.0.pathPoint.1.outgoingSpeedMultiplier",
          "nan"));
  const iggy3d::SaveDecodeResult missingSpeed =
      iggy3d::decodeSaveEnvelope(eraseLineForKey(
          encoded.encodedText,
          "creativeDocument.object.0.pathPoint.1.outgoingSpeedMultiplier"));

  return expect(encoded.status == iggy3d::SaveCodecStatus::Ok,
                "path malformed setup encode ok") &&
         expect(invalidCount.status == iggy3d::SaveCodecStatus::InvalidNumber,
                "invalid path point count status") &&
         expect(invalidCount.diagnosticKey ==
                    "creativeDocument.object.0.pathPoint.count",
                "invalid path point count key") &&
         expect(invalidPosition.status ==
                    iggy3d::SaveCodecStatus::InvalidNumber,
                "invalid path point position status") &&
         expect(invalidPosition.diagnosticKey ==
                    "creativeDocument.object.0.pathPoint.1.position",
                "invalid path point position key") &&
         expect(missingIndexedPosition.status != iggy3d::SaveCodecStatus::Ok,
                "missing indexed path point rejected") &&
         expect(invalidDwell.status == iggy3d::SaveCodecStatus::InvalidNumber &&
                    invalidDwell.diagnosticKey ==
                        "creativeDocument.object.0.pathPoint.1.dwellSeconds",
                "invalid waypoint dwell rejected at exact key") &&
         expect(missingDwell.status != iggy3d::SaveCodecStatus::Ok,
                "current save requires every waypoint dwell key") &&
         expect(invalidSpeed.status ==
                    iggy3d::SaveCodecStatus::InvalidNumber &&
                    invalidSpeed.diagnosticKey ==
                        "creativeDocument.object.0.pathPoint.1."
                        "outgoingSpeedMultiplier",
                "invalid segment speed rejected at exact key") &&
         expect(missingSpeed.status != iggy3d::SaveCodecStatus::Ok,
                "current save requires every segment speed key");
}

bool schemaCompatibilityAcceptsV1V2AndRejectsTooNew() {
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  const iggy3d::SaveEncodeResult encoded = iggy3d::encodeSaveEnvelope(envelope);
  const std::string v1Text =
      replaceFirst(encoded.encodedText, "metadata.schemaVersion=3\n",
                   "metadata.schemaVersion=1\n");
  const std::string v2Text =
      replaceFirst(encoded.encodedText, "metadata.schemaVersion=3\n",
                   "metadata.schemaVersion=2\n");
  const iggy3d::SaveDecodeResult decoded = iggy3d::decodeSaveEnvelope(v1Text);
  const iggy3d::SaveDecodeResult decodedV2 = iggy3d::decodeSaveEnvelope(v2Text);
  const iggy3d::SaveCompatibilityResult v1Compatibility =
      iggy3d::checkSaveCompatibility({decoded.envelope, "", ""});
  const iggy3d::SaveCompatibilityResult v2Compatibility =
      iggy3d::checkSaveCompatibility({decodedV2.envelope, "", ""});

  iggy3d::SaveEnvelope tooNew = minimalEnvelope();
  tooNew.metadata.schemaVersion = iggy3d::kSaveSchemaVersion + 1U;
  const iggy3d::SaveCompatibilityResult tooNewCompatibility =
      iggy3d::checkSaveCompatibility({tooNew, "", ""});

  return expect(decoded.status == iggy3d::SaveCodecStatus::Ok, "v1 decode ok") &&
         expect(decoded.envelope.metadata.schemaVersion == 1U, "v1 schema decoded") &&
         expect(!decoded.envelope.creativeDocument.present,
                "v1 save defaults creative document absent") &&
         expect(v1Compatibility.status == iggy3d::SaveCompatibilityStatus::Compatible,
                "v1 compatibility accepted") &&
         expect(decodedV2.status == iggy3d::SaveCodecStatus::Ok, "v2 decode ok") &&
         expect(decodedV2.envelope.metadata.schemaVersion == 2U, "v2 schema decoded") &&
         expect(v2Compatibility.status == iggy3d::SaveCompatibilityStatus::Compatible,
                "v2 compatibility accepted") &&
         expect(tooNewCompatibility.status ==
                    iggy3d::SaveCompatibilityStatus::UnsupportedSchemaVersion,
                "too new schema rejected") &&
         expect(tooNewCompatibility.fieldName == "metadata.schemaVersion",
                "too new schema field");
}

}  // namespace

int main() {
  bool ok = true;
  ok = defaultEnvelopeOmitsCreativeDocumentSection() && ok;
  ok = creativeDocumentSectionRoundTripsThroughSaveCodec() && ok;
  ok = version7MovingPlatformDefaultsRemainReadable() && ok;
  ok = version8WaypointDwellDefaultsRemainReadable() && ok;
  ok = version9SegmentSpeedDefaultsRemainReadable() && ok;
  ok = malformedCreativeDocumentPathPointKeysReject() && ok;
  ok = schemaCompatibilityAcceptsV1V2AndRejectsTooNew() && ok;
  return ok ? 0 : 1;
}
