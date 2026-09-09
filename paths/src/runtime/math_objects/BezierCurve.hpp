#pragma once
#include <array>
#include <cstddef>

namespace paths {
using BezierPoint = std::array<double,3>;
struct CubicBezier { std::array<BezierPoint,4> controls{}; };
struct BezierSample {
  BezierPoint position{}, first{}, second{}, tangent{};
  double speed=0, curvature=0;
  bool regular=false;
};
struct BezierConstruction {
  std::array<BezierPoint,3> first{};
  std::array<BezierPoint,2> second{};
  BezierPoint point{};
};
struct BezierLengthBounds {
  double lower=0,upper=0;
  [[nodiscard]] double estimate() const {return (lower+upper)/2;}
};
struct BezierArcTable {
  static constexpr std::size_t kKnots=129;
  CubicBezier curve;
  std::array<double,kKnots> cumulative{};
  BezierLengthBounds bounds;
  bool regular=false;
  [[nodiscard]] double length() const {return cumulative.back();}
};
struct BezierFrame { BezierPoint tangent{},normal{},binormal{}; bool defined=false; };

// Pure, fixed-storage kernels. Controls must be finite and within [-2,2],
// matching the lab's editable domain; t and distance fractions lie in [0,1].
// Length bounds use chord/control-polygon subdivision with depth at most 12.
// Bounds retain any remaining uncertainty instead of treating it as exact.
[[nodiscard]] BezierConstruction constructBezier(const CubicBezier&,double t);
[[nodiscard]] BezierSample sampleBezier(const CubicBezier&,double t);
[[nodiscard]] BezierArcTable analyzeBezier(const CubicBezier&);
[[nodiscard]] double bezierArcAt(const BezierArcTable&,double t);
[[nodiscard]] double bezierParameterAtFraction(const BezierArcTable&,double fraction);
// Discrete rotation-minimizing frames. A straight segment is valid; a zero
// tangent is not. At an exact 180-degree step, retain the previous normal.
[[nodiscard]] BezierFrame seedBezierFrame(const BezierPoint& tangent);
[[nodiscard]] BezierFrame transportBezierFrame(const BezierFrame&,const BezierPoint& tangent);
}
