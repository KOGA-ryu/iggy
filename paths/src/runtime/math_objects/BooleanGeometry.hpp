#pragma once
#include "runtime/math_objects/BooleanSolid.hpp"
namespace paths {
struct MathTriangleSurface;
struct BooleanDisplay { unsigned cells=20; bool section=false; double sectionZ=0; };
struct BooleanMeshReport { unsigned cells=0; double volume=0,maximumResidual=0; bool reduced=false; };
// Indexed marching cells with face-consistent contours and shared edge vertices.
// no heap allocation. Falls back in steps of four if the surface budget is full.
[[nodiscard]] BooleanMeshReport buildBooleanSurface(const BooleanSolid&,BooleanDisplay,MathTriangleSurface&);
} // namespace paths
