#include "app/iggy3d/creative/document/Document.hpp"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace iggy3d::creative {
namespace {

struct LogicLinkPair {
  CreativeObjectId sourceObjectId = kInvalidObjectId;
  CreativeObjectId targetObjectId = kInvalidObjectId;

  bool operator==(const LogicLinkPair&) const = default;
};

struct LogicLinkPairHash {
  std::size_t operator()(LogicLinkPair value) const noexcept {
    const std::size_t source =
        std::hash<CreativeObjectId>{}(value.sourceObjectId);
    const std::size_t target =
        std::hash<CreativeObjectId>{}(value.targetObjectId);
    return source ^ (target + 0x9e3779b9U + (source << 6U) + (source >> 2U));
  }
};

[[nodiscard]] bool linkPairLess(const CreativeLogicLink& lhs,
                                LogicLinkPair rhs) noexcept {
  if (lhs.sourceObjectId != rhs.sourceObjectId) {
    return lhs.sourceObjectId < rhs.sourceObjectId;
  }
  return lhs.targetObjectId < rhs.targetObjectId;
}

void setMutationStatus(CreativeLogicLinkMutationReceipt& receipt,
                       CreativeLogicLinkMutationStatus status,
                       std::string_view reasonCode,
                       bool accepted = false) noexcept {
  receipt.status = status;
  receipt.reasonCode = reasonCode;
  receipt.accepted = accepted;
}

void setValidationFailure(CreativeLogicLinkValidationReceipt& receipt,
                          CreativeLogicLinkValidationStatus status,
                          std::size_t index,
                          const CreativeLogicLink& link,
                          std::string_view reasonCode) noexcept {
  receipt.status = status;
  receipt.failedLinkIndex = index;
  receipt.sourceObjectId = link.sourceObjectId;
  receipt.targetObjectId = link.targetObjectId;
  receipt.reasonCode = reasonCode;
}

[[nodiscard]] const CreativeObject* findObject(
    std::span<const CreativeObject> objects,
    CreativeObjectId objectId) noexcept {
  const auto found = std::find_if(
      objects.begin(), objects.end(), [objectId](const CreativeObject& object) {
        return object.id == objectId;
      });
  return found == objects.end() ? nullptr : &*found;
}

void appendLogicDiagnostic(CreativeLogicDiagnosticReport& report,
                           CreativeLogicDiagnostic diagnostic) noexcept {
  if (diagnostic.severity == CreativeLogicDiagnosticSeverity::Error) {
    ++report.errorCount;
  } else {
    ++report.warningCount;
  }
  if (report.issueCount < report.issues.size()) {
    report.issues[report.issueCount++] = diagnostic;
    return;
  }
  report.capacityExceeded = true;
  ++report.droppedIssueCount;
}

}  // namespace

std::string_view toString(CreativeLogicLinkAction action) noexcept {
  switch (action) {
    case CreativeLogicLinkAction::Toggle:
      return "Toggle";
    case CreativeLogicLinkAction::Open:
      return "Open";
    case CreativeLogicLinkAction::Close:
      return "Close";
    case CreativeLogicLinkAction::Enable:
      return "Enable";
    case CreativeLogicLinkAction::Disable:
      return "Disable";
    case CreativeLogicLinkAction::Reverse:
      return "Reverse";
    case CreativeLogicLinkAction::Count:
      break;
  }
  return "Unknown";
}

bool parseCreativeLogicLinkAction(std::string_view value,
                                  CreativeLogicLinkAction& output) noexcept {
  for (std::uint8_t index = 0U;
       index < static_cast<std::uint8_t>(CreativeLogicLinkAction::Count);
       ++index) {
    const auto action = static_cast<CreativeLogicLinkAction>(index);
    if (toString(action) == value) {
      output = action;
      return true;
    }
  }
  return false;
}

bool isValidCreativeLogicLinkAction(
    CreativeLogicLinkAction action) noexcept {
  return static_cast<std::uint8_t>(action) <
         static_cast<std::uint8_t>(CreativeLogicLinkAction::Count);
}

