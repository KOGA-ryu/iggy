#pragma once
#include "BezierCurve.hpp"

namespace paths {
// Tensor-product cubic: controls[4*i+j] carries B_i(u) B_j(v).
struct BicubicPatch { std::array<BezierPoint,16> controls{}; };
struct PatchSample {
  BezierPoint position{},du{},dv{},duu{},duv{},dvv{},normal{};
  std::array<double,16> weights{};
  double jacobian=0,E=0,F=0,G=0,e=0,f=0,g=0;
  double gaussian=0,mean=0,principalMin=0,principalMax=0;
  bool regular=false;
};
struct PatchMeshMeasure { double area=0;unsigned triangles=0,skipped=0; };
// Finite controls in [-2,2], parameters in [0,1]. Normal orientation is du x dv.
// Curvature is available only for J>1e-12 and J>1e-10*|du|*|dv|. Otherwise
// regular=false and normal/curvature fields are placeholders, not measurements.
[[nodiscard]] PatchSample samplePatch(const BicubicPatch&,double u,double v);
// Composite 2x2 Gauss quadrature, 1..32 cells/axis. O(n^2), fixed storage.
// Integrates parameterized area with multiplicity; it is not an error bound.
[[nodiscard]] double integratePatchArea(const BicubicPatch&,unsigned subdivisions);
// Two consistently oriented triangles per parameter cell, 1..32 cells/axis.
// Triangles with cross-product magnitude <=1e-12 are omitted from the mesh.
[[nodiscard]] PatchMeshMeasure measurePatchMesh(const BicubicPatch&,unsigned subdivisions);
}
