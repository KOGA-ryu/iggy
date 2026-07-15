#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/tools/Clipboard.hpp"
#include "app/iggy3d/creative/world/DocumentSection.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeDocumentCreateReceipt create(
    cr::CreativeDocument& document,
    cr::CreativeObjectKind kind,
    std::string name) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::move(name);
  return document.createObject(request);
}

cr::CreativeDocument documentWithId() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Logic");
  static_cast<void>(document.assignId(91U));
  return document;
}

bool mutationsAreCanonicalAndReceipted() {
  cr::CreativeDocument document = documentWithId();
  const auto source = create(document, cr::CreativeObjectKind::Switch, "S");
  const auto target = create(document, cr::CreativeObjectKind::Door, "D");
  const auto floor = create(document, cr::CreativeObjectKind::Floor, "F");
  const std::uint64_t before = document.revision();
  const cr::CreativeLogicLinkMutationReceipt added = document.setLogicLink(
      {source.objectId, target.objectId, cr::CreativeLogicLinkAction::Toggle});
  const cr::CreativeLogicLinkMutationReceipt unchanged = document.setLogicLink(
      {source.objectId, target.objectId, cr::CreativeLogicLinkAction::Toggle});
  const cr::CreativeLogicLinkMutationReceipt updated = document.setLogicLink(
      {source.objectId, target.objectId, cr::CreativeLogicLinkAction::Open});
  const std::uint64_t afterUpdate = document.revision();
  const cr::CreativeLogicLinkMutationReceipt badSource = document.setLogicLink(
      {floor.objectId, target.objectId, cr::CreativeLogicLinkAction::Toggle});
  const cr::CreativeLogicLinkMutationReceipt badAction = document.setLogicLink(
      {source.objectId, target.objectId,
       static_cast<cr::CreativeLogicLinkAction>(255U)});
  const cr::CreativeLogicLinkMutationReceipt removed =
      document.removeLogicLink(source.objectId, target.objectId);

  return expect(added.accepted && added.changed &&
                    added.status == cr::CreativeLogicLinkMutationStatus::Added &&
                    document.logicLinks().empty(),
                "add and later remove use one canonical pair") &&
         expect(unchanged.accepted && !unchanged.changed &&
                    unchanged.status ==
                        cr::CreativeLogicLinkMutationStatus::NoChange,
                "same action is an accepted no-op") &&
         expect(updated.accepted && updated.changed &&
                    updated.status ==
                        cr::CreativeLogicLinkMutationStatus::Updated &&
                    afterUpdate == before + 2U,
                "action update increments one revision") &&
         expect(!badSource.accepted && !badSource.changed &&
                    badSource.status ==
                        cr::CreativeLogicLinkMutationStatus::UnsupportedSource &&
                    !badAction.accepted &&
                    badAction.status ==
                        cr::CreativeLogicLinkMutationStatus::InvalidAction &&
                    document.revision() == afterUpdate + 1U,
                "invalid endpoint and enum leave revision stable") &&
         expect(removed.accepted && removed.changed &&
                    removed.status ==
                        cr::CreativeLogicLinkMutationStatus::Removed,
                "remove reports its mutation");
}

bool endpointDeletionRemovesIncidentLinksInOneRevision() {
  cr::CreativeDocument document = documentWithId();
  const auto source = create(document, cr::CreativeObjectKind::Button, "B");
  const auto target = create(document, cr::CreativeObjectKind::Door, "D");
  static_cast<void>(document.setLogicLink(
      {source.objectId, target.objectId, cr::CreativeLogicLinkAction::Close}));
  const std::uint64_t before = document.revision();
  const cr::CreativeDocumentRemoveReceipt removed =
      document.removeDocumentObject(target.objectId);
  return expect(removed.accepted && removed.changed &&
                    removed.removedLogicLinkCount == 1U &&
                    document.logicLinks().empty(),
                "endpoint removal clears incident link") &&
         expect(document.revision() == before + 1U,
                "endpoint and link removal share one revision");
}

