#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace iggy3d::creative {
struct CreativeDocumentHistory;
}

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;

struct CreativeEditorWorldLayoutState;

enum class CreativeDesktopHistoryDomain : std::uint8_t {
  WorldLayout,
  Document,
};

struct CreativeDesktopHistoryEntry {
  CreativeDesktopHistoryDomain domain =
      CreativeDesktopHistoryDomain::Document;
  std::string source;
  std::string label;
  bool nextAction = false;
  bool blockedByUnsynchronizedSource = false;
};

struct CreativeDesktopHistoryModel {
  bool sourceSynchronized = true;
  bool canUndo = false;
  bool canRedo = false;
  std::size_t sourceMaxDepth = 0U;
  std::size_t documentMaxDepth = 0U;
  std::vector<CreativeDesktopHistoryEntry> undoEntries;
  std::vector<CreativeDesktopHistoryEntry> redoEntries;
};

// Projects both bounded history owners newest-first. World Layout source edits
// are the next action while present; document snapshots are action-disabled
// until the source is synchronized, matching the command router exactly.
[[nodiscard]] CreativeDesktopHistoryModel buildCreativeDesktopHistoryModel(
    const cr::CreativeDocumentHistory& documentHistory,
    const CreativeEditorWorldLayoutState* worldLayout);

}  // namespace iggy3d_creative_app
