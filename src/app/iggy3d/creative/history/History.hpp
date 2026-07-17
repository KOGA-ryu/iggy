#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/Facade.hpp"

namespace iggy3d::creative {

enum class CreativeHistoryDirection : std::uint8_t {
  Undo,
  Redo,
};

enum class CreativeHistoryStatus : std::uint8_t {
  NotRequested,
  InvalidDocument,
  InactiveTransaction,
  Disabled,
  NoChange,
  Recorded,
  Empty,
  InstallRejected,
  Applied,
};

struct CreativeHistorySidecar {
  std::string type;
  std::uint32_t version = 0U;
  std::string payload;

  friend bool operator==(const CreativeHistorySidecar&,
                         const CreativeHistorySidecar&) = default;
};

struct CreativeDocumentHistorySnapshot {
  CreativeDocument document;
  std::string source;
  std::optional<CreativeHistorySidecar> sidecar;
};

struct CreativeDocumentHistory {
  std::vector<CreativeDocumentHistorySnapshot> undoSnapshots;
  std::vector<CreativeDocumentHistorySnapshot> redoSnapshots;
  std::size_t maxDepth = 32;
};

struct CreativeDocumentHistoryTransaction {
  bool active = false;
  CreativeDocument before;
  std::string source;
  std::optional<CreativeHistorySidecar> beforeSidecar;
};

struct CreativeHistoryRecordReceipt {
  bool requested = false;
  bool accepted = false;
  bool recorded = false;
  bool trimmedOldestUndo = false;
  std::uint64_t clearedRedoCount = 0;
  std::uint64_t undoDepthBefore = 0;
  std::uint64_t undoDepthAfter = 0;
  std::uint64_t redoDepthBefore = 0;
  std::uint64_t redoDepthAfter = 0;
  CreativeHistoryStatus status = CreativeHistoryStatus::NotRequested;
  std::string_view reasonCode = "creative_history_not_requested";
};

struct CreativeHistoryApplyReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  bool hadSnapshot = false;
  CreativeHistoryDirection direction = CreativeHistoryDirection::Undo;
  CreativeHistoryStatus status = CreativeHistoryStatus::NotRequested;
  CreativeDocumentId documentId = kInvalidDocumentId;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::uint64_t objectCountBefore = 0;
  std::uint64_t objectCountAfter = 0;
  std::uint64_t undoDepthBefore = 0;
  std::uint64_t undoDepthAfter = 0;
  std::uint64_t redoDepthBefore = 0;
  std::uint64_t redoDepthAfter = 0;
  std::string source;
  std::string reasonCode = "creative_history_not_requested";
  CreativeFacadeDocumentInstallReceipt installReceipt;
  std::optional<CreativeHistorySidecar> targetSidecar;
};

[[nodiscard]] std::string_view toString(
    CreativeHistoryDirection direction) noexcept;
[[nodiscard]] std::string_view toString(CreativeHistoryStatus status) noexcept;

[[nodiscard]] bool creativeUndoAvailable(
    const CreativeDocumentHistory& history) noexcept;
[[nodiscard]] bool creativeRedoAvailable(
    const CreativeDocumentHistory& history) noexcept;
[[nodiscard]] std::uint64_t creativeUndoDepth(
    const CreativeDocumentHistory& history) noexcept;
[[nodiscard]] std::uint64_t creativeRedoDepth(
    const CreativeDocumentHistory& history) noexcept;
void clearCreativeHistory(CreativeDocumentHistory& history) noexcept;

[[nodiscard]] CreativeDocumentHistoryTransaction
beginCreativeHistoryTransaction(const Facade& facade, std::string_view source);
[[nodiscard]] CreativeDocumentHistoryTransaction
beginCreativeHistoryTransaction(
    const Facade& facade,
    std::string_view source,
    std::optional<CreativeHistorySidecar> beforeSidecar);
[[nodiscard]] CreativeHistoryRecordReceipt commitCreativeHistoryTransaction(
    CreativeDocumentHistory& history,
    CreativeDocumentHistoryTransaction transaction,
    const Facade& facade);
void cancelCreativeHistoryTransaction(
    CreativeDocumentHistoryTransaction& transaction) noexcept;

[[nodiscard]] CreativeHistoryApplyReceipt applyCreativeHistory(
    Facade& facade,
    CreativeDocumentHistory& history,
    CreativeHistoryDirection direction,
    std::optional<CreativeHistorySidecar> currentSidecar = std::nullopt);

[[nodiscard]] const CreativeHistorySidecar* creativeHistoryTargetSidecar(
    const CreativeDocumentHistory& history,
    CreativeHistoryDirection direction) noexcept;

}  // namespace iggy3d::creative
