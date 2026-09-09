#pragma once
#include <array>

namespace paths {
using SolidPoint = std::array<double,3>;
enum class SolidShape : unsigned { Box, Sphere, Cylinder, Arch, Count };
enum class SolidOperation : unsigned { Union, Intersection, Difference, SmoothUnion, Count };
struct SolidPrimitive {
  SolidShape shape=SolidShape::Box;
  double size=1;
  SolidPoint center{};
  double yaw=0,pitch=0; // R_y(yaw) R_x(pitch), in degrees.
};
struct BooleanInput {
  SolidPrimitive a{};
  SolidPrimitive b{SolidShape::Cylinder,.55};
  SolidOperation operation=SolidOperation::Difference;
  double blend=.4;
};
struct SolidSample {
  double value=0;
  SolidPoint gradient{};
  bool regular=false;
  double material=0; // 0 = A, 1 = B (also the newly exposed subtraction wall).
};
struct SolidBounds { SolidPoint minimum{},maximum{}; };
struct PreparedPrimitive { SolidPrimitive input; std::array<SolidPoint,3> axes; SolidBounds bounds; };
struct BooleanSolid { BooleanInput input; PreparedPrimitive a,b; SolidBounds bounds; };
struct SolidProbe { SolidPoint position{}; SolidSample sample; bool found=false; };
struct SolidVolume { unsigned cells=0,inside=0; double volume=0,cellVolume=0; };
[[nodiscard]] BooleanSolid prepareBoolean(const BooleanInput&);
[[nodiscard]] SolidSample samplePrimitive(const PreparedPrimitive&,SolidPoint);
[[nodiscard]] SolidSample combineSolids(SolidSample,SolidSample,SolidOperation,double blend);
[[nodiscard]] SolidSample sampleBoolean(const BooleanSolid&,SolidPoint);
[[nodiscard]] SolidProbe probeBooleanSurface(const BooleanSolid&,SolidPoint);
[[nodiscard]] SolidVolume measureBooleanVolume(const BooleanSolid&,unsigned cells);
[[nodiscard]] bool booleanTruth(SolidOperation,bool a,bool b); // SmoothUnion returns its hard-union baseline.
} // namespace paths
