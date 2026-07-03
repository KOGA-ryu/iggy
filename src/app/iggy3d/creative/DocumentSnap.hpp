#pragma once

#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeDocumentSnapMode : std::uint8_t {
  Disabled,
  Grid,
};

using CreativeDocumentSnapAxisMask = std::uint8_t;

inline constexpr CreativeDocumentSnapAxisMask kCreativeDocumentSnapAxisNone = 0;
inline constexpr CreativeDocumentSnapAxisMask kCreativeDocumentSnapAxisX =
    1u << 0;
inline constexpr CreativeDocumentSnapAxisMask kCreativeDocumentSnapAxisY =
    1u << 1;
inline constexpr CreativeDocumentSnapAxisMask kCreativeDocumentSnapAxisZ =
    1u << 2;
inline constexpr CreativeDocumentSnapAxisMask kCreativeDocumentSnapAxisXY =
    kCreativeDocumentSnapAxisX | kCreativeDocumentSnapAxisY;
inline constexpr CreativeDocumentSnapAxisMask kCreativeDocumentSnapAxisXZ =
    kCreativeDocumentSnapAxisX | kCreativeDocumentSnapAxisZ;
inline constexpr CreativeDocumentSnapAxisMask kCreativeDocumentSnapAxisYZ =
    kCreativeDocumentSnapAxisY | kCreativeDocumentSnapAxisZ;
inline constexpr CreativeDocumentSnapAxisMask kCreativeDocumentSnapAxisXYZ =
    kCreativeDocumentSnapAxisX | kCreativeDocumentSnapAxisY |
    kCreativeDocumentSnapAxisZ;

struct CreativeDocumentSnapPoint3 {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

struct CreativeDocumentSnapBounds3 {
  CreativeDocumentSnapPoint3 min;
  CreativeDocumentSnapPoint3 max;
};

struct CreativeDocumentSnapSettings {
  CreativeDocumentSnapMode mode = CreativeDocumentSnapMode::Grid;
  CreativeDocumentSnapAxisMask axes = kCreativeDocumentSnapAxisXYZ;
  double stepX = 1.0;
  double stepY = 1.0;
  double stepZ = 1.0;
  double originX = 0.0;
  double originY = 0.0;
  double originZ = 0.0;
};

struct CreativeDocumentSnapReceipt {
  bool requested = false;
  bool accepted = false;
  bool settingsValid = false;
  bool snapped = false;
  bool changed = false;
  CreativeDocumentSnapMode mode = CreativeDocumentSnapMode::Grid;
  CreativeDocumentSnapAxisMask axes = kCreativeDocumentSnapAxisXYZ;
  CreativeDocumentSnapPoint3 originalPoint;
  CreativeDocumentSnapPoint3 snappedPoint;
  CreativeDocumentSnapBounds3 originalBounds;
  CreativeDocumentSnapBounds3 snappedBounds;
  std::string_view message = "document_snap_not_requested";
  std::string_view status = "document_snap_not_requested";
  std::string_view reasonCode = "document_snap_not_requested";
};

[[nodiscard]] CreativeDocumentSnapSettings
makeDefaultCreativeDocumentSnapSettings() noexcept;
[[nodiscard]] bool isValidCreativeDocumentSnapSettings(
    CreativeDocumentSnapSettings settings) noexcept;
[[nodiscard]] double snapCreativeDocumentScalar(double value,
                                                double step,
                                                double origin) noexcept;
[[nodiscard]] CreativeDocumentSnapReceipt snapCreativeDocumentPoint(
    CreativeDocumentSnapPoint3 point,
    CreativeDocumentSnapSettings settings) noexcept;
[[nodiscard]] CreativeDocumentSnapReceipt snapCreativeDocumentBounds(
    CreativeDocumentSnapBounds3 bounds,
    CreativeDocumentSnapSettings settings) noexcept;

}  // namespace iggy3d::creative
