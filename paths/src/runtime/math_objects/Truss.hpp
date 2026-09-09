#pragma once
#include <array>
#include <string_view>

namespace paths {
using TrussVector=std::array<double,2>;
enum class TrussShape : unsigned { Triangle, Bridge, Crane, Roof, Count };
enum class TrussSupportMode : unsigned { Designed, Released, Extra, Count };
enum class TrussStatus : unsigned { Determinate, Indeterminate, Mechanism, Degenerate };
struct TrussInput {
  TrussShape shape=TrussShape::Triangle;
  double span=3,height=1.5,lean=0,position=.5;
  std::array<TrussVector,6> offsets{}; // per-joint XY offsets, metres
  TrussVector load{0,-1}; // resultant load, kN
  double tensionLimit=2,compressionLimit=2; // user-specified axial-force caps, kN
  bool braced=true;
  TrussSupportMode supports=TrussSupportMode::Designed;
};
struct TrussMember {
  unsigned a=0,b=0;
  bool active=true;
  double length=0;
  TrussVector direction{}; // a to b; positive force is tension
};
struct TrussSupport { unsigned node=0,axis=0; }; // ideal bilateral constraint
struct TrussAnalysis {
  static constexpr unsigned nodes=6,members=9,maxSupports=4,equations=12,maxUnknowns=13;
  TrussInput input{};
  std::array<TrussVector,nodes> points{};
  std::array<TrussMember,members> bars{};
  std::array<TrussSupport,maxSupports> restraints{};
  std::array<unsigned,5> loadPath{};
  unsigned pathCount=0,supportCount=0,activeMembers=0,unknowns=0,rank=0,testMember=0;
  TrussStatus status=TrussStatus::Degenerate;
  // Row-major E, row stride maxUnknowns. E*x + applied = 0, with member
  // columns first and reactions last. columnMember maps active member columns.
  std::array<double,equations*maxUnknowns> equilibrium{};
  std::array<unsigned,members> columnMember{};
  // Complete-pivot elimination: row transform and column permutation. A
  // prepared result is immutable input to sampleTruss; no new factorization
  // is needed for load-position plots or the quasistatic sweep.
  std::array<double,equations*equations> rowTransform{};
  std::array<unsigned,maxUnknowns> columnOrder{};
  double reciprocalCondition=0;
};
struct TrussSolution {
  double position=0;
  TrussVector loadPoint{};
  std::array<TrussVector,TrussAnalysis::nodes> applied{},jointResidual{};
  std::array<double,TrussAnalysis::members> forces{},utilization{};
  std::array<double,TrussAnalysis::maxSupports> reactions{};
  bool loadCompatible=false,forcesAvailable=false;
  TrussVector resultant{};
  double moment=0,residual=0,compatibilityResidual=0,maxUtilization=0;
};
// All finite: span [1,5], height [0,3], lean [-.15,.15], position [0,1],
// load components [-5,5], offsets [-.5,.5] metres, force caps [.25,10].
// Weightless rigid bars,
// frictionless pins; only nodal loads. Coincident joints are a returned
// Degenerate state, not a rejected UI edit. Rank uses complete pivoting with
// absolute dimensionless threshold 1e-10. Fixed storage; O(12^3) preparation.
TrussAnalysis prepareTruss(const TrussInput&);
// Unmodified prepared analysis only; finite position in [0,1]. Moving load is
// split between adjacent path joints, preserving force and moment. This is a
// sequence of static loads, not dynamics of a vehicle. O(12^2) per sample.
// Force arrays are unavailable (zeroed) unless equilibrium is determinate.
TrussSolution sampleTruss(const TrussAnalysis&,double position);
struct TrussSweep { bool available=false;double maxUtilization=0,position=0;unsigned member=0; };
// Exact worst force-cap utilization over the entire piecewise-linear load path:
// member forces are affine between path joints, so check the path vertices.
TrussSweep inspectTrussSweep(const TrussAnalysis&);
std::string_view trussStatusText(TrussStatus);
}
