#pragma once
#include "BezierPatch.hpp"
namespace paths {
struct MathTriangleSurface;
enum class PatchColour { Material, Influence, Gaussian, AreaDensity };
struct PatchDisplay {unsigned subdivisions=20,control=5;PatchColour colour=PatchColour::Material;};
// One open surface, bounded by (32+1)^2 vertices and 2*32^2 triangles.
// Invalid input is rejected before touching output. Singular points are grey;
// no thickness, closed-volume or global-injectivity claim is made.
void buildPatchSurface(const BicubicPatch&,PatchDisplay,MathTriangleSurface&);
}
