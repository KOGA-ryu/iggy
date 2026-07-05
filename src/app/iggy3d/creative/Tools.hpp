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

struct CreativeToolPointerPacket {
  double x = 0.0;
  double y = 0.0;
  CreativeToolPointerButton button = CreativeToolPointerButton::None;
  CreativeToolModifierFlags modifiers = kCreativeToolModifierNone;
  TargetRef target;
};

struct CreativeToolInputPacket {
  CreativeToolInputKind kind = CreativeToolInputKind::Unknown;
  CreativeToolPointerPacket pointer;
};

struct CreativeToolState {
  Tool activeTool = Tool::Select;
  CreativeToolPointerPacket pointer;
  bool measurementActive = false;
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
