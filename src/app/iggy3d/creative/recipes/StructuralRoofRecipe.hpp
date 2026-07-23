#pragma once

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace iggy3d::creative {

enum class CreativeStructuralRoofStyle : std::uint8_t {
  Flat = 0U,
  Gable = 1U,
  // Appended values preserve the schema-8 Flat/Gable wire values.
  Shed = 2U,
  Hip = 3U,
  Count,
};

enum class CreativeStructuralRoofRidgeAxis : std::uint8_t {
  X,
  Z,
  Count,
};

// Downhill direction for a shed roof. Gable and hip roofs use ridgeAxis.
enum class CreativeStructuralRoofSlopeDirection : std::uint8_t {
  PositiveX,
  NegativeX,
  PositiveZ,
  NegativeZ,
  Count,
};

inline constexpr double kDefaultCreativeStructuralRoofPitchDegrees = 30.0;
inline constexpr double kMinimumCreativeStructuralRoofPitchDegrees = 5.0;
inline constexpr double kMaximumCreativeStructuralRoofPitchDegrees = 75.0;

enum class CreativeStructuralRoofRecipeStatus : std::uint8_t {
  NotRequested,
  InvalidFootprint,
  InvalidSupportPlane,
  InvalidLayerCount,
  InvalidStyle,
  InvalidRidgeAxis,
  InvalidSlopeDirection,
  InvalidPitch,
  InvalidOverhang,
  InvalidMaterial,
  InvalidRidgeSpan,
  UnrepresentableGeometry,
  CapacityExceeded,
  Ready,
};

struct CreativeStructuralRoofRecipeRequest {
  CreativeStructuralRoofStyle style = CreativeStructuralRoofStyle::Flat;
  CreativeStructuralRoofRidgeAxis ridgeAxis =
      CreativeStructuralRoofRidgeAxis::X;
  CreativeStructuralRoofSlopeDirection slopeDirection =
      CreativeStructuralRoofSlopeDirection::PositiveZ;
  double minimumX = 0.0;
  double maximumX = 0.0;
  double minimumZ = 0.0;
  double maximumZ = 0.0;
  double supportPlaneMeters = 0.0;
  std::uint16_t layerCount = 1U;
  double pitchDegrees = kDefaultCreativeStructuralRoofPitchDegrees;
  double overhangMeters = 0.0;
  CreativeStructuralMaterial material = CreativeStructuralMaterial::Blockout;
};

enum class CreativeStructuralRoofPartKind : std::uint8_t {
  FlatPanel,
  ShedPanel,
  GableFirst,
  GableSecond,
  HipNorth,
  HipEast,
  HipSouth,
  HipWest,
  Count,
};

struct CreativeStructuralRoofPart {
  CreativeStructuralRoofPartKind partKind =
      CreativeStructuralRoofPartKind::Count;
  CreativeObjectKind kind = CreativeObjectKind::Unknown;
  CreativeBounds bounds;
  CreativeVec3 rotationEulerRadians;
};

enum class CreativeStructuralRoofPerimeterEdge : std::uint8_t {
  North,
  East,
  South,
  West,
  Count,
};

enum class CreativeStructuralRoofEdgeKind : std::uint8_t {
  Eave,
  Verge,
  Count,
};

struct CreativeStructuralRoofEdgePlan {
  CreativeStructuralRoofPerimeterEdge edge =
      CreativeStructuralRoofPerimeterEdge::Count;
  CreativeStructuralRoofEdgeKind kind = CreativeStructuralRoofEdgeKind::Count;
  CreativeVec3 startMeters;
  CreativeVec3 endMeters;
  CreativeVec3 outward;
  bool drainageEligible = false;
};

struct CreativeStructuralRoofDrainageSocketPlan {
  CreativeStructuralRoofPerimeterEdge edge =
      CreativeStructuralRoofPerimeterEdge::Count;
  CreativeVec3 positionMeters;
  CreativeVec3 outward;
};

struct CreativeStructuralRoofPartSocketPlan {
  CreativeStructuralRoofPerimeterEdge edge =
      CreativeStructuralRoofPerimeterEdge::Count;
  CreativeVec3 localPosition;
  CreativeVec3 forward;
  CreativeVec3 up;
};

inline constexpr std::size_t kCreativeStructuralRoofPartCapacity = 4U;
inline constexpr std::size_t kCreativeStructuralRoofEdgeCapacity = 4U;
inline constexpr std::size_t kCreativeStructuralRoofDrainageSocketCapacity =
    4U;
inline constexpr std::size_t kCreativeStructuralRoofApertureCapacity = 4U;
inline constexpr std::size_t kCreativeStructuralRoofAperturePieceCapacity =
    84U;
inline constexpr std::size_t kInvalidCreativeStructuralRoofApertureIndex =
    static_cast<std::size_t>(-1);

struct CreativeStructuralRoofRecipeResult {
  bool accepted = false;
  CreativeStructuralRoofRecipeStatus status =
      CreativeStructuralRoofRecipeStatus::NotRequested;
  std::array<CreativeStructuralRoofPart,
             kCreativeStructuralRoofPartCapacity>
      parts{};
  std::size_t partCount = 0U;
  std::array<CreativeStructuralRoofEdgePlan,
             kCreativeStructuralRoofEdgeCapacity>
      edges{};
  std::size_t edgeCount = 0U;
  std::array<CreativeStructuralRoofDrainageSocketPlan,
             kCreativeStructuralRoofDrainageSocketCapacity>
      drainageSockets{};
  std::size_t drainageSocketCount = 0U;
  CreativeBounds worldBounds;
  CreativeVec3 ridgeStart;
  CreativeVec3 ridgeEnd;
  double riseMeters = 0.0;
  double thicknessMeters = 0.0;
  CreativeStructuralMaterial material = CreativeStructuralMaterial::Blockout;
  std::string_view reasonCode = "creative_structural_roof_not_requested";
};

struct CreativeStructuralRoofPartSocketResult {
  bool accepted = false;
  std::array<CreativeStructuralRoofPartSocketPlan,
             kCreativeStructuralRoofDrainageSocketCapacity>
      sockets{};
  std::size_t socketCount = 0U;
  std::string_view reasonCode =
      "creative_structural_roof_part_sockets_not_requested";
};

enum class CreativeStructuralRoofApertureKind : std::uint8_t {
  Skylight,
  ChimneyClearance,
  Count,
};

struct CreativeStructuralRoofAperture {
  CreativeStructuralRoofApertureKind kind =
      CreativeStructuralRoofApertureKind::Skylight;
  double minimumX = 0.0;
  double maximumX = 0.0;
  double minimumZ = 0.0;
  double maximumZ = 0.0;
};

enum class CreativeStructuralRoofApertureStatus : std::uint8_t {
  NotRequested,
  InvalidRoof,
  InvalidCount,
  InvalidClearance,
  InvalidAperture,
  ApertureOutsideRoof,
  AperturesTooClose,
  UnsupportedRoofStyle,
  ApertureCrossesPanelBoundary,
  CapacityExceeded,
  UnrepresentableGeometry,
  Ready,
};

struct CreativeStructuralRoofApertureRequest {
  CreativeStructuralRoofRecipeRequest roof;
  std::array<CreativeStructuralRoofAperture,
             kCreativeStructuralRoofApertureCapacity>
      apertures{};
  std::size_t apertureCount = 0U;
  double minimumClearanceMeters = 0.1;
};

struct CreativeStructuralRoofAperturePiece {
  std::size_t sourcePartIndex = 0U;
  CreativeStructuralRoofPart part;
};

struct CreativeStructuralRoofApertureInsertPlan {
  std::size_t apertureIndex = 0U;
  std::size_t sourcePartIndex = 0U;
  CreativeObjectKind kind = CreativeObjectKind::Window;
  CreativeBounds bounds;
  CreativeVec3 rotationEulerRadians;
};

struct CreativeStructuralRoofApertureResult {
  bool accepted = false;
  CreativeStructuralRoofApertureStatus status =
      CreativeStructuralRoofApertureStatus::NotRequested;
  CreativeStructuralRoofRecipeResult roof;
  std::array<CreativeStructuralRoofAperturePiece,
             kCreativeStructuralRoofAperturePieceCapacity>
      pieces{};
  std::size_t pieceCount = 0U;
  std::array<CreativeStructuralRoofApertureInsertPlan,
             kCreativeStructuralRoofApertureCapacity>
      inserts{};
  std::size_t insertCount = 0U;
  std::size_t failedApertureIndex =
      kInvalidCreativeStructuralRoofApertureIndex;
  std::string_view reasonCode =
      "creative_structural_roof_aperture_not_requested";
};

static_assert(std::is_trivially_copyable_v<CreativeStructuralRoofRecipeRequest>);
static_assert(std::is_trivially_copyable_v<CreativeStructuralRoofPart>);
static_assert(std::is_trivially_copyable_v<CreativeStructuralRoofEdgePlan>);
static_assert(
    std::is_trivially_copyable_v<CreativeStructuralRoofDrainageSocketPlan>);
static_assert(std::is_trivially_copyable_v<CreativeStructuralRoofPartSocketPlan>);
static_assert(std::is_trivially_copyable_v<CreativeStructuralRoofRecipeResult>);
static_assert(
    std::is_trivially_copyable_v<CreativeStructuralRoofPartSocketResult>);
static_assert(std::is_trivially_copyable_v<CreativeStructuralRoofAperture>);
static_assert(
    std::is_trivially_copyable_v<CreativeStructuralRoofApertureRequest>);
static_assert(
    std::is_trivially_copyable_v<CreativeStructuralRoofAperturePiece>);
static_assert(std::is_trivially_copyable_v<
              CreativeStructuralRoofApertureInsertPlan>);
static_assert(
    std::is_trivially_copyable_v<CreativeStructuralRoofApertureResult>);

[[nodiscard]] bool validCreativeStructuralRoofSettings(
    CreativeStructuralRoofStyle style,
    CreativeStructuralRoofRidgeAxis ridgeAxis,
    CreativeStructuralRoofSlopeDirection slopeDirection,
    double pitchDegrees,
    double overhangMeters,
    CreativeStructuralMaterial material) noexcept;

[[nodiscard]] std::string_view toString(
    CreativeStructuralRoofStyle style) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeStructuralRoofRidgeAxis axis) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeStructuralRoofSlopeDirection direction) noexcept;

