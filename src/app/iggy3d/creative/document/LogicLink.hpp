#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

#include "app/iggy3d/creative/document/Object.hpp"

namespace iggy3d::creative {

enum class CreativeLogicLinkAction : std::uint8_t {
  Toggle,
  Open,
  Close,
  Enable,
  Disable,
  Reverse,
  Count,
};

enum class CreativeLogicSourceEvent : std::uint8_t {
  None,
  Manual,
  PulseOnEnter,
  HoldWhileOccupied,
  Count,
};

inline constexpr std::size_t kCreativeLogicTargetActionCapacity = 4U;

struct CreativeLogicEndpointDescriptor {
  CreativeObjectKind objectKind = CreativeObjectKind::Unknown;
  CreativeLogicSourceEvent sourceEvent = CreativeLogicSourceEvent::None;
  std::array<CreativeLogicLinkAction, kCreativeLogicTargetActionCapacity>
      targetActions{};
  std::uint8_t targetActionCount = 0U;

  [[nodiscard]] constexpr bool canSource() const noexcept {
    return sourceEvent != CreativeLogicSourceEvent::None &&
           sourceEvent != CreativeLogicSourceEvent::Count;
  }

  [[nodiscard]] constexpr bool canTarget() const noexcept {
    return targetActionCount > 0U &&
           targetActionCount <= targetActions.size();
  }

  [[nodiscard]] constexpr std::span<const CreativeLogicLinkAction>
  supportedTargetActions() const noexcept {
    const std::size_t count =
        targetActionCount < targetActions.size() ? targetActionCount
                                                 : targetActions.size();
    return {targetActions.data(), count};
  }
};

static_assert(std::is_trivially_copyable_v<CreativeLogicEndpointDescriptor>);
static_assert(std::is_standard_layout_v<CreativeLogicEndpointDescriptor>);

struct CreativeLogicLink {
  CreativeObjectId sourceObjectId = kInvalidObjectId;
  CreativeObjectId targetObjectId = kInvalidObjectId;
  CreativeLogicLinkAction action = CreativeLogicLinkAction::Toggle;
};

static_assert(std::is_trivially_copyable_v<CreativeLogicLink>);
static_assert(std::is_standard_layout_v<CreativeLogicLink>);

enum class CreativeLogicLinkValidationStatus : std::uint8_t {
  Unknown,
  Valid,
  InvalidAction,
  MissingSource,
  MissingTarget,
  UnsupportedSource,
  UnsupportedTarget,
  DuplicatePair,
};

struct CreativeLogicLinkValidationReceipt {
  bool valid = false;
  CreativeLogicLinkValidationStatus status =
      CreativeLogicLinkValidationStatus::Unknown;
  std::size_t failedLinkIndex = 0U;
  CreativeObjectId sourceObjectId = kInvalidObjectId;
  CreativeObjectId targetObjectId = kInvalidObjectId;
  std::string_view reasonCode = "creative_logic_link_not_validated";
};

inline constexpr std::size_t kCreativeLogicDiagnosticCapacity = 64U;

enum class CreativeLogicDiagnosticSeverity : std::uint8_t {
  Warning,
  Error,
};

enum class CreativeLogicDiagnosticCode : std::uint8_t {
  Unknown,
  UnlinkedSource,
  InvalidAction,
  MissingSource,
  MissingTarget,
  UnsupportedSource,
  UnsupportedTarget,
  DuplicatePair,
  ConflictingPressurePlates,
};

struct CreativeLogicDiagnostic {
  CreativeLogicDiagnosticSeverity severity =
      CreativeLogicDiagnosticSeverity::Warning;
  CreativeLogicDiagnosticCode code = CreativeLogicDiagnosticCode::Unknown;
  CreativeObjectId sourceObjectId = kInvalidObjectId;
  CreativeObjectId targetObjectId = kInvalidObjectId;
  CreativeObjectId relatedSourceObjectId = kInvalidObjectId;
  std::size_t linkIndex = 0U;
};

// Fixed-layout authoring report. It is cheap enough for an Inspector frame and
// is also consumed by map validation, keeping both views on one definition of
// missing links and conflicting automatic sources.
struct CreativeLogicDiagnosticReport {
  std::array<CreativeLogicDiagnostic, kCreativeLogicDiagnosticCapacity>
      issues{};
  std::size_t issueCount = 0U;
  std::size_t sourceCount = 0U;
  std::size_t linkedSourceCount = 0U;
  std::size_t warningCount = 0U;
  std::size_t errorCount = 0U;
  std::size_t droppedIssueCount = 0U;
  bool capacityExceeded = false;
};

enum class CreativeLogicLinkMutationStatus : std::uint8_t {
  NotRequested,
  InvalidDocument,
  InvalidAction,
  MissingSource,
  MissingTarget,
  UnsupportedSource,
  UnsupportedTarget,
  Added,
  Updated,
  Removed,
  NoChange,
  MissingLink,
};

struct CreativeLogicLinkMutationRequest {
  CreativeObjectId sourceObjectId = kInvalidObjectId;
  CreativeObjectId targetObjectId = kInvalidObjectId;
  CreativeLogicLinkAction action = CreativeLogicLinkAction::Toggle;
};

struct CreativeLogicLinkMutationReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeLogicLinkMutationStatus status =
      CreativeLogicLinkMutationStatus::NotRequested;
  CreativeLogicLink link;
  std::uint64_t revisionBefore = 0U;
  std::uint64_t revisionAfter = 0U;
  std::string_view reasonCode = "creative_logic_link_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeLogicLinkAction action) noexcept;
[[nodiscard]] bool parseCreativeLogicLinkAction(
    std::string_view value,
    CreativeLogicLinkAction& output) noexcept;
[[nodiscard]] bool isValidCreativeLogicLinkAction(
    CreativeLogicLinkAction action) noexcept;
[[nodiscard]] std::span<const CreativeLogicLinkAction>
creativeLogicLinkActions() noexcept;
[[nodiscard]] std::string_view toString(
    CreativeLogicSourceEvent event) noexcept;
[[nodiscard]] std::string_view creativeLogicSourceEventLabel(
    CreativeLogicSourceEvent event) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeLogicLinkValidationStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeLogicDiagnosticSeverity severity) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeLogicDiagnosticCode code) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeLogicLinkMutationStatus status) noexcept;

