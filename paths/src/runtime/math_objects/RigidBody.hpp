#pragma once
#include <array>
#include <cstdint>

namespace paths {
using RigidVector=std::array<double,3>;
using RigidQuaternion=std::array<double,4>; // w,x,y,z; active body-to-world rotation
using RigidMatrix=std::array<double,9>; // row major

enum class RigidShape : unsigned { Flywheel, Dumbbell, Book, Satellite, Count };
enum class RigidPrimitive : unsigned { Box, Cylinder };
struct RigidInput {
  RigidShape shape=RigidShape::Flywheel;
  RigidVector dimensions{2.4,.3,2.4}; // full body-space extents
  double mass=1,balance=.5; // left share of end-weight/panel mass
  RigidVector rotationDegrees{}; // apply fixed X, then Y, then Z at release
  RigidVector omega{0,3,0}; // initial body-frame rad/s
  bool operator==(const RigidInput&) const = default;
};
struct RigidComponent {
  RigidPrimitive primitive=RigidPrimitive::Box;
  double mass=0;
  RigidVector center{},dimensions{}; // center relative to assembly COM
  unsigned material=0;
};
struct RigidBody {
  std::array<RigidComponent,5> parts{};
  unsigned count=0;
  double mass=0,radius=0;
  RigidVector center{},inertia{}; // original COM offset, principal moments
  std::array<unsigned,3> order{}; // min, middle, max body-axis indices
  bool distinctMoments=false;
};
struct RigidState {
  double time=0,energy=0,energyError=0,momentumError=0;
  RigidQuaternion orientation{1,0,0,0};
  RigidVector bodyMomentum{},bodyOmega{},worldMomentum{},worldOmega{};
  RigidMatrix rotation{},worldInertia{};
};
// Finite inputs only: dimensions [.2,3.5], mass [.2,5], balance [.2,.8],
// Euler angles [-180,180] degrees, each initial spin component [-4,4] rad/s.
// Component interiors do not overlap. Homogeneous component formulas plus the
// parallel-axis theorem give diagonal body inertia; off-diagonal world terms
// arise from orientation. Repeated moments have no unique intermediate axis.
RigidBody makeRigidBody(const RigidInput&);
RigidQuaternion rigidReleaseOrientation(const RigidVector& degrees);
RigidVector rigidRotate(const RigidQuaternion&,const RigidVector&);
RigidMatrix rigidRotationMatrix(const RigidQuaternion&);
double rigidMagnitude(const RigidVector&);

class RigidMotion {
public:
  static constexpr double horizon=12;
  static constexpr unsigned samples=129,maxSteps=262144;
  // Validates and prepares a complete candidate cache before replacing state.
  // Identical input is a no-op. No allocations; at most maxSteps RK4 steps.
  bool configure(const RigidInput&);
  // Deterministic random access on [0,12], replaying at most one cache interval.
  // Access before configure or outside the domain throws invalid_argument.
  RigidState at(double time) const;
  const RigidBody& body() const;
  double speedBound() const { return speedBound_; }
  std::uint64_t revision() const { return revision_; }
  unsigned integrationSteps() const { return steps_; }
private:
  using State=std::array<double,7>; // quaternion followed by body angular momentum
  State advance(State,double duration) const;
  RigidState describe(const State&,double time) const;
  RigidInput input_{};
  RigidBody body_{};
  std::array<State,samples> cache_{};
  RigidVector initialWorldMomentum_{};
  double initialEnergy_=0,speedBound_=0,step_=0;
  unsigned steps_=0;
  std::uint64_t revision_=0;
};
}