std::string_view toString(CreativeLogicLinkValidationStatus status) noexcept {
  switch (status) {
    case CreativeLogicLinkValidationStatus::Unknown:
      return "Unknown";
    case CreativeLogicLinkValidationStatus::Valid:
      return "Valid";
    case CreativeLogicLinkValidationStatus::InvalidAction:
      return "InvalidAction";
    case CreativeLogicLinkValidationStatus::MissingSource:
      return "MissingSource";
    case CreativeLogicLinkValidationStatus::MissingTarget:
      return "MissingTarget";
    case CreativeLogicLinkValidationStatus::UnsupportedSource:
      return "UnsupportedSource";
    case CreativeLogicLinkValidationStatus::UnsupportedTarget:
      return "UnsupportedTarget";
    case CreativeLogicLinkValidationStatus::DuplicatePair:
      return "DuplicatePair";
  }
  return "Unknown";
}

std::string_view toString(
    CreativeLogicDiagnosticSeverity severity) noexcept {
  switch (severity) {
    case CreativeLogicDiagnosticSeverity::Warning:
      return "warning";
    case CreativeLogicDiagnosticSeverity::Error:
      return "error";
  }
  return "warning";
}

std::string_view toString(CreativeLogicDiagnosticCode code) noexcept {
  switch (code) {
    case CreativeLogicDiagnosticCode::Unknown:
      return "unknown";
    case CreativeLogicDiagnosticCode::UnlinkedSource:
      return "unlinked_source";
    case CreativeLogicDiagnosticCode::InvalidAction:
      return "invalid_action";
    case CreativeLogicDiagnosticCode::MissingSource:
      return "missing_source";
    case CreativeLogicDiagnosticCode::MissingTarget:
      return "missing_target";
    case CreativeLogicDiagnosticCode::UnsupportedSource:
      return "unsupported_source";
    case CreativeLogicDiagnosticCode::UnsupportedTarget:
      return "unsupported_target";
    case CreativeLogicDiagnosticCode::DuplicatePair:
      return "duplicate_pair";
    case CreativeLogicDiagnosticCode::ConflictingPressurePlates:
      return "conflicting_pressure_plates";
  }
  return "unknown";
}

std::string_view toString(CreativeLogicLinkMutationStatus status) noexcept {
  switch (status) {
    case CreativeLogicLinkMutationStatus::NotRequested:
      return "NotRequested";
    case CreativeLogicLinkMutationStatus::InvalidDocument:
      return "InvalidDocument";
    case CreativeLogicLinkMutationStatus::InvalidAction:
      return "InvalidAction";
    case CreativeLogicLinkMutationStatus::MissingSource:
      return "MissingSource";
    case CreativeLogicLinkMutationStatus::MissingTarget:
      return "MissingTarget";
    case CreativeLogicLinkMutationStatus::UnsupportedSource:
      return "UnsupportedSource";
    case CreativeLogicLinkMutationStatus::UnsupportedTarget:
      return "UnsupportedTarget";
    case CreativeLogicLinkMutationStatus::Added:
      return "Added";
    case CreativeLogicLinkMutationStatus::Updated:
      return "Updated";
    case CreativeLogicLinkMutationStatus::Removed:
      return "Removed";
    case CreativeLogicLinkMutationStatus::NoChange:
      return "NoChange";
    case CreativeLogicLinkMutationStatus::MissingLink:
      return "MissingLink";
  }
  return "NotRequested";
}

bool creativeObjectCanSourceLogicLink(CreativeObjectKind kind) noexcept {
  return kind == CreativeObjectKind::TriggerZone ||
         kind == CreativeObjectKind::Switch ||
         kind == CreativeObjectKind::Lever ||
         kind == CreativeObjectKind::PressurePlate ||
         kind == CreativeObjectKind::Button;
}

bool creativeObjectCanTargetLogicLink(CreativeObjectKind kind) noexcept {
  return kind == CreativeObjectKind::Door ||
         kind == CreativeObjectKind::Platform ||
         kind == CreativeObjectKind::MovingPlatform;
}

bool creativeLogicLinkActionSupported(CreativeObjectKind targetKind,
                                      CreativeLogicLinkAction action) noexcept {
  switch (targetKind) {
    case CreativeObjectKind::Door:
      return action == CreativeLogicLinkAction::Toggle ||
             action == CreativeLogicLinkAction::Open ||
             action == CreativeLogicLinkAction::Close;
    case CreativeObjectKind::Platform:
      return action == CreativeLogicLinkAction::Toggle ||
             action == CreativeLogicLinkAction::Enable ||
             action == CreativeLogicLinkAction::Disable;
    case CreativeObjectKind::MovingPlatform:
      return action == CreativeLogicLinkAction::Toggle ||
             action == CreativeLogicLinkAction::Enable ||
             action == CreativeLogicLinkAction::Disable ||
             action == CreativeLogicLinkAction::Reverse;
    default:
      return false;
  }
}

