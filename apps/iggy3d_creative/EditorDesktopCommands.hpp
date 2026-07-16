#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

#include "EditorDesktopCommandPayloads.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"

namespace iggy3d {

struct StaticMeshAssetCatalog;

}  // namespace iggy3d

namespace iggy3d_creative_app {

struct CreativeEditorState;
struct CreativeEditorPlayMode;

// Fixed-layout semantic command IDs the desktop UI emits. Widgets never touch
// documents/history/assets directly — they push one of these (plus a typed
// payload) into the bounded frame, and EditorDesktopCommands.cpp is the sole
// dispatcher (plan DD-7 / DL-3). Step 3 adds the selection/edit/asset families
// the functional panels (Step 4+) will emit. Import commands arrive later.
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
  // Step 3 — Desktop Command Expansion.
  SelectObjects,
  FocusObject,
  ClearSelection,
  SetLogicSource,
  ClearLogicSource,
  SetLogicLink,
  RemoveLogicLink,
  DeleteObjects,
  RenameObject,
  SetObjectsVisible,
  SetObjectsLocked,
  SetObjectTransform,
  SetMovingPlatformSettings,
  ToggleMovingPlatformPreview,
  RestartMovingPlatformPreview,
  SeekMovingPlatformPreview,
  EquipAsset,
  EditAssetSource,
  RenameAsset,
  DuplicateAsset,
  DeleteAsset,
  RefreshInstances,
  UpdateAssetFromInstance,
  Count,
};

struct CreativeDesktopCommand {
  CreativeDesktopCommandId id = CreativeDesktopCommandId::None;
  // Typed, discriminated payload (see EditorDesktopCommandPayloads.hpp).
  // monostate for the no-argument commands.
  CreativeDesktopCommandPayload payload;
};

inline constexpr std::size_t kCreativeDesktopCommandCapacity = 16U;

// Bounded per-frame command queue. Emitting widgets append; the dispatcher
// drains. Overflow is recorded, never a buffer overrun.
struct CreativeDesktopCommandFrame {
  std::array<CreativeDesktopCommand, kCreativeDesktopCommandCapacity> commands{};
  std::size_t count = 0U;
  bool overflowed = false;

  // No-payload commands (monostate).
  void push(CreativeDesktopCommandId id);
  // SaveDocumentAs convenience: wraps the target save id into a SaveAs payload.
  void push(CreativeDesktopCommandId id, std::string saveId);
  // Typed-payload commands.
  void push(CreativeDesktopCommandId id, CreativeDesktopCommandPayload payload);
  void clear() noexcept;
};

// Outcome of dispatching a frame — cached for the status/history bar (DD-11)
// and asserted by the headless command tests.
struct CreativeDesktopCommandResult {
  CreativeDesktopCommandId lastCommand = CreativeDesktopCommandId::None;
  bool accepted = false;
  bool changed = false;
  bool documentReplaced = false;
  std::uint64_t affectedObjectCount = 0U;  // objects a batch command touched.
  std::string message;
};

struct CreativeDesktopCommandContext {
  iggy3d::creative::CreativeAppState& appState;
  CreativeEditorState& editor;
  std::filesystem::path saveRoot;
  std::string* activeSaveId = nullptr;  // Save As rebinds the active id here.
  CreativeEditorPlayMode* playMode = nullptr;
  const iggy3d::StaticMeshAssetCatalog* staticMeshAssetCatalog = nullptr;
};

// Applies every command in the frame to the existing kernels (New/Open/Save/
// Save As over EditorPersistence, Undo/Redo/Duplicate/Delete over EditorEdits),
// following the same transaction/history discipline as the keyboard dispatcher.
// Pure of ImGui and the window — fully headless-testable.
CreativeDesktopCommandResult dispatchCreativeDesktopCommands(
    const CreativeDesktopCommandFrame& frame,
    const CreativeDesktopCommandContext& context);

}  // namespace iggy3d_creative_app
