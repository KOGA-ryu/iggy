#pragma once
#include "runtime/math_objects/BezierCurve.hpp"
#include <array>
#include <cstddef>

namespace paths {
struct LatheInput {
  std::array<double,7> radii{.65,.9,1.15,1,.55,.48,.62};
  std::array<double,7> heights{0,.12,.35,.6,.8,.92,1};
  double height=3, wall=.12, floor=.08;
  bool hollow=true;
};
struct LatheProfile { LatheInput input; std::array<CubicBezier,6> segments{}; double maximumRadius=0; };
struct LatheSample { double radius=0,inner=0,slope=0; };
struct LatheHeights { std::array<double,80> values{}; unsigned count=0; };
struct LatheSpan { double from=0,to=0; };
struct LatheShell { std::array<LatheSpan,16> spans{}; unsigned count=0; double height=0; };
struct LatheMeasure {
  double outerVolume=0,voidVolume=0,volume=0;
  double outerArea=0,innerArea=0,closureArea=0,cutArea=0,area=0,areaDifference=0;
};
struct LatheVolumeElement { double center=0,step=0,section=0,volume=0; };
struct LatheApproximation { std::array<LatheVolumeElement,32> elements{}; unsigned count=0; double volume=0; };

// Seven ordered profile knots, with normalized endpoint heights 0 and 1.
// Radius is nonnegative; PCHIP slopes produce shape-preserving cubic Beziers.
// Fixed storage throughout. Crossing solves use 44 bisections per monotone
// segment; integration is exact Gauss-4 for volume polynomials, adaptive
// Gauss-8 (depth <=10) for lateral area. No UI, rendering or persistence.
[[nodiscard]] LatheProfile prepareLathe(const LatheInput&);
[[nodiscard]] LatheSample sampleLathe(const LatheProfile&,double heightFraction);
[[nodiscard]] LatheHeights latheHeights(const LatheProfile&,unsigned subdivisions=0);
[[nodiscard]] LatheShell latheShell(const LatheProfile&,double radius);
[[nodiscard]] LatheMeasure measureLathe(const LatheProfile&,double turnFraction=1);
[[nodiscard]] LatheApproximation approximateLatheVolume(const LatheProfile&,unsigned subdivisions,bool shells,double turnFraction=1);
[[nodiscard]] double approximateLatheArea(const LatheProfile&,unsigned subdivisions,double turnFraction=1);
}