CreativeLogicLinkValidationReceipt validateCreativeLogicLinks(
    std::span<const CreativeLogicLink> links,
    std::span<const CreativeObject> objects) {
  CreativeLogicLinkValidationReceipt receipt;
  std::unordered_map<CreativeObjectId, CreativeObjectKind> objectKinds;
  objectKinds.reserve(objects.size());
  for (const CreativeObject& object : objects) {
    objectKinds.emplace(object.id, object.kind);
  }
  std::unordered_set<LogicLinkPair, LogicLinkPairHash> pairs;
  pairs.reserve(links.size());
  for (std::size_t index = 0U; index < links.size(); ++index) {
    const CreativeLogicLink& link = links[index];
    if (!isValidCreativeLogicLinkAction(link.action)) {
      setValidationFailure(receipt,
                           CreativeLogicLinkValidationStatus::InvalidAction,
                           index, link, "creative_logic_link_action_invalid");
      return receipt;
    }
    const auto source = objectKinds.find(link.sourceObjectId);
    if (source == objectKinds.end()) {
      setValidationFailure(receipt,
                           CreativeLogicLinkValidationStatus::MissingSource,
                           index, link, "creative_logic_link_source_missing");
      return receipt;
    }
    const auto target = objectKinds.find(link.targetObjectId);
    if (target == objectKinds.end()) {
      setValidationFailure(receipt,
                           CreativeLogicLinkValidationStatus::MissingTarget,
                           index, link, "creative_logic_link_target_missing");
      return receipt;
    }
    if (!creativeObjectCanSourceLogicLink(source->second)) {
      setValidationFailure(
          receipt, CreativeLogicLinkValidationStatus::UnsupportedSource,
          index, link, "creative_logic_link_source_unsupported");
      return receipt;
    }
    if (!creativeObjectCanTargetLogicLink(target->second) ||
        !creativeLogicLinkActionSupported(target->second, link.action)) {
      setValidationFailure(
          receipt, CreativeLogicLinkValidationStatus::UnsupportedTarget,
          index, link, "creative_logic_link_target_unsupported");
      return receipt;
    }
    if (!pairs.insert({link.sourceObjectId, link.targetObjectId}).second) {
      setValidationFailure(receipt,
                           CreativeLogicLinkValidationStatus::DuplicatePair,
                           index, link, "creative_logic_link_pair_duplicate");
      return receipt;
    }
  }
  receipt.valid = true;
  receipt.status = CreativeLogicLinkValidationStatus::Valid;
  receipt.failedLinkIndex = links.size();
  receipt.reasonCode = "creative_logic_links_valid";
  return receipt;
}

