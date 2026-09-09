#pragma once
#include <array>

namespace paths {
using DistanceEdges=std::array<double,6>; // SQUARED lengths: AB AC AD BC BD CD
using DistancePoint=std::array<double,3>;
using DistancePoints=std::array<DistancePoint,4>;
inline constexpr std::array<std::array<unsigned,2>,6> distancePairs{{{0,1},{0,2},{0,3},{1,2},{1,3},{2,3}}};
struct DistanceAnalysis {
  std::array<double,16> squared{}; // symmetric 4x4 D with zero diagonal
  std::array<double,9> gram{}; // G_ij=(D_Ai+D_Aj-D_ij)/2, i,j=B,C,D
  std::array<double,3> eigenvalues{}; // descending, units of length squared
  DistancePoints points{}; // A=origin, canonical axes; unavailable when invalid
  std::array<double,4> witness{}; // unit zero-sum x with x^T D x>0 when invalid
  bool realizable=false,trianglesPass=false;
  unsigned dimension=0; // meaningful only when realizable
  double scale=0,tolerance=0,minTriangleSlack=0,volume=0;
  double reconstructionError=0,eigenResidual=0,witnessValue=0;
};
// Fixed four-point kernel. Squared lengths are finite in [0,64]. Analysis is
// relative to max(D), with eigen/rank tolerance 1e-10 and bounded 3x3 Jacobi
// sweeps. No heap storage. O(3^3) work with a fixed 32-sweep cap.
DistanceAnalysis analyzeDistances(const DistanceEdges&,bool mirror=false);
DistanceEdges squaredDistances(const DistancePoints&);
// Nonnegative scaling of squared-distance interpolation. t in [0,1], scale
// in [0,4]; resulting entries must remain within [0,64].
DistanceEdges mixDistances(const DistanceEdges&,const DistanceEdges&,double t,double scale);
// 0 regular tetrahedron, 1 square, 2 line, 3 differently spaced line, 4 point.
DistanceEdges distanceExample(unsigned);
}