[[nodiscard]] std::string_view creativeStructuralRoofDrainageSocketName(
    CreativeStructuralRoofPerimeterEdge edge) noexcept;
[[nodiscard]] std::string_view creativeStructuralRoofDrainageCompatibility()
    noexcept;

// Derives local receiver frames for a generated roof child. Horizontal Roof
// parts publish all four perimeter edges; RoofSlope and HipRoof publish their
// single low eave. Imported meshes continue to own their authored sockets.
[[nodiscard]] CreativeStructuralRoofPartSocketResult
planCreativeStructuralRoofPartSockets(
    CreativeObjectKind kind,
    const CreativeBounds& authoredBounds,
    const CreativeTransform& transform) noexcept;

// Produces the exact thin generated panels for both preview and document
// creation. Flat emits one horizontal panel, shed one sloped panel, gable two
// rectangular panels, and hip four tapered panels. Complex intersections and
// dormers are intentionally outside this rectangular-footprint kernel.
[[nodiscard]] CreativeStructuralRoofRecipeResult planCreativeStructuralRoof(
    const CreativeStructuralRoofRecipeRequest& request) noexcept;

// Partitions Flat, Shed, and Gable roof panels around bounded plan-space
// apertures. Skylights receive one fitted glazing insert; chimney clearances
// remain open. Hip apertures require polygon-panel storage and fail explicitly.
[[nodiscard]] CreativeStructuralRoofApertureResult
planCreativeStructuralRoofApertures(
    const CreativeStructuralRoofApertureRequest& request) noexcept;

}  // namespace iggy3d::creative