CreativeLogicDiagnosticReport buildCreativeLogicDiagnostics(
    std::span<const CreativeLogicLink> links,
    std::span<const CreativeObject> objects) noexcept {
  CreativeLogicDiagnosticReport report;

  for (const CreativeObject& object : objects) {
    if (!creativeObjectCanSourceLogicLink(object.kind)) {
      continue;
    }
    ++report.sourceCount;
    const bool linked = std::any_of(
        links.begin(), links.end(), [&object](const CreativeLogicLink& link) {
          return link.sourceObjectId == object.id;
        });
    if (linked) {
      ++report.linkedSourceCount;
      continue;
    }
    appendLogicDiagnostic(
        report,
        {CreativeLogicDiagnosticSeverity::Warning,
         CreativeLogicDiagnosticCode::UnlinkedSource,
         object.id,
         kInvalidObjectId,
         kInvalidObjectId,
         links.size()});
  }

  for (std::size_t index = 0U; index < links.size(); ++index) {
    const CreativeLogicLink& link = links[index];
    const CreativeObject* source = findObject(objects, link.sourceObjectId);
    const CreativeObject* target = findObject(objects, link.targetObjectId);
    const auto appendError = [&](CreativeLogicDiagnosticCode code) {
      appendLogicDiagnostic(
          report,
          {CreativeLogicDiagnosticSeverity::Error,
           code,
           link.sourceObjectId,
           link.targetObjectId,
           kInvalidObjectId,
           index});
    };

    const bool validAction = isValidCreativeLogicLinkAction(link.action);
    if (!validAction) {
      appendError(CreativeLogicDiagnosticCode::InvalidAction);
    }
    if (source == nullptr) {
      appendError(CreativeLogicDiagnosticCode::MissingSource);
    } else if (!creativeObjectCanSourceLogicLink(source->kind)) {
      appendError(CreativeLogicDiagnosticCode::UnsupportedSource);
    }
    if (target == nullptr) {
      appendError(CreativeLogicDiagnosticCode::MissingTarget);
    } else if (!creativeObjectCanTargetLogicLink(target->kind) ||
               (validAction && !creativeLogicLinkActionSupported(
                                   target->kind, link.action))) {
      appendError(CreativeLogicDiagnosticCode::UnsupportedTarget);
    }

    for (std::size_t prior = 0U; prior < index; ++prior) {
      const CreativeLogicLink& previous = links[prior];
      if (previous.sourceObjectId == link.sourceObjectId &&
          previous.targetObjectId == link.targetObjectId) {
        appendError(CreativeLogicDiagnosticCode::DuplicatePair);
        break;
      }
    }

    if (source == nullptr ||
        source->kind != CreativeObjectKind::PressurePlate) {
      continue;
    }
    for (std::size_t prior = 0U; prior < index; ++prior) {
      const CreativeLogicLink& previous = links[prior];
      if (previous.targetObjectId != link.targetObjectId ||
          previous.sourceObjectId == link.sourceObjectId) {
        continue;
      }
      const CreativeObject* previousSource =
          findObject(objects, previous.sourceObjectId);
      if (previousSource == nullptr ||
          previousSource->kind != CreativeObjectKind::PressurePlate) {
        continue;
      }
      appendLogicDiagnostic(
          report,
          {CreativeLogicDiagnosticSeverity::Error,
           CreativeLogicDiagnosticCode::ConflictingPressurePlates,
           link.sourceObjectId,
           link.targetObjectId,
           previous.sourceObjectId,
           index});
      break;
    }
  }

  return report;
}

std::span<const CreativeLogicLink> CreativeDocument::logicLinks()
    const noexcept {
  return logicLinks_;
}

const CreativeLogicLink* CreativeDocument::findLogicLink(
    CreativeObjectId sourceObjectId,
    CreativeObjectId targetObjectId) const noexcept {
  const LogicLinkPair key{sourceObjectId, targetObjectId};
  const auto found =
      std::lower_bound(logicLinks_.begin(), logicLinks_.end(), key,
                       [](const CreativeLogicLink& link, LogicLinkPair pair) {
                         return linkPairLess(link, pair);
                       });
  return found != logicLinks_.end() &&
                 found->sourceObjectId == sourceObjectId &&
                 found->targetObjectId == targetObjectId
             ? &*found
             : nullptr;
}

