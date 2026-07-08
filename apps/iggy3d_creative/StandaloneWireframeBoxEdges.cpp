#include "StandaloneWireframeBoxEdges.hpp"

namespace iggy3d_creative_app {

void appendStandaloneWireframeBoxEdges(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& out,
    iggy3d::Vec3 boxMin,
    iggy3d::Vec3 boxMax,
    iggy3d::RenderLineColor color,
    float thickness) {
  // 8 corners indexed by (x bit0, y bit1, z bit2).
  const auto corner = [&](int c) -> iggy3d::Vec3 {
    return {(c & 1) ? boxMax.x : boxMin.x, (c & 2) ? boxMax.y : boxMin.y,
            (c & 4) ? boxMax.z : boxMin.z};
  };
  // 12 edges: pairs of corner indices differing in exactly one axis bit.
  static constexpr int kEdges[12][2] = {
      {0, 1}, {2, 3}, {4, 5}, {6, 7},  // along X
      {0, 2}, {1, 3}, {4, 6}, {5, 7},  // along Y
      {0, 4}, {1, 5}, {2, 6}, {3, 7},  // along Z
  };
  out.reserve(out.size() + 12);
  for (const auto& e : kEdges) {
    iggy3d::RenderCreativeWireframeDebugLine line;
    line.start = corner(e[0]);
    line.end = corner(e[1]);
    line.color = color;
    line.objectId = 0;  // Ghost is not a document object.
    line.thickness = thickness;
    out.push_back(line);
  }
}

}  // namespace iggy3d_creative_app
