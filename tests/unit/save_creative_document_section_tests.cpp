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
         expect(encoded.encodedText.find("metadata.schemaVersion=2\n") !=
                    std::string::npos,
                "schema v2 metadata encoded") &&
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
  section.version = 1;
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
  object.kind = "Room";
  object.name = "Visible Room";
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
  object.tags = {"boss=room", "line\nbreak"};
  section.objects.push_back(object);

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
            expect(encoded.encodedText.find("creativeDocument.object.0.kind=Room\n") !=
                       std::string::npos,
                   "object kind encoded") &&
            expect(encoded.encodedText.find("creativeDocument.object.0.hasParent=true\n") !=
                       std::string::npos,
                   "object parent presence encoded") &&
            expect(encoded.encodedText.find("creativeDocument.object.0.tag.count=2\n") !=
                       std::string::npos,
                   "object tag count encoded") &&
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
            expect(decodedSection.objects.size() == 1U, "object count decoded");

  ok = ok && expect(decodedObject != nullptr && decodedObject->id == 42U,
                    "object id decoded") &&
       expect(decodedObject != nullptr && decodedObject->kind == "Room",
              "object kind decoded") &&
       expect(decodedObject != nullptr && decodedObject->name == "Visible Room",
              "object name decoded") &&
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
       expect(decodedObject != nullptr && decodedObject->tags.size() == 2U &&
                  decodedObject->tags[0] == "boss=room" &&
                  decodedObject->tags[1] == "line\nbreak",
              "object tags decoded");
  return ok;
}

bool schemaCompatibilityAcceptsV1AndRejectsTooNew() {
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  const iggy3d::SaveEncodeResult encoded = iggy3d::encodeSaveEnvelope(envelope);
  const std::string v1Text =
      replaceFirst(encoded.encodedText, "metadata.schemaVersion=2\n",
                   "metadata.schemaVersion=1\n");
  const iggy3d::SaveDecodeResult decoded = iggy3d::decodeSaveEnvelope(v1Text);
  const iggy3d::SaveCompatibilityResult v1Compatibility =
      iggy3d::checkSaveCompatibility({decoded.envelope, "", ""});

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
  ok = schemaCompatibilityAcceptsV1AndRejectsTooNew() && ok;
  return ok ? 0 : 1;
}