bool restoreValidatesAndCanonicalizesLinks() {
  cr::CreativeDocument source = documentWithId();
  const auto sourceA = create(source, cr::CreativeObjectKind::Switch, "A");
  const auto sourceB = create(source, cr::CreativeObjectKind::Lever, "B");
  const auto targetA = create(source, cr::CreativeObjectKind::Door, "DA");
  const auto targetB = create(source, cr::CreativeObjectKind::Door, "DB");
  cr::CreativeDocumentRestoreRequest request;
  request.documentId = 92U;
  request.name = "Restored";
  request.units = source.units();
  request.gridSettings = source.gridSettings();
  request.snapSettings = source.documentSnapSettings();
  request.worldBounds = source.worldBounds();
  request.nextObjectId = source.nextObjectId();
  request.objects.assign(source.objects().begin(), source.objects().end());
  request.logicLinks = {
      {sourceB.objectId, targetB.objectId, cr::CreativeLogicLinkAction::Close},
      {sourceA.objectId, targetA.objectId, cr::CreativeLogicLinkAction::Open},
  };
  cr::CreativeDocument restored;
  const cr::CreativeDocumentRestoreReceipt accepted =
      restored.restoreForLoad(request);
  request.logicLinks.push_back(request.logicLinks.front());
  cr::CreativeDocument rejected;
  const cr::CreativeDocumentRestoreReceipt duplicate =
      rejected.restoreForLoad(request);
  return expect(accepted.accepted && accepted.logicLinkCount == 2U &&
                    restored.logicLinks().front().sourceObjectId ==
                        sourceA.objectId,
                "restore sorts valid links by endpoint pair") &&
         expect(!duplicate.accepted &&
                    duplicate.status ==
                        cr::CreativeDocumentRestoreStatus::InvalidLogicLink,
                "restore rejects duplicate pairs");
}

bool clipboardRemapsInternalLinks() {
  cr::CreativeDocument document = documentWithId();
  const auto source = create(document, cr::CreativeObjectKind::Switch, "S");
  const auto target = create(document, cr::CreativeObjectKind::Door, "D");
  static_cast<void>(document.setLogicLink(
      {source.objectId, target.objectId, cr::CreativeLogicLinkAction::Open}));
  const cr::CreativeObjectId ids[]{source.objectId, target.objectId};
  cr::CreativeClipboard clipboard;
  const cr::CreativeClipboardCopyReceipt copied =
      cr::copyDocumentObjectsToClipboard(document, ids, clipboard);
  cr::CreativeClipboard partialClipboard;
  const cr::CreativeObjectId sourceOnly[]{source.objectId};
  const cr::CreativeClipboardCopyReceipt partialCopied =
      cr::copyDocumentObjectsToClipboard(document, sourceOnly,
                                         partialClipboard);
  const cr::CreativeClipboardPasteReceipt pasted =
      cr::pasteCreativeClipboardAtomically(document, clipboard);
  if (!pasted.accepted || pasted.idRemaps.size() != 2U) {
    return expect(false, "clipboard setup pastes two linked objects");
  }
  cr::CreativeObjectId newSource = cr::kInvalidObjectId;
  cr::CreativeObjectId newTarget = cr::kInvalidObjectId;
  for (const cr::CreativeClipboardIdRemap& remap : pasted.idRemaps) {
    if (remap.sourceObjectId == source.objectId) {
      newSource = remap.pastedObjectId;
    } else if (remap.sourceObjectId == target.objectId) {
      newTarget = remap.pastedObjectId;
    }
  }
  const cr::CreativeLogicLink* link =
      document.findLogicLink(newSource, newTarget);
  return expect(copied.copiedLogicLinkCount == 1U &&
                    clipboard.logicLinks.size() == 1U,
                "copy includes only internal link") &&
         expect(partialCopied.accepted &&
                    partialCopied.copiedLogicLinkCount == 0U &&
                    partialClipboard.logicLinks.empty(),
                "copy excludes a link when only one endpoint is copied") &&
         expect(pasted.pastedLogicLinkCount == 1U && link != nullptr &&
                    link->action == cr::CreativeLogicLinkAction::Open,
                "paste remaps both link endpoints");
}

bool saveSectionRoundTripsLogicLinks() {
  cr::CreativeDocument document = documentWithId();
  const auto source = create(document, cr::CreativeObjectKind::Lever, "L");
  const auto target = create(document, cr::CreativeObjectKind::Door, "D");
  static_cast<void>(document.setLogicLink(
      {source.objectId, target.objectId, cr::CreativeLogicLinkAction::Close}));
  const iggy3d::ProductCreativeDocumentSectionBuildResult built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  const iggy3d::ProductCreativeDocumentSectionRestoreResult restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(built.section);
  const cr::CreativeLogicLink* link = restored.document.findLogicLink(
      source.objectId, target.objectId);
  return expect(built.receipt.accepted &&
                    built.section.logicLinks.size() == 1U &&
                    built.section.logicLinks.front().action == "Close",
                "save section writes logic link record") &&
         expect(restored.receipt.accepted && link != nullptr &&
                    link->action == cr::CreativeLogicLinkAction::Close,
                "save section restores logic link semantics");
}