// Canonical authoring policy shared by validation, editor presentation, and
// runtime activation. Source and target roles are intentionally disjoint, so
// the current one-hop graph cannot contain authored cycles.
[[nodiscard]] std::span<const CreativeLogicEndpointDescriptor>
creativeLogicEndpointDescriptors() noexcept;
[[nodiscard]] const CreativeLogicEndpointDescriptor*
creativeLogicEndpointDescriptor(CreativeObjectKind kind) noexcept;
[[nodiscard]] CreativeLogicSourceEvent creativeLogicSourceEventForObject(
    CreativeObjectKind kind) noexcept;
[[nodiscard]] std::span<const CreativeLogicLinkAction>
creativeLogicTargetActionsForObject(CreativeObjectKind kind) noexcept;
[[nodiscard]] bool creativeObjectCanSourceLogicLink(
    CreativeObjectKind kind) noexcept;
[[nodiscard]] bool creativeObjectCanTargetLogicLink(
    CreativeObjectKind kind) noexcept;
[[nodiscard]] bool creativeLogicLinkActionSupported(
    CreativeObjectKind targetKind,
    CreativeLogicLinkAction action) noexcept;

// Validates identity, endpoint existence, endpoint roles, supported actions,
// and one-link-per-source/target pair. Link order is not semantically relevant.
[[nodiscard]] CreativeLogicLinkValidationReceipt validateCreativeLogicLinks(
    std::span<const CreativeLogicLink> links,
    std::span<const CreativeObject> objects);

[[nodiscard]] CreativeLogicDiagnosticReport buildCreativeLogicDiagnostics(
    std::span<const CreativeLogicLink> links,
    std::span<const CreativeObject> objects) noexcept;

}  // namespace iggy3d::creative