CreativeLogicLinkMutationReceipt CreativeDocument::setLogicLink(
    const CreativeLogicLinkMutationRequest& request) {
  CreativeLogicLinkMutationReceipt receipt;
  receipt.requested = true;
  receipt.link = {request.sourceObjectId, request.targetObjectId,
                  request.action};
  receipt.revisionBefore = revision_;
  receipt.revisionAfter = revision_;
  if (!isValid()) {
    setMutationStatus(receipt,
                      CreativeLogicLinkMutationStatus::InvalidDocument,
                      "creative_logic_link_document_invalid");
    return receipt;
  }
  if (!isValidCreativeLogicLinkAction(request.action)) {
    setMutationStatus(receipt,
                      CreativeLogicLinkMutationStatus::InvalidAction,
                      "creative_logic_link_action_invalid");
    return receipt;
  }
  const CreativeObject* source = findObject(request.sourceObjectId);
  if (source == nullptr) {
    setMutationStatus(receipt,
                      CreativeLogicLinkMutationStatus::MissingSource,
                      "creative_logic_link_source_missing");
    return receipt;
  }
  const CreativeObject* target = findObject(request.targetObjectId);
  if (target == nullptr) {
    setMutationStatus(receipt,
                      CreativeLogicLinkMutationStatus::MissingTarget,
                      "creative_logic_link_target_missing");
    return receipt;
  }
  if (!creativeObjectCanSourceLogicLink(source->kind)) {
    setMutationStatus(receipt,
                      CreativeLogicLinkMutationStatus::UnsupportedSource,
                      "creative_logic_link_source_unsupported");
    return receipt;
  }
  if (!creativeObjectCanTargetLogicLink(target->kind) ||
      !creativeLogicLinkActionSupported(target->kind, request.action)) {
    setMutationStatus(receipt,
                      CreativeLogicLinkMutationStatus::UnsupportedTarget,
                      "creative_logic_link_target_unsupported");
    return receipt;
  }

  const LogicLinkPair key{request.sourceObjectId, request.targetObjectId};
  const auto found =
      std::lower_bound(logicLinks_.begin(), logicLinks_.end(), key,
                       [](const CreativeLogicLink& link, LogicLinkPair pair) {
                         return linkPairLess(link, pair);
                       });
  if (found != logicLinks_.end() &&
      found->sourceObjectId == request.sourceObjectId &&
      found->targetObjectId == request.targetObjectId) {
    if (found->action == request.action) {
      setMutationStatus(receipt, CreativeLogicLinkMutationStatus::NoChange,
                        "creative_logic_link_unchanged", true);
      return receipt;
    }
    found->action = request.action;
    markObjectMutationChanged(
        static_cast<CreativeObjectDirtyFlags>(CreativeObjectDirtyFlag::Logic));
    receipt.changed = true;
    receipt.revisionAfter = revision_;
    setMutationStatus(receipt, CreativeLogicLinkMutationStatus::Updated,
                      "creative_logic_link_updated", true);
    return receipt;
  }

  logicLinks_.insert(found, receipt.link);
  markObjectMutationChanged(
      static_cast<CreativeObjectDirtyFlags>(CreativeObjectDirtyFlag::Logic));
  receipt.changed = true;
  receipt.revisionAfter = revision_;
  setMutationStatus(receipt, CreativeLogicLinkMutationStatus::Added,
                    "creative_logic_link_added", true);
  return receipt;
}

CreativeLogicLinkMutationReceipt CreativeDocument::removeLogicLink(
    CreativeObjectId sourceObjectId,
    CreativeObjectId targetObjectId) {
  CreativeLogicLinkMutationReceipt receipt;
  receipt.requested = true;
  receipt.link.sourceObjectId = sourceObjectId;
  receipt.link.targetObjectId = targetObjectId;
  receipt.revisionBefore = revision_;
  receipt.revisionAfter = revision_;
  if (!isValid()) {
    setMutationStatus(receipt,
                      CreativeLogicLinkMutationStatus::InvalidDocument,
                      "creative_logic_link_document_invalid");
    return receipt;
  }
  const LogicLinkPair key{sourceObjectId, targetObjectId};
  const auto found =
      std::lower_bound(logicLinks_.begin(), logicLinks_.end(), key,
                       [](const CreativeLogicLink& link, LogicLinkPair pair) {
                         return linkPairLess(link, pair);
                       });
  if (found == logicLinks_.end() || found->sourceObjectId != sourceObjectId ||
      found->targetObjectId != targetObjectId) {
    setMutationStatus(receipt, CreativeLogicLinkMutationStatus::MissingLink,
                      "creative_logic_link_missing");
    return receipt;
  }
  receipt.link = *found;
  logicLinks_.erase(found);
  markObjectMutationChanged(
      static_cast<CreativeObjectDirtyFlags>(CreativeObjectDirtyFlag::Logic));
  receipt.changed = true;
  receipt.revisionAfter = revision_;
  setMutationStatus(receipt, CreativeLogicLinkMutationStatus::Removed,
                    "creative_logic_link_removed", true);
  return receipt;
}

std::size_t CreativeDocument::eraseLogicLinksForObject(
    CreativeObjectId objectId) noexcept {
  const std::size_t before = logicLinks_.size();
  std::erase_if(logicLinks_, [objectId](const CreativeLogicLink& link) {
    return link.sourceObjectId == objectId || link.targetObjectId == objectId;
  });
  return before - logicLinks_.size();
}

}  // namespace iggy3d::creative