bool automaticSourcesUseTheSameAuthoredLinkContract() {
  cr::CreativeDocument document = documentWithId();
  const auto trigger =
      create(document, cr::CreativeObjectKind::TriggerZone, "Trigger");
  const auto plate =
      create(document, cr::CreativeObjectKind::PressurePlate, "Plate");
  const auto door = create(document, cr::CreativeObjectKind::Door, "Door");
  const cr::CreativeLogicLinkMutationReceipt triggerLink =
      document.setLogicLink({trigger.objectId, door.objectId,
                             cr::CreativeLogicLinkAction::Toggle});
  const cr::CreativeLogicLinkMutationReceipt plateLink =
      document.setLogicLink(
          {plate.objectId, door.objectId, cr::CreativeLogicLinkAction::Open});

  return expect(cr::creativeObjectCanSourceLogicLink(
                    cr::CreativeObjectKind::TriggerZone) &&
                    cr::creativeObjectCanSourceLogicLink(
                        cr::CreativeObjectKind::PressurePlate),
                "automatic controls are authored link sources") &&
         expect(triggerLink.accepted && triggerLink.changed &&
                    plateLink.accepted && plateLink.changed &&
                    document.logicLinks().size() == 2U,
                "automatic controls use canonical source-target links");
}

bool diagnosticsExposeUnlinkedMissingAndCompetingSources() {
  cr::CreativeDocument document = documentWithId();
  const auto trigger =
      create(document, cr::CreativeObjectKind::TriggerZone, "Trigger");
  const auto plateA =
      create(document, cr::CreativeObjectKind::PressurePlate, "Plate A");
  const auto plateB =
      create(document, cr::CreativeObjectKind::PressurePlate, "Plate B");
  static_cast<void>(
      create(document, cr::CreativeObjectKind::Button, "Unlinked Button"));
  const auto door = create(document, cr::CreativeObjectKind::Door, "Door");
  const std::vector<cr::CreativeLogicLink> malformed{
      {trigger.objectId, 999'999U, cr::CreativeLogicLinkAction::Toggle},
      {plateA.objectId, door.objectId, cr::CreativeLogicLinkAction::Open},
      {plateB.objectId, door.objectId, cr::CreativeLogicLinkAction::Open},
  };
  const cr::CreativeLogicDiagnosticReport report =
      cr::buildCreativeLogicDiagnostics(malformed, document.objects());

  bool unlinked = false;
  bool missingTarget = false;
  bool competingPlates = false;
  for (std::size_t index = 0U; index < report.issueCount; ++index) {
    unlinked = unlinked ||
               report.issues[index].code ==
                   cr::CreativeLogicDiagnosticCode::UnlinkedSource;
    missingTarget = missingTarget ||
                    report.issues[index].code ==
                        cr::CreativeLogicDiagnosticCode::MissingTarget;
    competingPlates =
        competingPlates ||
        (report.issues[index].code ==
             cr::CreativeLogicDiagnosticCode::ConflictingPressurePlates &&
         report.issues[index].relatedSourceObjectId == plateA.objectId &&
         report.issues[index].sourceObjectId == plateB.objectId);
  }

  return expect(report.sourceCount == 4U &&
                    report.linkedSourceCount == 3U &&
                    report.warningCount == 1U && report.errorCount == 2U &&
                    !report.capacityExceeded,
                "logic diagnostics summarize bounded source health") &&
         expect(unlinked && missingTarget && competingPlates,
                "logic diagnostics expose authoring hazards") &&
         expect(cr::toString(
                    cr::CreativeLogicDiagnosticCode::
                        ConflictingPressurePlates) ==
                    "conflicting_pressure_plates",
                "logic diagnostic ids are stable");
}

}  // namespace

int main() {
  const bool ok = mutationsAreCanonicalAndReceipted() &&
                  endpointDeletionRemovesIncidentLinksInOneRevision() &&
                  restoreValidatesAndCanonicalizesLinks() &&
                  clipboardRemapsInternalLinks() &&
                  saveSectionRoundTripsLogicLinks() &&
                  automaticSourcesUseTheSameAuthoredLinkContract() &&
                  diagnosticsExposeUnlinkedMissingAndCompetingSources();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
