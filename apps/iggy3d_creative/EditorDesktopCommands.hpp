#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

#include "app/iggy3d/creative/CreativeAppState.hpp"

namespace iggy3d_creative_app {

struct CreativeEditorState;

// Fixed-layout semantic command IDs the desktop UI emits. Widgets never touch
// documents/history/assets directly — they push one of these into the bounded
// frame, and EditorDesktopCommands.cpp is the sole dispatcher (plan DD-7 /
// DL-3). Import + asset/instance/selection commands arrive in later slices.
enum class CreativeDesktopCommandId : std::uint8_t {
  None,
  NewDocument,
  OpenDocument,
  SaveDocument,
  SaveDocumentAs,
  Undo,
  Redo,
  DuplicateSelection,
  DeleteSelection,
  Play,
  Count,
};

struct CreativeDesktopCommand {
  CreativeDesktopCommandId id = CreativeDesktopCommandId::None;
  std::string arg;  // SaveDocumentAs: the target save id. Unused otherwise.
};

inline constexpr std::size_t kCreativeDesktopCommandCapacity = 16U;

// Bounded per-frame command queue. Emitting widgets append; the dispatcher
// drains. Overflow is recorded, never a buffer overrun.
struct CreativeDesktopCommandFrame {
  std::array<CreativeDesktopCommand, kCreativeDesktopCommandCapacity> commands{};
  std::size_t count = 0U;
  bool overflowed = false;

  void push(CreativeDesktopCommandId id, std::string arg = {});
  void clear() noexcept;
};

// Outcome of dispatching a frame — cached for the status/history bar (DD-11)
// and asserted by the headless command tests.
struct CreativeDesktopCommandResult {
  CreativeDesktopCommandId lastCommand = CreativeDesktopCommandId::None;
  bool accepted = false;
  bool changed = false;
  bool documentReplaced = false;
  std::string message;
};

struct CreativeDesktopCommandContext {
  iggy3d::creative::CreativeAppState& appState;
  CreativeEditorState& editor;
  std::filesystem::path saveRoot;
  std::string* activeSaveId = nullptr;  // Save As rebinds the active id here.
};

// Applies every command in the frame to the existing kernels (New/Open/Save/
// Save As over EditorPersistence, Undo/Redo/Duplicate/Delete over EditorEdits),
// following the same transaction/history discipline as the keyboard dispatcher.
// Pure of ImGui and the window — fully headless-testable.
CreativeDesktopCommandResult dispatchCreativeDesktopCommands(
    const CreativeDesktopCommandFrame& frame,
    const CreativeDesktopCommandContext& context);

}  // namespace iggy3d_creative_app
