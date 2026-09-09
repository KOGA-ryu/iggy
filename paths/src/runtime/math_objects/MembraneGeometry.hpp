#pragma once
#include "Membrane.hpp"
namespace paths {
struct MathTriangleSurface;
enum class MembraneColour { Displacement, SelectedBasis, Energy };
struct MembraneDisplay {unsigned subdivisions=32,selected=0;bool selectedOnly=false;MembraneColour colour=MembraneColour::Displacement;};
// Height graph with upward analytic normals. At most 2401 vertices/4608 triangles.
// World coordinates (width*(u-.5), displacement, depth*(.5-v)).
// Display changes never alter the state or global measurements.
void buildMembraneSurface(const MembraneState&,MembraneDisplay,MathTriangleSurface&);
}
