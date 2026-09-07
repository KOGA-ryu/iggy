#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

#include "core/math/Vec3.hpp"

namespace paths {

enum class MathObjectKind : std::uint8_t { Algebra, Trig, Calculus, Linear, Discrete, Function, Surface, Count };
enum class MathParameter : std::uint8_t {
  X, Gap, Angle, Slices, SliceGap, Sample, Shear, Scale, Depth, Shortcut,
  FunctionRule, FunctionX, DeltaX, IntegralStart, TaylorCenter, TaylorDegree,
  A00, A02, A10, A12, A20, A21, A22, VectorX, VectorY, VectorZ, ComposeAngle, SvdStage,
  SurfaceRule, SurfaceU, SurfaceV, DirectionAngle, DescentRate, Constraint, CircleAngle, Count
};
enum class MathActionKind : std::uint8_t { Select, SetParameter, Reset, VisitVertex, UndoRoute, ResetRoute, Check, SetLevel, SwapBounds, DescentStep, MatrixPreset, MoveSurfacePoint };
enum class MathShape : std::uint8_t { Box, Rod, Disk, Sphere, Ring, Cone, Count };
enum class MathFeedback : std::uint8_t { None, TryAgain, Solved };

struct MathParameterSpec {
  MathParameter id;
  MathObjectKind owner;
  std::string_view key, label;
  double minimum, maximum, step, initial;
  unsigned minimumLevel = 0;
  bool matrixEntry = false;
  std::string_view choices{}; // Null-separated labels for a discrete control.
};
struct MathObjectSpec {
  MathObjectKind id;
  std::string_view key, name, title, relationship, challenge, convention;
  std::array<std::string_view,3> progression;
  iggy3d::Vec3 cameraDirection;
};
[[nodiscard]] std::span<const MathObjectSpec> mathObjectSpecs();
[[nodiscard]] std::span<const MathParameterSpec> mathParameterSpecs();
struct MathLesson { std::string_view name, relationship, challenge, explanation; };
[[nodiscard]] std::span<const MathLesson> mathLessons(MathObjectKind);
[[nodiscard]] std::span<const std::string_view> mathMatrixPresetNames();

struct MathAction {
  MathActionKind kind;
  MathObjectKind object = MathObjectKind::Algebra;
  MathParameter parameter = MathParameter::X;
  double value = 0;
  unsigned vertex = 0;
  double secondary = 0;
};
struct MathActionResult { bool accepted = false; std::string_view reason; };

// Unit primitive plus an affine placement. Mathematical construction is owned
// here; a renderer tessellates these descriptions without recomputing policy.
struct MathPart {
  std::uint32_t id = 0;
  MathShape shape = MathShape::Box;
  iggy3d::Vec3 center{}, x{1,0,0}, y{0,1,0}, z{0,0,1}, color{};
  std::string_view role;
};
struct MathLabel { std::string_view text; iggy3d::Vec3 position; iggy3d::Vec3 color; };
struct MathMetric { std::string_view label, suffix; double value = 0; };
struct MathPlotPoint { double x = 0, y = 0; };
struct MathPlotSeries {
  std::string_view name;
  iggy3d::Vec3 color{};
  std::array<MathPlotPoint,129> points{};
  std::size_t count = 0;
  bool signedFill = false;
};
struct MathPlot {
  std::string_view title;
  std::array<MathPlotSeries,3> series{};
  std::size_t seriesCount = 0;
  MathPlotPoint marker{};
  bool hasMarker = false;
  MathParameter scrubParameter = MathParameter::Count;
};
struct MathMatrixView {
  std::string_view name;
  unsigned rows = 3, columns = 3;
  std::array<double,9> values{};
  std::array<MathParameter,9> parameters{};
  bool editable = false;
};
struct MathSurfaceVertex { iggy3d::Vec3 position{}, normal{}, color{}; };
struct MathSurfacePatch {
  static constexpr unsigned kResolution = 21;
  std::array<MathSurfaceVertex,kResolution*kResolution> vertices{};
  unsigned rows = 0, columns = 0;
};
struct MathContourSegment { MathPlotPoint a{}, b{}; double height = 0; };
struct MathContourMap {
  std::array<MathContourSegment,2048> segments{};
  std::size_t count = 0;
  MathPlotPoint point{}, gradient{};
  bool active = false, constrained = false;
};
struct MathObjectSnapshot {
  static constexpr std::size_t kPartCapacity = 192, kRouteCapacity = 64;
  MathObjectKind kind = MathObjectKind::Algebra;
  unsigned level = 0;
  std::uint64_t revision = 1;
  std::array<MathPart,kPartCapacity> parts{};
  std::size_t partCount = 0;
  std::array<MathLabel,16> labels{};
  std::size_t labelCount = 0;
  std::array<MathMetric,12> metrics{};
  std::size_t metricCount = 0;
  std::array<MathPlot,3> plots{};
  std::size_t plotCount = 0;
  std::array<MathMatrixView,3> matrices{};
  std::size_t matrixCount = 0;
  MathSurfacePatch surface;
  MathContourMap contours;
  std::array<unsigned,kRouteCapacity> route{};
  std::size_t routeCount = 1;
  std::array<bool,8> allowedVertices{};
  unsigned shortestHops = 3;
  MathFeedback feedback = MathFeedback::None;
  std::string_view feedbackText;
};

// One semantic action owner, independent of UI, SDL, Vulkan and persistence.
// Validation is atomic. Snapshot building is bounded by 192 parts and 64 disks;
// graph search is deterministic BFS over eight vertices. No per-action heap use.
class MathObjects {
public:
  MathObjects();
  [[nodiscard]] MathActionResult dispatch(const MathAction&);
  [[nodiscard]] double parameter(MathParameter p) const;
  [[nodiscard]] bool parameterAvailable(MathParameter p) const;
  [[nodiscard]] const MathObjectSnapshot& snapshot() const { return snapshot_; }
private:
  void rebuild();
  void check();
  std::array<double,static_cast<std::size_t>(MathParameter::Count)> parameters_{};
  MathObjectSnapshot snapshot_;
  bool reversedNegativeIntegral_ = false;
};

} // namespace paths
