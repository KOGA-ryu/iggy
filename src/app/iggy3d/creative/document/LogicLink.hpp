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
[[nodiscard]] std::string_view toString(
    CreativeLogicLinkValidationStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeLogicDiagnosticSeverity severity) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeLogicDiagnosticCode code) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeLogicLinkMutationStatus status) noexcept;

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
