#pragma once

#include "app/iggy3d/creative/Core.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

enum class CreativeToolInputKind : std::uint8_t {
  Unknown,
  PointerMove,
  PointerPress,
  PointerRelease,
  Cancel,
};

enum class CreativeToolIntentKind : std::uint8_t {
  NoIntent,
  SelectObjectCandidate,
  BeginMeasurement,
  UpdateMeasurement,
  EndMeasurement,
  CancelToolAction,
  PreviewPointer,
  // Move-tool drag lifecycle (TV1-G, TD-6 preview-then-commit): a press on the
  // Move tool begins a drag, held pointer moves preview only, release commits a
  // single snapped Move mutation, cancel/escape discards with no mutation.
  BeginMove,
  PreviewMove,
  CommitMove,
  CancelMove,
};

enum class CreativeToolPointerButton : std::uint8_t {
  None,
  Primary,
  Secondary,
  Middle,
};

using CreativeToolModifierFlags = std::uint32_t;

inline constexpr CreativeToolModifierFlags kCreativeToolModifierNone = 0;
inline constexpr CreativeToolModifierFlags kCreativeToolModifierShift = 1u << 0;
inline constexpr CreativeToolModifierFlags kCreativeToolModifierControl = 1u << 1;
inline constexpr CreativeToolModifierFlags kCreativeToolModifierAlt = 1u << 2;
inline constexpr CreativeToolModifierFlags kCreativeToolModifierCommand = 1u << 3;

// Grid/world destination the window layer resolves from the pointer's grid XZ
// (the same pointer->grid-cell conversion the viewport pick uses; TD-7). Only
// the Move tool's drag Move/Release lifecycle fills it — the tool core carries
// it to the facade, which snaps it (TL-4, document snap) and commits one Move.
struct CreativeToolWorldPoint {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

// Which world axis a Move drag HOLDS (keeps at the start anchor's value) while
// the other two follow worldDestination. The CALLER picks this from its camera:
// a flat FRONT view holds Z (screen = world XY), a GROUND-PLANE editor holds Y
// (slide along the floor in XZ). Defaults to Z for the product's legacy
// front-view projection, so existing callers keep their behavior unchanged.
enum class CreativeToolMoveHeldAxis : std::uint8_t { X, Y, Z };

struct CreativeToolPointerPacket {
  double x = 0.0;
  double y = 0.0;
  CreativeToolPointerButton button = CreativeToolPointerButton::None;
  CreativeToolModifierFlags modifiers = kCreativeToolModifierNone;
  TargetRef target;
  // XZ destination for a Move drag (Y is unchanged in v1, TD-7). Filled by the
  // window for Move/Release lifecycle packets only; `hasWorldDestination` gates
  // whether the facade may commit a mutation to it.
  bool hasWorldDestination = false;
  CreativeToolWorldPoint worldDestination;
  // The axis the Move drag holds at the start anchor (default Z = front-view).
  CreativeToolMoveHeldAxis moveHeldAxis = CreativeToolMoveHeldAxis::Z;
};

struct CreativeToolInputPacket {
  CreativeToolInputKind kind = CreativeToolInputKind::Unknown;
  CreativeToolPointerPacket pointer;
};

struct CreativeToolState {
  Tool activeTool = Tool::Select;
  CreativeToolPointerPacket pointer;
  bool measurementActive = false;
  // Move-tool drag in flight (TV1-G). Set on a Move-tool press, cleared on
  // release/cancel/tool-switch. `moveDragTarget` is the picked object from the
  // press (the facade falls back to the current selection when it is invalid).
  bool moveDragActive = false;
  TargetRef moveDragTarget;
};

struct CreativeToolIntent {
  CreativeToolIntentKind kind = CreativeToolIntentKind::NoIntent;
  Tool tool = Tool::Select;
  CreativeToolPointerPacket pointer;
};

using CreativeToolIntentList = std::vector<CreativeToolIntent>;

struct CreativeToolDispatchReceipt {
  Tool activeToolBefore = Tool::Select;
  Tool activeToolAfter = Tool::Select;
  CreativeToolInputKind inputKind = CreativeToolInputKind::Unknown;
  std::size_t emittedIntentCount = 0;
  bool changedState = false;
  bool accepted = false;
  std::string_view message = "unsupported_input";
  CreativeToolIntentList intents;
};

[[nodiscard]] CreativeToolState makeDefaultCreativeToolState() noexcept;
[[nodiscard]] bool setActiveTool(CreativeToolState& state,
                                 Tool tool) noexcept;
[[nodiscard]] CreativeToolDispatchReceipt dispatchToolInput(
    CreativeToolState& state,
    const CreativeToolInputPacket& input);

}  // namespace iggy3d::creative
