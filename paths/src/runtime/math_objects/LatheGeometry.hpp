#pragma once
#include "runtime/math_objects/LatheProfile.hpp"
namespace paths {
struct MathTriangleSurface;
struct LatheDisplay { double turn=1,cut=0,bandFrom=0,bandTo=0; };
// Display cut removes a fraction of the angular view only. The profile and
// reported physical measurements remain unchanged. Outputs replace the mesh.
void buildLatheSurface(const LatheProfile&,const LatheDisplay&,MathTriangleSurface&);
void buildLatheElement(const LatheProfile&,const LatheDisplay&,unsigned subdivisions,bool shells,unsigned selected,MathTriangleSurface&);
}
