#pragma once

#include <array>
#include <bitset>
#include <cstdint>
#include <span>
#include <string_view>

#include "core/math/Vec3.hpp"
#include "runtime/math_objects/RigidBody.hpp"

namespace paths {

enum class MathObjectKind : std::uint8_t { Algebra, Trig, Calculus, Linear, Discrete, Function, Surface, Symmetry, Harmonics, Oscillator, Modular, Gaussian, VectorField, Flux, Tensor, Probability, Binomial, Bayes, Covariance, Spherical, Quadratic, Roots, Psd, Norm, Curve, Lathe, Boolean, Patch, Membrane, Rigid, Truss, Simplex, Distance, Polar, Qr, Maps, Graph, FiniteAlgebra, Quotient, Count };
enum class MathParameter : std::uint16_t {
  X, Gap, Angle, Slices, SliceGap, Sample, Shear, Scale, Depth, Shortcut,
  FunctionRule, FunctionX, DeltaX, IntegralStart, TaylorCenter, TaylorDegree,
  A00, A02, A10, A12, A20, A21, A22, VectorX, VectorY, VectorZ, ComposeAngle, SvdStage,
  SurfaceRule, SurfaceU, SurfaceV, DirectionAngle, DescentRate, Constraint, CircleAngle,
  SymmetryFirst, SymmetrySecond, SymmetryGenerator, SymmetryPower, SymmetryElement, SymmetryVertex,
  HarmonicTime, Amplitude1, Frequency1, Phase1, Amplitude2, Frequency2, Phase2, Waveform, HarmonicTerms, ProbeFrequency,
  MotionSystem, InitialPosition, InitialVelocity, Stiffness, MotionTime, Damping, DriveAmplitude, DriveFrequency,
  Modulus, ModValue, ModStep, InverseGuess, SecondModulus, SecondResidue, CrtGuess,
  GaussianReal, GaussianImag, GaussianOtherReal, GaussianOtherImag, GaussianOperation, GaussianHeight, GaussianQuotient,
  FieldRule, FieldX, FieldY, FieldZ, FieldYaw, FieldPitch, FieldPath, FieldTime,
  FluxShape, FluxField, FluxRadius, FluxTilt, FluxOrientation, FluxProbe, FluxResolution, FluxTime,
  TensorU0, TensorU1, TensorU2, TensorV0, TensorV1, TensorV2, TensorW0, TensorW1, TensorW2,
  TensorI, TensorJ, TensorK, TensorGap, TensorBasis,
  ProbabilityRule, ProbabilityStay, ProbabilityStart, ProbabilityMix, ProbabilitySeed, ProbabilityRow, ProbabilitySteps,
  BinomialTrials, BinomialChance, BinomialCut, BinomialSeed,
  BayesPrior, BayesHit, BayesFalse, BayesEvent, BayesPositive, BayesNegative,
  CloudX, CloudY, CloudZ, CloudYaw, CloudPitch, CloudShear, CloudMeanX, CloudMeanY, CloudMeanZ, CloudComponent, CloudWhiten,
  SphereTheta, SpherePhi, SphereMode, SphereSecond, SphereMix, SphereHeat,
  QuadLambdaX, QuadLambdaY, QuadLambdaZ, QuadYaw, QuadPitch, QuadX, QuadY, QuadZ,
  RootN, RootIndex, RootMultiplier, RootPower, RootAutomorphism,
  PsdA, PsdB, PsdC, PsdProbeAngle, PsdMix, PsdOtherAngle, PsdRayScale, PsdSlice, PsdCostAngle, PsdObjective,
  NormP, NormInfinity, NormX, NormY, NormZ, NormOtherX, NormOtherY, NormOtherZ, NormSupport, NormWire,
  CurveControl, CurveP0X, CurveP0Y, CurveP0Z, CurveP1X, CurveP1Y, CurveP1Z,
  CurveP2X, CurveP2Y, CurveP2Z, CurveP3X, CurveP3Y, CurveP3Z,
  CurveProgress, CurveTravel, CurveProfile, CurveRadius, CurveAspect,
  CurveEndScale, CurveTwist, CurveNormP, CurveGuides,
  LatheControl, LatheR0, LatheR1, LatheR2, LatheR3, LatheR4, LatheR5, LatheR6,
  LatheH1, LatheH2, LatheH3, LatheH4, LatheH5, LatheHeight, LatheHollow,
  LatheWall, LatheFloor, LatheTurn, LatheCut, LatheProbe, LatheSlices, LatheMethod, LatheGuides,
  BooleanShapeA, BooleanShapeB, BooleanSizeA, BooleanSizeB, BooleanX, BooleanY, BooleanZ,
  BooleanYaw, BooleanPitch, BooleanOperation, BooleanBlend, BooleanProbeX, BooleanProbeY, BooleanProbeZ,
  BooleanResolution, BooleanGuides, BooleanSection, BooleanFit, BooleanClearance,
  PatchControl,
  PatchP00X, PatchP00Y, PatchP00Z, PatchP01X, PatchP01Y, PatchP01Z, PatchP02X, PatchP02Y, PatchP02Z, PatchP03X, PatchP03Y, PatchP03Z,
  PatchP10X, PatchP10Y, PatchP10Z, PatchP11X, PatchP11Y, PatchP11Z, PatchP12X, PatchP12Y, PatchP12Z, PatchP13X, PatchP13Y, PatchP13Z,
  PatchP20X, PatchP20Y, PatchP20Z, PatchP21X, PatchP21Y, PatchP21Z, PatchP22X, PatchP22Y, PatchP22Z, PatchP23X, PatchP23Y, PatchP23Z,
  PatchP30X, PatchP30Y, PatchP30Z, PatchP31X, PatchP31Y, PatchP31Z, PatchP32X, PatchP32Y, PatchP32Z, PatchP33X, PatchP33Y, PatchP33Z,
  PatchU, PatchV, PatchResolution, PatchGuides,
  MembraneSlot,
  MembraneM0, MembraneN0, MembraneA0, MembraneV0,
  MembraneM1, MembraneN1, MembraneA1, MembraneV1,
  MembraneM2, MembraneN2, MembraneA2, MembraneV2,
  MembraneM3, MembraneN3, MembraneA3, MembraneV3,
  MembraneWidth, MembraneDepth, MembraneTension, MembraneDensity, MembraneDamping,
  MembraneTime, MembraneU, MembraneV, MembraneResolution, MembraneGuides, MembraneView,
  RigidShape, RigidWidth, RigidHeight, RigidDepth, RigidMass, RigidBalance, RigidRotX, RigidRotY, RigidRotZ, RigidSpinX, RigidSpinY, RigidSpinZ, RigidTime, RigidAxis, RigidGuides,
  TrussShape, TrussSpan, TrussHeight, TrussLean, TrussJoint, TrussP0X, TrussP0Y, TrussP1X, TrussP1Y, TrussP2X, TrussP2Y, TrussP3X, TrussP3Y, TrussP4X, TrussP4Y, TrussP5X, TrussP5Y, TrussPosition, TrussLoadX, TrussLoadY, TrussMember, TrussBrace, TrussSupports, TrussTensionLimit, TrussCompressionLimit, TrussGuides,
  SimplexP0, SimplexP1, SimplexQ0, SimplexQ1, SimplexValue0, SimplexValue1, SimplexValue2, SimplexFunction, SimplexMix, SimplexGuides,
  DistanceAB, DistanceAC, DistanceAD, DistanceBC, DistanceBD, DistanceCD, DistanceEdge, DistanceMirror, DistanceSecond, DistanceMix, DistanceScale, DistanceGuides,
  PolarA00, PolarA01, PolarA02, PolarA10, PolarA11, PolarA12, PolarA20, PolarA21, PolarA22, PolarShape, PolarAmount, PolarIteration, PolarExtension, PolarGuides,
  QrA0X, QrA0Y, QrA0Z, QrA1X, QrA1Y, QrA1Z, QrBX, QrBY, QrBZ, QrVector, QrStage, QrC0, QrC1, QrUseSolution, QrNull0, QrNull1, QrGuides,
  MapsA, MapsB, MapsC, MapsSource, MapsMiddle, MapsFiber, MapsF0, MapsF1, MapsF2, MapsF3, MapsG0, MapsG1, MapsG2, MapsG3, MapsH0, MapsH1, MapsH2, MapsH3, MapsW0, MapsW1, MapsW2, MapsW3, MapsGroup, MapsTime, MapsCondition,
  GraphNode, GraphTarget, GraphTime, GraphProperty, GraphGroup, GraphMatchTime, GraphE00, GraphE01, GraphE02, GraphE03, GraphE04, GraphE05, GraphE10, GraphE11, GraphE12, GraphE13, GraphE14, GraphE15, GraphE20, GraphE21, GraphE22, GraphE23, GraphE24, GraphE25, GraphE30, GraphE31, GraphE32, GraphE33, GraphE34, GraphE35, GraphE40, GraphE41, GraphE42, GraphE43, GraphE44, GraphE45, GraphE50, GraphE51, GraphE52, GraphE53, GraphE54, GraphE55, FaSize, FaA, FaB, FaC, FaLaw, FaWitness, FaGroup, FaSide, FaTime, FaRingOperation, FaE00, FaE01, FaE02, FaE03, FaE04, FaE05, FaE10, FaE11, FaE12, FaE13, FaE14, FaE15, FaE20, FaE21, FaE22, FaE23, FaE24, FaE25, FaE30, FaE31, FaE32, FaE33, FaE34, FaE35, FaE40, FaE41, FaE42, FaE43, FaE44, FaE45, FaE50, FaE51, FaE52, FaE53, FaE54, FaE55, QuSource, QuTarget, QuRing, QuA, QuB, QuMap0, QuMap1, QuMap2, QuMap3, QuMap4, QuMap5, QuMember0, QuMember1, QuMember2, QuMember3, QuMember4, QuMember5, QuCollapse, QuWitness, QuRepA, QuRepB, QuRingOperation, Count
};
enum class MathActionKind : std::uint8_t { Select, SetParameter, Reset, VisitVertex, UndoRoute, ResetRoute, Check, SetLevel, SwapBounds, DescentStep, MatrixPreset, MoveSurfacePoint, SymmetryTurn, SymmetryUndo, SymmetryIdentity, TogglePlayback, AdvanceTime, ModularStep, ResetModularWalk, ReverseFieldPath, ProbabilityStep, ResetProbabilityWalk, BernoulliStep, ResetBernoulli, ObjectPreset, ResetParameters };
enum class MathShape : std::uint8_t { Box, Rod, Disk, Sphere, Ring, Cone, Count };
enum class MathFeedback : std::uint8_t { None, TryAgain, Solved };

enum class MathControlGroup : unsigned { Shape, ShapeA, ShapeB, Profile, Transform, Operation, Probe, Animation, Sampling, Display, Advanced, Count };
// Static asset wiring. Layer masks use bit 0 through bit 3; numerical
// availability remains with the model. Every parameter declares a valid group.
struct MathControlBinding {
  MathControlGroup group=MathControlGroup::Count;
  std::string_view label{}, rowLabel{};
  unsigned rowCount=1;
  std::array<std::string_view,3> components{"X","Y","Z"};
  MathParameter selector=MathParameter::Count;
  unsigned selectedValue=0, layers=15, playbackLayers=0;
};
struct MathParameterSpec {
  MathParameter id;
  MathObjectKind owner;
  std::string_view key, label;
  double minimum, maximum, step, initial;
  unsigned minimumLevel = 0;
  bool matrixEntry = false;
  std::string_view choices{}; // Null-separated labels for a discrete control.
  MathControlBinding control{};
};
struct MathObjectSpec {
  MathObjectKind id;
  std::string_view key, name, title, relationship, challenge, convention;
  std::array<std::string_view,3> progression;
  iggy3d::Vec3 cameraDirection;
  double playbackRate = 1; // Parameter units per second.
  std::string_view playbackRestart = "Restart time", playbackEnd = "Time window complete. Restart or scrub time to explore again.";
};
[[nodiscard]] std::span<const MathObjectSpec> mathObjectSpecs();
[[nodiscard]] std::span<const MathParameterSpec> mathParameterSpecs();
struct MathLesson { std::string_view name, relationship, challenge, explanation; };
[[nodiscard]] std::span<const MathLesson> mathLessons(MathObjectKind);
[[nodiscard]] std::span<const std::string_view> mathMatrixPresetNames();
struct MathObjectPreset {
  std::string_view name;
  static constexpr unsigned kCapacity = 64;
  std::array<MathParameter,kCapacity> parameters;
  std::array<double,kCapacity> values;
  unsigned count = 3;
};
[[nodiscard]] std::span<const MathObjectPreset> mathObjectPresets(MathObjectKind, unsigned level);

struct MathAction {
  MathActionKind kind;
  MathObjectKind object = MathObjectKind::Algebra;
  MathParameter parameter = MathParameter::X;
  double value = 0;
  unsigned vertex = 0;
  double secondary = 0;
  std::bitset<static_cast<unsigned>(MathParameter::Count)> resetParameters{};
};
struct MathActionResult { bool accepted = false; std::string_view reason; };

// Unit primitive plus an affine placement. Mathematical construction is owned
// here; a renderer tessellates these descriptions without recomputing policy.
struct MathPart {
  std::uint32_t id = 0;
  MathShape shape = MathShape::Box;
  iggy3d::Vec3 center{}, x{1,0,0}, y{0,1,0}, z{0,0,1}, color{};
  std::string_view role;
};
struct MathLabel { std::string_view text; iggy3d::Vec3 position; iggy3d::Vec3 color; };
struct MathMetric { std::string_view label, suffix; double value = 0; };
struct MathPlotPoint { double x = 0, y = 0; };
struct MathPlotSeries {
  std::string_view name;
  iggy3d::Vec3 color{};
  std::array<MathPlotPoint,129> points{};
  std::size_t count = 0;
  bool signedFill = false;
  bool stems = false;
};
struct MathPlot {
  std::string_view title;
  std::array<MathPlotSeries,3> series{};
  std::size_t seriesCount = 0;
  MathPlotPoint marker{};
  bool hasMarker = false, equalAspect = false;
  MathParameter scrubParameter = MathParameter::Count;
};
struct MathMatrixView {
  std::string_view name;
  unsigned rows = 3, columns = 3;
  std::array<double,9> values{};
  std::array<MathParameter,9> parameters{};
  bool editable = false;
};
struct MathSurfaceVertex { iggy3d::Vec3 position{}, normal{}, color{}; };
struct MathTriangleSurface {
  std::array<MathSurfaceVertex,4096> vertices{};
  std::array<std::uint16_t,24576> indices{};
  unsigned vertexCount=0,indexCount=0;
};
struct MathSurfacePatch {
  static constexpr unsigned kResolution = 21;
  std::array<MathSurfaceVertex,kResolution*kResolution> vertices{};
  unsigned rows = 0, columns = 0;
};
struct MathContourSegment { MathPlotPoint a{}, b{}; double height = 0; };
struct MathContourMap {
  std::array<MathContourSegment,2048> segments{};
  std::size_t count = 0;
  MathPlotPoint point{}, gradient{};
  bool active = false, constrained = false;
};
struct MathSymmetryView {
  bool active = false;
  std::array<unsigned,8> permutation{}; // Original labelled vertex -> destination slot.
  std::array<bool,8> orbit{};
  std::array<unsigned,64> moves{}; // World-axis turns: X, Y, Z, X inverse, Y inverse, Z inverse.
  std::size_t moveCount = 0;
};
struct MathValueTable {
  std::string_view title;
  std::array<std::string_view,4> columns{};
  std::array<std::string_view,32> rowLabels{};
  std::array<std::array<double,4>,32> values{};
  std::size_t rowCount = 0, columnCount = 0;
};
struct MathCurveView {
  bool active = false;
  std::array<iggy3d::Vec3,16> controls{};
  unsigned selected = 0, count = 4;
  MathParameter selectionParameter = MathParameter::CurveControl;
};
struct MathObjectSnapshot {
  static constexpr std::size_t kPartCapacity = 192, kRouteCapacity = 64;
  MathObjectKind kind = MathObjectKind::Algebra;
  unsigned level = 0;
  std::uint64_t revision = 1;
  std::array<MathPart,kPartCapacity> parts{};
  std::size_t partCount = 0;
  std::array<MathLabel,32> labels{};
  std::size_t labelCount = 0;
  std::array<MathMetric,12> metrics{};
  std::size_t metricCount = 0;
  std::array<MathPlot,3> plots{};
  std::size_t plotCount = 0;
  std::array<MathMatrixView,3> matrices{};
  std::size_t matrixCount = 0;
  MathSurfacePatch surface;
  MathTriangleSurface solid;
  MathContourMap contours;
  MathSymmetryView symmetry;
  MathCurveView curve;
  MathValueTable table;
  unsigned modularWalkSteps = 0, modularVisitedMask = 0;
  bool fieldPathReversed = false;
  std::array<unsigned,65> probabilityWalk{};
  std::size_t probabilityWalkCount = 1;
  std::array<unsigned,13> bernoulliPath{};
  unsigned bernoulliSteps = 0;
  bool playing = false;
  std::array<unsigned,kRouteCapacity> route{};
  std::size_t routeCount = 1;
  std::array<bool,8> allowedVertices{};
  unsigned shortestHops = 3;
  MathFeedback feedback = MathFeedback::None;
  std::string_view feedbackText;
};

// One semantic action owner, independent of UI, SDL, Vulkan and persistence.
// Validation is atomic. Snapshots use fixed capacities; graph/group closure and
// numerical motion/quadrature have bounded loops. No per-action heap use.
class MathObjects {
public:
  MathObjects();
  [[nodiscard]] MathActionResult dispatch(const MathAction&);
  [[nodiscard]] double parameter(MathParameter p) const;
  [[nodiscard]] bool parameterAvailable(MathParameter p) const;
  [[nodiscard]] double parameterMaximum(MathParameter p) const;
  [[nodiscard]] MathParameter playbackParameter() const;
  [[nodiscard]] const MathObjectSnapshot& snapshot() const { return snapshot_; }
private:
  void rebuild();
  void check();
  std::array<double,static_cast<std::size_t>(MathParameter::Count)> parameters_{};
  MathObjectSnapshot snapshot_;
  RigidMotion rigidMotion_;
  bool reversedNegativeIntegral_ = false;
  std::array<unsigned,64> symmetryMoves_{};
  std::size_t symmetryMoveCount_ = 0;
  unsigned modularWalkSteps_ = 0, modularVisitedMask_ = 1;
  bool fieldPathReversed_ = false;
  std::array<unsigned,65> probabilityWalk_{};
  std::size_t probabilityWalkCount_ = 1;
  std::uint32_t probabilityRng_ = 7;
  std::array<unsigned,13> bernoulliPath_{};
  unsigned bernoulliSteps_ = 0;
  std::uint32_t bernoulliRng_ = 11;
};

} // namespace paths
