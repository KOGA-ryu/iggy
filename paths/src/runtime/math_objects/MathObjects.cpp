#include "runtime/math_objects/MathObjects.hpp"
#include "runtime/math_objects/BezierCurve.hpp"
#include "runtime/math_objects/LatheProfile.hpp"
#include "runtime/math_objects/LatheGeometry.hpp"
#include "runtime/math_objects/BooleanSolid.hpp"
#include "runtime/math_objects/BezierPatch.hpp"
#include "runtime/math_objects/PatchGeometry.hpp"
#include "runtime/math_objects/Membrane.hpp"
#include "runtime/math_objects/MembraneGeometry.hpp"
#include "runtime/math_objects/BooleanGeometry.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace paths {
using namespace iggy3d;
namespace {
constexpr double pi = 3.14159265358979323846;
constexpr Vec3 teal{.19F,.72F,.65F}, blue{.34F,.56F,.91F}, coral{.96F,.57F,.39F},
  gold{.97F,.79F,.35F}, violet{.68F,.52F,.89F}, muted{.43F,.52F,.61F}, white{.85F,.91F,.94F};
constexpr std::array<MathParameterSpec,static_cast<std::size_t>(MathParameter::Count)> parameters{{
  {MathParameter::X,MathObjectKind::Algebra,"x","Variable x",.5,2.5,.1,1.6},
  {MathParameter::Gap,MathObjectKind::Algebra,"gap","Separate pieces",0,.6,.02,.24},
  {MathParameter::Angle,MathObjectKind::Trig,"angle","Angle (degrees)",0,360,1,45},
  {MathParameter::Slices,MathObjectKind::Calculus,"slices","Number of slices",3,64,1,8},
  {MathParameter::SliceGap,MathObjectKind::Calculus,"slice_gap","Separate slices",0,.12,.01,0},
  {MathParameter::Sample,MathObjectKind::Calculus,"sample","Sampling",0,2,1,1,0,false,"Left sample\0Midpoint sample\0Right sample\0"},
  {MathParameter::Shear,MathObjectKind::Linear,"shear","A[0,1] / shear k",-1.2,1.2,.1,.6,0,true},
  {MathParameter::Scale,MathObjectKind::Linear,"scale","A[1,1] / vertical scale",-2,2,.1,1,0,true},
  {MathParameter::Depth,MathObjectKind::Discrete,"depth","Layer spacing",.3,1.5,.1,1},
  {MathParameter::Shortcut,MathObjectKind::Discrete,"shortcut","Add A-H shortcut",0,1,1,0},
  {MathParameter::FunctionRule,MathObjectKind::Function,"function","Function",0,3,1,0,0,false,"x^2\0x^3 - 3x\0sin(x)\0exp(x) - 1\0"},
  {MathParameter::FunctionX,MathObjectKind::Function,"at","Input x",-2,2,.005,.75},
  {MathParameter::DeltaX,MathObjectKind::Function,"h","Secant step h",-1,1,.005,.5,1},
  {MathParameter::IntegralStart,MathObjectKind::Function,"from","Integral start a",-2,2,.005,0,2},
  {MathParameter::TaylorCenter,MathObjectKind::Function,"center","Taylor centre c",-1,1,.05,0,3},
  {MathParameter::TaylorDegree,MathObjectKind::Function,"degree","Taylor degree",0,5,1,1,3},
  {MathParameter::A00,MathObjectKind::Linear,"a00","A[0,0]",-2,2,.1,1,1,true},
  {MathParameter::A02,MathObjectKind::Linear,"a02","A[0,2]",-2,2,.1,0,1,true},
  {MathParameter::A10,MathObjectKind::Linear,"a10","A[1,0]",-2,2,.1,0,1,true},
  {MathParameter::A12,MathObjectKind::Linear,"a12","A[1,2]",-2,2,.1,0,1,true},
  {MathParameter::A20,MathObjectKind::Linear,"a20","A[2,0]",-2,2,.1,0,1,true},
  {MathParameter::A21,MathObjectKind::Linear,"a21","A[2,1]",-2,2,.1,0,1,true},
  {MathParameter::A22,MathObjectKind::Linear,"a22","A[2,2]",-2,2,.1,1,1,true},
  {MathParameter::VectorX,MathObjectKind::Linear,"vx","Vector x",-2,2,.05,1,1},
  {MathParameter::VectorY,MathObjectKind::Linear,"vy","Vector y",-2,2,.05,1,1},
  {MathParameter::VectorZ,MathObjectKind::Linear,"vz","Vector z",-2,2,.05,1,1},
  {MathParameter::ComposeAngle,MathObjectKind::Linear,"rotate","B: rotation about z (degrees)",-180,180,1,30,1},
  {MathParameter::SvdStage,MathObjectKind::Linear,"svd_stage","SVD stage",0,3,1,3,3,false,"Unit sphere\0Apply V transpose\0Then apply Sigma\0Then apply U\0"},
  {MathParameter::SurfaceRule,MathObjectKind::Surface,"surface","Surface",0,2,1,0,0,false,"Bowl: (u^2 + v^2)/2\0Saddle: (u^2 - v^2)/2\0Wave: sin(u) cos(v)\0"},
  {MathParameter::SurfaceU,MathObjectKind::Surface,"u","Input u",-2,2,.025,.75},
  {MathParameter::SurfaceV,MathObjectKind::Surface,"v","Input v",-2,2,.025,.5},
  {MathParameter::DirectionAngle,MathObjectKind::Surface,"direction","Direction (degrees)",0,360,1,45,1},
  {MathParameter::DescentRate,MathObjectKind::Surface,"rate","Descent step size",.05,1,.05,.25,2},
  {MathParameter::Constraint,MathObjectKind::Surface,"constraint","Constrain to unit circle",0,1,1,0,3,false,"Free point\0Unit circle\0"},
  {MathParameter::CircleAngle,MathObjectKind::Surface,"circle_angle","Position on circle (degrees)",0,360,1,45,3},
  {MathParameter::SymmetryFirst,MathObjectKind::Symmetry,"first_turn","First turn",0,5,1,0,1,false,"X +90\0Y +90\0Z +90\0X -90\0Y -90\0Z -90\0"},
  {MathParameter::SymmetrySecond,MathObjectKind::Symmetry,"second_turn","Second turn",0,5,1,1,1,false,"X +90\0Y +90\0Z +90\0X -90\0Y -90\0Z -90\0"},
  {MathParameter::SymmetryGenerator,MathObjectKind::Symmetry,"generator","Cyclic generator",0,2,1,0,2,false,"X quarter turn\0X half turn\0Body diagonal: (x,y,z) to (z,x,y)\0"},
  {MathParameter::SymmetryPower,MathObjectKind::Symmetry,"power","Generator power",0,12,1,1,2},
  {MathParameter::SymmetryElement,MathObjectKind::Symmetry,"rotation","Cube rotation",0,23,1,0,3},
  {MathParameter::SymmetryVertex,MathObjectKind::Symmetry,"probe_vertex","Track labelled vertex",0,7,1,0,3,false,"A\0B\0C\0D\0E\0F\0G\0H\0"},
  {MathParameter::HarmonicTime,MathObjectKind::Harmonics,"phase_time","Time t (radians at unit frequency)",0,2*pi,.005,.75},
  {MathParameter::Amplitude1,MathObjectKind::Harmonics,"amplitude1","First amplitude",0,1.5,.05,1},
  {MathParameter::Frequency1,MathObjectKind::Harmonics,"frequency1","First frequency",1,5,1,1},
  {MathParameter::Phase1,MathObjectKind::Harmonics,"phase1","First phase (degrees)",-180,180,1,0},
  {MathParameter::Amplitude2,MathObjectKind::Harmonics,"amplitude2","Second amplitude",0,1.5,.05,1,1},
  {MathParameter::Frequency2,MathObjectKind::Harmonics,"frequency2","Second frequency",1,5,1,2,1},
  {MathParameter::Phase2,MathObjectKind::Harmonics,"phase2","Second phase (degrees)",-180,180,1,0,1},
  {MathParameter::Waveform,MathObjectKind::Harmonics,"waveform","Target waveform",0,2,1,0,2,false,"Square wave\0Sawtooth\0Triangle wave\0"},
  {MathParameter::HarmonicTerms,MathObjectKind::Harmonics,"terms","Nonzero Fourier terms",1,8,1,1,2},
  {MathParameter::ProbeFrequency,MathObjectKind::Harmonics,"probe_frequency","Sine basis frequency",1,16,1,1,3},
  {MathParameter::MotionSystem,MathObjectKind::Oscillator,"system","System",0,1,1,0,0,false,"Unit-mass spring\0Unit-inertia pendulum\0"},
  {MathParameter::InitialPosition,MathObjectKind::Oscillator,"initial_position","Initial displacement / angle (rad)",-1.2,1.2,.05,.7},
  {MathParameter::InitialVelocity,MathObjectKind::Oscillator,"initial_velocity","Initial velocity / angular velocity",-2,2,.05,0},
  {MathParameter::Stiffness,MathObjectKind::Oscillator,"stiffness","Restoring coefficient k = omega0^2",.25,4,.25,1},
  {MathParameter::MotionTime,MathObjectKind::Oscillator,"time","Time (seconds)",0,12,.025,0},
  {MathParameter::Damping,MathObjectKind::Oscillator,"damping","Damping coefficient c",0,3,.05,.5,2},
  {MathParameter::DriveAmplitude,MathObjectKind::Oscillator,"drive","Driving force / torque amplitude",0,1,.05,.5,3},
  {MathParameter::DriveFrequency,MathObjectKind::Oscillator,"drive_frequency","Drive angular frequency",.5,3,.05,1,3},
  {MathParameter::Modulus,MathObjectKind::Modular,"modulus","First modulus n",2,12,1,8},
  {MathParameter::ModValue,MathObjectKind::Modular,"integer","Integer / first residue",-48,48,1,-3},
  {MathParameter::ModStep,MathObjectKind::Modular,"step","Step / multiplier k",0,12,1,2,1},
  {MathParameter::InverseGuess,MathObjectKind::Modular,"inverse","Your inverse",0,11,1,0,2},
  {MathParameter::SecondModulus,MathObjectKind::Modular,"modulus2","Second modulus m",2,12,1,5,3},
  {MathParameter::SecondResidue,MathObjectKind::Modular,"residue2","Second residue",-12,12,1,2,3},
  {MathParameter::CrtGuess,MathObjectKind::Modular,"crt","Your simultaneous solution",0,143,1,0,3},
  {MathParameter::GaussianReal,MathObjectKind::Gaussian,"real","z: real part",-3,3,1,1},
  {MathParameter::GaussianImag,MathObjectKind::Gaussian,"imag","z: imaginary part",-3,3,1,1},
  {MathParameter::GaussianOtherReal,MathObjectKind::Gaussian,"other_real","w: real part",-2,2,1,1},
  {MathParameter::GaussianOtherImag,MathObjectKind::Gaussian,"other_imag","w: imaginary part",-2,2,1,1},
  {MathParameter::GaussianOperation,MathObjectKind::Gaussian,"operation","Operation",0,1,1,1,0,false,"Add\0Multiply\0"},
  {MathParameter::GaussianHeight,MathObjectKind::Gaussian,"norm_height","Height view",0,1,1,0,0,false,"Complex plane\0Height = 0.12 * squared magnitude\0"},
  {MathParameter::GaussianQuotient,MathObjectKind::Gaussian,"quotient","Quotient ring",0,2,1,1,3,false,"Z[i] / (2): four classes\0Z[i] / (2+i): five classes\0Z[i] / (3): nine classes\0"},
  {MathParameter::FieldRule,MathObjectKind::VectorField,"field","Vector field",0,2,1,1,0,false,"Constant: (1, 0.5, -0.25)\0Radial: (x,y,z)\0Vortex: (-y,x,0)\0"},
  {MathParameter::FieldX,MathObjectKind::VectorField,"field_x","Probe x",-1.5,1.5,.05,.75},
  {MathParameter::FieldY,MathObjectKind::VectorField,"field_y","Probe y",-1.5,1.5,.05,.5},
  {MathParameter::FieldZ,MathObjectKind::VectorField,"field_z","Probe z",-1.5,1.5,.05,.25},
  {MathParameter::FieldYaw,MathObjectKind::VectorField,"direction_yaw","Direction yaw (degrees)",0,360,1,45},
  {MathParameter::FieldPitch,MathObjectKind::VectorField,"direction_pitch","Direction pitch (degrees)",-90,90,1,0},
  {MathParameter::FieldPath,MathObjectKind::VectorField,"path","Probe path",0,4,1,0,1,false,"Straight segment\0Upper semicircle\0Lower semicircle\0Nonplanar arch\0Closed circle\0"},
  {MathParameter::FieldTime,MathObjectKind::VectorField,"path_time","Path parameter t",0,1,.005,.3,1},
  {MathParameter::FluxShape,MathObjectKind::Flux,"flux_shape","Surface",0,2,1,0,0,false,"Sphere\0Box\0Open disk\0"},
  {MathParameter::FluxField,MathObjectKind::Flux,"flux_field","Field",0,3,1,1,0,false,"Constant: (0,0,1)\0Radial: (x,y,z)\0Vortex: (-y,x,0)\0Twist: (-y,x,z)\0"},
  {MathParameter::FluxRadius,MathObjectKind::Flux,"flux_radius","Radius / box half-width",0.5,1.5,0.05,1,0},
  {MathParameter::FluxTilt,MathObjectKind::Flux,"flux_tilt","Tilt about x (degrees)",0,180,1,0,0},
  {MathParameter::FluxOrientation,MathObjectKind::Flux,"flux_orientation","Orientation",0,1,1,0,0,false,"Outward / positive normal\0Inward / reversed normal\0"},
  {MathParameter::FluxProbe,MathObjectKind::Flux,"flux_probe","Surface probe",0,1,0.005,0.5,0},
  {MathParameter::FluxResolution,MathObjectKind::Flux,"flux_resolution","Integration resolution",2,12,1,4,1},
  {MathParameter::FluxTime,MathObjectKind::Flux,"boundary_time","Boundary parameter t",0,1,0.005,0.2,3},
  {MathParameter::TensorU0,MathObjectKind::Tensor,"tensor_u0","u: 1 component",-2,2,0.1,1,0},
  {MathParameter::TensorU1,MathObjectKind::Tensor,"tensor_u1","u: 2 component",-2,2,0.1,1,0},
  {MathParameter::TensorU2,MathObjectKind::Tensor,"tensor_u2","u: 3 component",-2,2,0.1,0,0},
  {MathParameter::TensorV0,MathObjectKind::Tensor,"tensor_v0","v: 1 component",-2,2,0.1,1,0},
  {MathParameter::TensorV1,MathObjectKind::Tensor,"tensor_v1","v: 2 component",-2,2,0.1,-1,0},
  {MathParameter::TensorV2,MathObjectKind::Tensor,"tensor_v2","v: 3 component",-2,2,0.1,1,0},
  {MathParameter::TensorW0,MathObjectKind::Tensor,"tensor_w0","w: 1 component",-2,2,0.1,1,1},
  {MathParameter::TensorW1,MathObjectKind::Tensor,"tensor_w1","w: 2 component",-2,2,0.1,0.5,1},
  {MathParameter::TensorW2,MathObjectKind::Tensor,"tensor_w2","w: 3 component",-2,2,0.1,-1,1},
  {MathParameter::TensorI,MathObjectKind::Tensor,"tensor_i","Inspect index i",0,2,1,0,0,false,"Index 1\0Index 2\0Index 3\0"},
  {MathParameter::TensorJ,MathObjectKind::Tensor,"tensor_j","Inspect index j",0,2,1,0,0,false,"Index 1\0Index 2\0Index 3\0"},
  {MathParameter::TensorK,MathObjectKind::Tensor,"tensor_k","Inspect index k",0,2,1,0,1,false,"Index 1\0Index 2\0Index 3\0"},
  {MathParameter::TensorGap,MathObjectKind::Tensor,"tensor_gap","Separate tensor slices",0,0.6,0.05,0.25,0},
  {MathParameter::TensorBasis,MathObjectKind::Tensor,"basis_angle","Rotate coordinate basis (degrees)",-180,180,1,45,3},
  {MathParameter::ProbabilityRule,MathObjectKind::Probability,"chain","Transition rule",0,3,1,0,0,false,"Lazy directed cycle\0Weighted mixing\0Periodic cycle\0Absorbing chain\0"},
  {MathParameter::ProbabilityStay,MathObjectKind::Probability,"stay","Self-loop / retention probability",0,1,0.05,0.5,0},
  {MathParameter::ProbabilityStart,MathObjectKind::Probability,"start_state","Initial state",0,2,1,0,0,false,"A\0B\0C\0"},
  {MathParameter::ProbabilityMix,MathObjectKind::Probability,"initial_mix","Mix initial state with uniform",0,1,0.05,0,2},
  {MathParameter::ProbabilitySeed,MathObjectKind::Probability,"walk_seed","Reproducible walk seed",0,65535,1,7,0},
  {MathParameter::ProbabilityRow,MathObjectKind::Probability,"transition_row","Inspect outgoing row",0,2,1,0,1,false,"A\0B\0C\0"},
  {MathParameter::ProbabilitySteps,MathObjectKind::Probability,"chain_steps","Number of transitions",0,64,1,0,2},
  {MathParameter::BinomialTrials,MathObjectKind::Binomial,"trials","Number of independent trials n",1,12,1,6,0},
  {MathParameter::BinomialChance,MathObjectKind::Binomial,"success_chance","Success probability p",0,1,0.05,0.5,0},
  {MathParameter::BinomialCut,MathObjectKind::Binomial,"count_cut","Count threshold k",0,12,1,3,1},
  {MathParameter::BinomialSeed,MathObjectKind::Binomial,"trial_seed","Reproducible trial seed",0,65535,1,11,0},
  {MathParameter::BayesPrior,MathObjectKind::Bayes,"prior_h","Prior P(H)",0,1,0.05,0.3,0},
  {MathParameter::BayesHit,MathObjectKind::Bayes,"e_given_h","P(E | H)",0,1,0.05,0.8,0},
  {MathParameter::BayesFalse,MathObjectKind::Bayes,"e_given_other","P(E | not H)",0,1,0.05,0.2,0},
  {MathParameter::BayesEvent,MathObjectKind::Bayes,"evidence","Condition on",0,1,1,0,1,false,"Event E\0Complement of E\0"},
  {MathParameter::BayesPositive,MathObjectKind::Bayes,"positive_evidence","Observed E outcomes",0,8,1,3,3},
  {MathParameter::BayesNegative,MathObjectKind::Bayes,"negative_evidence","Observed not-E outcomes",0,8,1,0,3},
  {MathParameter::CloudX,MathObjectKind::Covariance,"cloud_x","x stretch",0,2,0.1,1.4,0},
  {MathParameter::CloudY,MathObjectKind::Covariance,"cloud_y","y stretch",0,2,0.1,0.8,0},
  {MathParameter::CloudZ,MathObjectKind::Covariance,"cloud_z","z stretch",0,2,0.1,0.4,0},
  {MathParameter::CloudYaw,MathObjectKind::Covariance,"cloud_yaw","Rotate about z (degrees)",-180,180,1,30,0},
  {MathParameter::CloudPitch,MathObjectKind::Covariance,"cloud_pitch","Rotate about y (degrees)",-90,90,1,20,0},
  {MathParameter::CloudShear,MathObjectKind::Covariance,"cloud_shear","x from y shear",-1,1,0.1,0.5,1},
  {MathParameter::CloudMeanX,MathObjectKind::Covariance,"mean_x","Mean x",-1,1,0.1,0,0},
  {MathParameter::CloudMeanY,MathObjectKind::Covariance,"mean_y","Mean y",-1,1,0.1,0,0},
  {MathParameter::CloudMeanZ,MathObjectKind::Covariance,"mean_z","Mean z",-1,1,0.1,0,0},
  {MathParameter::CloudComponent,MathObjectKind::Covariance,"principal_axis","Project onto principal axis",0,2,1,0,2,false,"Largest variance\0Middle variance\0Smallest variance\0"},
  {MathParameter::CloudWhiten,MathObjectKind::Covariance,"whiten","Move towards whitened coordinates",0,1,0.05,1,3},
  {MathParameter::SphereTheta,MathObjectKind::Spherical,"polar","Polar angle theta (degrees)",0,180,1,60,0},
  {MathParameter::SpherePhi,MathObjectKind::Spherical,"azimuth","Azimuth phi (degrees)",0,360,1,45,0},
  {MathParameter::SphereMode,MathObjectKind::Spherical,"sphere_mode","First real harmonic",0,24,1,6,1,false,"l=0, m=+0\0l=1, m=-1\0l=1, m=+0\0l=1, m=+1\0l=2, m=-2\0l=2, m=-1\0l=2, m=+0\0l=2, m=+1\0l=2, m=+2\0l=3, m=-3\0l=3, m=-2\0l=3, m=-1\0l=3, m=+0\0l=3, m=+1\0l=3, m=+2\0l=3, m=+3\0l=4, m=-4\0l=4, m=-3\0l=4, m=-2\0l=4, m=-1\0l=4, m=+0\0l=4, m=+1\0l=4, m=+2\0l=4, m=+3\0l=4, m=+4\0"},
  {MathParameter::SphereSecond,MathObjectKind::Spherical,"sphere_second","Second real harmonic",0,24,1,3,2,false,"l=0, m=+0\0l=1, m=-1\0l=1, m=+0\0l=1, m=+1\0l=2, m=-2\0l=2, m=-1\0l=2, m=+0\0l=2, m=+1\0l=2, m=+2\0l=3, m=-3\0l=3, m=-2\0l=3, m=-1\0l=3, m=+0\0l=3, m=+1\0l=3, m=+2\0l=3, m=+3\0l=4, m=-4\0l=4, m=-3\0l=4, m=-2\0l=4, m=-1\0l=4, m=+0\0l=4, m=+1\0l=4, m=+2\0l=4, m=+3\0l=4, m=+4\0"},
  {MathParameter::SphereMix,MathObjectKind::Spherical,"sphere_mix","Second coefficient",-1,1,0.05,0.6,2},
  {MathParameter::SphereHeat,MathObjectKind::Spherical,"heat_time","Heat diffusion time",0,1,0.005,0.25,3},
  {MathParameter::QuadLambdaX,MathObjectKind::Quadratic,"lambda_x","Principal value 1",-2,2,0.25,1.5,0},
  {MathParameter::QuadLambdaY,MathObjectKind::Quadratic,"lambda_y","Principal value 2",-2,2,0.25,1,0},
  {MathParameter::QuadLambdaZ,MathObjectKind::Quadratic,"lambda_z","Principal value 3",-2,2,0.25,0.5,0},
  {MathParameter::QuadYaw,MathObjectKind::Quadratic,"form_yaw","Basis rotation about z (degrees)",-180,180,1,30,1},
  {MathParameter::QuadPitch,MathObjectKind::Quadratic,"form_pitch","Basis rotation about y (degrees)",-90,90,1,20,1},
  {MathParameter::QuadX,MathObjectKind::Quadratic,"form_x","Probe x",-2,2,0.05,1,0},
  {MathParameter::QuadY,MathObjectKind::Quadratic,"form_y","Probe y",-2,2,0.05,0.5,0},
  {MathParameter::QuadZ,MathObjectKind::Quadratic,"form_z","Probe z / height-map section",-1.5,1.5,0.05,0,0},
  {MathParameter::RootN,MathObjectKind::Roots,"root_n","Number of roots n",3,12,1,7,0},
  {MathParameter::RootIndex,MathObjectKind::Roots,"root_index","Probe exponent (reduced modulo n)",0,11,1,1,0},
  {MathParameter::RootMultiplier,MathObjectKind::Roots,"root_multiplier","Power-map exponent / subgroup generator",0,12,1,2,1},
  {MathParameter::RootPower,MathObjectKind::Roots,"root_power","Generator power",0,12,1,1,2},
  {MathParameter::RootAutomorphism,MathObjectKind::Roots,"root_auto","Candidate automorphism exponent a",1,11,1,2,3},
  {MathParameter::PsdA,MathObjectKind::Psd,"psd_a","Matrix entry a",-2,2,.05,1},
  {MathParameter::PsdB,MathObjectKind::Psd,"psd_b","Symmetric off-diagonal b",-2,2,.05,.5},
  {MathParameter::PsdC,MathObjectKind::Psd,"psd_c","Matrix entry c",-2,2,.05,1},
  {MathParameter::PsdProbeAngle,MathObjectKind::Psd,"psd_probe","Quadratic probe angle (degrees)",0,180,1,0,1},
  {MathParameter::PsdMix,MathObjectKind::Psd,"psd_mix","Mix from A to B",0,1,.05,.5,2},
  {MathParameter::PsdOtherAngle,MathObjectKind::Psd,"psd_other_angle","Rank-one B direction (degrees)",0,180,1,0,2},
  {MathParameter::PsdRayScale,MathObjectKind::Psd,"psd_ray_scale","Scale the mixture along its ray",0,1.5,.05,1,2},
  {MathParameter::PsdSlice,MathObjectKind::Psd,"psd_slice","Trace / 2: slice height t",0,2,.05,1,3},
  {MathParameter::PsdCostAngle,MathObjectKind::Psd,"psd_cost_angle","Objective direction (degrees)",-180,180,1,30,3},
  {MathParameter::PsdObjective,MathObjectKind::Psd,"psd_objective","Objective plane: fraction of best value",-1.25,1.25,.05,0,3},
  {MathParameter::NormP,MathObjectKind::Norm,"norm_p","Exponent p",1,32,.25,2},
  {MathParameter::NormInfinity,MathObjectKind::Norm,"norm_infinity","Norm family",0,1,1,0,0,false,"Finite p-norm\0Exact infinity norm\0"},
  {MathParameter::NormX,MathObjectKind::Norm,"norm_x","Vector x: first component",-1.5,1.5,.05,.8},
  {MathParameter::NormY,MathObjectKind::Norm,"norm_y","Vector x: second component",-1.5,1.5,.05,.6},
  {MathParameter::NormZ,MathObjectKind::Norm,"norm_z","Vector x: third component",-1.5,1.5,.05,.4},
  {MathParameter::NormOtherX,MathObjectKind::Norm,"norm_other_x","Vector y: first component",-1.5,1.5,.05,-.3,1},
  {MathParameter::NormOtherY,MathObjectKind::Norm,"norm_other_y","Vector y: second component",-1.5,1.5,.05,.6,1},
  {MathParameter::NormOtherZ,MathObjectKind::Norm,"norm_other_z","Vector y: third component",-1.5,1.5,.05,.2,1},
  {MathParameter::NormSupport,MathObjectKind::Norm,"norm_support","Plane: fraction of support value",0,1.4,.05,.7,3},
  {MathParameter::NormWire,MathObjectKind::Norm,"norm_wire","Boundary style",0,1,1,0,0,false,"Solid boundary\0Open cross-sections\0"},
  {MathParameter::CurveControl,MathObjectKind::Curve,"curve_control","Selected control point",0,3,1,1,0,false,"P0: start\0P1: first handle\0P2: second handle\0P3: end\0"},
  {MathParameter::CurveP0X,MathObjectKind::Curve,"curve_p0_x","P0: x coordinate",-2,2,.05,-1.5},
  {MathParameter::CurveP0Y,MathObjectKind::Curve,"curve_p0_y","P0: y coordinate",-2,2,.05,0},
  {MathParameter::CurveP0Z,MathObjectKind::Curve,"curve_p0_z","P0: z coordinate",-2,2,.05,0},
  {MathParameter::CurveP1X,MathObjectKind::Curve,"curve_p1_x","P1: x coordinate",-2,2,.05,-1},
  {MathParameter::CurveP1Y,MathObjectKind::Curve,"curve_p1_y","P1: y coordinate",-2,2,.05,1.3},
  {MathParameter::CurveP1Z,MathObjectKind::Curve,"curve_p1_z","P1: z coordinate",-2,2,.05,0},
  {MathParameter::CurveP2X,MathObjectKind::Curve,"curve_p2_x","P2: x coordinate",-2,2,.05,1},
  {MathParameter::CurveP2Y,MathObjectKind::Curve,"curve_p2_y","P2: y coordinate",-2,2,.05,1.3},
  {MathParameter::CurveP2Z,MathObjectKind::Curve,"curve_p2_z","P2: z coordinate",-2,2,.05,0},
  {MathParameter::CurveP3X,MathObjectKind::Curve,"curve_p3_x","P3: x coordinate",-2,2,.05,1.5},
  {MathParameter::CurveP3Y,MathObjectKind::Curve,"curve_p3_y","P3: y coordinate",-2,2,.05,0},
  {MathParameter::CurveP3Z,MathObjectKind::Curve,"curve_p3_z","P3: z coordinate",-2,2,.05,0},
  {MathParameter::CurveProgress,MathObjectKind::Curve,"curve_progress","Position along the curve",0,1,.005,.35},
  {MathParameter::CurveTravel,MathObjectKind::Curve,"curve_travel","Travel rule",0,1,1,0,0,false,"Equal parameter steps\0Equal distance steps\0"},
  {MathParameter::CurveProfile,MathObjectKind::Curve,"curve_profile","Cross-section",0,2,1,0,0,false,"Circle\0Square\0Norm profile\0"},
  {MathParameter::CurveRadius,MathObjectKind::Curve,"curve_radius","Starting radius / half width",.05,.45,.01,.2},
  {MathParameter::CurveAspect,MathObjectKind::Curve,"curve_aspect","Cross-section height / width",.05,1,.05,1},
  {MathParameter::CurveEndScale,MathObjectKind::Curve,"curve_end_scale","End radius / start radius",0,1,.05,1},
  {MathParameter::CurveTwist,MathObjectKind::Curve,"curve_twist","Total twist (degrees)",-360,360,5,0},
  {MathParameter::CurveNormP,MathObjectKind::Curve,"curve_norm_p","Norm profile exponent p",1,16,.25,2},
  {MathParameter::CurveGuides,MathObjectKind::Curve,"curve_guides","Construction guides",0,1,1,1,0,false,"Shape only\0Show construction\0"},
  {MathParameter::LatheControl,MathObjectKind::Lathe,"lathe_control","Selected profile point",0,6,1,3,0,false,"P0: base\0P1\0P2\0P3\0P4\0P5\0P6: top\0"},
  {MathParameter::LatheR0,MathObjectKind::Lathe,"lathe_r0","P0: radius",0,1.5,.01,0.65},
  {MathParameter::LatheR1,MathObjectKind::Lathe,"lathe_r1","P1: radius",0,1.5,.01,0.9},
  {MathParameter::LatheR2,MathObjectKind::Lathe,"lathe_r2","P2: radius",0,1.5,.01,1.15},
  {MathParameter::LatheR3,MathObjectKind::Lathe,"lathe_r3","P3: radius",0,1.5,.01,1},
  {MathParameter::LatheR4,MathObjectKind::Lathe,"lathe_r4","P4: radius",0,1.5,.01,0.55},
  {MathParameter::LatheR5,MathObjectKind::Lathe,"lathe_r5","P5: radius",0,1.5,.01,0.48},
  {MathParameter::LatheR6,MathObjectKind::Lathe,"lathe_r6","P6: radius",0,1.5,.01,0.62},
  {MathParameter::LatheH1,MathObjectKind::Lathe,"lathe_h1","P1: height fraction",.04,.96,.01,0.12},
  {MathParameter::LatheH2,MathObjectKind::Lathe,"lathe_h2","P2: height fraction",.04,.96,.01,0.35},
  {MathParameter::LatheH3,MathObjectKind::Lathe,"lathe_h3","P3: height fraction",.04,.96,.01,0.6},
  {MathParameter::LatheH4,MathObjectKind::Lathe,"lathe_h4","P4: height fraction",.04,.96,.01,0.8},
  {MathParameter::LatheH5,MathObjectKind::Lathe,"lathe_h5","P5: height fraction",.04,.96,.01,0.92},
  {MathParameter::LatheHeight,MathObjectKind::Lathe,"lathe_height","Total height",1,4,.05,3},
  {MathParameter::LatheHollow,MathObjectKind::Lathe,"lathe_hollow","Interior",0,1,1,1,0,false,"Solid\0Hollow\0"},
  {MathParameter::LatheWall,MathObjectKind::Lathe,"lathe_wall","Radial wall thickness",.03,.35,.01,.12},
  {MathParameter::LatheFloor,MathObjectKind::Lathe,"lathe_floor","Cavity floor: height fraction",.04,.8,.01,.08},
  {MathParameter::LatheTurn,MathObjectKind::Lathe,"lathe_turn","Revolution angle (degrees)",0,360,1,360},
  {MathParameter::LatheCut,MathObjectKind::Lathe,"lathe_cut","Cutaway (% hidden)",0,75,5,25},
  {MathParameter::LatheProbe,MathObjectKind::Lathe,"lathe_probe","Probe / highlighted element",0,1,.005,.5},
  {MathParameter::LatheSlices,MathObjectKind::Lathe,"lathe_slices","Integration subdivisions",4,32,1,8},
  {MathParameter::LatheMethod,MathObjectKind::Lathe,"lathe_method","Volume approximation",0,1,1,0,0,false,"Disks / washers\0Cylindrical shells\0"},
  {MathParameter::LatheGuides,MathObjectKind::Lathe,"lathe_guides","Construction guides",0,1,1,1,0,false,"Shape only\0Show construction\0"},
  {MathParameter::BooleanShapeA,MathObjectKind::Boolean,"boolean_shape_a","Shape A",0,3,1,0,0,false,"Box\0Sphere\0Cylinder\0Arch opening\0"},
  {MathParameter::BooleanShapeB,MathObjectKind::Boolean,"boolean_shape_b","Shape B / cutter",0,3,1,2,0,false,"Box\0Sphere\0Cylinder\0Arch opening\0"},
  {MathParameter::BooleanSizeA,MathObjectKind::Boolean,"boolean_size_a","Size A",0.2,1.5,0.05,1},
  {MathParameter::BooleanSizeB,MathObjectKind::Boolean,"boolean_size_b","Size B",0.2,1.5,0.05,0.55},
  {MathParameter::BooleanX,MathObjectKind::Boolean,"boolean_x","B: x position",-1.5,1.5,0.05,0},
  {MathParameter::BooleanY,MathObjectKind::Boolean,"boolean_y","B: y position",-1.5,1.5,0.05,0},
  {MathParameter::BooleanZ,MathObjectKind::Boolean,"boolean_z","B: z position",-1.5,1.5,0.05,0},
  {MathParameter::BooleanYaw,MathObjectKind::Boolean,"boolean_yaw","B: yaw (degrees)",-180,180,5,0},
  {MathParameter::BooleanPitch,MathObjectKind::Boolean,"boolean_pitch","B: pitch (degrees)",-180,180,5,0},
  {MathParameter::BooleanOperation,MathObjectKind::Boolean,"boolean_operation","Combine A and B",0,3,1,2,0,false,"Union\0Intersection\0A minus B\0Smooth union\0"},
  {MathParameter::BooleanBlend,MathObjectKind::Boolean,"boolean_blend","Blend width",0,0.6,0.02,0.4},
  {MathParameter::BooleanProbeX,MathObjectKind::Boolean,"boolean_probe_x","Probe: x",-3,3,0.01,0},
  {MathParameter::BooleanProbeY,MathObjectKind::Boolean,"boolean_probe_y","Probe: y",-3,3,0.01,0},
  {MathParameter::BooleanProbeZ,MathObjectKind::Boolean,"boolean_probe_z","Probe: z / section plane",-3,3,0.01,0},
  {MathParameter::BooleanResolution,MathObjectKind::Boolean,"boolean_resolution","Requested cells per axis",12,28,4,20},
  {MathParameter::BooleanGuides,MathObjectKind::Boolean,"boolean_guides","Construction guides",0,1,1,1,0,false,"Shape only\0Show construction\0"},
  {MathParameter::BooleanSection,MathObjectKind::Boolean,"boolean_section","Solid view",0,1,1,0,0,false,"Full solid\0Section at probe z\0"},
  {MathParameter::BooleanFit,MathObjectKind::Boolean,"boolean_fit","Matching ball preview",0,1,1,0,0,false,"Hidden\0Show beside socket\0"},
  {MathParameter::BooleanClearance,MathObjectKind::Boolean,"boolean_clearance","Ball radial clearance",0.02,0.15,0.01,0.08},
  {MathParameter::PatchControl,MathObjectKind::Patch,"patch_control","Control point",0,15,1,5,0,false,"P00\0P01\0P02\0P03\0P10\0P11\0P12\0P13\0P20\0P21\0P22\0P23\0P30\0P31\0P32\0P33\0"},
  {MathParameter::PatchP00X,MathObjectKind::Patch,"patch_p00x","P00: x",-2,2,0.01,-1.5},
  {MathParameter::PatchP00Y,MathObjectKind::Patch,"patch_p00y","P00: y",-2,2,0.01,0},
  {MathParameter::PatchP00Z,MathObjectKind::Patch,"patch_p00z","P00: z",-2,2,0.01,1.5},
  {MathParameter::PatchP01X,MathObjectKind::Patch,"patch_p01x","P01: x",-2,2,0.01,-1.5},
  {MathParameter::PatchP01Y,MathObjectKind::Patch,"patch_p01y","P01: y",-2,2,0.01,0.45},
  {MathParameter::PatchP01Z,MathObjectKind::Patch,"patch_p01z","P01: z",-2,2,0.01,0.5},
  {MathParameter::PatchP02X,MathObjectKind::Patch,"patch_p02x","P02: x",-2,2,0.01,-1.5},
  {MathParameter::PatchP02Y,MathObjectKind::Patch,"patch_p02y","P02: y",-2,2,0.01,0.45},
  {MathParameter::PatchP02Z,MathObjectKind::Patch,"patch_p02z","P02: z",-2,2,0.01,-0.5},
  {MathParameter::PatchP03X,MathObjectKind::Patch,"patch_p03x","P03: x",-2,2,0.01,-1.5},
  {MathParameter::PatchP03Y,MathObjectKind::Patch,"patch_p03y","P03: y",-2,2,0.01,0},
  {MathParameter::PatchP03Z,MathObjectKind::Patch,"patch_p03z","P03: z",-2,2,0.01,-1.5},
  {MathParameter::PatchP10X,MathObjectKind::Patch,"patch_p10x","P10: x",-2,2,0.01,-0.5},
  {MathParameter::PatchP10Y,MathObjectKind::Patch,"patch_p10y","P10: y",-2,2,0.01,0.65},
  {MathParameter::PatchP10Z,MathObjectKind::Patch,"patch_p10z","P10: z",-2,2,0.01,1.5},
  {MathParameter::PatchP11X,MathObjectKind::Patch,"patch_p11x","P11: x",-2,2,0.01,-0.5},
  {MathParameter::PatchP11Y,MathObjectKind::Patch,"patch_p11y","P11: y",-2,2,0.01,1.1},
  {MathParameter::PatchP11Z,MathObjectKind::Patch,"patch_p11z","P11: z",-2,2,0.01,0.5},
  {MathParameter::PatchP12X,MathObjectKind::Patch,"patch_p12x","P12: x",-2,2,0.01,-0.5},
  {MathParameter::PatchP12Y,MathObjectKind::Patch,"patch_p12y","P12: y",-2,2,0.01,1.1},
  {MathParameter::PatchP12Z,MathObjectKind::Patch,"patch_p12z","P12: z",-2,2,0.01,-0.5},
  {MathParameter::PatchP13X,MathObjectKind::Patch,"patch_p13x","P13: x",-2,2,0.01,-0.5},
  {MathParameter::PatchP13Y,MathObjectKind::Patch,"patch_p13y","P13: y",-2,2,0.01,0.65},
  {MathParameter::PatchP13Z,MathObjectKind::Patch,"patch_p13z","P13: z",-2,2,0.01,-1.5},
  {MathParameter::PatchP20X,MathObjectKind::Patch,"patch_p20x","P20: x",-2,2,0.01,0.5},
  {MathParameter::PatchP20Y,MathObjectKind::Patch,"patch_p20y","P20: y",-2,2,0.01,0.65},
  {MathParameter::PatchP20Z,MathObjectKind::Patch,"patch_p20z","P20: z",-2,2,0.01,1.5},
  {MathParameter::PatchP21X,MathObjectKind::Patch,"patch_p21x","P21: x",-2,2,0.01,0.5},
  {MathParameter::PatchP21Y,MathObjectKind::Patch,"patch_p21y","P21: y",-2,2,0.01,1.1},
  {MathParameter::PatchP21Z,MathObjectKind::Patch,"patch_p21z","P21: z",-2,2,0.01,0.5},
  {MathParameter::PatchP22X,MathObjectKind::Patch,"patch_p22x","P22: x",-2,2,0.01,0.5},
  {MathParameter::PatchP22Y,MathObjectKind::Patch,"patch_p22y","P22: y",-2,2,0.01,1.1},
  {MathParameter::PatchP22Z,MathObjectKind::Patch,"patch_p22z","P22: z",-2,2,0.01,-0.5},
  {MathParameter::PatchP23X,MathObjectKind::Patch,"patch_p23x","P23: x",-2,2,0.01,0.5},
  {MathParameter::PatchP23Y,MathObjectKind::Patch,"patch_p23y","P23: y",-2,2,0.01,0.65},
  {MathParameter::PatchP23Z,MathObjectKind::Patch,"patch_p23z","P23: z",-2,2,0.01,-1.5},
  {MathParameter::PatchP30X,MathObjectKind::Patch,"patch_p30x","P30: x",-2,2,0.01,1.5},
  {MathParameter::PatchP30Y,MathObjectKind::Patch,"patch_p30y","P30: y",-2,2,0.01,0},
  {MathParameter::PatchP30Z,MathObjectKind::Patch,"patch_p30z","P30: z",-2,2,0.01,1.5},
  {MathParameter::PatchP31X,MathObjectKind::Patch,"patch_p31x","P31: x",-2,2,0.01,1.5},
  {MathParameter::PatchP31Y,MathObjectKind::Patch,"patch_p31y","P31: y",-2,2,0.01,0.45},
  {MathParameter::PatchP31Z,MathObjectKind::Patch,"patch_p31z","P31: z",-2,2,0.01,0.5},
  {MathParameter::PatchP32X,MathObjectKind::Patch,"patch_p32x","P32: x",-2,2,0.01,1.5},
  {MathParameter::PatchP32Y,MathObjectKind::Patch,"patch_p32y","P32: y",-2,2,0.01,0.45},
  {MathParameter::PatchP32Z,MathObjectKind::Patch,"patch_p32z","P32: z",-2,2,0.01,-0.5},
  {MathParameter::PatchP33X,MathObjectKind::Patch,"patch_p33x","P33: x",-2,2,0.01,1.5},
  {MathParameter::PatchP33Y,MathObjectKind::Patch,"patch_p33y","P33: y",-2,2,0.01,0},
  {MathParameter::PatchP33Z,MathObjectKind::Patch,"patch_p33z","P33: z",-2,2,0.01,-1.5},
  {MathParameter::PatchU,MathObjectKind::Patch,"patch_u","Probe u",0,1,0.01,0.5},
  {MathParameter::PatchV,MathObjectKind::Patch,"patch_v","Probe v",0,1,0.01,0.5},
  {MathParameter::PatchResolution,MathObjectKind::Patch,"patch_resolution","Subdivisions / axis",4,32,4,20},
  {MathParameter::PatchGuides,MathObjectKind::Patch,"patch_guides","Construction guides",0,1,1,1,0,false,"Shape only\0Show construction\0"}
,
  {MathParameter::MembraneSlot,MathObjectKind::Membrane,"membrane_slot","Mode slot",0,3,1,0,0,false,"Slot 1\0Slot 2\0Slot 3\0Slot 4\0"},
  {MathParameter::MembraneM0,MathObjectKind::Membrane,"membrane_m0","m",1,6,1,1},
  {MathParameter::MembraneN0,MathObjectKind::Membrane,"membrane_n0","n",1,6,1,1},
  {MathParameter::MembraneA0,MathObjectKind::Membrane,"membrane_a0","Initial displacement",-0.6,0.6,0.01,0.35},
  {MathParameter::MembraneV0,MathObjectKind::Membrane,"membrane_v0","Initial velocity",-0.6,0.6,0.01,0},
  {MathParameter::MembraneM1,MathObjectKind::Membrane,"membrane_m1","m",1,6,1,2},
  {MathParameter::MembraneN1,MathObjectKind::Membrane,"membrane_n1","n",1,6,1,1},
  {MathParameter::MembraneA1,MathObjectKind::Membrane,"membrane_a1","Initial displacement",-0.6,0.6,0.01,0},
  {MathParameter::MembraneV1,MathObjectKind::Membrane,"membrane_v1","Initial velocity",-0.6,0.6,0.01,0},
  {MathParameter::MembraneM2,MathObjectKind::Membrane,"membrane_m2","m",1,6,1,1},
  {MathParameter::MembraneN2,MathObjectKind::Membrane,"membrane_n2","n",1,6,1,2},
  {MathParameter::MembraneA2,MathObjectKind::Membrane,"membrane_a2","Initial displacement",-0.6,0.6,0.01,0},
  {MathParameter::MembraneV2,MathObjectKind::Membrane,"membrane_v2","Initial velocity",-0.6,0.6,0.01,0},
  {MathParameter::MembraneM3,MathObjectKind::Membrane,"membrane_m3","m",1,6,1,2},
  {MathParameter::MembraneN3,MathObjectKind::Membrane,"membrane_n3","n",1,6,1,2},
  {MathParameter::MembraneA3,MathObjectKind::Membrane,"membrane_a3","Initial displacement",-0.6,0.6,0.01,0},
  {MathParameter::MembraneV3,MathObjectKind::Membrane,"membrane_v3","Initial velocity",-0.6,0.6,0.01,0},
  {MathParameter::MembraneWidth,MathObjectKind::Membrane,"membrane_width","Width",1,4,0.05,3},
  {MathParameter::MembraneDepth,MathObjectKind::Membrane,"membrane_depth","Depth",1,4,0.05,3},
  {MathParameter::MembraneTension,MathObjectKind::Membrane,"membrane_tension","Tension",0.25,4,0.05,1},
  {MathParameter::MembraneDensity,MathObjectKind::Membrane,"membrane_density","Areal density",0.5,2,0.05,1},
  {MathParameter::MembraneDamping,MathObjectKind::Membrane,"membrane_damping","Damping gamma",0,2,0.05,0},
  {MathParameter::MembraneTime,MathObjectKind::Membrane,"membrane_time","Time",0,12,0.01,0},
  {MathParameter::MembraneU,MathObjectKind::Membrane,"membrane_u","Probe u",0,1,0.01,0.5},
  {MathParameter::MembraneV,MathObjectKind::Membrane,"membrane_v","Probe v",0,1,0.01,0.5},
  {MathParameter::MembraneResolution,MathObjectKind::Membrane,"membrane_resolution","Subdivisions / axis",12,48,4,32},
  {MathParameter::MembraneGuides,MathObjectKind::Membrane,"membrane_guides","Construction guides",0,1,1,1,0,false,"Shape only\0Show construction\0"},
  {MathParameter::MembraneView,MathObjectKind::Membrane,"membrane_view","Surface",0,1,1,0,0,false,"Combined surface\0Selected slot\0"},
  {MathParameter::RigidShape,MathObjectKind::Rigid,"rigid_shape","Body",0,3,1,0,0,false,"Flywheel\0Dumbbell\0Book\0Satellite\0"},
  {MathParameter::RigidWidth,MathObjectKind::Rigid,"rigid_width","Width",0.2,3.5,0.05,2.4},
  {MathParameter::RigidHeight,MathObjectKind::Rigid,"rigid_height","Height",0.2,3.5,0.05,0.3},
  {MathParameter::RigidDepth,MathObjectKind::Rigid,"rigid_depth","Depth",0.2,3.5,0.05,2.4},
  {MathParameter::RigidMass,MathObjectKind::Rigid,"rigid_mass","Total mass",0.2,5,0.1,1},
  {MathParameter::RigidBalance,MathObjectKind::Rigid,"rigid_balance","Left mass share",0.2,0.8,0.01,0.5},
  {MathParameter::RigidRotX,MathObjectKind::Rigid,"rigid_rot_x","Release X (deg)",-180,180,1,0},
  {MathParameter::RigidRotY,MathObjectKind::Rigid,"rigid_rot_y","Release Y (deg)",-180,180,1,0},
  {MathParameter::RigidRotZ,MathObjectKind::Rigid,"rigid_rot_z","Release Z (deg)",-180,180,1,0},
  {MathParameter::RigidSpinX,MathObjectKind::Rigid,"rigid_spin_x","Initial spin X",-4,4,0.01,0},
  {MathParameter::RigidSpinY,MathObjectKind::Rigid,"rigid_spin_y","Initial spin Y",-4,4,0.01,3},
  {MathParameter::RigidSpinZ,MathObjectKind::Rigid,"rigid_spin_z","Initial spin Z",-4,4,0.01,0},
  {MathParameter::RigidTime,MathObjectKind::Rigid,"rigid_time","Time",0,12,0.01,0},
  {MathParameter::RigidAxis,MathObjectKind::Rigid,"rigid_axis","Track body axis",0,2,1,0,0,false,"Body X\0Body Y\0Body Z\0"},
  {MathParameter::RigidGuides,MathObjectKind::Rigid,"rigid_guides","Construction guides",0,1,1,1,0,false,"Shape only\0Show construction\0"}
}};
constexpr std::array<MathObjectSpec,static_cast<std::size_t>(MathObjectKind::Count)> objects{{
  {MathObjectKind::Algebra,"algebra","Algebra","A cube full of algebra",
   "(x+1)^3 = x^3 + 3x^2 + 3x + 1","Make a closed cube with total volume 27.",
   "x is a positive length. Gaps change placement, never piece dimensions.",
   {"Measure lengths and volumes. Count eight pieces.","Expand a cube by adding its component volumes.","Connect 1, 3, 3, 1 with binomial coefficients."},{-1,.7F,-1}},
  {MathObjectKind::Trig,"trig","Trigonometry","Turn a circle into a wave",
   "P = (cos(theta), sin(theta))    cos^2 + sin^2 = 1","Find a second-quadrant angle whose sine is 0.5.",
   "Radius = 1. The separate wave chart has its own angle axis, from 0 to 2*pi.",
   {"Connect angle and side ratios in a right triangle.","Read signed coordinates, degrees and radians.","Connect circular motion to waves and phasors."},{0,.05F,1}},
  {MathObjectKind::Calculus,"calculus","Calculus","Build volume one slice at a time",
   "V ~ sum(pi*r_i^2*dx) -> integral[0,2](pi*x^2 dx) = 8*pi/3",
   "Approximate the cone volume with less than 0.1% error.",
   "Error uses analytic disk volumes. Mesh circles have 24 sides; gaps change placement only.",
   {"Calculate cylinder volume as area times thickness.","Compare left, midpoint and right sums.","Take the limit to obtain a volume integral."},{-1,.6F,1}},
  {MathObjectKind::Linear,"linear","Linear algebra","Watch a matrix move space",
   "A = [[1,k,0], [0,s,0], [0,0,1]]    det(A) = s",
   "Collapse the cube into a plane. Watch determinant and rank.",
   "The coloured cage is the image of a unit cube. Volume is abs(det A); the grey cage is the original.",
   {"Explore coordinates, stretching, reflection and volume.","Read matrix columns as transformed basis vectors.","Connect determinant, orientation, rank and null space."},{1,.8F,1}},
  {MathObjectKind::Discrete,"discrete","Discrete maths","Find a path through a network",
   "Q3: each ordinary edge flips one bit. Every edge costs one hop.",
   "Reach H from A in the fewest edge hops.",
   "Layout changes do not change adjacency. Crossings are not vertices. The shortcut changes the graph.",
   {"Follow routes and count steps.","Use three-bit addresses to represent the vertices.","Explore shortest paths, Hamming distance and hypercubes."},{1,.7F,1}},
  {MathObjectKind::Function,"function","Functions","Connect a function to its calculus",
   "f(x), f'(x), and F(x) = integral[a,x] f(t) dt","Find an input where f(x) = 0.",
   "All linked views share x. Signed accumulation follows the order of the bounds. Curves are sampled; readouts are analytic.",
   {"Find inputs, outputs and roots.","Compare secants, tangents and derivatives.","Link accumulation, differentiation and Taylor approximation."},{0,0,1}},
  {MathObjectKind::Surface,"surface","Surfaces","Read a surface through its contours",
   "z = f(u,v)","On the bowl, find a point with height 1.",
   "World position is (u, height, v). Contours interpolate the surface grid; derivatives and readouts are analytic.",
   {"Connect coordinates, height and contours.","Read partial derivatives and a tangent plane.","Explore gradients, optimisation and constrained stationary points."},{1,.8F,1}},
  {MathObjectKind::Symmetry,"symmetry","Symmetry","Compose the rotations of a cube",
   "A rotation maps every labelled vertex to another vertex.","Move vertex A to slot H.",
   "Right-handed turns act about fixed world axes. A-H encode the signs of x,y,z. Only the 24 proper cube rotations are used.",
   {"Move labelled vertices while preserving edges.","Compose rotations and compare their order.","Explore cyclic subgroups, vertex orbits and stabilisers."},{1,.7F,1}},
  {MathObjectKind::Harmonics,"harmonics","Harmonics","Build curves and signals from rotations",
   "z(t) = a exp(i (n t + phase))","Make a unit phasor point upwards.",
   "Time spans 0 to 2*pi. The Lissajous wire uses height along its third axis to show time. Fourier targets have period 2*pi.",
   {"Connect a rotating arrow to its sine projection.","Combine frequencies and phases into Lissajous curves.","Build Fourier sums and recover coefficients by orthogonality."},{0,.35F,1}},
  {MathObjectKind::Oscillator,"oscillator","Motion","Watch an initial state evolve",
   "q'' + c q' + k q = F cos(Omega t)","Let a displaced spring cross its equilibrium.",
   "Mass/inertia = 1. Pendulum restoring force is k*sin(q); its angle is in radians. Geometry is scaled to fit. Readouts retain model units; time is 0-12 s.",
   {"Choose an initial displacement and velocity.","Read the phase curve and mechanical energy.","Explore damping, forcing and the balance of energy and work."},{.25F,.4F,1}},
  {MathObjectKind::Modular,"modular","Modular drums","Walk around residue classes",
   "a = q*n + r, with 0 <= r < n","With modulus 7, represent -3 by residue 4.",
   "Drum labels are canonical residues. Every step is exact integer arithmetic. Changing n or k starts a new observed walk.",
   {"Relate integers to their remainders.","Explore cycles, coprimality and modular inverses.","Solve simultaneous congruences on linked drums."},{.3F,.3F,1}},
  {MathObjectKind::Gaussian,"gaussian","Gaussian lattice","Give complex integers a structure",
   "N(a+bi) = a^2 + b^2","Multiply two non-real inputs to obtain a nonzero real result.",
   "Grid coordinates are real/imaginary parts. Scene scale adjusts uniformly to fit; optional height is 0.12*N. Abstract layers use the complex plane.",
   {"Add and multiply integer points in the complex plane.","Use norm, Euclidean division and principal ideals.","Compare finite quotient rings, units and zero divisors."},{.3F,.3F,1}},
  {MathObjectKind::VectorField,"field","Vector fields","Measure work along a path",
   "Directional component = F dot unit direction","Find a direction perpendicular to a nonzero radial field vector.",
   "Fields are smooth on all of R^3. Teal field arrows use a common 0.3 scale. Gold shows the tangent direction; orange shows F at the probe.",
   {"Read vector components and dot products.","Parameterise a path and accumulate a line integral.","Compare work along routes with the same endpoints."},{1,.7F,1}},
  {MathObjectKind::Flux,"flux","Flux shells","Measure a field across a surface",
   "Flux = integral of F dot n over oriented area","Turn the radial sphere's normals inward.",
   "Gold normals have length 0.35; orange field arrows use scale 0.25. Disk centre is (0,0,0.4). Teal/coral show positive/negative flux density (curl flux in Stokes). Integration and display sampling are independent.",
   {"Choose surface orientation and inspect a normal.","Sum flux across a sphere, box or disk.","Connect surfaces to divergence and oriented boundaries."},{1,.6F,1}},
  {MathObjectKind::Tensor,"tensor","Tensor blocks","Build arrays from vectors",
   "A[i,j] = u[i] v[j]","Make a nonzero outer product with zero trace.",
   "Indices run 1-3. Blocks show signed components with a common height scale. Contraction uses the Euclidean metric; basis changes are orthonormal.",
   {"Build an outer product one component at a time.","Add a third index and contract two indices.","Change the coordinate basis while preserving the underlying map."},{1,.8F,1}},
  {MathObjectKind::Probability,"probability","Probability network","Move probability through a network",
   "p(next) = p(now) P, using row vectors","Visit all three states in an actual walk.",
   "P[i,j] is the chance of moving from i to j. Coloured node volumes represent probabilities; grey markers mean p < 1e-9. Tables retain full values. Edge widths show transition weights; seeds make walks repeatable.",
   {"Step a sampled walk and inspect outgoing probabilities.","Evolve a complete probability distribution.","Compare stationary, periodic and absorbing behaviour."},{.3F,.4F,1}},
  {MathObjectKind::Binomial,"binomial","Binomial board","Build a distribution from trials",
   "P(X=k) = choose(n,k) p^k (1-p)^(n-k)","Complete a trial path that contains both successes and failures.",
   "Every trial is independent with fixed p. The path uses a reproducible seed. Bar heights show exact probabilities; grey baselines mean mass below 1e-9. The normal curve is an approximation.",
   {"Follow independent Bernoulli trials.","Measure binomial mass, mean, variance and tails.","Compare the exact distribution with a continuity-corrected normal approximation."},{.3F,.2F,1}},
  {MathObjectKind::Bayes,"bayes","Bayesian cube","Make conditioning a change of scale",
   "P(H | E) = P(H) P(E | H) / P(E)","Make H and E independent with a nonzero joint probability.",
   "The left unit cube partitions joint probability. The right cube renormalizes the selected evidence to one. Pieces below 1e-9 are omitted from geometry; tables retain their values.",
   {"Partition joint and marginal probabilities.","Condition on an event and update hypothesis odds.","Accumulate evidence that is conditionally independent given the hypothesis."},{1,.7F,1}},
  {MathObjectKind::Covariance,"covariance","Covariance cloud","Read the geometry of variation",
   "Cov(X) = mean[(X-mean)(X-mean)^T]","Move a nontrivial cloud to mean (0.5,-0.5,0).",
   "Eight equally weighted points define a population, using divisor 8. Covariance ellipses describe second moments, not Gaussian confidence regions. Scene scale is uniform; tables keep mathematical coordinates.",
   {"Move the mean of a finite point cloud.","Inspect covariance, correlation and principal components.","Whiten a full-rank cloud to identity covariance."},{1,.7F,1}},
  {MathObjectKind::Spherical,"spherical","Harmonic sphere","Read functions on a sphere",
   "Y(l,m) separates latitude and longitude.","Place the probe on the positive y axis.",
   "Unit-sphere polar coordinates use z as the polar axis. Real modes include the Condon-Shortley phase: positive m uses cosine, negative m sine. Lobes have radius 0.3 + 1.4*abs(f); teal/coral encode sign, so the radius is not the function value.",
   {"Locate directions using spherical coordinates.","Explore normalized real modes and superposition.","Diffuse spherical functions with the Laplace-Beltrami spectrum."},{1,.7F,1}},
  {MathObjectKind::Quadratic,"quadratic","Quadratic forms","Connect surfaces to symmetric matrices",
   "q(x) = x^T A x; A = Q D Q^T","Find a probe where q is negative.",
   "Layers 1-3 show the section q(x,y,z0) with displayed height 0.15*q, not the quadric q=1. Layer 4 colors a unit sphere by its Rayleigh quotient. Eigenvalues may be positive, negative or zero.",
   {"Evaluate a quadratic form and its sections.","Change orthonormal coordinates and inspect inertia.","Find constrained extrema using the Rayleigh quotient."},{1,.7F,1}},
  {MathObjectKind::Roots,"roots","Roots of unity","Build algebra from a root constellation",
   "zeta^k = exp(2*pi*i*k/n)","Choose a primitive root for a composite n.",
   "n is 3-12. Rings lie in parallel complex planes; height separates maps or records a power step. Cyclotomic automorphisms fix Q and exist exactly for gcd(a,n)=1. A nonunit exponent is a power map, not a field automorphism.",
   {"Locate roots and apply complex powers.","Trace cyclic subgroups and generator orders.","Inspect cyclotomic polynomials and their Galois action."},{.5F,.4F,1}},
  {MathObjectKind::Psd,"psd","PSD cone","Move matrices through a convex cone",
   "A = [[a,b],[b,c]]; t=(a+c)/2, x=(a-c)/2, y=b", "Place a nonzero matrix on the PSD boundary.",
   "Scene coordinates are (x,y,t); PSD means t >= sqrt(x*x+y*y), not entrywise positivity. The open cone is a finite guide to an unbounded set. This exact 3D model represents symmetric 2x2 matrices; symmetric 3x3 matrices need six coordinates.",
   {"Locate matrices, eigenvalues and the rank-one boundary.","Mix PSD matrices and scale along a cone ray.","Optimize a linear objective on a fixed-trace disk."},{1,.7F,1}},
  {MathObjectKind::Norm,"norm","Norm balls","Change the meaning of unit distance",
   "||x||_p = (sum |x_i|^p)^(1/p); ||x||_infinity = max |x_i|", "At p=1, put a vector with at least two nonzero components on the unit boundary.",
   "The boundary has norm one in real 3D. Finite p ranges from 1 to 32; the infinity norm is a separate exact mode. The finite mesh samples the true boundary. In the dual layer the two bodies are translated apart; their tables retain the original coordinates.",
   {"Morph between an octahedron, a sphere and a cube.","Measure triangle inequalities and the p-to-infinity limit.","Pair primal and dual balls with a supporting plane."},{1,.7F,1}},
  {MathObjectKind::Curve,"curve","Curves and sweeps","Grow a shaped object along a curve",
   "r(t) = sum B_i^3(t) P_i; sweep = r(t) + profile in a moving frame",
   "Choose an interior t and a non-coplanar control tetrahedron.",
   "A cubic Bezier curve uses four controls. Click a control point or choose it in the sidebar, then edit its x/y/z coordinates. Arc length and equal-distance positions are numerical with displayed length bounds. Sweeps use transported frames and capped profiles; a zero tangent disables the sweep, and broad sweeps can intersect themselves.",
   {"Construct a curve by repeated interpolation.","Inspect tangent, curvature and distance along the curve.","Sweep a circle, square or norm profile with taper and twist."},{1,.7F,1},.25,"Restart position","End of curve. Restart or scrub position."},
  {MathObjectKind::Lathe,"lathe","Lathe Lab","Turn a profile into a measured solid",
   "X(h,theta) = (R(h) cos(theta), h, R(h) sin(theta))",
   "Make an interior profile point wider than both endpoints.",
   "Seven ordered points define a smooth, shape-preserving Bezier profile. Select a point and edit its radius or height. Wall thickness is radial: narrow regions become solid. The cutaway changes visibility only; measurements use the complete selected revolution angle. The vertical axis is height.",
   {"Shape a vase, bottle, goblet or pawn.","Revolve the profile and compare volume elements.","Measure lateral bands, closures and surface normals."},{1,.7F,-1},90,"Restart revolution","Revolution complete. Restart or change the angle."},
  {MathObjectKind::Boolean,"boolean","Boolean Solids Lab","Cut, join and blend solids",
   "Union: min(a,b); intersection: max(a,b); A minus B: max(a,-b)",
   "Put the probe inside A and B, but outside the remaining material.",
   "Negative field values mean interior, zero means boundary, positive means exterior. A is teal; B and cut walls are coral. Composite fields are defining functions, not generally exact distances. Section view hides z above the probe plane; full-solid measurements stay unchanged. Thin features can be missed by a finite grid.",
   {"Classify points inside and outside a solid.","Link set operations to a live truth table.","Explore smooth blends, surface normals and numerical volume."},{1,.7F,1}}


,
  {MathObjectKind::Patch,"patch","Patch Lab","Shape a sheet with sixteen control points",
   "S(u,v) = sum[i,j] B_i(u) B_j(v) P_ij; 0 <= u,v <= 1","Select a corner control and put the probe at that corner.",
   "Open bicubic sheet, with world y vertical. Normal orientation is S_u cross S_v. Folded area counts overlap with multiplicity; no closed-solid volume is claimed.",
   {"Blend sixteen controls into one editable surface.","Use partial derivatives to construct tangent planes and normals.","Connect curvature, metric and area to the same patch."},{1,.8F,1}}
,
  {MathObjectKind::Membrane,"membrane","Membrane Lab","Set a stretched sheet in motion",
   "h_tt + 2*gamma*h_t = (T/rho)*(h_xx+h_yy)","Move a nonzero membrane below equilibrium at the probe.",
   "Linear motion with all four edges fixed. World y is displacement. Four sine-mode slots combine; duplicate (m,n) pairs combine before energy. Time is model seconds; playback runs at half speed. Colours and energy density follow the displayed surface; global energy readouts always describe the combined membrane.",
   {"Follow displacement and velocity on a moving sheet.","Explore eigenmodes, nodal lines and superposition.","Connect a wave equation to kinetic energy, strain energy and damping."},{1,.8F,1},.5}
,
  {MathObjectKind::Rigid,"rigid","Rigid-Body Rotation Lab","Spin a body and follow its momentum",
   "L = I*omega; E = omega dot L / 2","Turn a tracked body axis perpendicular to its release direction.",
   "Torque-free rotation about the centre of mass. Mass is in kg, dimensions in m, time in seconds, angular speed in rad/s. Playback runs at half speed. The same component assembly supplies geometry and inertia. No external torque, translation or collision is simulated.",
   {"Connect moving body frames, rotation matrices and quaternions.","Derive inertia from shape and mass distribution.","Explore free rotation, intermediate-axis flips and conservation."},{1,.7F,1},.5}
}};
constexpr std::array<MathLesson,4> functionLessons{{
  {"Inputs and roots","y = f(x)","Find an input where f(x) = 0.","Move the point or scrub a graph. The selected function owns every linked value."},
  {"Secants and limits","secant = [f(x+h)-f(x)]/h  ->  f'(x)","Use a NONZERO h with secant error below 0.02.","Compare the gold secant with the coral tangent. At h=0 the display uses the derivative limit; division by zero is never evaluated."},
  {"Signed accumulation","F(x) = integral[a,x] f(t) dt; F'(x) = f(x)","Make the signed integral negative, then use Swap bounds to reverse its sign.","The filled region lies between a and x. The integral includes both the sign of f and the orientation of the bounds."},
  {"Taylor approximation","T_n(x) = sum[k=0,n] f^(k)(c) (x-c)^k / k!","At least 0.5 from c, approximate f(x) within 0.001.","Increase the degree and compare the approximation with f. Accuracy depends on both degree and distance from the centre."}
}};
constexpr std::array<MathLesson,4> linearLessons{{
  {"Volume and rank","det(A) = s; volume = abs(det(A))","Collapse the cube into a plane.","Shear preserves volume; zero vertical scale removes one independent direction."},
  {"Composition and invariant directions","v -> A v -> B A v; compare B A with A B","Choose a nonzero v with A v = lambda v and a nonzero A v.","Edit A in the matrix diagram. B rotates about z. Gold is v, teal is Av, violet is BAv; an eigenvector stays on its original line."},
  {"Projection and least squares","p = projection of v onto span(A e1, A e2)","Make a nonzero v lie in the column plane: residual below 0.01.","The gold residual is perpendicular to the spanned subspace. If the columns become dependent, the projection uses their remaining span."},
  {"Singular value decomposition","A = U Sigma V^T","Set the smallest singular value to zero while retaining rank 2.","Step the sphere through V transpose, Sigma and U. Singular values measure axis stretches; the full matrix diagram still owns A."}
}};
constexpr std::array<MathLesson,4> surfaceLessons{{
  {"Height and contours","z = f(u,v)","On the bowl, find a point with height 1.","A contour connects equal heights. Click the contour map to move the point; the two section plots follow it."},
  {"Partials and tangent planes","D_d f = grad(f) dot d","On the bowl away from the origin, find a direction with slope below 0.02 in magnitude.","The u and v sections show the two partial derivatives. The tangent plane combines them; the direction arrow lives in the input plane."},
  {"Gradient descent and curvature","next point = point - step * grad(f)","On the bowl, reach gradient magnitude below 0.02.","Descent uses a bounded line search. Inspect the Hessian: positive curvature gives a minimum; opposite signs identify a saddle."},
  {"Constrained stationary points","u^2 + v^2 = 1; grad(f) = lambda grad(g)","On the saddle with the circle enabled, make the tangential slope smaller than 0.02.","Move around the circle. At a constrained stationary point, the gradient is normal to the constraint, even when it is not zero."}
}};
constexpr std::array<MathLesson,4> symmetryLessons{{
  {"Labelled rotations","p -> R p","Move labelled vertex A to slot H.","Apply quarter turns about world axes. Labels move with the cube; the grey cage marks the fixed slots. Undo removes the last turn."},
  {"Composition order","first R, then S gives S R; reversing gives R S","From identity, apply your first then second turn, with different resulting orders.","The two smaller cages compare both orders. Choose turns about different axes, return to identity, then apply the two selected turns."},
  {"Cyclic subgroups","<G> = {I, G, ..., G^(order-1)}","Choose an order-3 generator and return to identity with a positive power.","The power control applies repeated copies of one rotation. The highlighted slots form the orbit of A under that cyclic subgroup."},
  {"Orbits and stabilisers","|orbit(A)| * |stabiliser(A)| = 24","Choose a non-identity rotation that fixes the tracked vertex.","Browse all 24 proper rotations. The vertex action gives a faithful permutation representation. An orbit collects possible destinations; a stabiliser consists of rotations fixing the selected vertex."}
}};
constexpr std::array<MathLesson,4> harmonicLessons{{
  {"One rotating component","y(t) = a sin(n t + phase)","Make a unit phasor point upwards within 0.02.","Adjust amplitude, frequency and phase, then scrub or play time. The plotted marker and the phasor share t."},
  {"Lissajous curves","x(t)=a1 cos(n1 t+p1); y(t)=a2 sin(n2 t+p2)","Make a nonzero circle in the x-y projection using equal frequencies and phases.","Integer frequencies make the x-y projection repeat. The wire's third coordinate is time, so the lifted wire itself does not close."},
  {"Fourier synthesis","S_N(t) = sum b_n sin(n t)","Approximate the square wave with RMS error below 0.2.","Each arrow contributes one nonzero term. The spectrum lists its signed coefficient. RMS error is computed over a full period by Parseval; jump values use the midpoint convention."},
  {"Extracting a coefficient","b_n = (1/pi) integral[0,2*pi] f(t) sin(n t) dt","Find an absent even sine harmonic of the square wave.","Compare the analytic coefficient with a 1024-point midpoint integral. The product plot shows cancellation. This is a Fourier-series laboratory; it uses periodic targets."}
}};
constexpr std::array<MathLesson,4> oscillatorLessons{{
  {"Initial state and motion","spring: q''=-k q; pendulum: q''=-k sin(q)","Start a spring at least 0.4 from equilibrium with zero initial velocity, then reach |q| < 0.03.","Choose the initial state and restoring coefficient. Play, pause, advance or scrub time. Every time is recomputed from the same initial state."},
  {"Phase and conserved energy","E = v^2/2 + V(q); dE/dt = 0","Evolve a nonzero-energy system for at least 4 seconds while conserving energy within 1e-6.","The phase plot links displacement and velocity. V(q)=k*q^2/2 for the spring and k*(1-cos(q)) for the pendulum. Trajectories use bounded RK4 integration."},
  {"Damping and energy loss","E(t) + integral c*v^2 dt = E(0)","Use positive damping to reduce a nonzero initial energy below one quarter.","Positive damping removes mechanical energy. The energy plot separates remaining energy and accumulated loss; the phase curve approaches equilibrium."},
  {"Periodic forcing and work","E(t) + loss(t) = E(0) + work(t)","Drive a spring for at least 4 seconds, add net work above 0.1, and balance energy within 1e-5.","The forcing is F*cos(Omega*t). For the spring, H(s)=1/(s^2+c*s+k); convolving its impulse response with the drive gives the forced response. Compare it with the ODE and track work and loss. The nonlinear pendulum uses the ODE only."}
}};
constexpr std::array<MathLesson,4> modularLessons{{
  {"Residues","a = q*n + r; 0 <= r < n","With modulus 7, represent -3 by residue 4.","Negative integers wrap around the same drum. The table lists canonical residues and their neighbouring classes."},
  {"Cycles and GCD","cycle length = n / gcd(n,k)","Use a nonzero step and visit every residue by stepping around the drum.","Forward and backward steps update the observed walk. A step coprime to n reaches every class. Reset walk returns to zero; at most 64 steps are retained."},
  {"Multiplicative inverses","k*u = 1 (mod n) iff gcd(k,n)=1","Find the inverse of 3 modulo 7.","Try an inverse and read its product. The table shows multiplication by k. When k and n have a common factor, an inverse does not exist."},
  {"Chinese remainders","x = a (mod n), x = b (mod m)","Find the smallest nonnegative solution for two distinct moduli with period at least 6.","The common period is lcm(n,m). A solution exists exactly when the residues agree modulo gcd(n,m); noncoprime and incompatible pairs are supported."}
}};
constexpr std::array<MathLesson,4> gaussianLessons{{
  {"Complex integer operations","(a+bi)(c+di) = (ac-bd) + (ad+bc)i","Multiply two non-real inputs to obtain a nonzero real result.","The grey grid is the original lattice. In Multiply mode the teal grid is its image under multiplication by w. The table links z, w and the selected result. Zero multiplication collapses the image."},
  {"Norm and Euclidean division","N(z*w)=N(z)N(w); z=q*w+r, N(r)<N(w)","Divide a nonzero z by a nonunit w exactly, with quotient norm greater than 1.","For nonzero w, round each component of z/w to the nearest integer (ties away from zero). Division by zero is explicitly undefined. A zero remainder means divisibility."},
  {"Principal ideals","(w) = {w*q : q in Z[i]}","Find a nonzero multiple of i*w for a generator with norm at least 2.","Gold grid points belong to the ideal generated by w. Its cell is spanned by w and i*w and has area N(w). For w=0 the ideal contains only zero and has no finite-area quotient cell."},
  {"Quotient rings","z and v represent the same class iff g divides z-v","In Z[i]/(2), multiply two nonzero classes to get zero.","Equal colours indicate equal classes. Representatives lie in a half-open fundamental cell. Compare four classes modulo 2, five modulo 2+i and nine modulo 3; the latter two examples are fields."}
}};
constexpr std::array<MathLesson,4> fieldLessons{{
  {"Vectors and dot products","component along d = F dot d, |d|=1","Find a direction perpendicular to a nonzero radial field vector.","Move the probe and rotate its unit direction. A zero component means the field has no component along that direction."},
  {"Parameterised paths","velocity = r'(t); unit tangent = r'(t)/|r'(t)|","On the upper semicircle, reach its top with the tangent pointing right.","The path owns the probe position. Play or scrub t. Reverse changes orientation while preserving the current world position. The 3D arch has the same endpoints as the planar arcs."},
  {"Line integrals","W(t) = integral[0,t] F(r(u)) dot r'(u) du","Reverse the closed vortex path, finish it, and obtain negative work.","The integrand includes path speed. Simpson integration is checked against the exact formula for these examples. Reversing a complete path negates its integral."},
  {"Path dependence","gradient fields give potential(end)-potential(start)","In the vortex field, find a route whose work differs from the straight comparison by more than 2.","The comparison has the same endpoints; for a closed path it stays at the endpoint. Constant and radial fields have global potentials. The vortex has curl (0,0,2), so its work can depend on the route."}
}};
constexpr std::array<MathLesson,4> fluxLessons{{
  {"Surface orientation","local flux density = F dot n","Turn the radial sphere's normals inward.","The gold unit normal chooses the sign. The probe table links position, field and normal. Reversing orientation negates flux."},
  {"Surface integrals","flux is the sum of F dot n times area","Get positive flux through an open disk in the constant field.","Resolution changes midpoint quadrature, not the displayed mesh. Sphere samples have equal area; box faces and disk annuli use exact area weights."},
  {"Divergence theorem","outward flux = integral of div(F) over the enclosed volume","On the outward unit sphere in the twist field, reduce relative error below 1 percent.","Only closed shells enclose a volume. For inward normals compare against the negative volume integral. An open disk does not qualify for this theorem."},
  {"Stokes and boundaries","boundary circulation = integral of curl(F) dot n over the disk","Finish a reversed vortex boundary and get negative circulation.","The disk boundary follows its normal by the right-hand rule. Reverse the normal to reverse both signs. Play or scrub along the boundary; curl flux is compared with the line integral."}
}};
constexpr std::array<MathLesson,4> tensorLessons{{
  {"Outer products","A[i,j] = u[i] v[j]; trace(A) = u dot v","Make a nonzero outer product with zero trace.","Each block is one component. Teal is positive, coral is negative, and flat grey cells are zero. Gold identifies the selected indices."},
  {"Three-index tensors","T[i,j,k] = u[i] v[j] w[k]","Make the selected three-index component negative using three nonzero factors.","The three slices carry the k index. Changing one factor scales its corresponding family of components. Values in the table and slice matrices use the same indices."},
  {"Contraction","c[i] = sum_j T[i,j,j] = u[i] (v dot w)","Keep a nonzero tensor while making its contraction zero.","The Euclidean inner product identifies vectors with covectors for this contraction. Gold diagonal cells are summed over j and k. Orthogonal v and w can cancel the contraction without making the tensor zero."},
  {"Changing coordinates","A' = Q^T A Q, with Q orthogonal","Rotate the basis so components change while trace and Frobenius norm are preserved.","Q columns are the new basis in world coordinates. Vector coordinates become Q^T u and Q^T v. This layer treats u v^T as a linear map using the Euclidean inner product; the map is unchanged."}
}};
constexpr std::array<MathLesson,4> probabilityLessons{{
  {"Sampled random walks","choose the next state from the current row of P","Visit A, B and C in an actual walk.","Next walk step consumes one reproducible random draw. Reset starts from the chosen state and seed. Changing the setup clears the observed path; the walk is capped at 64 transitions."},
  {"Transition matrices","P[i,j] >= 0 and every row sums to 1","In weighted mixing at retention 0.5, inspect row C: (0.25, 0.15, 0.60).","The selected row is a one-step probability distribution. Arrow widths match its outgoing probabilities. Row vectors multiply P on the right."},
  {"Distribution evolution","p(n) = p(0) P^n","Start away from C and move over 95 percent of the mass to C in the absorbing chain.","Node volumes show the complete distribution, not one random draw. The plot records every integer transition. The initial mixture is a convex combination of the chosen state and a uniform distribution."},
  {"Stationarity and convergence","pi P = pi; distance = 0.5 sum_i |p[i]-pi[i]|","In weighted mixing, evolve at least 10 steps to within 0.001 of the stationary distribution.","The periodic cycle has a stationary distribution but usually never settles. At retention 1 the matrix is the identity and stationary distributions are not unique. The shown pi is then only one valid stationary example."}
}};
constexpr std::array<MathLesson,4> binomialLessons{{
  {"Independent trials","X is the sum of n independent Bernoulli(p) outcomes","Complete a path with at least one success and one failure.","Next trial moves the bead left for failure or right for success. Reset replays the seed. Changing n, p or seed starts a new path; the PMF remains a distribution, not an observed histogram."},
  {"Binomial mass","P(X=k) = choose(n,k) p^k (1-p)^(n-k)","With n=6 and p=0.5, inspect the central outcome k=3.","Bar heights are exact probabilities. The table also accumulates P(X<=k). A threshold above n has CDF 1 and point mass 0."},
  {"Mean, spread and tails","E[X]=n*p; Var(X)=n*p*(1-p)","Choose a nondegenerate model and a threshold whose upper tail is below 0.1.","Gold marks the mean, and blue marks one standard deviation either side. The upper tail is P(X>k), so it complements the displayed CDF exactly."},
  {"Normal approximation","P(X<=k) is approximated by Phi((k+0.5-n*p)/sqrt(n*p*(1-p)))","Use n=12 and p=0.5 to get maximum CDF error below 0.02.","Continuity correction includes half a count. Normal probability outside the finite binomial support is reported, not silently renormalized. At p=0 or 1 the variance is zero and no normal approximation is defined."}
}};
constexpr std::array<MathLesson,4> bayesLessons{{
  {"Joint probability","P(H and E) = P(H) P(E | H)","Make H and E independent with a nonzero joint probability.","Cube widths encode the prior; each column divides by its event likelihood. All four pieces sum to one. Equal event likelihoods make the event independent of the hypothesis."},
  {"Conditioning","P(H | E) = P(H and E) / P(E)","Choose evidence that raises the probability of H by more than 0.2.","The right cube rescales the selected event to total probability one. Conditioning on probability-zero evidence is undefined and the second cube is omitted."},
  {"Bayesian odds","posterior odds = prior odds times likelihood ratio","Overcome a prior below 0.5 and obtain posterior probability above 0.8.","The likelihood ratio measures evidence, while prior odds represent the starting balance. Infinite and undefined ratios are marked explicitly."},
  {"Repeated evidence","P(H | data) is proportional to P(H) times the data likelihood","Use at least three E outcomes to obtain posterior probability above 0.95.","Outcomes are independent conditional on H and on not-H. The plot places E outcomes first, then not-E; the final result is order-independent under this model. It stops at the first impossible prefix."}
}};
constexpr std::array<MathLesson,4> covarianceLessons{{
  {"Mean and centering","mean = sum of eight equally weighted points / 8","Move a nontrivial cloud to mean (0.5,-0.5,0).","The gold point is the population mean. Moving the mean translates every point equally. The table contains the actual point coordinates."},
  {"Covariance and correlation","Cov[i,j] = mean[(x[i]-mean[i])*(x[j]-mean[j])]","Create x-y correlation greater than 0.5 with both variances nonzero.","Changing stretches, shear and rotation changes the covariance. Correlation is undefined if either selected coordinate has zero variance. This is population covariance, not the unbiased sample estimator."},
  {"Principal components","Cov * q = lambda * q","Collapse one principal variance to zero while retaining rank 2.","The principal directions are orthonormal, ordered by decreasing variance. Blue points are orthogonal projections onto the selected principal axis. Equal eigenvalues allow more than one valid basis."},
  {"Whitening","y = diag(1/sqrt(lambda)) * Q^T * (x-mean)","Whiten a full-rank cloud until its covariance differs from identity by less than 1e-8.","The left cloud is centered input. The right starts in principal coordinates and rescales each axis towards unit variance. Whitening needs positive variance in every principal direction. Singular clouds remain valid examples, but the inverse and transformed cloud are omitted."}
}};
constexpr std::array<MathLesson,4> sphericalLessons{{
  {"Spherical coordinates","(x,y,z)=(sin(theta)cos(phi), sin(theta)sin(phi), cos(theta))","Place the probe on the positive y axis.","Theta starts at the north pole; phi turns about z. Gold marks the selected unit direction. Linked meridian and latitude plots can move the angular probe."},
  {"Real harmonic modes","Integral over S^2 of Y(l,m)^2 = 1","Choose degree 3 and absolute order 2.","Browse all 25 real modes through degree 4. Positive m means cosine; negative m means sine. Teal is positive, coral negative. The small base radius keeps nodal directions visible; it is not a physical radial wave."},
  {"Superposition and orthogonality","f = Y(a) + c*Y(b); distinct modes have inner product zero","Combine distinct modes with a nonzero second coefficient.","Eight Gauss-Legendre latitude nodes and sixteen azimuth samples measure inner products independently of the analytic mode labels. Equal modes combine coherently: energy is (1+c)^2, not 1+c^2."},
  {"Heat flow on the sphere","Delta Y(l,m) = -l(l+1)Y(l,m); coefficient(t)=coefficient(0)*exp(-l(l+1)t)","Diffuse two nonconstant modes past t=0.2 until energy falls below half its initial value.","Higher degrees decay faster. The constant mode remains unchanged. The energy plot uses the full sphere; a finite-difference probe separately checks the spherical Laplacian away from coordinate poles."}
}};
constexpr std::array<MathLesson,4> quadraticLessons{{
  {"Quadratic evaluation","q(x)=x^T A x","Find a probe where q is negative.","The surface is a height graph for the slice z=z0. The point's first two coordinates locate it on the graph; its height is 0.15*q. The table decomposes q into principal-coordinate contributions."},
  {"Orthogonal diagonalization","A=Q D Q^T; y=Q^T x; q=sum(lambda_i*y_i^2)","Rotate the basis away from identity while preserving the two evaluations of q.","The colored arrows are the columns of Q. Matrices show A, Q and D. Rotating coordinates changes matrix entries but preserves eigenvalues, trace, determinant and inertia."},
  {"Inertia and zero sets","inertia=(number positive, number negative, number zero)","Create one positive, one negative and one zero principal value.","Opposite signs make a form indefinite even when the selected slice misses its zero set. Pale curves in the contour tab approximate q(x,y,z0)=0 within the displayed square. The zero form has an entire zero plane, not an isolated contour."},
  {"Rayleigh quotient","R(x)=x^T A x/(x^T x), between the smallest and largest eigenvalues","Place a nonzero probe in a minimum-eigenvalue direction of a nonconstant form.","The unit sphere is colored by R. Gold is the normalized probe; arrows show principal directions. Minimum and maximum occur in the corresponding eigenspaces. At x=0 the quotient is undefined and no probe is shown."}
}};
constexpr std::array<MathLesson,4> rootsLessons{{
  {"Root constellation","z^n=1; order(zeta^k)=n/gcd(n,k)","Choose a primitive root for a composite n.","The points divide the unit circle equally. Gold marks the probe and teal marks other primitive roots. The table retains complex coordinates and multiplicative order."},
  {"Complex power maps","(zeta^k)^a=zeta^(a*k mod n)","Choose a power map that has collisions but does not collapse every root to one.","The lower ring is the domain and the upper ring the image positions. Connections show powers. There are n/gcd(a,n) distinct images, each with gcd(a,n) preimages."},
  {"Cyclic subgroups","<zeta^a> has n/gcd(n,a) elements","Choose a nontrivial proper subgroup and return to identity after a positive number of steps.","The rising path records successive powers of the generator. The last point closes the cycle in the complex plane while its height records the step. The selected power is reduced using the group law."},
  {"Cyclotomic field symmetries","Gal(Q(zeta_n)/Q) is the unit group modulo n","For n=8 choose a nonidentity valid automorphism that fixes more than two roots.","Only coprime exponents define field automorphisms. Their composition multiplies exponents modulo n. Primitive roots are precisely the zeros of Phi_n; the linked polynomial plot shows its real and imaginary parts along the unit circle."}
}};
constexpr std::array<MathLesson,4> psdLessons{{
  {"Matrices in the cone","PSD iff a>=0, c>=0 and a*c-b*b>=0","Place a nonzero matrix on the PSD boundary.","Three independent matrix entries become one point. Teal means inside, gold marks a PSD boundary and coral means outside. The tip is the zero matrix. All-positive entries can still give an indefinite matrix; try that preset."},
  {"Eigenvalues and quadratic directions","lambda = t +/- sqrt(x*x+y*y); q(u)=u^T A u","Find a unit direction where q is less than -0.1.","The companion surface is q(u,v), drawn with height 0.3*q. A positive definite bowl becomes flat in one direction on the rank-one boundary. Indefinite matrices have both signs; negative definite matrices give a downward bowl. The eigenvector basis is one valid choice when eigenvalues repeat."},
  {"Convex mixtures and cone rays","M = s*((1-alpha)*A + alpha*B), s>=0, 0<=alpha<=1","Mix two rank-one PSD endpoints to obtain a positive definite matrix.","B=2*v*v^T is always rank one. Move alpha along the blue-violet segment, then scale its gold point along a ray from zero. A remains editable: convexity requires both endpoints to be PSD. Nonparallel rank-one directions with an interior mixture give a positive definite result."},
  {"Slices and supporting objectives","trace(A)=2*t; maximize cos(phi)*x + sin(phi)*y over x*x+y*y<=t*t","For a positive slice height, move the objective plane to its maximum feasible value.","The fixed-trace slice is a disk. The white ring is its edge; gold is the maximizing matrix, with value t. The violet rectangle is a movable objective plane, not an extra constraint. At fraction 1 it supports the disk at the optimum; beyond 1 it misses the disk. At t=0 the feasible set is a single point."}
}};
constexpr std::array<MathLesson,4> normLessons{{
  {"Unit distance and shape","||x||_p = (sum |x_i|^p)^(1/p)","At p=1, put a vector with at least two nonzero components on the unit boundary.","The octahedron, sphere and infinity-norm cube all describe unit distance under different rules. Gold is x; the blue point normalizes a nonzero x onto the boundary. Choose open cross-sections to see points inside the body. Normalization is undefined at zero and its point is omitted."},
  {"Triangle inequality","||x+y|| <= ||x|| + ||y||","Create a strict triangle inequality with both vectors nonzero.","Gold is x, violet is y translated to the tip of x, and teal is their sum. The unit body uses open cross-sections at the origin so it cannot hide the vector addition. Tables compare the same vectors under different norms; the plot compares both sides as finite p changes."},
  {"The limit as p grows","max|x_i| <= ||x||_p <= 3^(1/p)*max|x_i|","With at least two nonzero components, use finite p>=8 and get within 0.05 of the max norm.","Keep x fixed while moving p. The wire cube is the exact limit. The two bounds squeeze the norm towards the largest component; dimension stays fixed at three. Exact infinity mode uses max directly and has no finite-p graph marker."},
  {"Dual norms and supporting planes","max_{||x||_p<=1} w dot x = ||w||_q; 1/p+1/q=1","Choose nonzero w and move the plane to its tight support value.","The vector controls now specify w. The left body uses p; the wire body on the right uses its dual q. Gold is a maximizing x, and the right-hand point is w/||w||_q. Move the plane to fraction 1. At p=1 the dual is infinity, and at infinity it is 1. A zero w has no unique supporting direction; ties may have many maximizers."}
}};
constexpr std::array<MathLesson,4> curveLessons{{
  {"Control points and interpolation","r(t) = (1-t)^3 P0 + 3t(1-t)^2 P1 + 3t^2(1-t) P2 + t^3 P3","Choose t between 0.2 and 0.8 and make the control tetrahedron non-coplanar.","The control polygon guides the curve. Blue points interpolate its edges, violet points interpolate those points, and gold is their final interpolation. With four non-coplanar controls and interior t, the positive Bernstein weights place r(t) inside their tetrahedron. Start by changing one handle's z coordinate."},
  {"Tangents, speed and curvature","speed=|r'(t)|; curvature=|r'(t) cross r''(t)|/|r'(t)|^3","Find a regular point whose curvature is greater than 0.5.","Gold follows the tangent direction. The coral normal points towards the local bend, when curvature is nonzero. At a zero derivative the tangent and curvature are undefined; their arrows and curvature value are omitted. A straight regular curve has curvature zero and still has a tangent."},
  {"Travel by distance","s(t)=integral[0,t] |r'(u)| du; equal-distance travel inverts s(t)","Use distance travel on a curve with unequal parameter-step distances; reduce relative distance spread below 0.0001.","The two copies compare eight equal parameter intervals with eight equal arc-length intervals. Their straight chords need not have equal lengths: the table measures distance along the curve. The moving probes use the same progress. Playback takes four seconds for a complete trip. A constant curve has no distance parameterization."},
  {"Profiles, taper and twist","S(s,theta)=r(t(s)) + radius(s)*(u(theta)*N(s)+aspect*v(theta)*B(s))","Create a regular sweep with end radius at most one quarter of the start and at least 90 degrees of twist.","A transported frame carries the profile without the Frenet frame's flip at a straight point. Radius and added twist vary with distance. Circle, square and norm profiles reuse the norm-ball construction. The ribbon preset flattens a square section; the horn ends at a tip. Shape only hides the guides. Stationary tangents disable the sweep, while the curve remains editable."}
}};
constexpr std::array<MathLesson,4> latheLessons{{
  {"Shape the profile","R(h) is a piecewise cubic Bezier profile","Make an interior radius at least 0.2 larger than both endpoint radii.","Seven profile points shape the base, belly, neck, stem and rim. Radius never becomes negative. Heights stay ordered; the first and last points stay at the endpoints. Blue is the outer profile, violet the cavity wall. Hollow interiors begin above the cavity floor."},
  {"Revolve the profile","V(angle) = angle/(2*pi) * V(full turn)","Make a half revolution of a nonzero solid.","Play rotates the generating profile through a full turn in four seconds. Rings trace circles around the height axis. A partial revolution has two closing radial faces. Cutaway removes part of the view, while readouts keep the original angle's volume and surface area."},
  {"Disks, washers and shells","V = integral pi*(R^2-r^2) dh = integral 2*pi*q*L(q) dq","Using at least 16 subdivisions, approximate a nonzero volume within 2 percent.","The highlighted element is a disk or washer at one height, or a shell at one radius. A shell may occupy several separate height intervals; all contribute. The table sums every midpoint element. The wire profile shows the full shape. The reference volume integrates each cubic piece's squared radius."},
  {"Surface bands and normals","A_lateral = integral 2*pi*R(h)*sqrt(1+R'(h)^2) dh","With at least 16 subdivisions, approximate the boundary area within 1 percent at a regular surface point.","Gold highlights an outer band. Frustum bands approximate outer and inner lateral area; readouts also include the base, rim, cavity floor and any physical radial faces. The normal is perpendicular to the meridian and circular tangent. On the axis the parameterized normal is undefined. Cutaway faces are for viewing only."}
}};
constexpr std::array<MathLesson,4> booleanLessons{{
  {"Inside and outside","F(p)<0: interior; F(p)=0: boundary; F(p)>0: exterior","With subtraction selected, put the probe strictly inside both A and B, in the removed material.","Move B or the gold probe. The graph is a line through the probe's y and z coordinates. Its three curves evaluate A, B and the result at the same points. Section view exposes the interior without changing the defining function. Cylinders and arch openings extrude along their local z axis; pitch is applied before yaw: R_y(yaw) R_x(pitch)."},
  {"Sets and Boolean logic","Union: A OR B; intersection: A AND B; difference: A AND NOT B","Choose intersection and put the probe strictly inside both inputs.","The table lists all four input combinations; Probe match selects the current row away from boundaries. On an input boundary there is no active binary row. Smooth union uses the hard-union table as a baseline: blending can add material outside both inputs. The displayed surface encloses the sampled negative region, so isolated zero-thickness contacts have no material volume."},
  {"Blends and surface normals","smin(a,b)=h*a+(1-h)*b-k*h*(1-h); h=clamp(1/2+(b-a)/(2k),0,1)","Use a positive smooth blend to add material outside both inputs; find a regular surface on the probe line.","Blend width k rounds the meeting region; k=0 gives ordinary union. The gradient points toward increasing field values. A unit surface normal is shown at the nearest detected crossing on the x-directed probe line, when regular. Sharp switches, primitive ridges and zero gradients do not have a unique normal. The table keeps the gradient magnitude instead of treating the field as an exact distance."},
  {"Sampling solids","V_n = occupied midpoint cells * cell volume","Request at least 24 cells per axis for a nonzero solid; make the 48- and 64-cell volume estimates agree within 3 percent.","The mesh and the midpoint volume sum are separate approximations in the same fixed domain. The table compares increasingly fine grids with the 64-cell estimate. Agreement is evidence, not a certified error bound, and errors need not decrease at every count. A gold cell follows the probe. Requested and actual mesh counts are shown if the fixed mesh budget reduces resolution. Section view changes visible mesh volume only."}
}};
constexpr std::array<MathLesson,4> rigidLessons{{
  {"Orientation and frames","v_world = R(q) v_body; qdot = q*(0,omega_body)/2","Make the tracked axis perpendicular to its initial world direction.","Coral, teal and blue are body X,Y,Z; muted arrows are fixed world axes. The gold tip tracks the chosen body axis. Release angles apply fixed X, then Y, then Z; quaternion (w,x,y,z) maps body coordinates into world coordinates. R is a proper rotation. The trail covers recent motion. Shape markings carry no mass."},
  {"Mass and inertia","I_COM = sum [I_part + m*(dot(r,r)*Id - r*r^T)]","Use unequal end masses or panels to move the assembly centre of mass by more than 0.05.","Dimensions are full body-space extents. Homogeneous boxes and an elliptical cylinder supply exact component moments. Parts meet without overlapping interiors. Left mass share changes the dumbbell weights or satellite panels; total mass stays fixed. Rotation is about the resulting centre of mass, shown in gold. The grey point marks the original assembly origin. Body axes are principal axes; the world tensor changes as the body turns."},
  {"Angular motion","L_body = I_body*omega_body; L_world = R*L_body","Make angular velocity and momentum differ in direction by more than 10 degrees.","Gold is angular momentum; teal is angular velocity. These two arrows use equal display lengths so their directions can be compared; the table gives their actual components and magnitudes. Free motion keeps world angular momentum fixed while body components change. Graphs resolve a local time window; scrubbing always starts from the same release conditions."},
  {"Stability and conservation","dL_body/dt = L_body cross (I_body^-1 L_body); E = omega dot L / 2","Flip the initially aligned intermediate axis past alignment -0.8 while preserving energy and world momentum within 0.001 percent.","Choose Tumbling book and play through 4 model seconds. A small perturbation near the intermediate principal axis grows into a flip. Spins near the smallest or largest distinct moment are stable. Exactly repeated moments have no unique intermediate axis. Alignment compares the tracked axis with the INITIAL world momentum; it is undefined at rest. The solver uses bounded RK4 with unit-quaternion normalization. Conservation errors measure numerical drift, not physical dissipation; they are shown as zero for exact rest. Editing any control pauses playback."}
}};
constexpr std::array<MathParameter,MathObjectPreset::kCapacity> rigidPresetParameters{MathParameter::RigidShape,MathParameter::RigidWidth,MathParameter::RigidHeight,MathParameter::RigidDepth,MathParameter::RigidMass,MathParameter::RigidBalance,MathParameter::RigidRotX,MathParameter::RigidRotY,MathParameter::RigidRotZ,MathParameter::RigidSpinX,MathParameter::RigidSpinY,MathParameter::RigidSpinZ,MathParameter::RigidTime,MathParameter::RigidAxis,MathParameter::RigidGuides};
constexpr std::array<MathObjectPreset,4> rigidPresets{{
  {"Flywheel",rigidPresetParameters,{0,2.4,.3,2.4,1,.5,0,0,0,0,3,0,0,0,1},15},
  {"Adjustable dumbbell",rigidPresetParameters,{1,3,.7,.7,1,.5,0,0,0,.8,2,0,0,0,1},15},
  {"Tumbling book",rigidPresetParameters,{2,2.4,.3,1.6,1,.5,0,0,0,.02,.02,3,0,2,1},15},
  {"Satellite",rigidPresetParameters,{3,3.4,1,1.4,1,.5,15,0,20,1.2,.3,1.8,0,2,1},15}
}};
constexpr std::array<MathLesson,4> membraneLessons{{
  {"Displacement and motion","h = sum q_mn(t)*sin(m*pi*u)*sin(n*pi*v)","At positive time, make the combined probe displacement less than -0.05.","Play, pause or scrub time. Coral is positive displacement, blue negative. The gold point follows the displayed surface; all numerical probe readings refer to the combined membrane. Initial displacement and velocity belong to the selected mode slot. Edits recompute the chosen time from those initial conditions and pause playback."},
  {"Modes and nodal lines","omega_mn = pi*sqrt(T/rho)*sqrt((m/width)^2+(n/depth)^2)","Find an interior node of an excited selected slot; do not use the fixed boundary.","Choose a mode slot and edit m,n. Its internal zero lines are u=k/m and v=k/n. Gold reference lines lie on the equilibrium plane and refer to that slot, not generally to the combined membrane. Choose Selected slot in Surface to isolate it. Basis colour is time independent, so an instant of zero amplitude is not confused with a spatial node. Frequencies in the table are undamped natural frequencies."},
  {"Superposition","h_total = h_selected + h_other; distinct modes are orthogonal over the rectangle","Make distinct modes cancel at the probe: |h_total| < 0.005 with cancelling contributions above 0.1.","The plots compare the combined displacement, selected slot and remaining slots at the same point or section. Cancellation at one point need not persist in time. The numerical cancellation measure groups equal mode pairs first. Equal frequencies do not make two different spatial modes identical. The Damped pluck preset is a four-term approximation of a centred tent, released from rest."},
  {"Energy and damping","E = integral [rho*h_t^2 + T*|grad h|^2]/2; dE/dt = -4*gamma*K","With positive damping and nonzero initial energy, advance to at least 3 seconds and retain less than 25 percent of the initial energy.","Kinetic and strain energy exchange while total energy stays constant for gamma=0. Positive damping removes energy. Global energies integrate the combined membrane analytically, independently of mesh resolution. Duplicate mode pairs combine coherently before squaring. The energy table lists distinct pairs; colours show local energy density of the displayed surface. Time plots adapt their window to resolve the mode frequencies."}
}};
constexpr std::array<MathParameter,MathObjectPreset::kCapacity> membranePresetParameters{MathParameter::MembraneSlot,MathParameter::MembraneM0,MathParameter::MembraneN0,MathParameter::MembraneA0,MathParameter::MembraneV0,MathParameter::MembraneM1,MathParameter::MembraneN1,MathParameter::MembraneA1,MathParameter::MembraneV1,MathParameter::MembraneM2,MathParameter::MembraneN2,MathParameter::MembraneA2,MathParameter::MembraneV2,MathParameter::MembraneM3,MathParameter::MembraneN3,MathParameter::MembraneA3,MathParameter::MembraneV3,MathParameter::MembraneWidth,MathParameter::MembraneDepth,MathParameter::MembraneTension,MathParameter::MembraneDensity,MathParameter::MembraneDamping,MathParameter::MembraneTime,MathParameter::MembraneU,MathParameter::MembraneV,MathParameter::MembraneResolution,MathParameter::MembraneGuides,MathParameter::MembraneView};
constexpr std::array<MathObjectPreset,4> membranePresets{{
  {"Drumhead",membranePresetParameters,{0,1,1,0.35,0,2,1,0,0,1,2,0,0,2,2,0,0,3,3,1,1,0,0,0.5,0.5,32,1,0},28},
  {"Divided membrane",membranePresetParameters,{0,2,3,0.35,0,1,1,0,0,1,2,0,0,2,2,0,0,3,3,1,1,0,0,0.5,0.5,32,1,0},28},
  {"Interference",membranePresetParameters,{0,1,1,0.3,0,3,1,-0.3,0,1,2,0,0,2,2,0,0,3,3,1,1,0,0,0.25,0.5,32,1,0},28},
  {"Damped pluck",membranePresetParameters,{0,1,1,0.3285114321498988,0,1,3,-0.03650127023887765,0,3,1,-0.03650127023887765,0,3,3,0.004055696693208627,0,3,3,1,1,0.35,0,0.5,0.5,32,1,0},28}
}};
constexpr std::array<MathLesson,4> patchLessons{{
  {"Control net and blending","S(u,v) = sum B_i(u) B_j(v) P_ij; weights >= 0 and sum to 1","Select a corner control and move the UV probe onto that corner, so its weight becomes 1.","Click a control or choose P00 through P33; edit its XYZ row. The first index follows u and the second follows v. Interior controls influence the sheet without generally lying on it. Gold tint shows the selected control's influence; red and blue curves trace u and v through the gold probe."},
  {"Tangents and normals","N = (S_u cross S_v) / |S_u cross S_v|","At a regular probe, make the unit normal nearly horizontal: |N_y| < 0.2.","Red is the u direction, blue is v and gold is the normal. Arrows use fixed display lengths; the table retains actual derivatives. The framed tangent plane uses an orthonormal basis. A singular probe has no normal or tangent plane. Try the Sail preset to inspect a nearly vertical sheet."},
  {"Curvature and metric","K = (e*g-f*f)/(E*G-F*F); H = (e*G-2*f*F+g*E)/(2*(E*G-F*F))","Find a regular saddle point with Gaussian curvature below -0.03.","The first fundamental form measures tangent lengths; the second measures normal bending. Opposite principal-curvature signs identify a saddle. Coral marks negative K, teal positive K, grey near-zero or undefined K. H changes sign if normal orientation reverses; K does not. Curvature traces are omitted when any sampled point on their line is singular."},
  {"Area and mesh refinement","Area = integral integral |S_u cross S_v| du dv","Use at least 24 subdivisions; make mesh area agree with the quadrature estimate within 0.5 percent, with fine/coarse quadrature agreement within 0.1 percent.","A parameter cell becomes two triangles. Gold edges mark the probe's cell; colour shows area density. The graph compares triangulated area as resolution increases. Quadrature uses separate 8- and 16-cell Gauss grids. Their agreement is not a certified error bound. Folded patches count overlapping sheets separately; collapsed triangles are omitted."}
}};
constexpr std::array<MathParameter,MathObjectPreset::kCapacity> patchPresetParameters{MathParameter::PatchControl,MathParameter::PatchP00X,MathParameter::PatchP00Y,MathParameter::PatchP00Z,MathParameter::PatchP01X,MathParameter::PatchP01Y,MathParameter::PatchP01Z,MathParameter::PatchP02X,MathParameter::PatchP02Y,MathParameter::PatchP02Z,MathParameter::PatchP03X,MathParameter::PatchP03Y,MathParameter::PatchP03Z,MathParameter::PatchP10X,MathParameter::PatchP10Y,MathParameter::PatchP10Z,MathParameter::PatchP11X,MathParameter::PatchP11Y,MathParameter::PatchP11Z,MathParameter::PatchP12X,MathParameter::PatchP12Y,MathParameter::PatchP12Z,MathParameter::PatchP13X,MathParameter::PatchP13Y,MathParameter::PatchP13Z,MathParameter::PatchP20X,MathParameter::PatchP20Y,MathParameter::PatchP20Z,MathParameter::PatchP21X,MathParameter::PatchP21Y,MathParameter::PatchP21Z,MathParameter::PatchP22X,MathParameter::PatchP22Y,MathParameter::PatchP22Z,MathParameter::PatchP23X,MathParameter::PatchP23Y,MathParameter::PatchP23Z,MathParameter::PatchP30X,MathParameter::PatchP30Y,MathParameter::PatchP30Z,MathParameter::PatchP31X,MathParameter::PatchP31Y,MathParameter::PatchP31Z,MathParameter::PatchP32X,MathParameter::PatchP32Y,MathParameter::PatchP32Z,MathParameter::PatchP33X,MathParameter::PatchP33Y,MathParameter::PatchP33Z,MathParameter::PatchU,MathParameter::PatchV,MathParameter::PatchResolution,MathParameter::PatchGuides};
constexpr std::array<MathObjectPreset,4> patchPresets{{
  {"Canopy",patchPresetParameters,{5,-1.5,0,1.5,-1.5,0.45,0.5,-1.5,0.45,-0.5,-1.5,0,-1.5,-0.5,0.65,1.5,-0.5,1.1,0.5,-0.5,1.1,-0.5,-0.5,0.65,-1.5,0.5,0.65,1.5,0.5,1.1,0.5,0.5,1.1,-0.5,0.5,0.65,-1.5,1.5,0,1.5,1.5,0.45,0.5,1.5,0.45,-0.5,1.5,0,-1.5,0.5,0.5,20,1},53},
  {"Sail",patchPresetParameters,{5,-1.4,-1.3,0.1,-1.4,-0.43,0.1,-1.4,0.43,0.1,-1.4,1.3,0.1,-0.47,-1.3,0.1,-0.47,-0.43,1,-0.47,0.43,1,-0.47,1.3,0.1,0.47,-1.3,0.1,0.47,-0.43,1,0.47,0.43,1,0.47,1.3,0.1,1.4,-1.3,0.1,1.4,-0.43,0.1,1.4,0.43,0.1,1.4,1.3,0.1,0.5,0.5,20,1},53},
  {"Curved ramp",patchPresetParameters,{5,-1.5,-0.9,1.5,-1.5,-0.9,0.5,-1.5,-0.9,-0.5,-1.5,-0.9,-1.5,-0.5,-0.9,1.5,-0.5,-0.9,0.5,-0.5,-0.9,-0.5,-0.5,-0.9,-1.5,0.5,0.9,1.5,0.5,0.9,0.5,0.5,0.9,-0.5,0.5,0.9,-1.5,1.5,0.9,1.5,1.5,0.9,0.5,1.5,0.9,-0.5,1.5,0.9,-1.5,0.5,0.5,20,1},53},
  {"Saddle terrain",patchPresetParameters,{5,-1.5,0.9,1.5,-1.5,0.3,0.5,-1.5,-0.3,-0.5,-1.5,-0.9,-1.5,-0.5,0.3,1.5,-0.5,0.1,0.5,-0.5,-0.1,-0.5,-0.5,-0.3,-1.5,0.5,-0.3,1.5,0.5,-0.1,0.5,0.5,0.1,-0.5,0.5,0.3,-1.5,1.5,-0.9,1.5,1.5,-0.3,0.5,1.5,0.3,-0.5,1.5,0.9,-1.5,0.5,0.5,20,1},53}
}};
constexpr std::array<MathParameter,MathObjectPreset::kCapacity> booleanPresetParameters{MathParameter::BooleanShapeA,MathParameter::BooleanShapeB,MathParameter::BooleanSizeA,MathParameter::BooleanSizeB,MathParameter::BooleanX,MathParameter::BooleanY,MathParameter::BooleanZ,MathParameter::BooleanYaw,MathParameter::BooleanPitch,MathParameter::BooleanOperation,MathParameter::BooleanBlend,MathParameter::BooleanProbeX,MathParameter::BooleanProbeY,MathParameter::BooleanProbeZ,MathParameter::BooleanResolution,MathParameter::BooleanGuides,MathParameter::BooleanSection,MathParameter::BooleanFit,MathParameter::BooleanClearance};
constexpr std::array<MathObjectPreset,4> booleanPresets{{
  {"Drilled block",booleanPresetParameters,{0,2,1,.55,0,0,0,0,0,2,.4,0,0,0,20,1,0,0,.08},19},
  {"Archway",booleanPresetParameters,{0,3,1,.75,0,-.1,0,0,0,2,.4,0,0,0,20,1,0,0,.08},19},
  {"Ball-and-socket",booleanPresetParameters,{0,1,1,.85,0,.2,.6,0,0,2,.4,.9,.2,.6,20,1,1,1,.08},19},
  {"Blended stones",booleanPresetParameters,{1,1,.9,.9,1.2,0,0,0,0,3,.5,.6,.8,0,20,1,0,0,.08},19}
}};
constexpr std::array<MathParameter,MathObjectPreset::kCapacity> lathePresetParameters{
  MathParameter::LatheR0,MathParameter::LatheR1,MathParameter::LatheR2,MathParameter::LatheR3,MathParameter::LatheR4,MathParameter::LatheR5,MathParameter::LatheR6,
  MathParameter::LatheH1,MathParameter::LatheH2,MathParameter::LatheH3,MathParameter::LatheH4,MathParameter::LatheH5,
  MathParameter::LatheHeight,MathParameter::LatheHollow,MathParameter::LatheWall,MathParameter::LatheFloor,MathParameter::LatheTurn,MathParameter::LatheCut,MathParameter::LatheProbe,MathParameter::LatheSlices,MathParameter::LatheMethod,MathParameter::LatheGuides,MathParameter::LatheControl
};
constexpr std::array<MathObjectPreset,4> lathePresets{{
  {"Vase",lathePresetParameters,{.65,.9,1.15,1,.55,.48,.62, .12,.35,.6,.8,.92, 3,1,.12,.08,360,25,.5,8,0,1,3},23},
  {"Bottle",lathePresetParameters,{.65,.75,.75,.5,.25,.25,.32, .08,.55,.75,.83,.96, 3.5,1,.1,.06,360,25,.5,8,0,1,4},23},
  {"Goblet",lathePresetParameters,{.9,.85,.18,.18,.55,.9,.85, .08,.18,.52,.64,.82, 3,1,.1,.6,360,25,.75,8,0,1,4},23},
  {"Chess pawn",lathePresetParameters,{.95,1,.45,.25,.6,.68,0, .08,.22,.55,.65,.83, 3,0,.12,.08,360,0,.5,8,0,1,3},23}
}};
constexpr std::array<std::array<MathParameter,3>,4> curveCoordinates{{
  {MathParameter::CurveP0X,MathParameter::CurveP0Y,MathParameter::CurveP0Z},
  {MathParameter::CurveP1X,MathParameter::CurveP1Y,MathParameter::CurveP1Z},
  {MathParameter::CurveP2X,MathParameter::CurveP2Y,MathParameter::CurveP2Z},
  {MathParameter::CurveP3X,MathParameter::CurveP3Y,MathParameter::CurveP3Z}
}};
constexpr std::array<MathParameter,MathObjectPreset::kCapacity> curvePresetParameters{
  MathParameter::CurveP0X,MathParameter::CurveP0Y,MathParameter::CurveP0Z,
  MathParameter::CurveP1X,MathParameter::CurveP1Y,MathParameter::CurveP1Z,
  MathParameter::CurveP2X,MathParameter::CurveP2Y,MathParameter::CurveP2Z,
  MathParameter::CurveP3X,MathParameter::CurveP3Y,MathParameter::CurveP3Z,
  MathParameter::CurveProfile,MathParameter::CurveRadius,MathParameter::CurveAspect,
  MathParameter::CurveEndScale,MathParameter::CurveTwist,MathParameter::CurveNormP,
  MathParameter::CurveGuides,MathParameter::CurveTravel,MathParameter::CurveProgress,MathParameter::CurveControl
};
constexpr std::array<MathObjectPreset,4> curvePresets{{
  {"Curved pipe",curvePresetParameters,{-1.5,0,0, -1,1.3,0, 1,1.3,0, 1.5,0,0, 0,.2,1,1,0,2, 1,0,.35,1},22},
  {"Arched cable",curvePresetParameters,{-1.5,0,0, -1.4,1.8,0, -.6,1.8,.4, 1.5,0,.4, 0,.08,1,1,0,2, 1,0,.35,1},22},
  {"Twisted ribbon",curvePresetParameters,{-1.5,0,0, -1,1.4,-.4, 1,-1,.6, 1.5,.2,.8, 1,.4,.1,1,180,2, 1,0,.35,1},22},
  {"Tapered horn",curvePresetParameters,{-1.2,-.8,0, -1.1,.6,0, .2,1.4,.2, 1.5,1.1,1.1, 2,.4,1,0,0,1.5, 1,0,.35,1},22}
}};
constexpr std::array<MathParameter,MathObjectPreset::kCapacity> psdEntries{MathParameter::PsdA,MathParameter::PsdB,MathParameter::PsdC};
constexpr std::array<MathObjectPreset,6> psdPresets{{
  {"Positive definite bowl",psdEntries,{1,0,1}},
  {"Rank-one boundary",psdEntries,{1,1,1}},
  {"Indefinite saddle",psdEntries,{1,0,-1}},
  {"Zero / cone tip",psdEntries,{0,0,0}},
  {"Positive entries, indefinite",psdEntries,{1,2,1}},
  {"Negative definite bowl",psdEntries,{-1,0,-1}}
}};
constexpr std::array<MathParameter,MathObjectPreset::kCapacity> normEntries{MathParameter::NormP,MathParameter::NormInfinity,MathParameter::Count};
constexpr std::array<MathObjectPreset,4> normPresets{{
  {"Octahedron: p=1",normEntries,{1,0,0},2},
  {"Sphere: p=2",normEntries,{2,0,0},2},
  {"Rounded cube: p=8",normEntries,{8,0,0},2},
  {"Exact cube: infinity norm",normEntries,{2,1,0},2}
}};
constexpr std::array<MathParameter,9> matrixParameters{MathParameter::A00,MathParameter::Shear,MathParameter::A02,MathParameter::A10,MathParameter::Scale,MathParameter::A12,MathParameter::A20,MathParameter::A21,MathParameter::A22};
constexpr std::array<std::string_view,8> nodeLabels{"A 000","B 001","C 010","D 011","E 100","F 101","G 110","H 111"};
constexpr std::array<std::string_view,4> termNames{"1","x","x^2","x^3"};
std::size_t index(auto value) { return static_cast<std::size_t>(value); }
bool edge(unsigned a,unsigned b,bool shortcut) {
  const auto bits=a^b;
  return (bits==1 || bits==2 || bits==4) || (shortcut && ((a==0 && b==7)||(a==7 && b==0)));
}
unsigned shortest(bool shortcut) {
  std::array<int,8> distance;distance.fill(-1);distance[0]=0;
  std::array<unsigned,8> queue{};unsigned tail=1;
  for(unsigned head=0;head<tail;++head) {
    const auto a=queue[head];
    for(unsigned b=0;b<8;++b) if(distance[b]<0 && edge(a,b,shortcut)) {
      distance[b]=distance[a]+1;queue[tail++]=b;
    }
  }
  return static_cast<unsigned>(distance[7]);
}
double functionDerivative(unsigned rule,double x,unsigned order=0) {
  switch(rule) {
    case 0: {const std::array<double,3> d{x*x,2*x,2};return order<d.size()?d[order]:0;}
    case 1: {const std::array<double,4> d{x*x*x-3*x,3*x*x-3,6*x,6};return order<d.size()?d[order]:0;}
    case 2:return std::sin(x+order*pi/2);
    case 3:return std::exp(x)-(order==0?1:0);
    default:throw std::logic_error("invalid function rule");
  }
}
double primitive(unsigned rule,double x) {
  switch(rule) {
    case 0:return x*x*x/3;
    case 1:return x*x*x*x/4-1.5*x*x;
    case 2:return -std::cos(x);
    case 3:return std::exp(x)-x;
    default:throw std::logic_error("invalid function rule");
  }
}
double taylor(unsigned rule,double x,double center,unsigned degree) {
  double sum=0,power=1;
  for(unsigned k=0;k<=degree;++k) {sum+=functionDerivative(rule,center,k)*power;power*=(x-center)/(k+1);}
  return sum;
}
struct SurfaceValue { double height,du,dv,duu,duv,dvv; };
SurfaceValue surfaceValue(unsigned rule,double u,double v) {
  switch(rule) {
    case 0:return {.5*(u*u+v*v),u,v,1,0,1};
    case 1:return {.5*(u*u-v*v),u,-v,1,0,-1};
    case 2: {const double f=std::sin(u)*std::cos(v);return {f,std::cos(u)*std::cos(v),-std::sin(u)*std::sin(v),-f,-std::cos(u)*std::sin(v),-f};}
    default:throw std::logic_error("invalid surface rule");
  }
}
using Matrix = std::array<double,9>;
constexpr Matrix identity{1,0,0,0,1,0,0,0,1};
Matrix transpose(const Matrix& a) {Matrix r{};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)r[3*i+j]=a[3*j+i];return r;}
Matrix multiply(const Matrix& a,const Matrix& b) {
  Matrix r{};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)for(unsigned k=0;k<3;++k)r[3*i+j]+=a[3*i+k]*b[3*k+j];return r;
}
Vec3 mapped(const Matrix& a,Vec3 v) {
  return {static_cast<float>(a[0]*v.x+a[1]*v.y+a[2]*v.z),static_cast<float>(a[3]*v.x+a[4]*v.y+a[5]*v.z),static_cast<float>(a[6]*v.x+a[7]*v.y+a[8]*v.z)};
}
double determinant(const Matrix& a) {return a[0]*(a[4]*a[8]-a[5]*a[7])-a[1]*(a[3]*a[8]-a[5]*a[6])+a[2]*(a[3]*a[7]-a[4]*a[6]);}
unsigned rank(Matrix a) {
  unsigned r=0;
  for(unsigned c=0;c<3&&r<3;++c) {
    unsigned pivot=r;for(unsigned i=r+1;i<3;++i)if(std::fabs(a[3*i+c])>std::fabs(a[3*pivot+c]))pivot=i;
    if(std::fabs(a[3*pivot+c])<1e-9)continue;
    for(unsigned j=0;j<3;++j)std::swap(a[3*r+j],a[3*pivot+j]);
    for(unsigned i=r+1;i<3;++i) {const double factor=a[3*i+c]/a[3*r+c];for(unsigned j=c;j<3;++j)a[3*i+j]-=factor*a[3*r+j];}
    ++r;
  }
  return r;
}
// Bounded Jacobi diagonalisation of A^T A. Columns are sorted by descending
// singular value; null columns of U are completed by deterministic Gram-Schmidt.
struct Svd { Matrix u=identity,v=identity;std::array<double,3> sigma{}; };
Svd svd(const Matrix& a) {
  Matrix gram=multiply(transpose(a),a),v=identity;
  for(unsigned sweep=0;sweep<24;++sweep)for(unsigned p=0;p<2;++p)for(unsigned q=p+1;q<3;++q) {
    const double off=gram[3*p+q];if(std::fabs(off)<1e-14)continue;
    const double theta=.5*std::atan2(2*off,gram[3*q+q]-gram[3*p+p]);
    Matrix rotation=identity;rotation[3*p+p]=rotation[3*q+q]=std::cos(theta);rotation[3*p+q]=std::sin(theta);rotation[3*q+p]=-std::sin(theta);
    gram=multiply(multiply(transpose(rotation),gram),rotation);v=multiply(v,rotation);
  }
  std::array<unsigned,3> order{0,1,2};std::sort(order.begin(),order.end(),[&](unsigned i,unsigned j){return gram[3*i+i]==gram[3*j+j]?i<j:gram[3*i+i]>gram[3*j+j];});
  Svd result;result.u={};result.v={};
  for(unsigned c=0;c<3;++c) {
    result.sigma[c]=std::sqrt(std::max(0.0,gram[3*order[c]+order[c]]));
    if(result.sigma[c]<1e-7)result.sigma[c]=0;
    for(unsigned r=0;r<3;++r)result.v[3*r+c]=v[3*r+order[c]];
    std::array<double,3> column{};
    if(result.sigma[c]>0)for(unsigned r=0;r<3;++r)for(unsigned k=0;k<3;++k)column[r]+=a[3*r+k]*result.v[3*k+c]/result.sigma[c];
    else {
      double best=-1;
      for(unsigned axis=0;axis<3;++axis) {
        std::array<double,3> candidate{};candidate[axis]=1;
        for(unsigned j=0;j<c;++j) {double d=0;for(unsigned r=0;r<3;++r)d+=candidate[r]*result.u[3*r+j];for(unsigned r=0;r<3;++r)candidate[r]-=d*result.u[3*r+j];}
        double norm=0;for(auto n:candidate)norm+=n*n;
        if(norm>best){best=norm;column=candidate;}
      }
    }
    double norm=0;for(auto n:column)norm+=n*n;norm=std::sqrt(norm);
    for(unsigned r=0;r<3;++r)result.u[3*r+c]=column[r]/norm;
  }
  return result;
}
// Cube rotations use exact signed-permutation matrices. Every product remains
// integral; enumeration is deterministic breadth-first closure under X,Y,Z.
constexpr std::array<Matrix,3> quarterTurns{{{1,0,0,0,0,-1,0,1,0},{0,0,1,0,1,0,-1,0,0},{0,-1,0,1,0,0,0,0,1}}};
Matrix turn(unsigned op) {return op<3?quarterTurns[op]:transpose(quarterTurns[op-3]);}
Matrix power(Matrix a,unsigned n) {Matrix r=identity;for(unsigned i=0;i<n;++i)r=multiply(a,r);return r;}
Vec3 cubeVertex(unsigned i) {return {i&1?1.0F:-1.0F,i&2?1.0F:-1.0F,i&4?1.0F:-1.0F};}
unsigned destination(const Matrix& r,unsigned vertex) {
  const auto p=mapped(r,cubeVertex(vertex));return (p.x>0?1U:0U)|(p.y>0?2U:0U)|(p.z>0?4U:0U);
}
const std::array<Matrix,24>& cubeGroup() {
  static const auto group=[] {
    std::array<Matrix,24> result{};result[0]=identity;std::size_t size=1;
    for(std::size_t head=0;head<size;++head)for(const auto& generator:quarterTurns) {
      const auto candidate=multiply(generator,result[head]);
      if(std::find(result.begin(),result.begin()+size,candidate)!=result.begin()+size)continue;
      if(size==result.size())throw std::logic_error("cube group overflow");result[size++]=candidate;
    }
    if(size!=24)throw std::logic_error("incomplete cube group");return result;
  }();return group;
}
unsigned rotationOrder(const Matrix& r) {for(unsigned n=1;n<=4;++n)if(power(r,n)==identity)return n;throw std::logic_error("invalid cube rotation order");}
Matrix cyclicGenerator(unsigned rule) {
  static constexpr Matrix diagonal{0,0,1,1,0,0,0,1,0};
  return rule==0?quarterTurns[0]:rule==1?power(quarterTurns[0],2):diagonal;
}
unsigned gcd(unsigned a,unsigned b) {while(b){const auto r=a%b;a=b;b=r;}return a;}
struct FourierTerm { unsigned frequency;double coefficient; };
FourierTerm fourierTerm(unsigned waveform,unsigned term) {
  if(waveform==1){const unsigned n=term+1;return {n,-2/(pi*n)};}
  const unsigned n=2*term+1;
  return {n,waveform==0?4/(pi*n):8/(pi*pi*n*n)*(term%2?-1:1)};
}
double coefficient(unsigned waveform,unsigned frequency) {
  if(waveform!=1&&frequency%2==0)return 0;
  return fourierTerm(waveform,waveform==1?frequency-1:(frequency-1)/2).coefficient;
}
double targetWave(unsigned waveform,double t) {
  if(waveform==2)return 2/pi*std::asin(std::clamp(std::sin(t),-1.0,1.0));
  if(waveform==0)return std::fabs(std::sin(t))<1e-12?0:std::sin(t)>0?1:-1;
  return t<1e-12||t>2*pi-1e-12?0:(t-pi)/pi;
}
double fourierSum(unsigned waveform,unsigned terms,double t) {
  double sum=0;for(unsigned i=0;i<terms;++i){const auto term=fourierTerm(waveform,i);sum+=term.coefficient*std::sin(term.frequency*t);}return sum;
}
struct MotionConfig { bool pendulum;double position,velocity,k,damping,drive,frequency; };
struct MotionState { double q=0,v=0,loss=0,work=0; };
MotionState add(MotionState a,MotionState b,double scale) {return {a.q+scale*b.q,a.v+scale*b.v,a.loss+scale*b.loss,a.work+scale*b.work};}
MotionState motionRate(const MotionConfig& c,double t,MotionState s) {
  const double force=c.drive*std::cos(c.frequency*t);
  return {s.v,-c.k*(c.pendulum?std::sin(s.q):s.q)-c.damping*s.v+force,c.damping*s.v*s.v,force*s.v};
}
MotionState integrate(const MotionConfig& c,MotionState state,double from,double to) {
  // At most 3,072 steps across 12 seconds. Use equal subdivisions to avoid a
  // floating remainder step and integrate work/loss with the same RK4 stages.
  const unsigned count=static_cast<unsigned>(std::ceil((to-from)*256));
  if(count==0)return state;const double h=(to-from)/count;
  for(unsigned i=0;i<count;++i) {
    const double t=from+i*h;const auto a=motionRate(c,t,state),b=motionRate(c,t+h/2,add(state,a,h/2)),
      d=motionRate(c,t+h/2,add(state,b,h/2)),e=motionRate(c,t+h,add(state,d,h));
    state=add(add(add(add(state,a,h/6),b,h/3),d,h/3),e,h/6);
  }
  return state;
}
double energy(const MotionConfig& c,MotionState s) {return .5*s.v*s.v+c.k*(c.pendulum?1-std::cos(s.q):.5*s.q*s.q);}
// For the linear spring, H(s)=1/(s^2+c*s+k). Its causal impulse response
// also covers critical damping and undamped resonance without a pole division.
double impulse(const MotionConfig& c,double t) {
  const double a=c.damping/2,d=c.k-a*a;
  if(std::fabs(d)<1e-12)return t*std::exp(-a*t);
  if(d>0){const double w=std::sqrt(d);return std::exp(-a*t)*std::sin(w*t)/w;}
  const double b=std::sqrt(-d);return (std::exp((-a+b)*t)-std::exp((-a-b)*t))/(2*b);
}
double impulseDerivative(const MotionConfig& c,double t) {
  const double a=c.damping/2,d=c.k-a*a;
  if(std::fabs(d)<1e-12)return (1-a*t)*std::exp(-a*t);
  if(d>0){const double w=std::sqrt(d);return std::exp(-a*t)*(std::cos(w*t)-a*std::sin(w*t)/w);}
  const double b=std::sqrt(-d);return ((-a+b)*std::exp((-a+b)*t)-(-a-b)*std::exp((-a-b)*t))/(2*b);
}
double freeResponse(const MotionConfig& c,double t) {return c.position*impulseDerivative(c,t)+(c.damping*c.position+c.velocity)*impulse(c,t);}
double convolutionResponse(const MotionConfig& c,double t) {
  constexpr unsigned panels=512;const double h=t/panels;double sum=0;
  for(unsigned i=0;i<=panels;++i){const double u=i*h;sum+=(i==0||i==panels?1:i%2?4:2)*impulse(c,t-u)*c.drive*std::cos(c.frequency*u);}
  return sum*h/3;
}
int remainder(int value,int modulus) {const int r=value%modulus;return r<0?r+modulus:r;}
int floorQuotient(int value,int divisor) {return (value-remainder(value,divisor))/divisor;}
struct CrtSolution {int period=0,solution=-1;};
CrtSolution chineseRemainder(int a,int n,int b,int m) {
  const int period=n/static_cast<int>(gcd(static_cast<unsigned>(n),static_cast<unsigned>(m)))*m;
  for(int x=0;x<period;++x)if(remainder(x-a,n)==0&&remainder(x-b,m)==0)return {period,x};
  return {period,-1};
}
struct GaussianInt {int real=0,imag=0;bool operator==(const GaussianInt&) const = default;};
GaussianInt gaussianProduct(GaussianInt a,GaussianInt b){return {a.real*b.real-a.imag*b.imag,a.real*b.imag+a.imag*b.real};}
GaussianInt gaussianAdd(GaussianInt a,GaussianInt b){return {a.real+b.real,a.imag+b.imag};}
GaussianInt gaussianSubtract(GaussianInt a,GaussianInt b){return {a.real-b.real,a.imag-b.imag};}
int norm(GaussianInt a){return a.real*a.real+a.imag*a.imag;}
GaussianInt divisionNumerator(GaussianInt z,GaussianInt w){return gaussianProduct(z,{w.real,-w.imag});}
bool gaussianDivides(GaussianInt w,GaussianInt z) {
  const int n=norm(w);if(!n)return z==GaussianInt{};const auto a=divisionNumerator(z,w);
  return remainder(a.real,n)==0&&remainder(a.imag,n)==0;
}
GaussianInt gaussianQuotient(GaussianInt z,GaussianInt w) {
  const auto a=divisionNumerator(z,w);const double n=norm(w);
  return {static_cast<int>(std::round(a.real/n)),static_cast<int>(std::round(a.imag/n))};
}
GaussianInt canonicalGaussian(GaussianInt z,GaussianInt generator) {
  const int n=norm(generator);const auto a=divisionNumerator(z,generator);
  return gaussianSubtract(z,gaussianProduct(generator,{floorQuotient(a.real,n),floorQuotient(a.imag,n)}));
}
struct GaussianRing {
  GaussianInt generator;
  std::array<GaussianInt,9> representatives{};
  unsigned size=0;
  unsigned classify(GaussianInt z) const {
    const auto r=canonicalGaussian(z,generator);
    for(unsigned i=0;i<size;++i)if(representatives[i]==r)return i;
    throw std::logic_error("unclassified Gaussian residue");
  }
};
GaussianRing gaussianRing(unsigned preset) {
  static constexpr std::array<GaussianInt,3> generators{{{2,0},{2,1},{3,0}}};
  GaussianRing ring;ring.generator=generators.at(preset);const unsigned count=static_cast<unsigned>(norm(ring.generator));
  for(unsigned y=0;y<count&&ring.size<count;++y)for(unsigned x=0;x<count&&ring.size<count;++x) {
    const auto r=canonicalGaussian({static_cast<int>(x),static_cast<int>(y)},ring.generator);
    if(std::find(ring.representatives.begin(),ring.representatives.begin()+ring.size,r)==ring.representatives.begin()+ring.size)ring.representatives[ring.size++]=r;
  }
  if(ring.size!=count)throw std::logic_error("incomplete Gaussian quotient");return ring;
}
struct FieldVector {double x=0,y=0,z=0;};
FieldVector operator+(FieldVector a,FieldVector b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
FieldVector operator-(FieldVector a,FieldVector b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
FieldVector operator*(FieldVector a,double k){return {a.x*k,a.y*k,a.z*k};}
double fieldDot(FieldVector a,FieldVector b){return a.x*b.x+a.y*b.y+a.z*b.z;}
double fieldLength(FieldVector a){return std::sqrt(fieldDot(a,a));}
Vec3 sceneVector(FieldVector a){return {static_cast<float>(a.x),static_cast<float>(a.y),static_cast<float>(a.z)};}
FieldVector fieldValue(unsigned rule,FieldVector p) {
  switch(rule){case 0:return {1,.5,-.25};case 1:return p;case 2:return {-p.y,p.x,0};default:throw std::logic_error("unknown vector field");}
}
struct FieldPathValue {FieldVector position,velocity;};
FieldPathValue fieldPath(unsigned path,double time,bool reversed) {
  const double t=reversed?1-time:time,a=pi*t;FieldPathValue r;
  switch(path) {
    case 0:r={{-1+2*t,0,0},{2,0,0}};break;
    case 1:r={{-std::cos(a),std::sin(a),0},{pi*std::sin(a),pi*std::cos(a),0}};break;
    case 2:r={{-std::cos(a),-std::sin(a),0},{pi*std::sin(a),-pi*std::cos(a),0}};break;
    case 3:r={{-std::cos(a),std::sin(a),.5*std::sin(2*a)},{pi*std::sin(a),pi*std::cos(a),pi*std::cos(2*a)}};break;
    case 4:r={{std::cos(2*a),std::sin(2*a),0},{-2*pi*std::sin(2*a),2*pi*std::cos(2*a),0}};break;
    default:throw std::logic_error("unknown field path");
  }
  if(t==0||t==1)r.position={path==4?1.0:t==0?-1.0:1.0,0,0};
  if(reversed)r.velocity=r.velocity*-1;return r;
}
double fieldIntegrand(unsigned rule,unsigned path,double t,bool reversed) {
  const auto p=fieldPath(path,t,reversed);return fieldDot(fieldValue(rule,p.position),p.velocity);
}
double fieldIntegral(unsigned rule,unsigned path,double t,bool reversed) {
  constexpr unsigned n=256;double sum=0;
  for(unsigned i=0;i<=n;++i)sum+=(i==0||i==n?1:i%2?4:2)*fieldIntegrand(rule,path,t*i/n,reversed);
  return sum*t/(3*n);
}
double exactFieldIntegral(unsigned rule,unsigned path,double t,bool reversed) {
  const auto a=fieldPath(path,0,reversed).position,b=fieldPath(path,t,reversed).position;
  if(rule==0)return fieldDot(fieldValue(0,{}),b-a);
  if(rule==1)return .5*(fieldDot(b,b)-fieldDot(a,a));
  static constexpr std::array<double,5> work{0,-pi,pi,-pi,2*pi};return work.at(path)*t*(reversed?-1:1);
}
double comparisonWork(unsigned rule,FieldVector a,FieldVector b) {
  switch(rule){case 0:return fieldDot(fieldValue(0,{}),b-a);case 1:return .5*(fieldDot(b,b)-fieldDot(a,a));case 2:return a.x*b.y-a.y*b.x;default:throw std::logic_error("unknown vector field");}
}
FieldVector rotateX(FieldVector p,double angle) {const double c=std::cos(angle),s=std::sin(angle);return {p.x,c*p.y-s*p.z,s*p.y+c*p.z};}
FieldVector fluxField(unsigned rule,FieldVector p) {
  switch(rule){case 0:return {0,0,1};case 1:return p;case 2:return {-p.y,p.x,0};case 3:return {-p.y,p.x,p.z};default:throw std::logic_error("invalid flux field");}
}
double fluxDivergence(unsigned rule){return rule==1?3:rule==3?1:0;}
struct FluxSample {FieldVector point,normal;double area=0;};
FluxSample fluxSample(unsigned shape,unsigned n,unsigned index,double radius,double tilt,double sign) {
  FluxSample s;const unsigned around=2*n;
  switch(shape) {
    case 0: {const double mu=-1+(2.0*(index/around)+1)/n,a=2*pi*((index%around)+.5)/around,h=std::sqrt(std::max(0.0,1-mu*mu));s.normal={h*std::cos(a),h*std::sin(a),mu};s.point=s.normal*radius;s.area=4*pi*radius*radius/(n*around);break;}
    case 1: {const unsigned face=index/(n*n),cell=index%(n*n),axis=face/2;const double side=face%2?1:-1,u=radius*(-1+(2.0*(cell%n)+1)/n),v=radius*(-1+(2.0*(cell/n)+1)/n);std::array<double,3> p{},normal{};p[axis]=side*radius;p[(axis+1)%3]=u;p[(axis+2)%3]=v;normal[axis]=side;s.point={p[0],p[1],p[2]};s.normal={normal[0],normal[1],normal[2]};s.area=4*radius*radius/(n*n);break;}
    case 2: {const double rho=radius*std::sqrt(((index/around)+.5)/n),a=2*pi*((index%around)+.5)/around;s.point={rho*std::cos(a),rho*std::sin(a),0};s.normal={0,0,1};s.area=pi*radius*radius/(n*around);break;}
    default:throw std::logic_error("invalid flux shape");
  }
  s.point=rotateX(s.point,tilt);s.normal=rotateX(s.normal,tilt)*sign;if(shape==2)s.point.z+=.4;return s;
}
unsigned fluxSampleCount(unsigned shape,unsigned n){return shape==1?6*n*n:2*n*n;}
double surfaceFlux(unsigned shape,unsigned rule,unsigned n,double radius,double tilt,double sign,bool curl=false) {
  double sum=0;for(unsigned i=0;i<fluxSampleCount(shape,n);++i){const auto s=fluxSample(shape,n,i,radius,tilt,sign);const auto f=curl?FieldVector{0,0,rule>=2?2.0:0.0}:fluxField(rule,s.point);sum+=fieldDot(f,s.normal)*s.area;}return sum;
}
double shellVolume(unsigned shape,double r){return shape==0?4*pi*r*r*r/3:shape==1?8*r*r*r:0;}
double exactSurfaceFlux(unsigned shape,unsigned rule,double r,double tilt,double sign) {
  if(shape<2)return sign*fluxDivergence(rule)*shellVolume(shape,r);
  return sign*pi*r*r*std::cos(tilt)*(rule==0?1:rule==1||rule==3?.4:0);
}
FieldPathValue fluxBoundary(double t,double r,double tilt,double sign) {
  const double a=sign*2*pi*t;return {rotateX({r*std::cos(a),r*std::sin(a),0},tilt)+FieldVector{0,0,.4},rotateX({-sign*2*pi*r*std::sin(a),sign*2*pi*r*std::cos(a),0},tilt)};
}
double boundaryIntegral(unsigned rule,unsigned n,double end,double r,double tilt,double sign) {
  const unsigned panels=8*n;double sum=0;for(unsigned i=0;i<=panels;++i){const auto p=fluxBoundary(end*i/panels,r,tilt,sign);sum+=(i==0||i==panels?1:i%2?4:2)*fieldDot(fluxField(rule,p.position),p.velocity);}return sum*end/(3*panels);
}
using Triple=std::array<double,3>;
double tripleDot(const Triple& a,const Triple& b){double sum=0;for(unsigned i=0;i<3;++i)sum+=a[i]*b[i];return sum;}
Matrix outerProduct(const Triple& a,const Triple& b){Matrix m{};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)m[3*i+j]=a[i]*b[j];return m;}
Triple coordinates(const Matrix& q,const Triple& v){Triple result{};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)result[i]+=q[3*j+i]*v[j];return result;}
double frobenius(const Matrix& m){double sum=0;for(double v:m)sum+=v*v;return std::sqrt(sum);}
Matrix probabilityMatrix(unsigned rule,double stay) {
  Matrix p{};
  switch(rule) {
    case 0:for(unsigned i=0;i<3;++i){p[3*i+i]=stay;p[3*i+(i+1)%3]=1-stay;}break;
    case 1:for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)p[3*i+j]=(1-stay)*Triple{.5,.3,.2}[j]+(i==j?stay:0);break;
    case 2:p={0,1,0,0,0,1,1,0,0};break;
    case 3:p={stay,1-stay,0,0,stay,1-stay,0,0,1};break;
    default:throw std::logic_error("invalid Markov rule");
  }
  return p;
}
Triple probabilityNext(const Triple& p,const Matrix& matrix){Triple next{};for(unsigned j=0;j<3;++j)for(unsigned i=0;i<3;++i)next[j]+=p[i]*matrix[3*i+j];return next;}
Triple probabilityInitial(unsigned start,double mix){Triple p{mix/3,mix/3,mix/3};p[start]+=1-mix;return p;}
Triple stationaryExample(unsigned rule){switch(rule){case 0:case 2:return {1.0/3,1.0/3,1.0/3};case 1:return {.5,.3,.2};case 3:return {0,0,1};default:throw std::logic_error("invalid stationary rule");}}
double totalVariation(const Triple& a,const Triple& b){double sum=0;for(unsigned i=0;i<3;++i)sum+=std::fabs(a[i]-b[i]);return .5*sum;}
constexpr std::array<std::string_view,3> probabilityLabels{"A","B","C"};
constexpr std::array<Vec3,3> probabilityPositions{{{-1,-.7F,0},{1,-.7F,0},{0,1,.7F}}};
constexpr std::array<std::string_view,27> tensorLabels{"111","121","131","211","221","231","311","321","331","112","122","132","212","222","232","312","322","332","113","123","133","213","223","233","313","323","333"};
double randomUnit(std::uint32_t& state){state^=state<<13;state^=state>>17;state^=state<<5;return state/4294967296.0;}
std::array<double,13> binomialMass(unsigned n,double p) {
  std::array<double,13> mass{};mass[0]=1;
  for(unsigned trial=0;trial<n;++trial){std::array<double,13> next{};for(unsigned k=0;k<=trial;++k){next[k]+=mass[k]*(1-p);next[k+1]+=mass[k]*p;}mass=next;}
  return mass;
}
double binomialCdf(const std::array<double,13>& mass,int k){double sum=0;for(int i=0;i<=std::min(k,12);++i)sum+=mass[static_cast<unsigned>(i)];return sum;}
double normalCdf(double x,double mean,double sd){return .5*std::erfc((mean-x)/(sd*std::sqrt(2.0)));}
struct BayesResult {double h=0,other=0,evidence=0,posterior=0,posteriorOther=0;bool defined=false;};
BayesResult bayesUpdate(double prior,double hLikelihood,double otherLikelihood) {
  BayesResult result;result.h=prior*hLikelihood;result.other=(1-prior)*otherLikelihood;result.evidence=result.h+result.other;result.defined=result.evidence>0;if(result.defined){result.posterior=result.h/result.evidence;result.posteriorOther=result.other/result.evidence;}return result;
}
double evidenceLikelihood(double chance,unsigned yes,unsigned no){return std::pow(chance,yes)*std::pow(1-chance,no);}
Triple tripleTransform(const Matrix& matrix,const Triple& vector){Triple out{};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)out[i]+=matrix[3*i+j]*vector[j];return out;}
Triple tripleAdd(const Triple& a,const Triple& b){return {a[0]+b[0],a[1]+b[1],a[2]+b[2]};}
Vec3 tripleScene(const Triple& p){return {static_cast<float>(p[0]),static_cast<float>(p[1]),static_cast<float>(p[2])};}
Matrix cloudTransform(const Triple& scales,double shear,double yaw,double pitch) {
  const double c=std::cos(yaw),s=std::sin(yaw),cp=std::cos(pitch),sp=std::sin(pitch);const Matrix rz{c,-s,0,s,c,0,0,0,1},ry{cp,0,sp,0,1,0,-sp,0,cp},stretch{scales[0],shear*scales[1],0,0,scales[1],0,0,0,scales[2]};return multiply(multiply(rz,ry),stretch);
}
struct CloudMoments {Triple mean{};Matrix covariance{};};
CloudMoments cloudMoments(const std::array<Triple,8>& points) {
  CloudMoments out;for(const auto& p:points)for(unsigned i=0;i<3;++i)out.mean[i]+=p[i]/8;
  for(const auto& p:points)for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)out.covariance[3*i+j]+=(p[i]-out.mean[i])*(p[j]-out.mean[j])/8;return out;
}
// Bounded degree-4 real spherical harmonics, with Condon-Shortley phase.
// Conventions: DLMF 14.30.1/2; this is an original implementation.
struct SphereMode {unsigned degree;int order;};
SphereMode sphereMode(unsigned index) {unsigned l=0;while((l+1)*(l+1)<=index)++l;return {l,static_cast<int>(index-l*l)-static_cast<int>(l)};}
double sphereHarmonic(unsigned index,double theta,double phi) {
  const auto [l,signedM]=sphereMode(index);const unsigned m=static_cast<unsigned>(std::abs(signedM));const double x=std::cos(theta);
  double p=1;for(unsigned i=1;i<=m;++i)p*=-(2.0*i-1)*std::sqrt(std::max(0.0,1-x*x));
  if(l>m){double previous=p;p=x*(2*m+1)*p;for(unsigned k=m+2;k<=l;++k){const double next=((2*k-1)*x*p-(k+m-1)*previous)/(k-m);previous=p;p=next;}}
  double ratio=1;for(unsigned k=l-m+1;k<=l+m;++k)ratio/=k;
  const double normal=std::sqrt((2*l+1)*ratio/(4*pi));
  return normal*p*(m?std::sqrt(2.0)*(signedM>0?std::cos(m*phi):std::sin(m*phi)):1);
}
Triple sphereDirection(double theta,double phi){return {std::sin(theta)*std::cos(phi),std::sin(theta)*std::sin(phi),std::cos(theta)};}
double sphereInner(unsigned a,unsigned b) {
  constexpr std::array<double,8> nodes{-.9602898564975363,-.7966664774136267,-.5255324099163290,-.1834346424956498,.1834346424956498,.5255324099163290,.7966664774136267,.9602898564975363};
  constexpr std::array<double,8> weights{.1012285362903763,.2223810344533745,.3137066458778873,.3626837833783620,.3626837833783620,.3137066458778873,.2223810344533745,.1012285362903763};
  double integral=0;for(unsigned i=0;i<8;++i)for(unsigned j=0;j<16;++j){const double t=std::acos(nodes[i]),p=2*pi*j/16;integral+=weights[i]*(2*pi/16)*sphereHarmonic(a,t,p)*sphereHarmonic(b,t,p);}return integral;
}
double sphereLaplaceError(unsigned mode) {
  constexpr double t=1.1,p=.7,h=1e-4;const double f=sphereHarmonic(mode,t,p),up=sphereHarmonic(mode,t+h,p),down=sphereHarmonic(mode,t-h,p);
  const double lap=(up-2*f+down)/(h*h)+(up-down)/(2*h)*std::cos(t)/std::sin(t)+(sphereHarmonic(mode,t,p+h)-2*f+sphereHarmonic(mode,t,p-h))/(h*h*std::sin(t)*std::sin(t));
  const auto l=sphereMode(mode).degree;return std::fabs(lap+l*(l+1)*f);
}
// Exact monic division for Phi_n, n<=12, starting from x^n-1.
struct Cyclotomic {std::array<int,13> coefficients{};unsigned degree=0;};
Cyclotomic cyclotomic(unsigned n) {
  std::array<Cyclotomic,13> polys{};
  for(unsigned k=1;k<=n;++k){auto& p=polys[k];p.degree=k;p.coefficients[0]=-1;p.coefficients[k]=1;
    for(unsigned d=1;d<k;++d)if(k%d==0){const auto& divisor=polys[d];Cyclotomic quotient;quotient.degree=p.degree-divisor.degree;
      for(int i=static_cast<int>(p.degree);i>=static_cast<int>(divisor.degree);--i){const unsigned offset=static_cast<unsigned>(i)-divisor.degree;const int c=p.coefficients[static_cast<unsigned>(i)];quotient.coefficients[offset]=c;for(unsigned j=0;j<=divisor.degree;++j)p.coefficients[j+offset]-=c*divisor.coefficients[j];}p=quotient;
    }
  }return polys[n];
}
std::array<double,2> evaluateCyclotomic(const Cyclotomic& p,double theta) {
  const double x=std::cos(theta),y=std::sin(theta);double real=p.coefficients[p.degree],imag=0;
  for(int i=static_cast<int>(p.degree)-1;i>=0;--i){const double next=real*x-imag*y+p.coefficients[static_cast<unsigned>(i)];imag=real*y+imag*x;real=next;}return {real,imag};
}
unsigned rootUnitOrder(unsigned a,unsigned n) {if(gcd(a,n)!=1)return 0;unsigned v=1;for(unsigned order=1;order<=n;++order){v=v*a%n;if(v==1)return order;}return 0;}
constexpr std::array<std::string_view,13> countLabels{"0","1","2","3","4","5","6","7","8","9","10","11","12"};
constexpr std::array<std::string_view,8> cloudLabels{"P0","P1","P2","P3","P4","P5","P6","P7"},whitenLabels{"W0","W1","W2","W3","W4","W5","W6","W7"};
constexpr std::array<std::string_view,12> residueLabels{"0","1","2","3","4","5","6","7","8","9","10","11"};
constexpr std::array<Vec3,9> classColors{muted,teal,coral,gold,blue,violet,white,Vec3{.85F,.35F,.62F},Vec3{.6F,.85F,.3F}};
// Symmetric 2x2 matrices form a three-dimensional vector space. These
// coordinates turn its PSD condition into the circular cone t >= hypot(x,y).
struct ConeMatrix { double a,b,c; };
struct ConeSpectrum {
  double x,y,t,lo,hi,tolerance;
  unsigned rank;
  bool psd;
};
ConeSpectrum coneSpectrum(ConeMatrix a) {
  const double x=(a.a-a.c)/2,t=(a.a+a.c)/2,r=std::hypot(x,a.b);
  const double tolerance=1e-10*std::max({std::fabs(a.a),std::fabs(a.b),std::fabs(a.c)});
  const double lo=t-r,hi=t+r;
  return {x,a.b,t,lo,hi,tolerance,static_cast<unsigned>(std::fabs(lo)>tolerance)+static_cast<unsigned>(std::fabs(hi)>tolerance),lo>=-tolerance};
}
Vec3 conePosition(ConeMatrix a) {const auto e=coneSpectrum(a);return {static_cast<float>(e.x),static_cast<float>(e.y),static_cast<float>(e.t)};}
double quadratic2(ConeMatrix a,double u,double v) {return a.a*u*u+2*a.b*u*v+a.c*v*v;}
// Scale before taking powers: stable for all supported p and bounded vectors.
// Infinity is a distinct operation, never a numeric exponent or a large-p alias.
double normLength(const Triple& x,double p,bool infinity=false) {
  const double scale=std::max({std::fabs(x[0]),std::fabs(x[1]),std::fabs(x[2])});
  if(scale==0||infinity)return scale;
  double sum=0;for(double v:x)sum+=std::pow(std::fabs(v)/scale,p);
  return scale*std::pow(sum,1/p);
}
Triple normBoundary(Triple direction,double p,bool infinity) {
  const double length=normLength(direction,p,infinity);
  if(length>0)for(auto& x:direction)x/=length;
  return direction;
}
Triple normSupport(const Triple& w,double p,bool infinity) {
  Triple result{};const double maximum=normLength(w,1,true);if(maximum==0)return result;
  if(infinity){for(unsigned i=0;i<3;++i)result[i]=(w[i]>0)-(w[i]<0);return result;}
  if(p==1){for(unsigned i=0;i<3;++i)if(std::fabs(w[i])==maximum){result[i]=(w[i]>0)?1:-1;break;}return result;}
  const double q=p/(p-1),h=normLength(w,q);
  for(unsigned i=0;i<3;++i)result[i]=((w[i]>0)-(w[i]<0))*std::pow(std::fabs(w[i])/h,q-1);
  return result;
}
class SnapshotBuilder {
public:
  explicit SnapshotBuilder(MathObjectSnapshot& snapshot):s(snapshot) {}
  void part(MathShape shape,Vec3 center,Vec3 x,Vec3 y,Vec3 z,Vec3 color,std::string_view role) {
    if(s.partCount==s.parts.size())throw std::logic_error("math part capacity exceeded");
    const auto id=static_cast<std::uint32_t>((index(s.kind)+1)*1000+s.partCount+1);
    s.parts[s.partCount++]={id,shape,center,x,y,z,color,role};
  }
  void scaled(MathShape shape,Vec3 center,Vec3 scale,Vec3 color,std::string_view role) {
    part(shape,center,{scale.x,0,0},{0,scale.y,0},{0,0,scale.z},color,role);
  }
  void ball(Vec3 center,float radius,Vec3 color,std::string_view role) { scaled(MathShape::Sphere,center,{radius,radius,radius},color,role); }
  void rod(Vec3 a,Vec3 b,Vec3 color,float radius=.016F,std::string_view role="guide") {
    const auto delta=b-a;const float len=length(delta);if(len<1e-6F)return;
    const auto y=delta*(1/len), x=normalized(cross(std::fabs(y.y)>.9F?Vec3{1,0,0}:Vec3{0,1,0},y)), z=cross(x,y);
    part(MathShape::Rod,(a+b)*.5F,x*radius,delta,z*radius,color,role);
  }
  void arrow(Vec3 a,Vec3 b,Vec3 color,std::string_view role) {
    const auto delta=b-a;const float len=length(delta);if(len<1e-6F)return;
    const auto y=delta*(1/len), x=normalized(cross(std::fabs(y.y)>.9F?Vec3{1,0,0}:Vec3{0,1,0},y)), z=cross(x,y);
    const float tip=std::min(.15F,len*.23F);
    rod(a,b-y*tip,color,.024F,role);
    part(MathShape::Cone,b-y*(tip*.5F),x*.065F,y*tip,z*.065F,color,role);
  }
  void label(std::string_view text,Vec3 pos,Vec3 color=white) {
    if(s.labelCount==s.labels.size())throw std::logic_error("math label capacity exceeded");
    s.labels[s.labelCount++]={text,pos,color};
  }
  void metric(std::string_view text,double value,std::string_view suffix={}) {
    if(s.metricCount==s.metrics.size())throw std::logic_error("math metric capacity exceeded");
    s.metrics[s.metricCount++]={text,suffix,value};
  }
  MathPlot& plot(std::string_view title,MathParameter scrub=MathParameter::Count) {
    if(s.plotCount==s.plots.size())throw std::logic_error("math plot capacity exceeded");
    auto& p=s.plots[s.plotCount++];p={};p.title=title;p.scrubParameter=scrub;return p;
  }
  template<class F> void curve(MathPlot& p,std::string_view name,Vec3 color,double from,double to,F f,bool filled=false) {
    if(p.seriesCount==p.series.size())throw std::logic_error("math series capacity exceeded");
    auto& line=p.series[p.seriesCount++];line={};line.name=name;line.color=color;line.count=line.points.size();line.signedFill=filled;
    for(std::size_t i=0;i<line.count;++i) {const double x=from+(to-from)*i/(line.count-1);line.points[i]={x,f(x)};}
  }
  void matrix(std::string_view name,const Matrix& values,bool editable=false) {
    if(s.matrixCount==s.matrices.size())throw std::logic_error("math matrix capacity exceeded");
    auto& view=s.matrices[s.matrixCount++];view={};view.name=name;view.values=values;view.editable=editable;view.parameters=matrixParameters;
  }
  void matrix2(std::string_view name,std::array<double,4> values) {
    if(s.matrixCount==s.matrices.size())throw std::logic_error("math matrix capacity exceeded");
    auto& view=s.matrices[s.matrixCount++];view={};view.name=name;view.rows=view.columns=2;
    view.parameters.fill(MathParameter::Count);std::copy(values.begin(),values.end(),view.values.begin());
  }
  void wireBox(Vec3 center,float radius,Vec3 color,std::string_view role) {
    const auto vertex=[&](unsigned i){return center+Vec3{(i&1)?radius:-radius,(i&2)?radius:-radius,(i&4)?radius:-radius};};
    for(unsigned i=0;i<8;++i)for(unsigned bit:{1U,2U,4U})if(!(i&bit))rod(vertex(i),vertex(i|bit),color,.012F,role);
  }
  void normWire(Vec3 center,double p,bool infinity,Vec3 color,std::string_view role) {
    if(infinity){wireBox(center,1,color,role);return;}
    for(unsigned axis=0;axis<3;++axis)for(unsigned i=0;i<24;++i) {
      const auto at=[&](unsigned step){const double angle=2*pi*step/24;Triple d{};d[(axis+1)%3]=std::cos(angle);d[(axis+2)%3]=std::sin(angle);return center+tripleScene(normBoundary(d,p,false));};
      rod(at(i),at(i+1),color,.015F,role);
    }
  }
  void planeFrame(Vec3 center,Vec3 u,Vec3 v,Vec3 color,std::string_view role) {
    const std::array<Vec3,4> corners{center-u-v,center+u-v,center+u+v,center-u+v};
    for(unsigned i=0;i<4;++i)rod(corners[i],corners[(i+1)%4],color,.014F,role);
  }
  void table(std::string_view title,std::array<std::string_view,4> columns,std::size_t count) {
    s.table={};s.table.title=title;s.table.columns=columns;s.table.columnCount=count;
  }
  void row(std::string_view label,std::array<double,4> values) {
    if(s.table.rowCount==s.table.values.size())throw std::logic_error("math table capacity exceeded");
    const auto i=s.table.rowCount++;s.table.rowLabels[i]=label;s.table.values[i]=values;
  }
  MathObjectSnapshot& s;
};
} // namespace

std::span<const MathObjectSpec> mathObjectSpecs(){return objects;}
std::span<const MathParameterSpec> mathParameterSpecs(){return parameters;}
std::span<const MathLesson> mathLessons(MathObjectKind kind) {
  switch(kind) {
    case MathObjectKind::Function:return functionLessons;
    case MathObjectKind::Linear:return linearLessons;
    case MathObjectKind::Surface:return surfaceLessons;
    case MathObjectKind::Symmetry:return symmetryLessons;
    case MathObjectKind::Harmonics:return harmonicLessons;
    case MathObjectKind::Oscillator:return oscillatorLessons;
    case MathObjectKind::Modular:return modularLessons;
    case MathObjectKind::Gaussian:return gaussianLessons;
    case MathObjectKind::VectorField:return fieldLessons;
    case MathObjectKind::Flux:return fluxLessons;
    case MathObjectKind::Tensor:return tensorLessons;
    case MathObjectKind::Probability:return probabilityLessons;
    case MathObjectKind::Binomial:return binomialLessons;
    case MathObjectKind::Bayes:return bayesLessons;
    case MathObjectKind::Covariance:return covarianceLessons;
    case MathObjectKind::Spherical:return sphericalLessons;
    case MathObjectKind::Quadratic:return quadraticLessons;
    case MathObjectKind::Roots:return rootsLessons;
    case MathObjectKind::Psd:return psdLessons;
    case MathObjectKind::Norm:return normLessons;
    case MathObjectKind::Curve:return curveLessons;
    case MathObjectKind::Lathe:return latheLessons;
    case MathObjectKind::Boolean:return booleanLessons;
    case MathObjectKind::Patch:return patchLessons;
    case MathObjectKind::Membrane:return membraneLessons;
    case MathObjectKind::Rigid:return rigidLessons;
    default:return {};
  }
}
std::span<const std::string_view> mathMatrixPresetNames() {
  static constexpr std::array<std::string_view,5> names{"Identity","Shear","Project onto xy","Stretch and reflect","Rotate 90 degrees about z"};return names;
}
std::span<const MathObjectPreset> mathObjectPresets(MathObjectKind kind,unsigned level) {
  switch(kind) {
    case MathObjectKind::Psd:return level<3?std::span<const MathObjectPreset>(psdPresets):std::span<const MathObjectPreset>{};
    case MathObjectKind::Norm:return normPresets;
    case MathObjectKind::Curve:return curvePresets;
    case MathObjectKind::Lathe:return lathePresets;
    case MathObjectKind::Boolean:return booleanPresets;
    case MathObjectKind::Patch:return patchPresets;
    case MathObjectKind::Membrane:return membranePresets;
    case MathObjectKind::Rigid:return rigidPresets;
    default:return {};
  }
}
MathObjects::MathObjects() {for(const auto& p:parameters)parameters_[index(p.id)]=p.initial;rebuild();}
double MathObjects::parameter(MathParameter p) const {
  if(index(p)>=parameters_.size())throw std::invalid_argument("unknown math parameter");
  return parameters_[index(p)];
}
bool MathObjects::parameterAvailable(MathParameter p) const {
  if(index(p)>=parameters.size())return false;
  const auto& spec=parameters[index(p)];
  if(spec.owner!=snapshot_.kind||spec.minimumLevel>snapshot_.level)return false;
  switch(p) {
    case MathParameter::RigidBalance:return parameter(MathParameter::RigidShape)==1||parameter(MathParameter::RigidShape)==3;
    case MathParameter::BooleanBlend:return parameter(MathParameter::BooleanOperation)==3;
    case MathParameter::BooleanFit:return parameter(MathParameter::BooleanShapeB)==1&&parameter(MathParameter::BooleanOperation)==2;
    case MathParameter::BooleanClearance:return parameter(MathParameter::BooleanShapeB)==1&&parameter(MathParameter::BooleanOperation)==2&&parameter(MathParameter::BooleanFit)==1;
    case MathParameter::SurfaceU:case MathParameter::SurfaceV:case MathParameter::DirectionAngle:case MathParameter::DescentRate:
      return parameter(MathParameter::Constraint)==0;
    case MathParameter::CircleAngle:return parameter(MathParameter::Constraint)!=0;
    case MathParameter::ComposeAngle:return snapshot_.level==1;
    case MathParameter::SymmetryFirst:case MathParameter::SymmetrySecond:return snapshot_.level==1;
    case MathParameter::SymmetryGenerator:case MathParameter::SymmetryPower:return snapshot_.level==2;
    case MathParameter::ModValue:return snapshot_.level==0||snapshot_.level==3;
    case MathParameter::ModStep:return snapshot_.level==1||snapshot_.level==2;
    case MathParameter::InverseGuess:return snapshot_.level==2;
    case MathParameter::FluxShape:case MathParameter::FluxProbe:return snapshot_.level<3;
    case MathParameter::TensorW0:case MathParameter::TensorW1:case MathParameter::TensorW2:case MathParameter::TensorK:return snapshot_.level==1||snapshot_.level==2;
    case MathParameter::TensorGap:return snapshot_.level==1||snapshot_.level==2;
    case MathParameter::ProbabilityStart:return snapshot_.level!=1;
    case MathParameter::BinomialSeed:return snapshot_.level==0;
    case MathParameter::BayesEvent:return snapshot_.level==1||snapshot_.level==2;
    case MathParameter::RootMultiplier:return snapshot_.level==1||snapshot_.level==2;
    case MathParameter::RootPower:return snapshot_.level==2;
    case MathParameter::CloudComponent:return snapshot_.level==2;
    case MathParameter::PsdA:case MathParameter::PsdB:case MathParameter::PsdC:return snapshot_.level<3;
    case MathParameter::PsdProbeAngle:return snapshot_.level==1;
    case MathParameter::PsdMix:case MathParameter::PsdOtherAngle:case MathParameter::PsdRayScale:return snapshot_.level==2;
    case MathParameter::NormP:return parameter(MathParameter::NormInfinity)==0;
    case MathParameter::NormWire:return snapshot_.level!=1;
    case MathParameter::LatheTurn:case MathParameter::LatheCut:return snapshot_.level>=1;
    case MathParameter::LatheGuides:return snapshot_.level==1||snapshot_.level==3;
    case MathParameter::LatheWall:case MathParameter::LatheFloor:return parameter(MathParameter::LatheHollow)==1;
    case MathParameter::LatheSlices:return snapshot_.level>=2;
    case MathParameter::LatheMethod:return snapshot_.level==2;
    case MathParameter::CurveTravel:return snapshot_.level>=2;
    case MathParameter::CurveProfile:case MathParameter::CurveRadius:case MathParameter::CurveAspect:
    case MathParameter::CurveEndScale:case MathParameter::CurveTwist:case MathParameter::CurveGuides:return snapshot_.level==3;
    case MathParameter::CurveNormP:return snapshot_.level==3&&parameter(MathParameter::CurveProfile)==2;
    case MathParameter::NormOtherX:case MathParameter::NormOtherY:case MathParameter::NormOtherZ:return snapshot_.level==1;
    case MathParameter::CloudWhiten:return parameter(MathParameter::CloudX)>0&&parameter(MathParameter::CloudY)>0&&parameter(MathParameter::CloudZ)>0;
    case MathParameter::ProbabilitySeed:return snapshot_.level==0;
    case MathParameter::ProbabilityRow:return snapshot_.level==1;
    case MathParameter::ProbabilityStay:return parameter(MathParameter::ProbabilityRule)!=2;
    case MathParameter::GaussianOperation:return snapshot_.level==0||snapshot_.level==3;
    case MathParameter::GaussianHeight:return snapshot_.level<2;
    case MathParameter::FieldX:case MathParameter::FieldY:case MathParameter::FieldZ:
    case MathParameter::FieldYaw:case MathParameter::FieldPitch:return snapshot_.level==0;
    case MathParameter::Amplitude1:case MathParameter::Frequency1:case MathParameter::Phase1:
    case MathParameter::Amplitude2:case MathParameter::Frequency2:case MathParameter::Phase2:return snapshot_.level<2;
    default:return true;
  }
}
MathParameter MathObjects::playbackParameter() const {
  switch(snapshot_.kind) {
    case MathObjectKind::Harmonics:return MathParameter::HarmonicTime;
    case MathObjectKind::Membrane:return MathParameter::MembraneTime;
    case MathObjectKind::Rigid:return MathParameter::RigidTime;
    case MathObjectKind::Oscillator:return MathParameter::MotionTime;
    case MathObjectKind::VectorField:return MathParameter::FieldTime;
    case MathObjectKind::Flux:return MathParameter::FluxTime;
    case MathObjectKind::Curve:return MathParameter::CurveProgress;
    case MathObjectKind::Lathe:return snapshot_.level==1?MathParameter::LatheTurn:MathParameter::Count;
    default:return MathParameter::Count;
  }
}
MathActionResult MathObjects::dispatch(const MathAction& a) {
  switch(a.kind) {
    case MathActionKind::Select:
      if(index(a.object)>=objects.size())return {false,"unknown_object"};
      snapshot_.kind=a.object;snapshot_.level=0;
      snapshot_.playing=false;symmetryMoveCount_=0;fieldPathReversed_=false;
      for(const auto& p:parameters)if(p.owner==snapshot_.kind&&p.minimumLevel>0)parameters_[index(p.id)]=p.initial;
      break;
    case MathActionKind::SetParameter: {
      if(index(a.parameter)>=parameters.size())return {false,"unknown_parameter"};
      const auto& p=parameters[index(a.parameter)];
      if(p.owner!=snapshot_.kind)return {false,"parameter_not_owned_by_object"};
      if(!parameterAvailable(a.parameter))return {false,"parameter_not_available_at_this_level"};
      if(!std::isfinite(a.value)||a.value<p.minimum-1e-6||a.value>p.maximum+1e-6)return {false,"parameter_out_of_range"};
      const double next=std::clamp(std::round(a.value/p.step)*p.step,p.minimum,p.maximum);
      if(a.parameter>=MathParameter::LatheH1&&a.parameter<=MathParameter::LatheH5){const auto i=index(a.parameter);const double below=a.parameter==MathParameter::LatheH1?0:parameters_[i-1],above=a.parameter==MathParameter::LatheH5?1:parameters_[i+1];if(next<below+.04-1e-12||next>above-.04+1e-12)return {false,"profile heights must remain ordered with a 0.04 gap"};}
      parameters_[index(a.parameter)]=next;
      if(a.parameter==MathParameter::Shortcut)snapshot_.routeCount=1;
      switch(snapshot_.kind){case MathObjectKind::Curve:case MathObjectKind::Lathe:case MathObjectKind::Membrane:case MathObjectKind::Rigid:snapshot_.playing=false;break;default:break;}
      if(a.parameter==playbackParameter())snapshot_.playing=false;
      if(a.parameter==MathParameter::FieldPath){fieldPathReversed_=false;parameters_[index(MathParameter::FieldTime)]=0;snapshot_.playing=false;}
      break;
    }
    case MathActionKind::ResetParameters: {
      if(a.resetParameters.none())return {false,"empty_parameter_reset"};
      auto next=parameters_;
      for(const auto& p:parameters)if(a.resetParameters.test(index(p.id))){if(p.owner!=snapshot_.kind)return {false,"parameter_not_owned_by_object"};next[index(p.id)]=p.initial;}
      if(snapshot_.kind==MathObjectKind::Lathe){double previous=0;for(unsigned i=index(MathParameter::LatheH1);i<=index(MathParameter::LatheH5);++i){if(next[i]<previous+.04-1e-12)return {false,"profile heights must remain ordered with a 0.04 gap"};previous=next[i];}if(previous>1-.04+1e-12)return {false,"profile heights must remain ordered with a 0.04 gap"};}
      parameters_=next;snapshot_.playing=false;
      if(a.resetParameters.test(index(MathParameter::Shortcut)))snapshot_.routeCount=1;
      if(a.resetParameters.test(index(MathParameter::FieldPath))){fieldPathReversed_=false;parameters_[index(MathParameter::FieldTime)]=0;}
      break;
    }
    case MathActionKind::Reset:
      snapshot_.playing=false;symmetryMoveCount_=0;fieldPathReversed_=false;
      for(const auto& p:parameters)if(p.owner==snapshot_.kind)parameters_[index(p.id)]=p.initial;
      if(snapshot_.kind==MathObjectKind::Discrete)snapshot_.routeCount=1;
      break;
    case MathActionKind::VisitVertex:
      if(snapshot_.kind!=MathObjectKind::Discrete || a.vertex>=8 || !snapshot_.allowedVertices[a.vertex])return {false,"vertex_not_available"};
      if(snapshot_.routeCount==snapshot_.route.size())return {false,"route_capacity_reached"};
      snapshot_.route[snapshot_.routeCount++]=a.vertex;break;
    case MathActionKind::UndoRoute:
      if(snapshot_.kind!=MathObjectKind::Discrete || snapshot_.routeCount<=1)return {false,"no_route_step_to_undo"};
      --snapshot_.routeCount;break;
    case MathActionKind::ResetRoute:
      if(snapshot_.kind!=MathObjectKind::Discrete)return {false,"wrong_object"};
      snapshot_.routeCount=1;break;
    case MathActionKind::SetLevel: {
      const auto count=std::max<std::size_t>(1,mathLessons(snapshot_.kind).size());
      if(!std::isfinite(a.value)||a.value<0||a.value>=count||std::floor(a.value)!=a.value)return {false,"unknown_learning_level"};
      snapshot_.level=static_cast<unsigned>(a.value);
      snapshot_.playing=false;symmetryMoveCount_=0;fieldPathReversed_=false;
      for(const auto& p:parameters)if(p.owner==snapshot_.kind&&p.minimumLevel>snapshot_.level)parameters_[index(p.id)]=p.initial;
      break;
    }
    case MathActionKind::SwapBounds: {
      if(snapshot_.kind!=MathObjectKind::Function||snapshot_.level<2)return {false,"accumulation_not_available"};
      const auto rule=static_cast<unsigned>(parameter(MathParameter::FunctionRule));
      reversedNegativeIntegral_=primitive(rule,parameter(MathParameter::FunctionX))-primitive(rule,parameter(MathParameter::IntegralStart))<-.001;
      std::swap(parameters_[index(MathParameter::FunctionX)],parameters_[index(MathParameter::IntegralStart)]);break;
    }
    case MathActionKind::DescentStep: {
      if(snapshot_.kind!=MathObjectKind::Surface||!parameterAvailable(MathParameter::DescentRate))return {false,"descent_not_available"};
      const auto rule=static_cast<unsigned>(parameter(MathParameter::SurfaceRule));
      const double u=parameter(MathParameter::SurfaceU),v=parameter(MathParameter::SurfaceV);const auto current=surfaceValue(rule,u,v);
      if(std::hypot(current.du,current.dv)<1e-9)return {false,"already_stationary"};
      double rate=parameter(MathParameter::DescentRate);bool accepted=false;
      for(unsigned trial=0;trial<12;++trial,rate*=.5) {
        const double nu=std::clamp(u-rate*current.du,-2.0,2.0),nv=std::clamp(v-rate*current.dv,-2.0,2.0);
        if(surfaceValue(rule,nu,nv).height<current.height) {parameters_[index(MathParameter::SurfaceU)]=nu;parameters_[index(MathParameter::SurfaceV)]=nv;accepted=true;break;}
      }
      if(!accepted)return {false,"no_decreasing_step_in_domain"};break;
    }
    case MathActionKind::MatrixPreset: {
      if(snapshot_.kind!=MathObjectKind::Linear||snapshot_.level==0)return {false,"matrix_editor_not_available"};
      static constexpr std::array<Matrix,5> presets{identity,Matrix{1,.6,0,0,1,0,0,0,1},Matrix{1,0,0,0,1,0,0,0,0},Matrix{2,0,0,0,1,0,0,0,-1},Matrix{0,-1,0,1,0,0,0,0,1}};
      if(a.vertex>=presets.size())return {false,"unknown_matrix_preset"};
      for(unsigned i=0;i<9;++i)parameters_[index(matrixParameters[i])]=presets[a.vertex][i];break;
    }
    case MathActionKind::MoveSurfacePoint: {
      MathParameter horizontal,vertical;
      switch(snapshot_.kind) {
        case MathObjectKind::Surface:
          if(!parameterAvailable(MathParameter::SurfaceU))return {false,"surface_point_not_available"};
          horizontal=MathParameter::SurfaceU;vertical=MathParameter::SurfaceV;break;
        case MathObjectKind::Quadratic:
          if(snapshot_.level!=2)return {false,"quadratic_contour_not_available"};
          horizontal=MathParameter::QuadX;vertical=MathParameter::QuadY;break;
        default:return {false,"surface_point_not_available"};
      }
      if(!std::isfinite(a.value)||!std::isfinite(a.secondary)||std::fabs(a.value)>2||std::fabs(a.secondary)>2)return {false,"surface_point_out_of_range"};
      parameters_[index(horizontal)]=a.value;parameters_[index(vertical)]=a.secondary;break;
    }
    case MathActionKind::SymmetryTurn:
      if(snapshot_.kind!=MathObjectKind::Symmetry||snapshot_.level>1||a.vertex>5)return {false,"turn_not_available"};
      if(symmetryMoveCount_==symmetryMoves_.size())return {false,"turn_history_full"};
      symmetryMoves_[symmetryMoveCount_++]=a.vertex;break;
    case MathActionKind::SymmetryUndo:
      if(snapshot_.kind!=MathObjectKind::Symmetry||snapshot_.level>1||symmetryMoveCount_==0)return {false,"no_turn_to_undo"};
      --symmetryMoveCount_;break;
    case MathActionKind::SymmetryIdentity:
      if(snapshot_.kind!=MathObjectKind::Symmetry||snapshot_.level>1)return {false,"turn_history_not_available"};
      symmetryMoveCount_=0;break;
    case MathActionKind::ModularStep: {
      if(snapshot_.kind!=MathObjectKind::Modular||snapshot_.level!=1)return {false,"modular_walk_not_available"};
      if(a.value!=1&&a.value!=-1)return {false,"step_direction_must_be_plus_or_minus_one"};
      if(modularWalkSteps_==64)return {false,"modular_walk_full"};
      const auto next=remainder(static_cast<int>(parameter(MathParameter::ModValue)+a.value*parameter(MathParameter::ModStep)),static_cast<int>(parameter(MathParameter::Modulus)));
      parameters_[index(MathParameter::ModValue)]=next;modularVisitedMask_|=1U<<next;++modularWalkSteps_;break;
    }
    case MathActionKind::ResetModularWalk:
      if(snapshot_.kind!=MathObjectKind::Modular||snapshot_.level!=1)return {false,"modular_walk_not_available"};
      parameters_[index(MathParameter::ModValue)]=0;modularVisitedMask_=1;modularWalkSteps_=0;break;
    case MathActionKind::ProbabilityStep: {
      if(snapshot_.kind!=MathObjectKind::Probability||snapshot_.level!=0)return {false,"probability_walk_not_available"};
      if(probabilityWalkCount_==probabilityWalk_.size())return {false,"probability_walk_full"};
      const auto matrix=probabilityMatrix(static_cast<unsigned>(parameter(MathParameter::ProbabilityRule)),parameter(MathParameter::ProbabilityStay));
      const double sample=randomUnit(probabilityRng_);double cumulative=0;unsigned next=2;
      for(unsigned j=0;j<3;++j){cumulative+=matrix[3*probabilityWalk_[probabilityWalkCount_-1]+j];if(sample<cumulative){next=j;break;}}
      probabilityWalk_[probabilityWalkCount_++]=next;break;
    }
    case MathActionKind::ResetProbabilityWalk:
      if(snapshot_.kind!=MathObjectKind::Probability||snapshot_.level!=0)return {false,"probability_walk_not_available"};
      break;
    case MathActionKind::BernoulliStep:
      if(snapshot_.kind!=MathObjectKind::Binomial||snapshot_.level!=0)return {false,"trial_path_not_available"};
      if(bernoulliSteps_>=parameter(MathParameter::BinomialTrials))return {false,"trial_path_complete"};
      bernoulliPath_[bernoulliSteps_+1]=bernoulliPath_[bernoulliSteps_]+(randomUnit(bernoulliRng_)<parameter(MathParameter::BinomialChance)?1:0);++bernoulliSteps_;break;
    case MathActionKind::ResetBernoulli:
      if(snapshot_.kind!=MathObjectKind::Binomial||snapshot_.level!=0)return {false,"trial_path_not_available"};
      break;
    case MathActionKind::ReverseFieldPath:
      if(snapshot_.kind!=MathObjectKind::VectorField||snapshot_.level==0)return {false,"field_path_not_available"};
      fieldPathReversed_=!fieldPathReversed_;parameters_[index(MathParameter::FieldTime)]=1-parameter(MathParameter::FieldTime);snapshot_.playing=false;break;
    case MathActionKind::TogglePlayback: {
      const auto time=playbackParameter();
      if(!parameterAvailable(time))return {false,"playback_not_available"};
      if(!snapshot_.playing&&parameter(time)>=parameters[index(time)].maximum)return {false,"restart_time_before_playing"};
      snapshot_.playing=!snapshot_.playing;break;
    }
    case MathActionKind::AdvanceTime: {
      const auto time=playbackParameter();
      if(!parameterAvailable(time))return {false,"playback_not_available"};
      if(!std::isfinite(a.value)||a.value<=0||a.value>12)return {false,"invalid_time_step"};
      const auto end=parameters[index(time)].maximum;
      if(parameter(time)>=end)return {false,"time_window_finished"};
      parameters_[index(time)]=std::min(end,parameter(time)+a.value*objects[index(snapshot_.kind)].playbackRate);
      if(parameter(time)>=end)snapshot_.playing=false;
      break;
    }
    case MathActionKind::ObjectPreset: {
      const auto presets=mathObjectPresets(snapshot_.kind,snapshot_.level);
      if(a.vertex>=presets.size())return {false,"object_preset_not_available"};
      const auto& preset=presets[a.vertex];snapshot_.playing=false;
      for(unsigned i=0;i<preset.count;++i)parameters_[index(preset.parameters[i])]=preset.values[i];
      break;
    }
    case MathActionKind::Check:snapshot_.playing=false;check();return {true,"challenge_checked"};
    default:return {false,"unknown_action"};
  }
  const auto touched=[&](MathParameter p){return (a.kind==MathActionKind::SetParameter&&a.parameter==p)||(a.kind==MathActionKind::ResetParameters&&a.resetParameters.test(index(p)));};
  if(a.kind==MathActionKind::Select||a.kind==MathActionKind::Reset||a.kind==MathActionKind::SetLevel||
     (touched(MathParameter::Modulus)||touched(MathParameter::ModValue)||touched(MathParameter::ModStep))) {
    modularWalkSteps_=0;modularVisitedMask_=1U<<remainder(static_cast<int>(parameter(MathParameter::ModValue)),static_cast<int>(parameter(MathParameter::Modulus)));
  }
  if(a.kind==MathActionKind::Select||a.kind==MathActionKind::Reset||a.kind==MathActionKind::SetLevel||a.kind==MathActionKind::ResetProbabilityWalk||
     (touched(MathParameter::ProbabilityRule)||touched(MathParameter::ProbabilityStay)||touched(MathParameter::ProbabilityStart)||touched(MathParameter::ProbabilitySeed))) {
    probabilityWalkCount_=1;probabilityWalk_[0]=static_cast<unsigned>(parameter(MathParameter::ProbabilityStart));
    probabilityRng_=static_cast<std::uint32_t>(parameter(MathParameter::ProbabilitySeed));if(!probabilityRng_)probabilityRng_=0x9e3779b9U;
  }
  if(a.kind==MathActionKind::Select||a.kind==MathActionKind::Reset||a.kind==MathActionKind::SetLevel||a.kind==MathActionKind::ResetBernoulli||
     (touched(MathParameter::BinomialTrials)||touched(MathParameter::BinomialChance)||touched(MathParameter::BinomialSeed))) {
    bernoulliSteps_=0;bernoulliPath_.fill(0);bernoulliRng_=static_cast<std::uint32_t>(parameter(MathParameter::BinomialSeed));if(!bernoulliRng_)bernoulliRng_=0x9e3779b9U;
  }
  if(a.kind!=MathActionKind::SwapBounds)reversedNegativeIntegral_=false;
  snapshot_.feedback=MathFeedback::None;snapshot_.feedbackText={};++snapshot_.revision;rebuild();return {true,"applied"};
}
void MathObjects::check() {
  bool solved=false;std::string_view good,bad;
  const auto measured=[&](std::string_view name) {
    for(std::size_t i=0;i<snapshot_.metricCount;++i)if(snapshot_.metrics[i].label==name)return snapshot_.metrics[i].value;
    throw std::logic_error("missing challenge measurement");
  };
  switch(snapshot_.kind) {
    case MathObjectKind::Algebra:
      solved=std::fabs(parameter(MathParameter::X)-2)<1e-6 && parameter(MathParameter::Gap)==0;
      good="Yes: x=2 gives volume 8+12+6+1=27.";bad="A volume of 27 needs side length 3. Then close the gaps.";break;
    case MathObjectKind::Trig:
      solved=std::fabs(parameter(MathParameter::Angle)-150)<1e-6;
      good="Yes: at 150 degrees (5*pi/6), sine is 0.5 and cosine is negative.";bad="Look between 90 and 180 degrees for height 0.5.";break;
    case MathObjectKind::Calculus:
      solved=snapshot_.metrics[2].value<.1;
      good="Yes: the disk-sum volume is within 0.1% of the integral.";bad="Increase the slice count and compare sampling rules.";break;
    case MathObjectKind::Linear:
      switch(snapshot_.level) {
        case 0:solved=measured("Rank")==2;good="Yes: rank 2 and zero three-dimensional volume.";bad="Change vertical scale. A shear alone preserves volume.";break;
        case 1:solved=measured("Vector length")>.1&&measured("Length of A v")>.01&&measured("Eigenvector residual")<.01;good="Yes: A v remains on the line spanned by nonzero v.";bad="Try the stretch/reflection preset and align v with a coordinate axis.";break;
        case 2:solved=measured("Vector length")>.1&&measured("Residual length")<.01;good="Yes: the nonzero vector lies in the column span.";bad="Choose v in the span of the first two columns. The projection residual must vanish.";break;
        case 3:solved=measured("Rank")==2&&measured("Singular value 3")<1e-7;good="Yes: one stretch vanished and two independent directions remain.";bad="Try the xy projection preset and inspect the three singular values.";break;
      }
      break;
    case MathObjectKind::Discrete:
      solved=snapshot_.route[snapshot_.routeCount-1]==7 && snapshot_.routeCount-1==snapshot_.shortestHops;
      good="Yes: your route reaches H in the minimum number of edge hops.";bad="Reach H using the fewest hops. Every edge costs one.";break;
    case MathObjectKind::Function:
      switch(snapshot_.level) {
        case 0:solved=std::fabs(measured("f(x)"))<.01;good="Yes: the output is zero to within 0.01.";bad="Move x toward a crossing or touch of the horizontal axis.";break;
        case 1:solved=std::fabs(parameter(MathParameter::DeltaX))>1e-12&&measured("Secant error")<.02;good="Yes: a nonzero secant step approximates the derivative within 0.02.";bad="Reduce the nonzero magnitude of h and compare the slopes.";break;
        case 2:solved=reversedNegativeIntegral_&&measured("Signed integral")>.001;good="Yes: reversing the bounds turned the negative integral into its positive opposite.";bad="First make the integral negative, then press Swap bounds and check.";break;
        case 3:solved=std::fabs(parameter(MathParameter::FunctionX)-parameter(MathParameter::TaylorCenter))>=.5-1e-9&&measured("Taylor error")<.001;good="Yes: the Taylor value is within 0.001 at least 0.5 from its centre.";bad="Move at least 0.5 from the centre and adjust the approximation degree.";break;
      }
      break;
    case MathObjectKind::Surface:
      switch(snapshot_.level) {
        case 0:solved=parameter(MathParameter::SurfaceRule)==0&&std::fabs(measured("Height")-1)<.01;good="Yes: the bowl has height 1 here.";bad="Select the bowl and try u=1, v=1.";break;
        case 1:solved=parameter(MathParameter::SurfaceRule)==0&&measured("Gradient magnitude")>.5&&std::fabs(measured("Directional derivative"))<.02;good="Yes: that direction is tangent to a contour and has almost zero slope.";bad="Away from the origin on the bowl, rotate the direction perpendicular to the gradient.";break;
        case 2:solved=parameter(MathParameter::SurfaceRule)==0&&measured("Gradient magnitude")<.02;good="Yes: descent reached the bowl's minimum to within the gradient tolerance.";bad="Select the bowl and take descent steps toward its minimum.";break;
        case 3:solved=parameter(MathParameter::SurfaceRule)==1&&parameter(MathParameter::Constraint)==1&&std::fabs(measured("Tangential derivative"))<.02;good="Yes: this is stationary along the circle although the full gradient need not vanish.";bad="Select the saddle, enable the circle, and try a point on a coordinate axis.";break;
      }
      break;
    case MathObjectKind::Symmetry:
      switch(snapshot_.level) {
        case 0:solved=snapshot_.symmetry.permutation[0]==7;good="Yes: labelled A now occupies fixed slot H.";bad="Try two X quarter turns followed by one Y quarter turn.";break;
        case 1:solved=symmetryMoveCount_==2&&symmetryMoves_[0]==static_cast<unsigned>(parameter(MathParameter::SymmetryFirst))&&symmetryMoves_[1]==static_cast<unsigned>(parameter(MathParameter::SymmetrySecond))&&measured("Order-dependent destinations")>0;good="Yes: your two turns realise S R, and R S maps vertices differently.";bad="Choose different axes, return to identity, then apply first and second in that order.";break;
        case 2:solved=measured("Generator order")==3&&parameter(MathParameter::SymmetryPower)>0&&measured("Moved vertices")==0;good="Yes: a positive multiple of three diagonal turns returns every vertex.";bad="Choose the body-diagonal generator and try power 3.";break;
        case 3:solved=parameter(MathParameter::SymmetryElement)>0&&measured("Tracked vertex fixed")==1;good="Yes: this non-identity rotation is in the tracked vertex's stabiliser.";bad="Browse rotations until the selected label stays in its original slot.";break;
      }
      break;
    case MathObjectKind::Harmonics:
      switch(snapshot_.level) {
        case 0:solved=std::fabs(parameter(MathParameter::Amplitude1)-1)<1e-9&&std::fabs(measured("First x"))<.02&&measured("First y")>.99;good="Yes: the unit phasor points upwards and its sine projection is one.";bad="Set amplitude 1 and move the angle near pi/2, allowing for frequency and phase.";break;
        case 1:solved=parameter(MathParameter::Amplitude1)>.1&&std::fabs(parameter(MathParameter::Amplitude1)-parameter(MathParameter::Amplitude2))<1e-9&&parameter(MathParameter::Frequency1)==parameter(MathParameter::Frequency2)&&std::fabs(std::remainder(parameter(MathParameter::Phase1)-parameter(MathParameter::Phase2),360))<1e-9;good="Yes: equal nonzero amplitudes, equal frequencies and matching phases trace a circle in x-y.";bad="Match the two nonzero amplitudes, frequencies and phases.";break;
        case 2:solved=parameter(MathParameter::Waveform)==0&&measured("Full-period RMS error")<.2;good="Yes: the square-wave Fourier approximation has RMS error below 0.2.";bad="Select the square wave and add more nonzero odd harmonics.";break;
        case 3:solved=parameter(MathParameter::Waveform)==0&&static_cast<unsigned>(parameter(MathParameter::ProbeFrequency))%2==0&&std::fabs(measured("Midpoint coefficient"))<1e-3;good="Yes: positive and negative contributions cancel for this absent even sine harmonic.";bad="On the square wave, probe an even frequency and inspect the product integral.";break;
      }
      break;
    case MathObjectKind::Oscillator:
      switch(snapshot_.level) {
        case 0:solved=parameter(MathParameter::MotionSystem)==0&&std::fabs(parameter(MathParameter::InitialPosition))>=.4&&parameter(MathParameter::InitialVelocity)==0&&parameter(MathParameter::MotionTime)>.1&&std::fabs(measured("Displacement"))<.03;good="Yes: the initially displaced spring has reached equilibrium with nonzero motion.";bad="Try initial displacement 1, zero velocity, k=1, and time near 1.57 seconds.";break;
        case 1:solved=parameter(MathParameter::MotionTime)>=4&&measured("Initial energy")>.1&&measured("Energy balance error")<1e-6;good="Yes: the unforced, undamped motion conserves energy within the numerical tolerance.";bad="Choose a nonzero initial state and advance to at least 4 seconds.";break;
        case 2:solved=parameter(MathParameter::Damping)>0&&measured("Initial energy")>.1&&measured("Mechanical energy")<.25*measured("Initial energy");good="Yes: damping removed more than three quarters of the initial mechanical energy.";bad="Use positive damping and advance time. Try a spring with c=1 and k=1.";break;
        case 3:solved=parameter(MathParameter::MotionSystem)==0&&parameter(MathParameter::DriveAmplitude)>0&&parameter(MathParameter::MotionTime)>=4&&measured("Driving work")>.1&&measured("Energy balance error")<1e-5;good="Yes: the driver added net work, balanced by stored energy and damping loss.";bad="Drive a spring for at least 4 seconds and inspect net work and energy balance.";break;
      }
      break;
    case MathObjectKind::Modular:
      switch(snapshot_.level) {
        case 0:solved=parameter(MathParameter::Modulus)==7&&parameter(MathParameter::ModValue)==-3&&measured("Canonical residue")==4;good="Yes: -3 = (-1)*7 + 4, so its canonical residue is 4.";bad="Choose modulus 7 and integer -3, then read the remainder.";break;
        case 1:solved=parameter(MathParameter::ModStep)>0&&measured("Visited residues")==parameter(MathParameter::Modulus);good="Yes: your actual walk has visited every residue.";bad="Choose a step coprime to the modulus and keep stepping. Changing the setup clears observed visits.";break;
        case 2:solved=parameter(MathParameter::Modulus)==7&&remainder(static_cast<int>(parameter(MathParameter::ModStep)),7)==3&&measured("Your product residue")==1;good="Yes: 3 times 5 is 1 modulo 7.";bad="Use modulus 7, multiplier 3 and an inverse whose product leaves remainder 1.";break;
        case 3:solved=parameter(MathParameter::Modulus)!=parameter(MathParameter::SecondModulus)&&measured("Compatible")==1&&measured("Common period")>=6&&parameter(MathParameter::CrtGuess)==measured("Smallest solution");good="Yes: that is the least nonnegative solution, repeated once per common period.";bad="Use a compatible pair of distinct moduli. Try x=2 modulo 3 and x=3 modulo 5, then choose x=8.";break;
      }
      break;
    case MathObjectKind::Gaussian:
      switch(snapshot_.level) {
        case 0:solved=parameter(MathParameter::GaussianOperation)==1&&parameter(MathParameter::GaussianImag)!=0&&parameter(MathParameter::GaussianOtherImag)!=0&&measured("Result imaginary")==0&&measured("Result real")!=0;good="Yes: two non-real factors produced a nonzero real Gaussian integer.";bad="Try multiplying 1+i by 1-i.";break;
        case 1:solved=measured("Norm z")>0&&measured("Norm w")>1&&measured("Divides exactly")==1&&measured("Quotient norm")>1;good="Yes: z is an exact nonunit multiple of this nonunit divisor.";bad="Try z=2+2i and w=1+i. Inspect the quotient and zero remainder.";break;
        case 2:solved=measured("Generator norm")>=2&&measured("Selected point norm")>0&&measured("In principal ideal")==1&&measured("Quotient real")==0&&measured("Quotient imaginary")!=0;good="Yes: the selected point is a nonzero imaginary multiple of the generator.";bad="Try generator 2+i and selected point -1+2i, which is i times the generator.";break;
        case 3:solved=parameter(MathParameter::GaussianQuotient)==0&&parameter(MathParameter::GaussianOperation)==1&&measured("z class")!=0&&measured("w class")!=0&&measured("Result class")==0;good="Yes: two nonzero classes multiply to zero in Z[i]/(2).";bad="Choose the quotient by 2 and try z=w=1+i. Its square is 2i, in the ideal (2).";break;
      }
      break;
    case MathObjectKind::VectorField:
      switch(snapshot_.level) {
        case 0:solved=parameter(MathParameter::FieldRule)==1&&measured("Field magnitude")>.25&&std::fabs(measured("Directional component"))<.02;good="Yes: the direction is perpendicular to the nonzero radial field vector.";bad="Try probe (1,0,0), yaw 90 degrees and pitch 0.";break;
        case 1: {const auto p=fieldPath(static_cast<unsigned>(parameter(MathParameter::FieldPath)),parameter(MathParameter::FieldTime),fieldPathReversed_);solved=parameter(MathParameter::FieldPath)==1&&std::fabs(p.position.y-1)<.01&&p.velocity.x>0&&std::fabs(p.velocity.y)<.05;good="Yes: the probe is at the top and the tangent points right.";bad="Select the forward upper semicircle and scrub t to 0.5.";break;}
        case 2:solved=parameter(MathParameter::FieldRule)==2&&parameter(MathParameter::FieldPath)==4&&fieldPathReversed_&&parameter(MathParameter::FieldTime)>=1&&measured("Total work")<-6;good="Yes: completing the reversed circle gives work -2*pi in the vortex.";bad="Choose the vortex and closed circle, reverse its orientation, then finish the path.";break;
        case 3:solved=parameter(MathParameter::FieldRule)==2&&std::fabs(measured("Path difference"))>2;good="Yes: the two routes have the same endpoints but different work in this field.";bad="Choose the vortex and a semicircle, then compare it with the straight route.";break;
      }
      break;
    case MathObjectKind::Flux:
      switch(snapshot_.level) {
        case 0:solved=parameter(MathParameter::FluxShape)==0&&parameter(MathParameter::FluxField)==1&&parameter(MathParameter::FluxOrientation)==1&&measured("Probe flux density")<0;good="Yes: inward normals give negative local flux for the radial field.";bad="Select the sphere and radial field, then reverse the orientation.";break;
        case 1:solved=parameter(MathParameter::FluxShape)==2&&parameter(MathParameter::FluxField)==0&&measured("Numerical flux")>.5;good="Yes: the constant field crosses the oriented disk with positive net flux.";bad="Choose an open disk, constant field and positive orientation; tilt it towards the field.";break;
        case 2:solved=parameter(MathParameter::FluxShape)==0&&parameter(MathParameter::FluxField)==3&&parameter(MathParameter::FluxRadius)==1&&parameter(MathParameter::FluxTilt)==0&&parameter(MathParameter::FluxOrientation)==0&&measured("Relative error")<.01;good="Yes: the surface sum approaches 4*pi/3, matching the divergence integral.";bad="Use the outward unit sphere, twist field and tilt zero. Raise the integration resolution.";break;
        case 3:solved=parameter(MathParameter::FluxField)==2&&parameter(MathParameter::FluxOrientation)==1&&parameter(MathParameter::FluxTime)==1&&measured("Boundary circulation")<-1&&measured("Stokes error")<1e-8;good="Yes: the reversed boundary and curl flux agree with negative sign.";bad="Choose the vortex, tilt zero and reversed normal, then finish the boundary.";break;
      }
      break;
    case MathObjectKind::Tensor:
      switch(snapshot_.level) {
        case 0:solved=measured("Tensor norm")>.1&&std::fabs(measured("Trace"))<1e-8;good="Yes: orthogonal nonzero factors give a nonzero outer product with zero trace.";bad="Try u=(1,1,0) and v=(1,-1,0).";break;
        case 1:solved=measured("Selected component")<-.1;good="Yes: the selected three-factor product is negative.";bad="Select indices whose three factors are nonzero and whose product has negative sign.";break;
        case 2:solved=measured("Tensor norm")>.1&&measured("Contraction norm")<1e-8;good="Yes: the tensor is nonzero but summing the paired indices cancels its contraction.";bad="Choose nonzero u, v=(1,1,0) and w=(1,-1,0).";break;
        case 3:solved=measured("Component change")>.2&&measured("Trace error")<1e-10&&measured("Norm error")<1e-10;good="Yes: changing the orthonormal basis changed components while preserving the invariants.";bad="Choose a nonzero nonscalar outer product and rotate the coordinate basis.";break;
      }
      break;
    case MathObjectKind::Probability:
      switch(snapshot_.level) {
        case 0:solved=measured("Visited states")==3;good="Yes: the recorded walk has reached all three states.";bad="Step through a cycle until the observed path includes A, B and C.";break;
        case 1:solved=parameter(MathParameter::ProbabilityRule)==1&&parameter(MathParameter::ProbabilityStay)==.5&&parameter(MathParameter::ProbabilityRow)==2;good="Yes: row C assigns probabilities 0.25, 0.15 and 0.60 to A, B and C.";bad="Choose weighted mixing, retention 0.5 and outgoing row C.";break;
        case 2:solved=parameter(MathParameter::ProbabilityRule)==3&&parameter(MathParameter::ProbabilityStart)!=2&&parameter(MathParameter::ProbabilityMix)<.5&&parameter(MathParameter::ProbabilitySteps)>0&&measured("p(C)")>.95;good="Yes: over 95 percent of the initially external mass has reached absorbing state C.";bad="Start at A or B in the absorbing chain, retain less than 1, and advance the distribution.";break;
        case 3:solved=parameter(MathParameter::ProbabilityRule)==1&&parameter(MathParameter::ProbabilityStay)<1&&parameter(MathParameter::ProbabilitySteps)>=10&&measured("Distance to pi")<.001;good="Yes: the distribution is close to the stationary probabilities (0.5, 0.3, 0.2).";bad="Choose weighted mixing with retention below 1 and advance at least 10 transitions.";break;
      }
      break;
    case MathObjectKind::Binomial:
      switch(snapshot_.level) {
        case 0:solved=bernoulliSteps_==parameter(MathParameter::BinomialTrials)&&bernoulliPath_[bernoulliSteps_]>0&&bernoulliPath_[bernoulliSteps_]<bernoulliSteps_;good="Yes: the completed independent-trial path contains both outcomes.";bad="Choose several trials with p between 0 and 1, and finish the path.";break;
        case 1:solved=parameter(MathParameter::BinomialTrials)==6&&parameter(MathParameter::BinomialChance)==.5&&parameter(MathParameter::BinomialCut)==3;good="Yes: P(X=3)=20/64=0.3125 at the centre of this binomial distribution.";bad="Set n=6, p=0.5 and k=3.";break;
        case 2:solved=parameter(MathParameter::BinomialChance)>0&&parameter(MathParameter::BinomialChance)<1&&parameter(MathParameter::BinomialCut)<parameter(MathParameter::BinomialTrials)&&measured("Tail P(X>k)")>0&&measured("Tail P(X>k)")<.1;good="Yes: this nonempty upper tail has probability below 0.1.";bad="Try n=6, p=0.5 and k=5.";break;
        case 3:solved=parameter(MathParameter::BinomialTrials)==12&&parameter(MathParameter::BinomialChance)==.5&&measured("Maximum CDF error")<.02;good="Yes: the continuity-corrected normal approximation is close for this symmetric example.";bad="Use twelve trials with p=0.5, then compare the CDF error.";break;
      }
      break;
    case MathObjectKind::Bayes:
      switch(snapshot_.level) {
        case 0:solved=parameter(MathParameter::BayesHit)==parameter(MathParameter::BayesFalse)&&parameter(MathParameter::BayesHit)>0&&parameter(MathParameter::BayesHit)<1&&parameter(MathParameter::BayesPrior)>0&&parameter(MathParameter::BayesPrior)<1;good="Yes: equal conditional likelihoods make E independent of H.";bad="Give both hypotheses the same event likelihood, with prior and likelihood between zero and one.";break;
        case 1:solved=measured("Conditioning defined")==1&&measured("Posterior shift")>.2;good="Yes: the selected evidence raised the probability of H by over 0.2.";bad="Choose an event more likely under H than under not-H.";break;
        case 2:solved=measured("Conditioning defined")==1&&parameter(MathParameter::BayesPrior)>0&&parameter(MathParameter::BayesPrior)<.5&&measured("Posterior P(H)")>.8;good="Yes: strong likelihood evidence overcame the initial odds against H.";bad="Try prior 0.3, P(E|H)=0.95 and P(E|not H)=0.05.";break;
        case 3:solved=measured("Conditioning defined")==1&&parameter(MathParameter::BayesPositive)>=3&&measured("Posterior P(H)")>.95;good="Yes: repeated conditionally independent evidence strongly favours H.";bad="Try three E outcomes, no not-E outcomes, and an event much more likely under H.";break;
      }
      break;
    case MathObjectKind::Covariance:
      switch(snapshot_.level) {
        case 0:solved=std::fabs(measured("Mean x")-.5)<1e-10&&std::fabs(measured("Mean y")+.5)<1e-10&&std::fabs(measured("Mean z"))<1e-10&&measured("Total variance")>.1;good="Yes: the cloud's population mean is (0.5,-0.5,0).";bad="Set the mean controls to (0.5,-0.5,0), retaining some spread.";break;
        case 1:solved=measured("Correlation defined")==1&&measured("Correlation xy")>.5;good="Yes: the x and y deviations have positive correlation above 0.5.";bad="Try equal x/y stretches, no rotation and shear 1.";break;
        case 2:solved=measured("Covariance rank")==2&&measured("Smallest variance")==0&&measured("Middle variance")>0;good="Yes: the cloud occupies a plane and its third principal variance is zero.";bad="Keep two independent stretches and set the third to zero.";break;
        case 3:solved=measured("Whitening defined")==1&&parameter(MathParameter::CloudWhiten)==1&&measured("Identity covariance error")<1e-8;good="Yes: the transformed population has identity covariance.";bad="Keep all three stretches positive and move fully to whitened coordinates.";break;
      }
      break;
    case MathObjectKind::Spherical:
      switch(snapshot_.level) {
        case 0:solved=std::fabs(measured("Direction y")-1)<1e-10;good="Yes: theta=90 and phi=90 point along positive y.";bad="Move both angular controls to 90 degrees.";break;
        case 1:solved=measured("Degree")==3&&std::fabs(measured("Order"))==2;good="Yes: this degree-3 mode has absolute order 2.";bad="Select l=3, m=+2 or m=-2.";break;
        case 2:solved=parameter(MathParameter::SphereMode)!=parameter(MathParameter::SphereSecond)&&std::fabs(parameter(MathParameter::SphereMix))>.1&&std::fabs(measured("Cross inner product"))<1e-10;good="Yes: the distinct modes are orthogonal and both contribute.";bad="Choose two different modes and a nonzero second coefficient.";break;
        case 3:solved=sphereMode(static_cast<unsigned>(parameter(MathParameter::SphereMode))).degree>0&&sphereMode(static_cast<unsigned>(parameter(MathParameter::SphereSecond))).degree>0&&std::fabs(parameter(MathParameter::SphereMix))>.1&&parameter(MathParameter::SphereHeat)>.2&&measured("Initial energy")>0&&measured("Energy")<.5*measured("Initial energy");good="Yes: diffusion reduced the nonconstant modes' total energy by more than half.";bad="Choose two nonconstant modes, keep a nonzero second coefficient and increase heat time beyond 0.2.";break;
      }
      break;
    case MathObjectKind::Quadratic:
      switch(snapshot_.level) {
        case 0:solved=measured("Quadratic value")<-.1;good="Yes: this form takes a negative value at the selected point.";bad="Set the first principal value negative and place the probe on the x axis.";break;
        case 1:solved=(std::fabs(parameter(MathParameter::QuadYaw))>10||std::fabs(parameter(MathParameter::QuadPitch))>10)&&std::fabs(measured("Quadratic value"))>.1&&measured("Coordinate identity error")<1e-10;good="Yes: the rotated basis preserves the quadratic evaluation.";bad="Rotate the basis and use a probe with a nonzero quadratic value.";break;
        case 2:solved=measured("Positive directions")==1&&measured("Negative directions")==1&&measured("Null directions")==1;good="Yes: inertia (1,1,1) is singular and indefinite.";bad="Choose one positive, one negative and one zero principal value.";break;
        case 3:solved=measured("Rayleigh defined")==1&&measured("Largest eigenvalue")-measured("Smallest eigenvalue")>.1&&std::fabs(measured("Rayleigh quotient")-measured("Smallest eigenvalue"))<1e-8;good="Yes: the normalized probe attains the smallest eigenvalue.";bad="With zero basis rotation and values (1.5,1,0.5), use the z-axis probe (0,0,1).";break;
      }
      break;
    case MathObjectKind::Roots: {
      const unsigned n=static_cast<unsigned>(parameter(MathParameter::RootN));bool composite=false;for(unsigned d=2;d<n;++d)composite=composite||n%d==0;
      switch(snapshot_.level) {
        case 0:solved=composite&&measured("Probe order")==n;good="Yes: the selected root generates all roots even though n is composite.";bad="Try n=8 and probe exponent 3.";break;
        case 1:solved=measured("Distinct images")>1&&measured("Distinct images")<n;good="Yes: several roots share each image, but the map still has multiple outputs.";bad="Try n=8 and power exponent 2.";break;
        case 2:solved=measured("Subgroup order")>1&&measured("Subgroup order")<n&&measured("Selected power")>0&&measured("Power result exponent")==0;good="Yes: the nontrivial proper subgroup returned to identity.";bad="Try n=8, generator exponent 2 and selected power 4.";break;
        case 3:solved=n==8&&measured("Automorphism defined")==1&&measured("Map exponent")!=1&&measured("Fixed roots")>2;good="Yes: exponent 5 modulo 8 is an order-2 automorphism fixing four roots.";bad="Use n=8 and candidate exponent 5.";break;
      }
      break;
    }
    case MathObjectKind::Psd:
      switch(snapshot_.level) {
        case 0:solved=measured("PSD")==1&&measured("Rank")==1;good="Yes: a nonzero PSD boundary matrix has rank one.";bad="Try a=c=1 and move b to 1, or use the rank-one preset.";break;
        case 1:solved=measured("Probe quadratic value")<-.1;good="Yes: this direction witnesses a negative quadratic value.";bad="Choose the saddle preset and move the probe angle to 90 degrees.";break;
        case 2:solved=measured("Endpoint A PSD")==1&&measured("Endpoint A rank")==1&&measured("Smallest eigenvalue")>.1&&parameter(MathParameter::PsdMix)>0&&parameter(MathParameter::PsdMix)<1;good="Yes: the two rank-one directions combine into a positive definite matrix.";bad="Use the rank-one A preset, B angle 0, an interior mixture and positive ray scale.";break;
        case 3:solved=parameter(MathParameter::PsdSlice)>0&&std::fabs(parameter(MathParameter::PsdObjective)-1)<1e-10;good="Yes: the objective plane supports the disk at its maximum.";bad="Keep the slice height positive and move the objective fraction to 1.";break;
      }
      break;
    case MathObjectKind::Norm: {
      unsigned nonzero=0;for(auto p:{MathParameter::NormX,MathParameter::NormY,MathParameter::NormZ})nonzero+=std::fabs(parameter(p))>1e-10;
      switch(snapshot_.level) {
        case 0:solved=parameter(MathParameter::NormInfinity)==0&&parameter(MathParameter::NormP)==1&&nonzero>=2&&std::fabs(measured("Vector norm")-1)<1e-10;good="Yes: this non-axis point is on the octahedron's unit boundary.";bad="Choose p=1 and set x=(0.5,0.5,0).";break;
        case 1:solved=measured("Vector norm")>.1&&measured("Other vector norm")>.1&&measured("Triangle slack")>.05;good="Yes: the norm of the sum is strictly smaller than the sum of the norms.";bad="Choose two nonzero vectors that point in different directions, such as (1,0,0) and (0,1,0) at p=2.";break;
        case 2:solved=parameter(MathParameter::NormInfinity)==0&&parameter(MathParameter::NormP)>=8&&nonzero>=2&&measured("Limit gap")<.05;good="Yes: a finite-p norm is now within 0.05 of the fixed vector's max norm.";bad="Keep two components nonzero and raise finite p to 8 or more.";break;
        case 3:solved=measured("Support defined")==1&&std::fabs(parameter(MathParameter::NormSupport)-1)<1e-10;good="Yes: the plane is tight and the dual norm gives its support value.";bad="Choose a nonzero w and set the plane fraction to 1.";break;
      }
      break;
    }
    case MathObjectKind::Curve:
      switch(snapshot_.level) {
        case 0:solved=parameter(MathParameter::CurveProgress)>=.2&&parameter(MathParameter::CurveProgress)<=.8&&measured("Control tetrahedron volume")>.001&&measured("Smallest weight")>0;good="Yes: positive Bernstein weights place this curve point inside the control tetrahedron.";bad="Keep t between 0.2 and 0.8 and move one handle out of the controls' plane.";break;
        case 1:solved=measured("Curvature defined")==1&&measured("Curvature")>.5;good="Yes: this regular point has curvature above 0.5.";bad="Use the curved pipe and move the position to 0.5, or bring its handles closer together.";break;
        case 2:solved=measured("Distance travel defined")==1&&measured("Distance travel")==1&&measured("Parameter distance spread")>.1&&measured("Equal distance spread")<.0001;good="Yes: distance travel equalizes arc-length intervals despite the uneven parameter speed.";bad="Try the arched cable preset and choose equal distance steps.";break;
        case 3:solved=measured("Sweep available")==1&&parameter(MathParameter::CurveEndScale)<=.25&&std::fabs(parameter(MathParameter::CurveTwist))>=90;good="Yes: the transported profile tapers and twists along a regular curve.";bad="Use a regular curve, reduce end scale to 0.25 or less and add at least 90 degrees of twist.";break;
      }
      break;
    case MathObjectKind::Lathe:
      switch(snapshot_.level) {
        case 0:solved=measured("Interior bulge")>=.2-1e-12;good="Yes: an interior bulge is wider than both ends.";bad="Increase an interior point's radius or narrow both endpoint radii.";break;
        case 1:solved=measured("Material volume")>1e-8&&std::fabs(parameter(MathParameter::LatheTurn)-180)<1e-10;good="Yes: a half revolution contains half the full-turn volume.";bad="Keep a nonzero profile and set the revolution angle to 180 degrees.";break;
        case 2:solved=measured("Reference volume")>1e-8&&parameter(MathParameter::LatheSlices)>=16&&measured("Relative volume error")<.02;good="Yes: the midpoint sum is within two percent of the reference volume.";bad="Use at least 16 subdivisions and compare the errors from washers and shells.";break;
        case 3:solved=measured("Boundary area reference")>1e-8&&parameter(MathParameter::LatheSlices)>=16&&measured("Relative area error")<.01&&measured("Normal defined")==1;good="Yes: the frustum bands approximate the boundary area within one percent at a regular point.";bad="Increase subdivisions to 16 or more and move the probe away from the axis.";break;
      }
      break;
    case MathObjectKind::Boolean:
      switch(snapshot_.level){
        case 0:solved=parameter(MathParameter::BooleanOperation)==2&&measured("Field A")<-.01&&measured("Field B")<-.01&&measured("Result field")>.01;good="Yes: the cutter removed this point from the interior of A.";bad="Choose Drilled block and put the probe at (0,0,0).";break;
        case 1:solved=parameter(MathParameter::BooleanOperation)==1&&measured("Field A")<-.01&&measured("Field B")<-.01;good="Yes: intersection retains points inside both inputs.";bad="Choose Drilled block, switch to Intersection and put the probe at the origin.";break;
        case 2:solved=parameter(MathParameter::BooleanOperation)==3&&parameter(MathParameter::BooleanBlend)>0&&measured("Field A")>.001&&measured("Field B")>.001&&measured("Result field")<-.001&&measured("Surface normal defined")==1;good="Yes: the smooth blend adds material outside both inputs and has a regular sampled surface crossing.";bad="Choose Blended stones and keep the probe at (0.6,0.8,0) with blend width 0.5.";break;
        case 3:solved=parameter(MathParameter::BooleanResolution)>=24&&measured("64-cell volume estimate")>1e-6&&measured("48/64 relative change")<.03;good="Yes: the two finer volume estimates agree within three percent. This is not an exact-error certificate.";bad="Try Drilled block with at least 24 requested cells; inspect the 48/64 comparison.";break;
      }break;
    case MathObjectKind::Patch:
      switch(snapshot_.level){
        case 0:solved=measured("Selected basis weight")>1-1e-9;good="Yes: the corner's Bernstein weight is 1, so the patch interpolates that control.";bad="Choose P00 and set the probe to u=0, v=0, or match another corner.";break;
        case 1:solved=measured("Regular probe")==1&&std::fabs(measured("Normal y"))<.2;good="Yes: this regular sheet has a nearly horizontal unit normal.";bad="Try the Sail preset and inspect the normal near the centre.";break;
        case 2:solved=measured("Regular probe")==1&&measured("Gaussian curvature")<-.03;good="Yes: negative Gaussian curvature identifies a saddle with opposite principal-curvature signs.";bad="Try Saddle terrain near u=0.5, v=0.5 and inspect K.";break;
        case 3:solved=parameter(MathParameter::PatchResolution)>=24&&measured("Quadrature area")>.1&&measured("Mesh difference")<.5&&measured("Quadrature change")<.1;good="Yes: both comparisons agree at the requested tolerance. This is numerical evidence, not an exact error bound.";bad="Use Canopy with at least 24 subdivisions and inspect both percentage comparisons.";break;
      }break;
    case MathObjectKind::Membrane:
      switch(snapshot_.level){
        case 0:solved=parameter(MathParameter::MembraneTime)>0&&measured("Initial energy")>1e-6&&measured("Combined displacement")<-.05;good="Yes: the membrane has crossed below equilibrium at the probe.";bad="Choose Drumhead and advance model time to about 2 seconds with the probe at the centre.";break;
        case 1:solved=parameter(MathParameter::MembraneU)>.01&&parameter(MathParameter::MembraneU)<.99&&parameter(MathParameter::MembraneV)>.01&&parameter(MathParameter::MembraneV)<.99&&measured("Selected excitation")>1e-9&&measured("Internal nodal lines")>0&&std::fabs(measured("Selected spatial weight"))<1e-8;good="Yes: this interior point lies on a spatial node of the excited selected mode.";bad="Choose Divided membrane, Slot 1, and put the probe at u=0.5, v=0.5. An instant of zero displacement is not enough.";break;
        case 2:solved=measured("Distinct active modes")>=2&&std::fabs(measured("Combined displacement"))<.005&&measured("Cancellation magnitude")>.1;good="Yes: nonzero contributions from distinct spatial modes cancel at this point.";bad="Choose Interference at time 0 with u=0.25 and v=0.5; inspect the component curves.";break;
        case 3:solved=parameter(MathParameter::MembraneDamping)>0&&parameter(MathParameter::MembraneTime)>=3&&measured("Initial energy")>.01&&measured("Energy retained")<25;good="Yes: damping has removed more than three quarters of the initial energy.";bad="Choose Damped pluck and advance model time to 6 seconds. Compare total and initial energy.";break;
      }break;
    case MathObjectKind::Rigid:
      switch(snapshot_.level){
        case 0:solved=std::fabs(measured("Release-axis alignment"))<.05;good="Yes: the tracked body axis is perpendicular to its release direction.";bad="Choose Flywheel, track body X, and scrub time to about 0.52 seconds.";break;
        case 1:solved=measured("COM offset")>.05;good="Yes: the mass imbalance shifts the centre of mass while the total mass stays fixed.";bad="Choose Adjustable dumbbell and change Left mass share to 0.7.";break;
        case 2:solved=measured("Angular speed")>1e-8&&measured("Velocity-momentum angle")>10;good="Yes: angular velocity and momentum are not parallel for this rotation.";bad="Choose Adjustable dumbbell or Satellite and compare the two directions.";break;
        case 3:solved=measured("Distinct principal moments")==1&&measured("Tracked intermediate axis")==1&&measured("Initial momentum alignment")>.99&&measured("Momentum alignment")<-.8&&std::fabs(measured("Relative energy error"))<1e-5&&measured("Relative momentum error")<1e-5;good="Yes: the intermediate axis has flipped while the measured invariants remain within tolerance.";bad="Choose Tumbling book, keep body Z tracked, and advance model time to 4 seconds.";break;
      }break;
    case MathObjectKind::Count:return;
  }
  snapshot_.feedback=solved?MathFeedback::Solved:MathFeedback::TryAgain;snapshot_.feedbackText=solved?good:bad;
}
void MathObjects::rebuild() {
  snapshot_.bernoulliPath=bernoulliPath_;snapshot_.bernoulliSteps=bernoulliSteps_;
  snapshot_.probabilityWalk=probabilityWalk_;snapshot_.probabilityWalkCount=probabilityWalkCount_;
  snapshot_.table.rowCount=0;snapshot_.table.columnCount=0;snapshot_.modularWalkSteps=modularWalkSteps_;snapshot_.modularVisitedMask=modularVisitedMask_;snapshot_.fieldPathReversed=fieldPathReversed_;
  snapshot_.partCount=0;snapshot_.labelCount=0;snapshot_.metricCount=0;snapshot_.allowedVertices.fill(false);
  snapshot_.curve={};snapshot_.solid.vertexCount=snapshot_.solid.indexCount=0;
  snapshot_.plotCount=0;snapshot_.matrixCount=0;snapshot_.surface.rows=0;snapshot_.surface.columns=0;
  snapshot_.contours.count=0;snapshot_.contours.active=false;snapshot_.symmetry.active=false;
  SnapshotBuilder b(snapshot_);const auto value=[&](MathParameter p){return static_cast<float>(parameter(p));};
  switch(snapshot_.kind) {
    case MathObjectKind::Algebra: {
      const float x=value(MathParameter::X),gap=value(MathParameter::Gap);
      const std::array<Vec3,4> colors{gold,coral,blue,teal};
      for(unsigned i=0;i<8;++i) {
        const unsigned bx=i&1,by=(i>>1)&1,bz=(i>>2)&1,degree=3-bx-by-bz;
        const Vec3 size{bx?1:x,by?1:x,bz?1:x};
        b.scaled(MathShape::Box,{bx?x+gap+.5F:x*.5F,by?x+gap+.5F:x*.5F,bz?x+gap+.5F:x*.5F},size,colors[degree],termNames[degree]);
      }
      b.label("x^3",{-.05F,x*.5F,x*.5F},teal);b.label("x^2",{-.05F,x+gap+.5F,x*.5F},blue);
      b.label("x",{x+gap+.5F,x+gap+.5F,-.05F},coral);b.label("1",{x+gap+.5F,x+gap+1.15F,x+gap+.5F},gold);
      const double d=parameter(MathParameter::X);
      b.metric("Total volume",std::pow(d+1,3),"units^3");b.metric("x^3",d*d*d);b.metric("3x^2",3*d*d);b.metric("3x",3*d);b.metric("Constant",1);break;
    }
    case MathObjectKind::Trig: {
      const double theta=parameter(MathParameter::Angle)*pi/180,cosine=std::cos(theta),sine=std::sin(theta);
      const float c=static_cast<float>(cosine),s=static_cast<float>(sine),start=1.8F,scale=.55F;
      b.scaled(MathShape::Ring,{},{1,1,1},white,"unit_circle");
      b.arrow({-1.2F,0,0},{1.3F,0,0},muted,"x_axis");b.arrow({0,-1.2F,0},{0,1.3F,0},muted,"y_axis");
      b.arrow({0,0,.04F},{c,s,.04F},gold,"radius");b.rod({0,0,.07F},{c,0,.07F},coral,.025F,"cosine");b.rod({c,0,.07F},{c,s,.07F},teal,.025F,"sine");
      b.ball({c,s,.07F},.065F,gold,"point_P");b.ball({0,0,.07F},.035F,white,"origin");
      b.arrow({start,0,0},{start+static_cast<float>(2*pi)*scale+.15F,0,0},muted,"theta_axis");
      for(unsigned i=0;i<96;++i) {
        const double a=2*pi*i/96,c1=2*pi*(i+1)/96;
        b.rod({start+scale*static_cast<float>(a),static_cast<float>(std::sin(a)),0},{start+scale*static_cast<float>(c1),static_cast<float>(std::sin(c1)),0},teal,.016F,"sine_wave");
      }
      const float wx=start+scale*static_cast<float>(theta);
      b.ball({wx,s,.04F},.065F,coral,"wave_cursor");b.rod({wx,0,0},{wx,s,0},coral,.012F,"wave_projection");
      for(unsigned i=0;i<20;++i)b.rod({c+(wx-c)*i/20,s,-.045F},{c+(wx-c)*(i+.45F)/20,s,-.045F},muted,.006F,"equal_height");
      b.label("P",{c,s+.2F,.07F},gold);b.label("x",{1.4F,-.1F,0});b.label("y",{.15F,1.35F,0});
      b.label("0",{start,-.18F,0});b.label("pi",{start+scale*static_cast<float>(pi),-.18F,0});b.label("2*pi",{start+scale*static_cast<float>(2*pi),-.18F,0});
      b.metric("cos(theta)",cosine);b.metric("sin(theta)",sine);b.metric("Radians",theta);b.metric("cos^2 + sin^2",cosine*cosine+sine*sine);break;
    }
    case MathObjectKind::Calculus: {
      const unsigned n=static_cast<unsigned>(parameter(MathParameter::Slices));const double dx=2.0/n,f=parameter(MathParameter::Sample)/2;
      const float gap=value(MathParameter::SliceGap);double estimate=0;
      for(unsigned i=0;i<n;++i) {
        const double sample=(i+f)*dx;estimate+=pi*sample*sample*dx;if(sample==0)continue;
        const float radius=static_cast<float>(sample),thickness=static_cast<float>(dx),center=static_cast<float>((i+.5)*dx)+(static_cast<float>(i)-(n-1)*.5F)*gap;
        b.part(MathShape::Disk,{center,0,0},{0,radius,0},{thickness,0,0},{0,0,-radius},i%2?blue:teal,"integration_disk");
      }
      const float extension=static_cast<float>(n-1)*.5F*gap,start=-extension,end=2+extension;
      b.arrow({start-.25F,0,0},{end+.3F,0,0},white,"x_axis");
      if(gap==0) {
        for(unsigned i=0;i<8;++i) {const double a=i*pi/4;b.rod({},{2,2*static_cast<float>(std::cos(a)),2*static_cast<float>(std::sin(a))},gold,.008F,"cone_guide");}
        b.part(MathShape::Ring,{2,0,0},{0,2,0},{0,0,2},{2,0,0},gold,"cone_rim");
      }
      b.label("x=0",{start,-.25F,0});b.label("x=2",{end,-2.2F,0});b.label("r(x)=x",{end,1.5F,1.5F},teal);
      const double exact=8*pi/3;b.metric("Disk-sum volume",estimate);b.metric("Exact volume",exact);b.metric("Relative error",std::fabs(estimate-exact)/exact*100,"%");b.metric("Slice width",dx);break;
    }
    case MathObjectKind::Linear: {
      Matrix a{};for(unsigned i=0;i<9;++i)a[i]=parameter(matrixParameters[i]);
      const auto transformed=[&](Vec3 p){return mapped(a,p);};
      const auto cage=[&](const Matrix& matrix,Vec3 color,float radius,std::string_view role) {
        for(unsigned i=0;i<8;++i)for(unsigned axis=0;axis<3;++axis) {
          const unsigned j=i^(1U<<axis);if(j<i)continue;
          const auto corner=[](unsigned n){return Vec3{static_cast<float>(n&1),static_cast<float>((n>>1)&1),static_cast<float>((n>>2)&1)};};
          b.rod(mapped(matrix,corner(i)),mapped(matrix,corner(j)),color,radius,role);
        }
      };
      cage(identity,muted,.008F,"original_cube");cage(a,teal,.018F,"transformed_cube");
      if(snapshot_.level==0)for(unsigned t=1;t<4;++t)for(float face:{0.0F,1.0F}) {
        const float q=t*.25F;
        b.rod(transformed({q,face,0}),transformed({q,face,1}),teal,.005F,"lattice");
        b.rod(transformed({0,q,face}),transformed({1,q,face}),teal,.005F,"lattice");
        b.rod(transformed({face,0,q}),transformed({face,1,q}),teal,.005F,"lattice");
      }
      const std::array<Vec3,3> basis{{{1,0,0},{0,1,0},{0,0,1}}},colors{coral,teal,violet};
      const std::array<std::string_view,3> names{"A e1","A e2","A e3"};
      for(unsigned i=0;i<3;++i) {b.arrow({},transformed(basis[i]),colors[i],names[i]);b.label(names[i],transformed(basis[i])+Vec3{.05F,.12F,.05F},colors[i]);}
      b.ball({},.03F,white,"origin");
      const double d=determinant(a);b.metric("Signed determinant",d);b.metric("Volume",std::fabs(d),"units^3");b.metric("Rank",rank(a));
      b.matrix("A",a,snapshot_.level>0);
      if(snapshot_.level==0)break;
      const Vec3 v{value(MathParameter::VectorX),value(MathParameter::VectorY),value(MathParameter::VectorZ)},av=transformed(v);
      b.arrow({},v,gold,"input_vector");b.label("v",v+Vec3{0,.15F,0},gold);b.metric("Vector length",length(v));
      switch(snapshot_.level) {
        case 1: {
          const double angle=parameter(MathParameter::ComposeAngle)*pi/180,c=std::cos(angle),s=std::sin(angle);
          const Matrix rotation{c,-s,0,s,c,0,0,0,1},ba=multiply(rotation,a),ab=multiply(a,rotation);
          b.matrix("B A",ba);b.matrix("A B",ab);cage(ba,violet,.012F,"composed_cube");
          const Vec3 bav=mapped(ba,v);b.arrow({},av,teal,"mapped_vector");b.arrow({},bav,violet,"composed_vector");
          b.label("A v",av+Vec3{0,.15F,0},teal);b.label("B A v",bav+Vec3{0,.15F,0},violet);
          const double norm2=dot(v,v),lambda=norm2>1e-12?dot(v,av)/norm2:0;
          b.metric("Length of A v",length(av));b.metric("Eigenvalue candidate",lambda);b.metric("Eigenvector residual",length(av-v*static_cast<float>(lambda)));
          double orderError=0;for(unsigned i=0;i<9;++i)orderError+=(ba[i]-ab[i])*(ba[i]-ab[i]);b.metric("Composition order difference",std::sqrt(orderError));break;
        }
        case 2: {
          std::array<Vec3,2> q{};unsigned count=0;
          for(unsigned i=0;i<2;++i) {
            Vec3 candidate=transformed(basis[i]);for(unsigned j=0;j<count;++j)candidate=candidate-q[j]*dot(candidate,q[j]);
            if(length(candidate)>1e-6F)q[count++]=normalized(candidate);
          }
          Vec3 projected{};Matrix projection{};
          for(unsigned i=0;i<count;++i) {
            projected=projected+q[i]*dot(v,q[i]);const std::array<double,3> coordinates{q[i].x,q[i].y,q[i].z};
            for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)projection[3*r+c]+=coordinates[r]*coordinates[c];
            b.rod(q[i]*-2,q[i]*2,blue,.012F,"column_span");
          }
          const Vec3 residual=v-projected;b.arrow({},projected,teal,"projection");b.rod(projected,v,gold,.022F,"projection_residual");
          b.label("projection",projected+Vec3{0,.15F,0},teal);b.matrix("Projection P",projection);
          b.metric("Subspace dimension",count);b.metric("Residual length",length(residual));b.metric("Projected length",length(projected));
          double error=0;for(unsigned i=0;i<count;++i)error=std::max(error,static_cast<double>(std::fabs(dot(residual,q[i]))));b.metric("Orthogonality error",error);break;
        }
        case 3: {
          const auto decomposition=svd(a);Matrix sigma{};for(unsigned i=0;i<3;++i)sigma[4*i]=decomposition.sigma[i];
          const auto vt=transpose(decomposition.v),stretch=multiply(sigma,vt),reconstructed=multiply(decomposition.u,stretch);
          const std::array<Matrix,4> stages{identity,vt,stretch,reconstructed};const auto& transform=stages[static_cast<unsigned>(parameter(MathParameter::SvdStage))];
          const std::array<std::string_view,3> stageNames{"stage e1","stage e2","stage e3"};
          for(unsigned i=0;i<3;++i) {const auto tip=mapped(transform,basis[i]);b.arrow({},tip,colors[i],stageNames[i]);b.label(stageNames[i],tip+Vec3{.1F,.15F,.1F},colors[i]);}
          b.matrix("U",decomposition.u);b.matrix("V transpose",vt);
          b.metric("Singular value 1",decomposition.sigma[0]);b.metric("Singular value 2",decomposition.sigma[1]);b.metric("Singular value 3",decomposition.sigma[2]);
          double error=0;for(unsigned i=0;i<9;++i)error=std::max(error,std::fabs(reconstructed[i]-a[i]));b.metric("SVD reconstruction error",error);
          auto& patch=snapshot_.surface;patch.rows=17;patch.columns=21;
          for(unsigned row=0;row<patch.rows;++row)for(unsigned col=0;col<patch.columns;++col) {
            const double latitude=pi*row/(patch.rows-1),longitude=2*pi*col/(patch.columns-1);
            const Vec3 unit{static_cast<float>(std::sin(latitude)*std::cos(longitude)),static_cast<float>(std::cos(latitude)),static_cast<float>(std::sin(latitude)*std::sin(longitude))};
            auto& vertex=patch.vertices[row*patch.columns+col];vertex.position=mapped(transform,unit);vertex.normal=unit;
            const float weight=std::fabs(unit.x)+std::fabs(unit.y)+std::fabs(unit.z);
            vertex.color=(coral*std::fabs(unit.x)+teal*std::fabs(unit.y)+violet*std::fabs(unit.z))*(1/weight);
          }
          // Normals from transformed tangents also cover reflected and collapsed surfaces.
          for(unsigned row=0;row<patch.rows;++row)for(unsigned col=0;col<patch.columns;++col) {
            const unsigned lo=row?row-1:row,hi=std::min(row+1,patch.rows-1),left=col?col-1:patch.columns-2,right=(col+1)%(patch.columns-1);
            const auto across=patch.vertices[row*patch.columns+right].position-patch.vertices[row*patch.columns+left].position;
            const auto down=patch.vertices[hi*patch.columns+col].position-patch.vertices[lo*patch.columns+col].position;
            auto& vertex=patch.vertices[row*patch.columns+col];const auto n=cross(across,down);vertex.normal=length(n)>1e-8F?normalized(n):Vec3{0,1,0};
          }
          break;
        }
      }
      break;
    }
    case MathObjectKind::Discrete: {
      const float depth=value(MathParameter::Depth);const bool shortcut=parameter(MathParameter::Shortcut)!=0;
      std::array<Vec3,8> points{};
      for(unsigned i=0;i<8;++i)points[i]={static_cast<float>(((i>>2)&1)*2)-1,static_cast<float>(((i>>1)&1)*2)-1,(static_cast<float>((i&1)*2)-1)*depth};
      unsigned edgeCount=0;
      for(unsigned a=0;a<8;++a)for(unsigned c=a+1;c<8;++c)if(edge(a,c,shortcut)) {
        bool chosen=false;for(std::size_t j=1;j<snapshot_.routeCount;++j)chosen|=(snapshot_.route[j-1]==a&&snapshot_.route[j]==c)||(snapshot_.route[j-1]==c&&snapshot_.route[j]==a);
        b.rod(points[a],points[c],chosen?gold:muted,chosen?.035F:.021F,"graph_edge");++edgeCount;
      }
      for(unsigned i=0;i<8;++i) {
        const bool visited=std::find(snapshot_.route.begin(),snapshot_.route.begin()+snapshot_.routeCount,i)!=snapshot_.route.begin()+snapshot_.routeCount;
        b.ball(points[i],.115F,i==0?teal:i==7?coral:visited?gold:blue,nodeLabels[i]);b.label(nodeLabels[i],points[i]+Vec3{0,.25F,0});
        snapshot_.allowedVertices[i]=snapshot_.route[snapshot_.routeCount-1]!=7 && edge(snapshot_.route[snapshot_.routeCount-1],i,shortcut);
      }
      snapshot_.shortestHops=shortest(shortcut);b.metric("Vertices",8);b.metric("Edges",edgeCount);b.metric("Your hops",snapshot_.routeCount-1);b.metric("Shortest hops",snapshot_.shortestHops);break;
    }
    case MathObjectKind::Function: {
      const unsigned rule=static_cast<unsigned>(parameter(MathParameter::FunctionRule));
      const double x=parameter(MathParameter::FunctionX),h=parameter(MathParameter::DeltaX),a=parameter(MathParameter::IntegralStart),center=parameter(MathParameter::TaylorCenter);
      const auto f=[&](double t){return functionDerivative(rule,t);};
      const auto derivative=[&](double t){return functionDerivative(rule,t,1);};
      const double fx=f(x),slope=derivative(x),secant=h==0?slope:(f(x+h)-fx)/h;
      auto& graph=b.plot("Function f(x)",MathParameter::FunctionX);b.curve(graph,"f",teal,-3,3,f);graph.marker={x,fx};graph.hasMarker=true;
      for(unsigned i=0;i<128;i+=2) {
        const auto p=graph.series[0].points[i],q=graph.series[0].points[i+2];
        b.rod({static_cast<float>(p.x),static_cast<float>(p.y),0},{static_cast<float>(q.x),static_cast<float>(q.y),0},teal,.022F,"function_curve");
      }
      b.arrow({-3.2F,0,0},{3.2F,0,0},muted,"x_axis");b.arrow({0,-3.2F,0},{0,4.2F,0},muted,"y_axis");
      const Vec3 point{static_cast<float>(x),static_cast<float>(fx),.05F};b.ball(point,.07F,gold,"function_point");b.label("f(x)",point+Vec3{.1F,.2F,0},gold);
      b.metric("f(x)",fx);b.metric("f'(x)",slope);
      if(snapshot_.level>=1) {
        b.rod({static_cast<float>(x-.5),static_cast<float>(fx-.5*slope),.025F},{static_cast<float>(x+.5),static_cast<float>(fx+.5*slope),.025F},coral,.024F,"tangent");
        const Vec3 end{static_cast<float>(x+h),static_cast<float>(f(x+h)),.07F};b.ball(end,.05F,blue,"secant_point");b.rod(point,end,gold,.019F,"secant");
        auto& plot=b.plot("Derivative f'(x)",MathParameter::FunctionX);b.curve(plot,"f'",coral,-3,3,derivative);plot.marker={x,slope};plot.hasMarker=true;
        b.metric("Secant slope",secant);b.metric("Secant error",std::fabs(secant-slope));
      }
      if(snapshot_.level>=2) {
        const auto accumulated=[&](double t){return primitive(rule,t)-primitive(rule,a);};
        auto& plot=b.plot("Accumulation F(x)",MathParameter::FunctionX);b.curve(plot,"Integral from a",blue,-3,3,accumulated);plot.marker={x,accumulated(x)};plot.hasMarker=true;
        b.curve(graph,"Between bounds",blue,std::min(a,x),std::max(a,x),f,true);
        b.metric("Signed integral",accumulated(x));b.metric("F'(x) = f(x)",fx);
        if(std::fabs(x-a)>1e-9) {
          auto& patch=snapshot_.surface;patch.rows=2;patch.columns=65;
          for(unsigned i=0;i<patch.columns;++i) {
            const double t=std::min(a,x)+std::fabs(x-a)*i/(patch.columns-1),height=f(t);const auto color=height>=0?blue:coral;
            patch.vertices[i]={{static_cast<float>(t),0,-.015F},{0,0,1},color};
            patch.vertices[patch.columns+i]={{static_cast<float>(t),static_cast<float>(height),-.015F},{0,0,1},color};
          }
        }
      }
      if(snapshot_.level==3) {
        const auto degree=static_cast<unsigned>(parameter(MathParameter::TaylorDegree));const auto approximation=[&](double t){return taylor(rule,t,center,degree);};
        b.curve(graph,"Taylor",violet,-3,3,approximation);
        b.metric("Taylor value",approximation(x));b.metric("Taylor error",std::fabs(approximation(x)-fx));
      }
      break;
    }
    case MathObjectKind::Surface: {
      const unsigned rule=static_cast<unsigned>(parameter(MathParameter::SurfaceRule));const bool constrained=parameter(MathParameter::Constraint)!=0;
      const double angle=parameter(MathParameter::CircleAngle)*pi/180;
      const double u=constrained?std::cos(angle):parameter(MathParameter::SurfaceU),v=constrained?std::sin(angle):parameter(MathParameter::SurfaceV);
      const auto sample=surfaceValue(rule,u,v);auto& patch=snapshot_.surface;patch.rows=patch.columns=MathSurfacePatch::kResolution;
      for(unsigned row=0;row<patch.rows;++row)for(unsigned col=0;col<patch.columns;++col) {
        const double pu=-2+4.0*col/(patch.columns-1),pv=-2+4.0*row/(patch.rows-1);const auto value=surfaceValue(rule,pu,pv);
        const float blend=static_cast<float>(std::clamp((value.height+2)/6,0.0,1.0));
        patch.vertices[row*patch.columns+col]={{static_cast<float>(pu),static_cast<float>(value.height),static_cast<float>(pv)},normalized(Vec3{static_cast<float>(-value.du),1,static_cast<float>(-value.dv)}),blue*(1-blend)+teal*blend};
      }
      auto& contours=snapshot_.contours;contours.active=true;contours.point={u,v};contours.gradient={sample.du,sample.dv};contours.constrained=constrained;
      const auto contourTriangle=[&](unsigned ia,unsigned ib,unsigned ic) {
        const std::array<Vec3,3> vertices{patch.vertices[ia].position,patch.vertices[ib].position,patch.vertices[ic].position};
        for(double height:std::array<double,6>{-1,0,.5,1,2,3}) {
          std::array<MathPlotPoint,2> crossings{};unsigned count=0;
          for(unsigned edge=0;edge<3;++edge) {
            const auto p=vertices[edge],q=vertices[(edge+1)%3];
            if((p.y<=height&&q.y>height)||(q.y<=height&&p.y>height)) {
              const double t=(height-p.y)/(q.y-p.y);crossings[count++]={p.x+t*(q.x-p.x),p.z+t*(q.z-p.z)};
            }
          }
          if(count==2&&std::hypot(crossings[0].x-crossings[1].x,crossings[0].y-crossings[1].y)>1e-10) {
            if(contours.count==contours.segments.size())throw std::logic_error("contour capacity exceeded");
            contours.segments[contours.count++]={crossings[0],crossings[1],height};
          }
        }
      };
      for(unsigned row=0;row+1<patch.rows;++row)for(unsigned col=0;col+1<patch.columns;++col) {
        const unsigned i=row*patch.columns+col,j=i+patch.columns;contourTriangle(i,j,i+1);contourTriangle(i+1,j,j+1);
      }
      const Vec3 point{static_cast<float>(u),static_cast<float>(sample.height+.04),static_cast<float>(v)};
      b.ball(point,.08F,gold,"surface_point");b.label("f(u,v)",point+Vec3{0,.2F,0},gold);
      auto& uPlot=b.plot("u section: v fixed",constrained?MathParameter::Count:MathParameter::SurfaceU);
      b.curve(uPlot,"f(u,v0)",coral,-2,2,[&](double t){return surfaceValue(rule,t,v).height;});uPlot.marker={u,sample.height};uPlot.hasMarker=true;
      auto& vPlot=b.plot("v section: u fixed",constrained?MathParameter::Count:MathParameter::SurfaceV);
      b.curve(vPlot,"f(u0,v)",violet,-2,2,[&](double t){return surfaceValue(rule,u,t).height;});vPlot.marker={v,sample.height};vPlot.hasMarker=true;
      for(unsigned i=0;i<32;++i) {
        const double a=-2+i*.125,c=a+.125;
        b.rod({static_cast<float>(a),static_cast<float>(surfaceValue(rule,a,v).height+.012),static_cast<float>(v)},{static_cast<float>(c),static_cast<float>(surfaceValue(rule,c,v).height+.012),static_cast<float>(v)},coral,.017F,"u_section");
        b.rod({static_cast<float>(u),static_cast<float>(surfaceValue(rule,u,a).height+.012),static_cast<float>(a)},{static_cast<float>(u),static_cast<float>(surfaceValue(rule,u,c).height+.012),static_cast<float>(c)},violet,.017F,"v_section");
      }
      b.metric("Height",sample.height);b.metric("Partial u",sample.du);b.metric("Partial v",sample.dv);b.metric("Gradient magnitude",std::hypot(sample.du,sample.dv));
      if(snapshot_.level>=1) {
        const auto plane=[&](double x,double y){return Vec3{static_cast<float>(u+x),static_cast<float>(sample.height+sample.du*x+sample.dv*y+.025),static_cast<float>(v+y)};};
        for(double offset:{-.45,0.0,.45}) {b.rod(plane(-.45,offset),plane(.45,offset),gold,.009F,"tangent_plane");b.rod(plane(offset,-.45),plane(offset,.45),gold,.009F,"tangent_plane");}
        const double angle=parameter(MathParameter::DirectionAngle)*pi/180,dx=std::cos(angle),dy=std::sin(angle);
        b.arrow(point,point+Vec3{static_cast<float>(dx*.6),0,static_cast<float>(dy*.6)},white,"input_direction");
        b.arrow(point,point+Vec3{static_cast<float>(sample.du*.3),0,static_cast<float>(sample.dv*.3)},teal,"gradient_direction_scaled");
        b.metric("Directional derivative",sample.du*dx+sample.dv*dy);
      }
      if(snapshot_.level>=2) {
        const double hessianDet=sample.duu*sample.dvv-sample.duv*sample.duv;
        b.metric("Hessian determinant",hessianDet);b.metric("Curvature in u",sample.duu);
      }
      if(snapshot_.level==3&&constrained) {
        const double tangent=-sample.du*v+sample.dv*u,lambda=(sample.du*u+sample.dv*v)/2;
        b.metric("Tangential derivative",tangent);b.metric("Lagrange multiplier",lambda);b.metric("Constraint error",std::fabs(u*u+v*v-1));
        for(unsigned i=0;i<48;++i) {
          const double a=2*pi*i/48,c=2*pi*(i+1)/48;
          const auto position=[&](double t){return Vec3{static_cast<float>(std::cos(t)),static_cast<float>(surfaceValue(rule,std::cos(t),std::sin(t)).height+.03),static_cast<float>(std::sin(t))};};
          b.rod(position(a),position(c),white,.02F,"constraint_curve");
        }
      }
      break;
    }
    case MathObjectKind::Symmetry: {
      const auto level=snapshot_.level;Matrix rotation=identity;
      const unsigned first=static_cast<unsigned>(parameter(MathParameter::SymmetryFirst)),second=static_cast<unsigned>(parameter(MathParameter::SymmetrySecond));
      if(level<2)for(std::size_t i=0;i<symmetryMoveCount_;++i)rotation=multiply(turn(symmetryMoves_[i]),rotation);
      else if(level==2)rotation=power(cyclicGenerator(static_cast<unsigned>(parameter(MathParameter::SymmetryGenerator))),static_cast<unsigned>(parameter(MathParameter::SymmetryPower)));
      else rotation=cubeGroup()[static_cast<unsigned>(parameter(MathParameter::SymmetryElement))];
      const unsigned probe=level==3?static_cast<unsigned>(parameter(MathParameter::SymmetryVertex)):0;
      auto& view=snapshot_.symmetry;view={};view.active=true;
      if(level<2){view.moveCount=symmetryMoveCount_;std::copy_n(symmetryMoves_.begin(),symmetryMoveCount_,view.moves.begin());}
      unsigned moved=0;for(unsigned i=0;i<8;++i){view.permutation[i]=destination(rotation,i);moved+=view.permutation[i]!=i;}
      static constexpr std::array<std::string_view,8> labels{"A","B","C","D","E","F","G","H"};
      static constexpr std::array<Vec3,8> colors{coral,teal,gold,blue,violet,white,Vec3{.85F,.35F,.62F},Vec3{.6F,.85F,.3F}};
      const auto cage=[&](const Matrix& r,Vec3 center,float scale,bool labelled) {
        for(unsigned i=0;i<8;++i) {
          const auto at=center+mapped(r,cubeVertex(i))*scale;
          b.ball(at,(labelled&&i==probe?.13F:.075F)*scale,colors[i],"labelled_vertex");
          if(labelled)b.label(labels[i],center+mapped(r,cubeVertex(i))*(scale*1.16F),colors[i]);
          for(unsigned bit=0;bit<3;++bit)if(!(i&(1U<<bit)))b.rod(at,center+mapped(r,cubeVertex(i|(1U<<bit)))*scale,bit==0?coral:bit==1?teal:blue,.018F*scale,"cube_edge");
        }
      };
      cage(rotation,{},1,true);b.matrix("Current rotation R",rotation);
      b.metric("Rotation order",rotationOrder(rotation));b.metric("Moved vertices",moved);b.metric("Applied turns",level<2?symmetryMoveCount_:0);
      if(level==0)b.label("Fixed slot H",{1.55F,1.2F,1.2F},muted);
      if(level==1) {
        const auto forward=multiply(turn(second),turn(first)),reverse=multiply(turn(first),turn(second));unsigned different=0;
        for(unsigned i=0;i<8;++i)different+=destination(forward,i)!=destination(reverse,i);
        cage(forward,{-2.7F,-.3F,0},.55F,false);cage(reverse,{2.7F,-.3F,0},.55F,false);
        b.label("First then second",{-2.7F,.65F,0},gold);b.label("Second then first",{2.7F,.65F,0},violet);
        b.matrix("First then second: S R",forward);b.matrix("Second then first: R S",reverse);b.metric("Order-dependent destinations",different);
      }
      if(level>=2) {
        unsigned groupSize=24,stabiliser=0;
        if(level==2) {
          const auto generator=cyclicGenerator(static_cast<unsigned>(parameter(MathParameter::SymmetryGenerator)));groupSize=rotationOrder(generator);
          for(unsigned i=0;i<groupSize;++i){const auto to=destination(power(generator,i),probe);view.orbit[to]=true;stabiliser+=to==probe;}
          b.metric("Generator order",groupSize);
        } else for(const auto& r:cubeGroup()){const auto to=destination(r,probe);view.orbit[to]=true;stabiliser+=to==probe;}
        unsigned orbit=0;for(unsigned i=0;i<8;++i)if(view.orbit[i]){++orbit;b.scaled(MathShape::Box,cubeVertex(i)*1.35F,{.065F,.065F,.065F},gold,"orbit_slot");}
        b.metric("Group size",groupSize);b.metric("Orbit size",orbit);b.metric("Stabiliser size",stabiliser);b.metric("Orbit times stabiliser",orbit*stabiliser);
        b.metric("Tracked vertex fixed",view.permutation[probe]==probe?1:0);
      }
      break;
    }
    case MathObjectKind::Harmonics: {
      const auto level=snapshot_.level;const double t=parameter(MathParameter::HarmonicTime);
      if(level<2) {
        const double a=parameter(MathParameter::Amplitude1),n=parameter(MathParameter::Frequency1),phase=parameter(MathParameter::Phase1)*pi/180;
        const auto x=[&](double time){return a*std::cos(n*time+phase);};const auto y=[&](double time){return a*std::sin(n*time+phase);};
        b.metric("Time",t);b.metric("First x",x(t));b.metric("First y",y(t));b.metric("First period",2*pi/n);
        if(level==0) {
          b.scaled(MathShape::Ring,{},{1,1,1},muted,"unit_reference");
          b.arrow({},{static_cast<float>(x(t)),static_cast<float>(y(t)),0},gold,"phasor");
          b.ball({static_cast<float>(x(t)),static_cast<float>(y(t)),0},.065F,teal,"phasor_tip");
          b.arrow({-1.7F,0,0},{1.7F,0,0},muted,"real_axis");b.arrow({0,-1.7F,0},{0,1.7F,0},muted,"imaginary_axis");
          for(unsigned i=0;i<64;++i){const double u=2*pi*i/64,v=2*pi*(i+1)/64;b.rod({2+static_cast<float>(u)*.6F,static_cast<float>(y(u)),0},{2+static_cast<float>(v)*.6F,static_cast<float>(y(v)),0},teal,.014F,"sine_trace");}
          b.ball({2+static_cast<float>(t)*.6F,static_cast<float>(y(t)),0},.07F,gold,"time_marker");
          b.label("Complex rotor",{0,1.85F,0});b.label("Sine projection over time",{3.8F,1.85F,0},teal);
          auto& plot=b.plot("Sine projection",MathParameter::HarmonicTime);b.curve(plot,"a sin(n t + phase)",teal,0,2*pi,y);plot.hasMarker=true;plot.marker={t,y(t)};
        } else {
          const double a2=parameter(MathParameter::Amplitude2),n2=parameter(MathParameter::Frequency2),p2=parameter(MathParameter::Phase2)*pi/180;
          const auto secondY=[&](double time){return a2*std::sin(n2*time+p2);};
          const auto wire=[&](double time){return Vec3{static_cast<float>(x(time)),static_cast<float>(secondY(time)),static_cast<float>(time/pi-1)};};
          for(unsigned i=0;i<96;++i)b.rod(wire(2*pi*i/96),wire(2*pi*(i+1)/96),teal,.014F,"lissajous_time_wire");
          b.ball(wire(t),.08F,gold,"time_marker");
          b.arrow({-1.7F,0,-1},{1.8F,0,-1},muted,"x_axis");b.arrow({0,-1.7F,-1},{0,1.8F,-1},muted,"y_axis");b.arrow({1.7F,0,-1},{1.7F,0,1.2F},muted,"time_axis");
          b.label("Time axis",{1.7F,0,1.35F});
          auto& xp=b.plot("First coordinate x(t)",MathParameter::HarmonicTime);b.curve(xp,"x",coral,0,2*pi,x);xp.hasMarker=true;xp.marker={t,x(t)};
          auto& yp=b.plot("Second coordinate y(t)",MathParameter::HarmonicTime);b.curve(yp,"y",teal,0,2*pi,secondY);yp.hasMarker=true;yp.marker={t,secondY(t)};
          auto& xy=b.plot("Lissajous x-y projection");xy.equalAspect=true;auto& line=xy.series[xy.seriesCount++];line={};line.name="x-y";line.color=gold;line.count=129;
          for(unsigned i=0;i<line.count;++i){const double time=2*pi*i/(line.count-1);line.points[i]={x(time),secondY(time)};}xy.hasMarker=true;xy.marker={x(t),secondY(t)};
          b.metric("Second y",secondY(t));b.metric("Shared period",2*pi/gcd(static_cast<unsigned>(n),static_cast<unsigned>(n2)));
        }
      } else {
        const unsigned waveform=static_cast<unsigned>(parameter(MathParameter::Waveform)),terms=static_cast<unsigned>(parameter(MathParameter::HarmonicTerms)),probe=static_cast<unsigned>(parameter(MathParameter::ProbeFrequency));
        const auto sum=[&](double u){return fourierSum(waveform,terms,u);};const auto target=[&](double u){return targetWave(waveform,u);};
        Vec3 tip{-2,0,0};double capturedEnergy=0;
        for(unsigned i=0;i<terms;++i) {
          const auto term=fourierTerm(waveform,i);const auto next=tip+Vec3{static_cast<float>(term.coefficient*std::cos(term.frequency*t)),static_cast<float>(term.coefficient*std::sin(term.frequency*t)),0};
          b.arrow(tip,next,i%2?coral:teal,"fourier_component");tip=next;capturedEnergy+=.5*term.coefficient*term.coefficient;
        }
        b.ball(tip,.075F,gold,"sum_tip");b.label("Sum of rotating components",{-2,2,0});
        for(unsigned i=0;i<64;++i){const double u=2*pi*i/64,v=2*pi*(i+1)/64;b.rod({2+static_cast<float>(u)*.6F,static_cast<float>(sum(u)),0},{2+static_cast<float>(v)*.6F,static_cast<float>(sum(v)),0},teal,.014F,"fourier_signal");}
        b.ball({2+static_cast<float>(t)*.6F,static_cast<float>(sum(t)),0},.075F,gold,"time_marker");
        b.arrow({2,0,0},{6,0,0},muted,"time_axis");
        auto& signal=b.plot("Target and Fourier sum",MathParameter::HarmonicTime);b.curve(signal,"target",muted,0,2*pi,target);b.curve(signal,"sum",teal,0,2*pi,sum);signal.hasMarker=true;signal.marker={t,sum(t)};
        auto& spectrum=b.plot("Signed sine coefficients");auto& line=spectrum.series[spectrum.seriesCount++];line={};line.name="included b_n";line.color=coral;line.count=17;line.stems=true;
        for(unsigned i=0;i<line.count;++i)line.points[i]={static_cast<double>(i),0};
        for(unsigned i=0;i<terms;++i){const auto term=fourierTerm(waveform,i);line.points[term.frequency].y=term.coefficient;}
        const double rms=std::sqrt(std::max(0.0,(waveform==0?1.0:1.0/3)-capturedEnergy));
        b.metric("Time",t);b.metric("Signal value",sum(t));b.metric("Target value",target(t));b.metric("Pointwise error",std::fabs(sum(t)-target(t)));b.metric("Full-period RMS error",rms);b.metric("Nonzero terms",terms);
        if(level==3) {
          const auto product=[&](double u){return target(u)*std::sin(probe*u);};double numerical=0;
          for(unsigned i=0;i<1024;++i)numerical+=product(2*pi*(i+.5)/1024)*2/1024;
          auto& projection=b.plot("Target times selected sine",MathParameter::HarmonicTime);b.curve(projection,"f(t) sin(n t)",violet,0,2*pi,product);projection.hasMarker=true;projection.marker={t,product(t)};
          b.metric("Analytic coefficient",coefficient(waveform,probe));b.metric("Midpoint coefficient",numerical);b.metric("Coefficient error",std::fabs(numerical-coefficient(waveform,probe)));
        }
      }
      break;
    }
    case MathObjectKind::Oscillator: {
      const unsigned level=snapshot_.level;const double time=parameter(MathParameter::MotionTime);
      const MotionConfig c{parameter(MathParameter::MotionSystem)==1,parameter(MathParameter::InitialPosition),parameter(MathParameter::InitialVelocity),parameter(MathParameter::Stiffness),level>=2?parameter(MathParameter::Damping):0,level>=3?parameter(MathParameter::DriveAmplitude):0,parameter(MathParameter::DriveFrequency)};
      const MotionState initial{c.position,c.velocity};const auto current=integrate(c,initial,0,time);const double e0=energy(c,initial),e=energy(c,current);
      std::array<MotionState,129> trace{};trace[0]=initial;double maxQ=1,maxV=1;
      for(unsigned i=1;i<trace.size();++i)trace[i]=integrate(c,trace[i-1],12.0*(i-1)/(trace.size()-1),12.0*i/(trace.size()-1));
      for(const auto& state:trace){maxQ=std::max(maxQ,std::fabs(state.q));maxV=std::max(maxV,std::fabs(state.v));}
      if(c.pendulum) {
        const Vec3 pivot{0,1.6F,0},bob{2*static_cast<float>(std::sin(current.q)),1.6F-2*static_cast<float>(std::cos(current.q)),0};
        b.rod({-1,1.6F,0},{1,1.6F,0},muted,.035F,"support");b.rod(pivot,bob,white,.025F,"pendulum_arm");b.ball(bob,.18F,teal,"pendulum_bob");
        b.arrow(bob,bob+Vec3{static_cast<float>(std::cos(current.q)*current.v/maxV),static_cast<float>(std::sin(current.q)*current.v/maxV),0},gold,"velocity_direction");b.label("Pendulum angle q",{0,2,0});
      } else {
        const float position=static_cast<float>(current.q*2.3/maxQ);b.scaled(MathShape::Box,{-3,0,0},{.12F,1,.7F},muted,"spring_anchor");
        const auto coil=[&](double u){return Vec3{-2.94F+static_cast<float>(u)*(position+2.76F),static_cast<float>(.13*std::sin(12*pi*u)),static_cast<float>(.13*std::cos(12*pi*u))};};
        for(unsigned i=0;i<64;++i)b.rod(coil(i/64.0),coil((i+1)/64.0),white,.018F,"spring_coil");
        b.scaled(MathShape::Box,{position,0,0},{.36F,.45F,.45F},teal,"spring_mass");b.rod({-2.8F,-.32F,0},{2.8F,-.32F,0},muted,.012F,"track");
        b.rod({0,-.45F,0},{0,.45F,0},coral,.008F,"equilibrium");b.arrow({position,.5F,0},{position+static_cast<float>(current.v/maxV),.5F,0},gold,"velocity_direction");b.label("Spring displacement q",{0,1.4F,0});
      }
      if(level>=1) {
        const auto phasePoint=[&](const MotionState& state,double t){return Vec3{4+static_cast<float>(state.q/maxQ),static_cast<float>(state.v/maxV),static_cast<float>(t/6-1)};};
        for(unsigned i=0;i+2<trace.size();i+=2)b.rod(phasePoint(trace[i],12.0*i/128),phasePoint(trace[i+2],12.0*(i+2)/128),violet,.012F,"phase_time_trajectory");
        b.ball(phasePoint(current,time),.07F,gold,"phase_time_marker");b.label("q / v / time",{4,1.6F,0},violet);
      }
      auto& motion=b.plot("Displacement through time",MathParameter::MotionTime);auto& qLine=motion.series[motion.seriesCount++];qLine={};qLine.name=c.pendulum?"q (radians)":"q";qLine.color=teal;qLine.count=trace.size();
      for(unsigned i=0;i<trace.size();++i)qLine.points[i]={12.0*i/(trace.size()-1),trace[i].q};motion.hasMarker=true;motion.marker={time,current.q};
      if(level>=1) {
        if(level==3&&!c.pendulum) {
          auto& response=b.plot("Impulse and response decomposition",MathParameter::MotionTime);
          b.curve(response,"h(t)",muted,0,12,[&](double t){return impulse(c,t);});
          b.curve(response,"free",violet,0,12,[&](double t){return freeResponse(c,t);});
          b.curve(response,"convolution",gold,0,12,[&](double t){return convolutionResponse(c,t);});
          response.hasMarker=true;response.marker={time,convolutionResponse(c,time)};
        } else {
          auto& phase=b.plot("Phase: displacement / velocity");auto& path=phase.series[phase.seriesCount++];path={};path.name="(q,v)";path.color=violet;path.count=trace.size();
          for(unsigned i=0;i<trace.size();++i)path.points[i]={trace[i].q,trace[i].v};phase.hasMarker=true;phase.marker={current.q,current.v};
        }
        auto& energies=b.plot("Energy, loss and driving work",MathParameter::MotionTime);
        static constexpr std::array<std::string_view,3> names{"energy","loss","work"};static constexpr std::array<Vec3,3> colors{teal,coral,gold};
        for(unsigned j=0;j<(level==1?1U:level==2?2U:3U);++j){auto& line=energies.series[energies.seriesCount++];line={};line.name=names[j];line.color=colors[j];line.count=trace.size();for(unsigned i=0;i<trace.size();++i)line.points[i]={12.0*i/(trace.size()-1),j==0?energy(c,trace[i]):j==1?trace[i].loss:trace[i].work};}
        energies.hasMarker=true;energies.marker={time,e};
      }
      b.metric("Time",time);b.metric("Displacement",current.q);b.metric("Velocity",current.v);b.metric("Acceleration",motionRate(c,time,current).v);
      b.metric("Initial energy",e0);b.metric("Mechanical energy",e);b.metric("Dissipated energy",current.loss);b.metric("Driving work",current.work);b.metric("Energy balance error",std::fabs(e+current.loss-e0-current.work));
      if(level==3&&!c.pendulum) {
        const double convolution=convolutionResponse(c,time);
        b.metric("Impulse response",impulse(c,time));b.metric("Convolution response",convolution);b.metric("ODE/convolution difference",std::fabs(current.q-freeResponse(c,time)-convolution));
      } else {b.metric("Linearised angular frequency",std::sqrt(c.k));b.metric("Damping ratio",c.damping/(2*std::sqrt(c.k)));}
      break;
    }
    case MathObjectKind::Modular: {
      const unsigned level=snapshot_.level;const int n=static_cast<int>(parameter(MathParameter::Modulus)),a=static_cast<int>(parameter(MathParameter::ModValue)),r=remainder(a,n),k=static_cast<int>(parameter(MathParameter::ModStep));
      unsigned reachable=0;for(int i=0;i<n;++i)reachable|=1U<<remainder(r+i*k,n);
      const auto drum=[&](int modulus,int current,int target,Vec3 center,bool walk) {
        const auto at=[&](int residue,float radius=1.3F,float z=.3F){const double angle=pi/2+2*pi*residue/modulus;return center+Vec3{radius*static_cast<float>(std::cos(angle)),radius*static_cast<float>(std::sin(angle)),z};};
        for(float z:{-.3F,.3F})b.scaled(MathShape::Ring,center+Vec3{0,0,z},{1.3F,1.3F,1},muted,"drum_rim");
        for(int i=0;i<modulus;++i) {
          const auto color=i==current?gold:i==target?teal:walk?(modularVisitedMask_&(1U<<i)?white:reachable&(1U<<i)?teal:muted):blue;
          b.rod(at(i,1.3F,-.3F),at(i),muted,.009F,"drum_side");b.ball(at(i),i==current?.105F:.07F,color,"residue");
          b.label(residueLabels[i],at(i,1.58F,.32F),color);
          if(walk)b.rod(at(i,1.17F,.33F),at(remainder(i+k,modulus),1.17F,.33F),reachable&(1U<<i)?teal:muted,.006F,"step_chord");
        }
        b.arrow(center+Vec3{0,0,.35F},at(current,1.2F,.35F),gold,"residue_pointer");
      };
      if(level<3) {
        drum(n,level==2?remainder(k,n):r,-1,{},level==1);b.metric("Canonical residue",level==2?remainder(k,n):r);b.metric("Integer quotient",floorQuotient(level==2?k:a,n));
        if(level==0) {
          b.table("Neighbouring residue classes",{"next","previous","",""},2);
          for(int i=0;i<n;++i)b.row(residueLabels[i],{static_cast<double>(remainder(i+1,n)),static_cast<double>(remainder(i-1,n)),0,0});
        } else if(level==1) {
          b.metric("GCD",gcd(n,k));b.metric("Cycle length",n/gcd(n,k));b.metric("Visited residues",std::popcount(modularVisitedMask_));b.metric("Walk steps",modularWalkSteps_);
          b.table("Walk by repeated addition",{"next by k","reachable","visited",""},3);
          for(int i=0;i<n;++i)b.row(residueLabels[i],{static_cast<double>(remainder(i+k,n)),reachable&(1U<<i)?1.0:0.0,modularVisitedMask_&(1U<<i)?1.0:0.0,0});
        } else {
          const int inverse=static_cast<int>(parameter(MathParameter::InverseGuess));int actual=-1;
          b.table("Multiplication by k",{"k*r mod n","inverse?","",""},2);
          for(int i=0;i<n;++i){const int product=remainder(k*i,n);if(product==1)actual=i;b.row(residueLabels[i],{static_cast<double>(product),product==1?1.0:0.0,0,0});}
          b.metric("GCD",gcd(n,k));b.metric("Inverse exists",actual>=0?1:0);b.metric("Your product residue",remainder(k*inverse,n));if(actual>=0)b.metric("Canonical inverse",actual);
          // The second arrow applies the candidate multiplication to class 1.
          const double angle=pi/2+2*pi*remainder(k*inverse,n)/n;b.arrow({0,0,.45F},{1.1F*static_cast<float>(std::cos(angle)),1.1F*static_cast<float>(std::sin(angle)),.45F},coral,"inverse_product_pointer");
        }
      } else {
        const int m=static_cast<int>(parameter(MathParameter::SecondModulus)),second=remainder(static_cast<int>(parameter(MathParameter::SecondResidue)),m),guess=static_cast<int>(parameter(MathParameter::CrtGuess));const auto solution=chineseRemainder(r,n,second,m);
        drum(n,remainder(guess,n),r,{-2,0,0},false);drum(m,remainder(guess,m),second,{2,0,0},false);
        b.label("First congruence",{-2,1.95F,.3F},teal);b.label("Second congruence",{2,1.95F,.3F},teal);
        b.metric("Compatible",solution.solution>=0?1:0);b.metric("GCD",gcd(n,m));b.metric("Common period",solution.period);if(solution.solution>=0)b.metric("Smallest solution",solution.solution);
        b.metric("First residue match",remainder(guess,n)==r?1:0);b.metric("Second residue match",remainder(guess,m)==second?1:0);
        b.table("The two simultaneous conditions",{"modulus","target","guess residue","match"},4);
        b.row("First",{static_cast<double>(n),static_cast<double>(r),static_cast<double>(remainder(guess,n)),remainder(guess,n)==r?1.0:0.0});
        b.row("Second",{static_cast<double>(m),static_cast<double>(second),static_cast<double>(remainder(guess,m)),remainder(guess,m)==second?1.0:0.0});
        auto& plot=b.plot("Solutions within one common period");
        for(unsigned i=0;i<2;++i){auto& series=plot.series[plot.seriesCount++];series={};series.name=i==0?"first condition":"second condition";series.color=i==0?teal:coral;series.stems=true;for(int x=i==0?r:second;x<solution.period;x+=i==0?n:m)series.points[series.count++]={static_cast<double>(x),i==0?1.0:2.0};}
        if(solution.solution>=0){auto& series=plot.series[plot.seriesCount++];series={};series.name="both";series.color=gold;series.stems=true;series.points[series.count++]={static_cast<double>(solution.solution),3};}
      }
      break;
    }
    case MathObjectKind::Gaussian: {
      const unsigned level=snapshot_.level;const GaussianInt z{static_cast<int>(parameter(MathParameter::GaussianReal)),static_cast<int>(parameter(MathParameter::GaussianImag))},w{static_cast<int>(parameter(MathParameter::GaussianOtherReal)),static_cast<int>(parameter(MathParameter::GaussianOtherImag))};
      const bool product=level==1||parameter(MathParameter::GaussianOperation)==1,height=level<2&&parameter(MathParameter::GaussianHeight)==1;
      const auto raw=product?gaussianProduct(z,w):gaussianAdd(z,w);const auto ring=gaussianRing(static_cast<unsigned>(parameter(MathParameter::GaussianQuotient)));
      const auto result=level==3?canonicalGaussian(raw,ring.generator):raw;const auto generator=level==3?ring.generator:w;
      GaussianInt quotient{},rest=z;if(norm(w)){quotient=gaussianQuotient(z,w);rest=gaussianSubtract(z,gaussianProduct(quotient,w));}
      double bound=3.3;const auto includePoint=[&](GaussianInt p){bound=std::max({bound,static_cast<double>(std::abs(p.real)),static_cast<double>(std::abs(p.imag)),height?.12*norm(p):0});};
      includePoint(z);includePoint(w);if(level!=2)includePoint(result);includePoint(generator);includePoint(gaussianProduct(generator,{0,1}));includePoint(gaussianProduct(generator,{1,1}));
      for(int x=-3;x<=3;++x)for(int y=-3;y<=3;++y){includePoint({x,y});if(level<2&&product)includePoint(gaussianProduct({x,y},w));}
      const float scale=static_cast<float>(3.3/bound);
      const auto point=[&](GaussianInt p){return Vec3{p.real*scale,p.imag*scale,height?static_cast<float>(.12*norm(p))*scale:0};};
      for(int x=-3;x<=3;++x)for(int y=-3;y<=3;++y) {
        const GaussianInt p{x,y};const auto color=level==3?classColors[ring.classify(p)]:level==2&&gaussianDivides(w,p)?gold:muted;
        b.scaled(MathShape::Box,point(p),{.038F,.038F,.038F},color,"integer_lattice_point");
        if(height)b.rod({p.real*scale,p.imag*scale,0},point(p),muted,.003F,"norm_height_stem");
        if(level<2&&product&&(norm(w)||p==GaussianInt{}))b.scaled(MathShape::Box,point(gaussianProduct(p,w))+Vec3{0,0,.018F},{.033F,.033F,.033F},teal,"multiplied_lattice_point");
      }
      if(!height)for(int i=-3;i<=3;++i){b.rod(point({i,-3}),point({i,3}),muted,.003F,"lattice_line");b.rod(point({-3,i}),point({3,i}),muted,.003F,"lattice_line");}
      b.arrow({-3*scale,0,0},{3*scale,0,0},white,"real_axis");b.arrow({0,-3*scale,0},{0,3*scale,0},white,"imaginary_axis");
      b.ball(point(z)+Vec3{0,0,.04F},.075F,gold,"input_z");b.ball(point(w)+Vec3{0,0,.04F},.065F,coral,"input_w");
      if(level!=2)b.ball(point(result)+Vec3{0,0,.04F},.08F,teal,"operation_result");
      const Vec3 offset{0,.17F,.1F};
      if(level==2){b.label(z==w?"z = generator":"z",point(z)+offset,gold);if(z!=w)b.label("generator w",point(w)+offset,coral);}
      else if(z==w&&z==result)b.label("z = w = result",point(z)+offset,gold);
      else {b.label(z==w?"z = w":z==result?"z = result":"z",point(z)+offset,gold);if(w!=z)b.label(w==result?"w = result":"w",point(w)+offset,coral);if(result!=z&&result!=w)b.label("result",point(result)+offset,teal);}
      if(level>=2&&norm(generator)) {
        const auto iw=gaussianProduct(generator,{0,1});const std::array<GaussianInt,4> corners{{{},generator,gaussianAdd(generator,iw),iw}};
        for(unsigned i=0;i<4;++i)b.rod(point(corners[i])+Vec3{0,0,.025F},point(corners[(i+1)%4])+Vec3{0,0,.025F},teal,.02F,"fundamental_cell");
      }
      if(level<2) {
        b.table("Gaussian values",{"real","imaginary","norm",""},3);
        const auto row=[&](std::string_view label,GaussianInt p){b.row(label,{static_cast<double>(p.real),static_cast<double>(p.imag),static_cast<double>(norm(p)),0});};
        row("z",z);row("w",w);row(product?"z*w":"z+w",result);
        b.metric("Norm z",norm(z));b.metric("Norm w",norm(w));b.metric("Norm product",norm(gaussianProduct(z,w)));b.metric("Norm identity error",norm(gaussianProduct(z,w))-norm(z)*norm(w));
        if(level==0){b.metric("Result real",result.real);b.metric("Result imaginary",result.imag);}
        else {
          b.metric("Division defined",norm(w)>0?1:0);
          if(norm(w)){row("quotient q",quotient);row("remainder r",rest);b.metric("Quotient real",quotient.real);b.metric("Quotient imaginary",quotient.imag);b.metric("Remainder real",rest.real);b.metric("Remainder imaginary",rest.imag);b.metric("Remainder norm",norm(rest));b.metric("Divides exactly",norm(rest)==0?1:0);b.metric("Quotient norm",norm(quotient));}
        }
      } else if(level==2) {
        b.table("Multiples of the ideal generator",{"real","imaginary","norm",""},3);
        static constexpr std::array<GaussianInt,5> multipliers{{{0,0},{1,0},{0,1},{1,1},{2,0}}};static constexpr std::array<std::string_view,5> labels{"0*w","1*w","i*w","(1+i)*w","2*w"};
        for(unsigned i=0;i<multipliers.size();++i){const auto p=gaussianProduct(w,multipliers[i]);b.row(labels[i],{static_cast<double>(p.real),static_cast<double>(p.imag),static_cast<double>(norm(p)),0});}
        b.metric("Generator norm",norm(w));b.metric("Selected point norm",norm(z));b.metric("In principal ideal",gaussianDivides(w,z)?1:0);b.metric("Finite index",norm(w)>0?1:0);if(norm(w)){b.metric("Ideal index",norm(w));b.metric("Quotient real",quotient.real);b.metric("Quotient imaginary",quotient.imag);}
      } else {
        const unsigned az=ring.classify(z),bw=ring.classify(w),answer=ring.classify(raw),one=ring.classify({1,0});unsigned units=0,zeroDivisors=0,characteristic=0;
        b.table("Classes in the half-open cell",{"real","imaginary","z times class","unit?"},4);
        for(unsigned i=0;i<ring.size;++i) {
          bool unit=false,zeroDivisor=false;for(unsigned j=0;j<ring.size;++j){const auto productClass=ring.classify(gaussianProduct(ring.representatives[i],ring.representatives[j]));unit|=productClass==one;zeroDivisor|=i!=0&&j!=0&&productClass==0;}
          units+=unit;zeroDivisors+=zeroDivisor;const auto p=ring.representatives[i];
          b.row(residueLabels[i],{static_cast<double>(p.real),static_cast<double>(p.imag),static_cast<double>(ring.classify(gaussianProduct(z,p))),unit?1.0:0.0});
        }
        for(unsigned i=1;i<=ring.size;++i)if(ring.classify({static_cast<int>(i),0})==0){characteristic=i;break;}
        b.metric("Quotient size",ring.size);b.metric("Characteristic",characteristic);b.metric("Units",units);b.metric("Nonzero zero divisors",zeroDivisors);b.metric("Is a field",units==ring.size-1?1:0);
        b.metric("z class",az);b.metric("w class",bw);b.metric("Result class",answer);b.metric("z squared class",ring.classify(gaussianProduct(z,z)));
      }
      break;
    }
    case MathObjectKind::VectorField: {
      const unsigned level=snapshot_.level,rule=static_cast<unsigned>(parameter(MathParameter::FieldRule)),path=static_cast<unsigned>(parameter(MathParameter::FieldPath));const double t=parameter(MathParameter::FieldTime);const bool reversed=fieldPathReversed_;
      FieldVector p,d;double speed=1;
      if(level==0){p={parameter(MathParameter::FieldX),parameter(MathParameter::FieldY),parameter(MathParameter::FieldZ)};const double yaw=parameter(MathParameter::FieldYaw)*pi/180,pitch=parameter(MathParameter::FieldPitch)*pi/180;d={std::cos(pitch)*std::cos(yaw),std::cos(pitch)*std::sin(yaw),std::sin(pitch)};}
      else {const auto sample=fieldPath(path,t,reversed);p=sample.position;speed=fieldLength(sample.velocity);d=sample.velocity*(1/speed);}
      const auto f=fieldValue(rule,p);const auto a=fieldPath(path,0,reversed).position,end=fieldPath(path,1,reversed).position;
      for(int x=-1;x<=1;++x)for(int y=-1;y<=1;++y)for(int z=-1;z<=1;++z){const FieldVector point{static_cast<double>(x),static_cast<double>(y),static_cast<double>(z)};b.arrow(sceneVector(point),sceneVector(point+fieldValue(rule,point)*.3),teal,"field_sample");}
      for(unsigned i=0;i<8;++i)for(unsigned bit=0;bit<3;++bit)if(!(i&(1U<<bit)))b.rod(cubeVertex(i)*1.5F,cubeVertex(i|(1U<<bit))*1.5F,muted,.004F,"field_chamber");
      b.ball(sceneVector(p),.085F,white,"field_probe");b.arrow(sceneVector(p),sceneVector(p+f*.3),coral,"probe_field");b.arrow(sceneVector(p),sceneVector(p+d*.65),gold,"probe_direction");b.label("Probe",sceneVector(p)+Vec3{0,.17F,.08F});
      if(level>=1) {
        for(unsigned i=0;i<64;++i)b.rod(sceneVector(fieldPath(path,i/64.0,reversed).position),sceneVector(fieldPath(path,(i+1)/64.0,reversed).position),violet,.015F,"integration_path");
        if(level==3)for(unsigned i=0;i<12;++i)b.rod(sceneVector(a+(end-a)*(i/12.0)),sceneVector(a+(end-a)*((i+.5)/12.0)),white,.01F,"comparison_path");
        b.label("Start",sceneVector(a)+Vec3{-.12F,-.2F,.1F},violet);if(fieldLength(end-a)>.01)b.label("End",sceneVector(end)+Vec3{.12F,-.2F,.1F},violet);
        auto& coordinates=b.plot("Path coordinates",MathParameter::FieldTime);b.curve(coordinates,"x",coral,0,1,[&](double u){return fieldPath(path,u,reversed).position.x;});b.curve(coordinates,"y",teal,0,1,[&](double u){return fieldPath(path,u,reversed).position.y;});b.curve(coordinates,"z",blue,0,1,[&](double u){return fieldPath(path,u,reversed).position.z;});coordinates.hasMarker=true;coordinates.marker={t,p.x};
        if(level==1){auto& velocity=b.plot("Speed along the path",MathParameter::FieldTime);b.curve(velocity,"|r'(t)|",gold,0,1,[&](double u){return fieldLength(fieldPath(path,u,reversed).velocity);});velocity.hasMarker=true;velocity.marker={t,speed};}
        else {
          auto& integrand=b.plot("Work rate",MathParameter::FieldTime);b.curve(integrand,"F dot r'(t)",gold,0,1,[&](double u){return fieldIntegrand(rule,path,u,reversed);});integrand.hasMarker=true;integrand.marker={t,fieldIntegrand(rule,path,t,reversed)};
          auto& accumulation=b.plot("Accumulated work",MathParameter::FieldTime);b.curve(accumulation,"selected path",violet,0,1,[&](double u){return exactFieldIntegral(rule,path,u,reversed);});
          if(level==3)b.curve(accumulation,"straight comparison",white,0,1,[&](double u){return comparisonWork(rule,a,a+(end-a)*u);});accumulation.hasMarker=true;accumulation.marker={t,fieldIntegral(rule,path,t,reversed)};
        }
      }
      b.table("Probe and path vectors",{"x","y","z",""},3);const auto row=[&](std::string_view label,FieldVector v){b.row(label,{v.x,v.y,v.z,0});};
      if(level>=1)row("Start",a);row("Probe",p);row("Field F",f);row("Unit direction",d);if(level>=1)row("End",end);
      b.metric("Field x",f.x);b.metric("Field y",f.y);b.metric("Field z",f.z);b.metric("Field magnitude",fieldLength(f));b.metric("Directional component",fieldDot(f,d));
      if(level>=1) {
        const double partial=fieldIntegral(rule,path,t,reversed),total=fieldIntegral(rule,path,1,reversed);
        b.metric("Path speed",speed);b.metric("Integrand",fieldIntegrand(rule,path,t,reversed));b.metric("Work so far",partial);b.metric("Total work",total);b.metric("Quadrature error",std::max(std::fabs(partial-exactFieldIntegral(rule,path,t,reversed)),std::fabs(total-exactFieldIntegral(rule,path,1,reversed))));
        if(level==3){const double comparison=comparisonWork(rule,a,end);b.metric("Comparison work",comparison);b.metric("Path difference",total-comparison);}
      }
      break;
    }
    case MathObjectKind::Flux: {
      const unsigned level=snapshot_.level,shape=level==3?2:static_cast<unsigned>(parameter(MathParameter::FluxShape)),rule=static_cast<unsigned>(parameter(MathParameter::FluxField)),n=static_cast<unsigned>(parameter(MathParameter::FluxResolution));
      const double r=parameter(MathParameter::FluxRadius),tilt=parameter(MathParameter::FluxTilt)*pi/180,sign=parameter(MathParameter::FluxOrientation)==0?1:-1,t=parameter(MathParameter::FluxTime);
      const auto normal=rotateX({0,0,1},tilt)*sign;const unsigned count=fluxSampleCount(shape,n),probe=static_cast<unsigned>(parameter(MathParameter::FluxProbe)*(count-1));const auto sample=fluxSample(shape,n,probe,r,tilt,sign);
      const auto color=[&](FieldVector p,FieldVector normal){const double density=fieldDot(level==3?FieldVector{0,0,rule>=2?2.0:0.0}:fluxField(rule,p),normal);return std::fabs(density)<1e-8?muted:density>0?teal:coral;};
      if(shape!=1) {
        auto& surface=snapshot_.surface;surface.rows=surface.columns=MathSurfacePatch::kResolution;
        for(unsigned i=0;i<surface.rows;++i)for(unsigned j=0;j<surface.columns;++j) {
          const double u=static_cast<double>(i)/(surface.rows-1),a=2*pi*j/(surface.columns-1);FieldVector p,out;
          if(shape==0){const double mu=2*u-1,h=std::sqrt(std::max(0.0,1-mu*mu));out=rotateX({h*std::cos(a),h*std::sin(a),mu},tilt);p=out*r;}
          else {out=rotateX({0,0,1},tilt);p=rotateX({r*u*std::cos(a),r*u*std::sin(a),0},tilt)+FieldVector{0,0,.4};}
          surface.vertices[i*surface.columns+j]={sceneVector(p),sceneVector(out),color(p,out*sign)};
        }
      } else {
        const auto x=sceneVector(rotateX({1,0,0},tilt)),y=sceneVector(rotateX({0,1,0},tilt)),z=sceneVector(rotateX({0,0,1},tilt));
        for(unsigned face=0;face<6;++face){const auto f=fluxSample(1,1,face,r,tilt,sign);const float h=.008F,width=static_cast<float>(2*r);b.part(MathShape::Box,sceneVector(f.point),x*(face/2==0?h:width),y*(face/2==1?h:width),z*(face/2==2?h:width),color(f.point,f.normal),"shell_face");}
      }
      const unsigned displayN=shape==1?1:3;
      for(unsigned i=0;i<fluxSampleCount(shape,displayN);++i){const auto a=fluxSample(shape,displayN,i,r,tilt,sign);b.arrow(sceneVector(a.point),sceneVector(a.point+a.normal*.35),gold,"surface_normal");}
      FieldVector point=sample.point,direction=sample.normal;
      if(level==3) {
        const auto boundary=fluxBoundary(t,r,tilt,sign);point=boundary.position;direction=boundary.velocity*(1/fieldLength(boundary.velocity));
        for(unsigned i=0;i<48;++i)b.rod(sceneVector(fluxBoundary(i/48.0,r,tilt,sign).position),sceneVector(fluxBoundary((i+1)/48.0,r,tilt,sign).position),violet,.022F,"stokes_boundary");
        b.arrow(sceneVector(point),sceneVector(point+direction*.45),violet,"boundary_tangent");
        auto& work=b.plot("Boundary circulation",MathParameter::FluxTime);b.curve(work,"accumulated line integral",violet,0,1,[&](double u){return boundaryIntegral(rule,n,u,r,tilt,sign);});work.hasMarker=true;work.marker={t,boundaryIntegral(rule,n,t,r,tilt,sign)};
      } else {b.arrow(sceneVector(point),sceneVector(point+direction*.6),gold,"probe_normal");}
      b.ball(sceneVector(point),.065F,white,"surface_probe");b.arrow(sceneVector(point),sceneVector(point+fluxField(rule,point)*.25),coral,"probe_flux_field");b.label("Probe",sceneVector(point)+Vec3{.08F,.13F,.1F});
      b.table("Surface probe vectors",{"x","y","z",""},3);const auto row=[&](std::string_view name,FieldVector p){b.row(name,{p.x,p.y,p.z,0});};row("Position",point);row("Field",fluxField(rule,point));row(level==3?"Disk normal":"Unit normal",level==3?normal:sample.normal);if(level==3)row("Unit tangent",direction);
      b.metric("Surface area",shape==0?4*pi*r*r:shape==1?24*r*r:pi*r*r);b.metric("Normal length",fieldLength(level==3?normal:sample.normal));b.metric("Probe flux density",fieldDot(fluxField(rule,point),level==3?normal:sample.normal));b.metric("Orientation sign",sign);
      if(level>0&&level<3) {
        const double numerical=surfaceFlux(shape,rule,n,r,tilt,sign),exact=exactSurfaceFlux(shape,rule,r,tilt,sign),error=std::fabs(numerical-exact);
        b.metric("Numerical flux",numerical);b.metric("Exact flux",exact);b.metric("Absolute error",error);if(std::fabs(exact)>1e-12)b.metric("Relative error",error/std::fabs(exact));b.metric("Quadrature samples",count);
        auto& convergence=b.plot("Flux quadrature by resolution",MathParameter::FluxResolution);auto& line=convergence.series[convergence.seriesCount++];line.name="absolute error";line.color=gold;line.count=11;
        for(unsigned i=0;i<11;++i)line.points[i]={static_cast<double>(i+2),std::fabs(surfaceFlux(shape,rule,i+2,r,tilt,sign)-exact)};convergence.hasMarker=true;convergence.marker={static_cast<double>(n),error};
        if(level==2){b.metric("Closed surface",shape<2?1:0);if(shape<2){b.metric("Enclosed volume",shellVolume(shape,r));b.metric("Oriented divergence integral",sign*fluxDivergence(rule)*shellVolume(shape,r));}}
      }
      if(level==3){const double curlFlux=surfaceFlux(2,rule,n,r,tilt,sign,true),line=boundaryIntegral(rule,n,1,r,tilt,sign);b.metric("Curl flux",curlFlux);b.metric("Boundary circulation",line);b.metric("Stokes error",std::fabs(line-curlFlux));b.metric("Circulation so far",boundaryIntegral(rule,n,t,r,tilt,sign));b.metric("Boundary speed",2*pi*r);}
      break;
    }
    case MathObjectKind::Tensor: {
      const unsigned level=snapshot_.level,selectedI=static_cast<unsigned>(parameter(MathParameter::TensorI)),selectedJ=static_cast<unsigned>(parameter(MathParameter::TensorJ)),selectedK=static_cast<unsigned>(parameter(MathParameter::TensorK));
      const Triple u{parameter(MathParameter::TensorU0),parameter(MathParameter::TensorU1),parameter(MathParameter::TensorU2)},v{parameter(MathParameter::TensorV0),parameter(MathParameter::TensorV1),parameter(MathParameter::TensorV2)},w{parameter(MathParameter::TensorW0),parameter(MathParameter::TensorW1),parameter(MathParameter::TensorW2)};
      const double angle=parameter(MathParameter::TensorBasis)*pi/180,c=std::cos(angle),s=std::sin(angle);const Matrix q{c,-s,0,s,c,0,0,0,1},a=outerProduct(u,v),changed=multiply(multiply(transpose(q),a),q);const auto shown=level==3?changed:a;
      const bool third=level==1||level==2;const unsigned slices=third?3:1;const double contractionScale=tripleDot(v,w);double maxAbs=0,tensorNorm=0;
      for(unsigned k=0;k<slices;++k)for(double value:shown){const double x=value*(third?w[k]:1);maxAbs=std::max(maxAbs,std::fabs(x));tensorNorm+=x*x;}
      const double heightScale=.5/std::max(1.0,maxAbs),gap=.65+parameter(MathParameter::TensorGap);static constexpr std::array<std::string_view,3> sliceNames{"k = 1","k = 2","k = 3"};
      b.table(third?"Tensor components: index i j k":level==3?"Components and new vector coordinates":"Matrix components: index i j",level==3?std::array<std::string_view,4>{"value / x","y","z","selected"}:std::array<std::string_view,4>{"component","selected","",""},level==3?4:2);
      static constexpr std::array<std::string_view,9> matrixLabels{"11","12","13","21","22","23","31","32","33"};
      for(unsigned k=0;k<slices;++k) {
        Matrix slice=shown;for(auto& x:slice)x*=third?w[k]:1;
        if(level<3)b.matrix(third?sliceNames[k]:"u v^T",slice);
        for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j) {
          const double component=slice[3*i+j];const bool selected=i==selectedI&&j==selectedJ&&(!third||k==selectedK);const bool contracted=level==2&&j==k;
          const Vec3 base{.65F+.55F*(static_cast<float>(j)-1),.55F*(1-static_cast<float>(i)),static_cast<float>(k*gap)};const float height=static_cast<float>(heightScale*component);
          b.scaled(MathShape::Box,base+Vec3{0,0,height*.5F},{.35F,.35F,std::max(.012F,std::fabs(height))},selected||contracted?gold:std::fabs(component)<1e-9?muted:component>0?teal:coral,"tensor_component");
          b.row(third?tensorLabels[9*k+3*i+j]:matrixLabels[3*i+j],level==3?std::array<double,4>{component,0,0,selected?1.0:0.0}:std::array<double,4>{component,selected?1.0:0.0,0,0});
        }
        if(third)b.label(sliceNames[k],{1.7F,.8F,static_cast<float>(k*gap)},white);
      }
      const Vec3 origin{-1.6F,-.4F,0};unsigned vectorLabel=0;const auto arrow=[&](const Triple& x,Vec3 color,std::string_view name){const Vec3 end=origin+Vec3{static_cast<float>(x[0]*.35),static_cast<float>(x[1]*.35),static_cast<float>(x[2]*.35)};b.arrow(origin,end,color,name);b.label(name,end+Vec3{0,.1F+.12F*vectorLabel++,.1F},color);};arrow(u,coral,"u");arrow(v,teal,"v");if(third)arrow(w,violet,"w");
      b.metric("Selected component",shown[3*selectedI+selectedJ]*(third?w[selectedK]:1));b.metric("Tensor norm",std::sqrt(tensorNorm));b.metric("u norm",std::sqrt(tripleDot(u,u)));b.metric("v norm",std::sqrt(tripleDot(v,v)));b.metric("Component height scale",heightScale);
      if(third){b.metric("w norm",std::sqrt(tripleDot(w,w)));if(level==2){b.metric("v dot w",contractionScale);b.metric("Contraction x",u[0]*contractionScale);b.metric("Contraction y",u[1]*contractionScale);b.metric("Contraction z",u[2]*contractionScale);b.metric("Contraction norm",std::sqrt(tripleDot(u,u))*std::fabs(contractionScale));if(level==2){Triple contracted=u;for(auto& x:contracted)x*=contractionScale;const double length=std::sqrt(tripleDot(contracted,contracted));if(length>0)for(auto& x:contracted)x/=std::max(1.0,length);arrow(contracted,blue,"contraction (scaled)");}}}
      else {b.metric("Trace",a[0]+a[4]+a[8]);if(level==3){Matrix difference{};for(unsigned i=0;i<9;++i)difference[i]=changed[i]-a[i];b.metric("Component change",frobenius(difference));b.metric("Trace error",std::fabs(a[0]+a[4]+a[8]-changed[0]-changed[4]-changed[8]));b.metric("Norm error",std::fabs(frobenius(a)-frobenius(changed)));b.matrix("World components A",a);b.matrix("New basis components Q^T A Q",changed);b.matrix("Basis columns Q",q);for(unsigned j=0;j<2;++j){Triple axis{};for(unsigned i=0;i<3;++i)axis[i]=q[3*i+j];arrow(axis,j?blue:gold,j?"basis 2":"basis 1");}
          const auto newU=coordinates(q,u),newV=coordinates(q,v);b.row("u in new basis",{newU[0],newU[1],newU[2],0});b.row("v in new basis",{newV[0],newV[1],newV[2],0});}}
      break;
    }
    case MathObjectKind::Probability: {
      const unsigned level=snapshot_.level,rule=static_cast<unsigned>(parameter(MathParameter::ProbabilityRule)),start=static_cast<unsigned>(parameter(MathParameter::ProbabilityStart)),row=static_cast<unsigned>(parameter(MathParameter::ProbabilityRow)),steps=static_cast<unsigned>(parameter(MathParameter::ProbabilitySteps));const double stay=parameter(MathParameter::ProbabilityStay);
      const auto matrix=probabilityMatrix(rule,stay);const auto stationary=stationaryExample(rule);const auto initial=probabilityInitial(level==1?row:start,level==1?0:parameter(MathParameter::ProbabilityMix));std::array<Triple,65> trace{};trace[0]=initial;for(unsigned i=1;i<trace.size();++i)trace[i]=probabilityNext(trace[i-1],matrix);
      Triple shown=trace[steps];unsigned current=probabilityWalk_[probabilityWalkCount_-1];Triple counts{};for(unsigned i=0;i<probabilityWalkCount_;++i)++counts[probabilityWalk_[i]];
      if(level==0){shown={};shown[current]=1;}else if(level==1)for(unsigned j=0;j<3;++j)shown[j]=matrix[3*row+j];
      for(unsigned i=0;i<3;++i) {
        const Vec3 p=probabilityPositions[i];const bool visibleMass=shown[i]>=1e-9;const float radius=visibleMass?static_cast<float>(.42*std::cbrt(shown[i])):.045F;b.ball(p,radius,visibleMass?teal:muted,"probability_mass");b.label(probabilityLabels[i],p+Vec3{0,-.18F,.5F},i==current&&level==0?gold:white);
        for(unsigned j=0;j<3;++j) {
          const double weight=matrix[3*i+j];if(weight==0)continue;const auto color=(level==0?i==current:level==1?i==row:true)?blue:muted;
          if(i==j){for(unsigned k=0;k<12;++k){const double a=2*pi*k/12,beta=2*pi*(k+1)/12;const Vec3 centre=p+Vec3{0,0,.56F};b.rod(centre+Vec3{static_cast<float>(.16*std::cos(a)),static_cast<float>(.16*std::sin(a)),0},centre+Vec3{static_cast<float>(.16*std::cos(beta)),static_cast<float>(.16*std::sin(beta)),0},color,static_cast<float>(.012+.025*weight),"self_loop");}}
          else {const auto delta=probabilityPositions[j]-p,dir=normalized(delta),side=normalized(cross(dir,Vec3{0,0,1}))*.09F;const auto from=p+dir*.46F+side,to=probabilityPositions[j]-dir*.46F+side;b.rod(from,to,color,static_cast<float>(.009+.05*weight),"transition_weight");b.arrow(to-dir*.15F,to,color,"transition_direction");}
        }
      }
      b.matrix("Transition matrix P (rows: from; columns: to)",matrix);b.table(level==0?"Observed walk":"Probability by state",level==0?std::array<std::string_view,4>{"visits","fraction","current",""}:std::array<std::string_view,4>{"initial","shown p","stationary pi",""},3);
      for(unsigned i=0;i<3;++i)b.row(probabilityLabels[i],level==0?std::array<double,4>{counts[i],counts[i]/probabilityWalkCount_,i==current?1.0:0.0,0}:std::array<double,4>{initial[i],shown[i],stationary[i],0});
      b.metric("Probability sum",shown[0]+shown[1]+shown[2]);b.metric("p(A)",shown[0]);b.metric("p(B)",shown[1]);b.metric("p(C)",shown[2]);double rowError=0;for(unsigned i=0;i<3;++i)rowError=std::max(rowError,std::fabs(matrix[3*i]+matrix[3*i+1]+matrix[3*i+2]-1));b.metric("Row sum error",rowError);
      if(level==0){unsigned visited=0;for(double count:counts)visited+=count>0;b.metric("Walk transitions",probabilityWalkCount_-1);b.metric("Visited states",visited);b.metric("Current state",current);auto& plot=b.plot("Observed state by walk step");auto& line=plot.series[plot.seriesCount++];line.name="sampled state (A=0,B=1,C=2)";line.color=gold;line.count=probabilityWalkCount_;for(unsigned i=0;i<line.count;++i)line.points[i]={static_cast<double>(i),static_cast<double>(probabilityWalk_[i])};}
      if(level>=2){auto& plot=b.plot("Distribution at each transition",MathParameter::ProbabilitySteps);for(unsigned state=0;state<3;++state){auto& line=plot.series[plot.seriesCount++];line.name=probabilityLabels[state];line.color=std::array<Vec3,3>{coral,teal,blue}[state];line.count=trace.size();for(unsigned i=0;i<trace.size();++i)line.points[i]={static_cast<double>(i),trace[i][state]};}plot.hasMarker=true;plot.marker={static_cast<double>(steps),shown[0]};b.metric("Transitions",steps);}
      if(level==3){const bool unique=rule==2||stay<1,mixing=rule==1?stay<1:rule==3?stay<1:rule==0?stay>0&&stay<1:false;b.metric("Unique stationary distribution",unique?1:0);b.metric("Convergence to pi guaranteed",mixing?1:0);b.metric("Stationary residual",totalVariation(probabilityNext(stationary,matrix),stationary));b.metric("Distance to pi",totalVariation(shown,stationary));auto& distance=b.plot("Distance to the shown stationary distribution",MathParameter::ProbabilitySteps);auto& line=distance.series[distance.seriesCount++];line.name="total variation";line.color=gold;line.count=trace.size();for(unsigned i=0;i<trace.size();++i)line.points[i]={static_cast<double>(i),totalVariation(trace[i],stationary)};distance.hasMarker=true;distance.marker={static_cast<double>(steps),totalVariation(shown,stationary)};}
      break;
    }
    case MathObjectKind::Binomial: {
      const unsigned level=snapshot_.level,n=static_cast<unsigned>(parameter(MathParameter::BinomialTrials)),cut=static_cast<unsigned>(parameter(MathParameter::BinomialCut));const double p=parameter(MathParameter::BinomialChance),mean=n*p,variance=n*p*(1-p),sd=std::sqrt(variance);const auto mass=binomialMass(n,p);const float spacing=3.5F/(n+1);
      const auto at=[&](unsigned row,unsigned successes){return Vec3{spacing*(static_cast<float>(successes)-row*.5F),1.2F-2.4F*row/n,.09F};};
      for(unsigned row=0;row<n;++row)for(unsigned j=0;j<=row;++j)b.scaled(MathShape::Box,at(row,j)-Vec3{0,0,.07F},{.045F,.045F,.045F},muted,"binomial_peg");
      for(unsigned k=0;k<=n;++k){const bool visible=mass[k]>=1e-9;const float h=visible?static_cast<float>(2.4*mass[k]):.006F;const Vec3 base{spacing*(static_cast<float>(k)-n*.5F),-1.65F,0};b.scaled(MathShape::Box,base+Vec3{0,0,h*.5F},{spacing*.65F,.24F,h},!visible?muted:level>0&&k==cut?gold:teal,"binomial_mass");b.label(countLabels[k],base+Vec3{0,-.22F,0});}
      if(level==0){for(unsigned i=0;i<bernoulliSteps_;++i)b.rod(at(i,bernoulliPath_[i]),at(i+1,bernoulliPath_[i+1]),gold,.023F,"observed_trial");b.ball(at(bernoulliSteps_,bernoulliPath_[bernoulliSteps_]),.075F,coral,"trial_bead");b.metric("Trials completed",bernoulliSteps_);b.metric("Successes",bernoulliPath_[bernoulliSteps_]);b.metric("Failures",bernoulliSteps_-bernoulliPath_[bernoulliSteps_]);}
      if(level>=2){const auto marker=[&](double x,Vec3 color){const float at=spacing*static_cast<float>(x-n*.5);b.rod({at,-1.84F,.05F},{at,-1.84F,.5F},color,.015F,"distribution_marker");};marker(mean,gold);marker(mean-sd,blue);marker(mean+sd,blue);}
      const bool approximate=level==3&&variance>0;b.table("Count probabilities",{"P(X=k)","P(X<=k)","normal bin mass",""},approximate?3:2);
      auto& plot=b.plot("Count probability mass",level>0?MathParameter::BinomialCut:MathParameter::Count);auto& exactLine=plot.series[plot.seriesCount++];exactLine.name="binomial";exactLine.color=teal;exactLine.stems=true;exactLine.count=n+1;
      double sum=0,normalSum=0,maxError=0;MathPlotSeries* normalLine=nullptr;if(approximate){normalLine=&plot.series[plot.seriesCount++];normalLine->name="normal with continuity correction";normalLine->color=violet;normalLine->stems=true;normalLine->count=n+1;maxError=normalCdf(-.5,mean,sd);}
      for(unsigned k=0;k<=n;++k){sum+=mass[k];const double normal=approximate?normalCdf(k+.5,mean,sd)-normalCdf(static_cast<double>(k)-.5,mean,sd):0;b.row(countLabels[k],{mass[k],sum,normal,0});exactLine.points[k]={static_cast<double>(k),mass[k]};if(approximate){normalLine->points[k]={static_cast<double>(k),normal};normalSum+=normal;maxError=std::max(maxError,std::fabs(sum-normalCdf(k+.5,mean,sd)));}}
      if(level>0){plot.hasMarker=true;plot.marker={static_cast<double>(cut),mass[cut]};b.metric("Selected count",cut);b.metric("Point probability",mass[cut]);b.metric("CDF P(X<=k)",binomialCdf(mass,static_cast<int>(cut)));double tail=0;for(unsigned k=cut+1;k<=n;++k)tail+=mass[k];b.metric("Tail P(X>k)",tail);}
      b.metric("Probability sum",sum);b.metric("Mean",mean);b.metric("Variance",variance);b.metric("Standard deviation",sd);
      if(level==3){b.metric("Normal approximation defined",approximate?1:0);if(approximate){b.metric("Normal CDF at threshold",normalCdf(cut+.5,mean,sd));b.metric("Maximum CDF error",maxError);b.metric("Normal mass outside support",std::max(0.0,1-normalSum));}}
      break;
    }
    case MathObjectKind::Bayes: {
      const unsigned level=snapshot_.level,yes=static_cast<unsigned>(parameter(MathParameter::BayesPositive)),no=static_cast<unsigned>(parameter(MathParameter::BayesNegative));const double prior=parameter(MathParameter::BayesPrior),hit=parameter(MathParameter::BayesHit),falseHit=parameter(MathParameter::BayesFalse);const bool complement=level==1||level==2?parameter(MathParameter::BayesEvent)==1:false;
      const double lh=level==3?evidenceLikelihood(hit,yes,no):complement?1-hit:hit,lo=level==3?evidenceLikelihood(falseHit,yes,no):complement?1-falseHit:falseHit;const auto result=bayesUpdate(prior,lh,lo);
      const Vec3 left{-1.8F,-.5F,-.5F},right{.65F,-.5F,-.5F};const auto piece=[&](Vec3 origin,double x,double y,double width,double height,Vec3 color){if(width*height<1e-9)return;b.scaled(MathShape::Box,origin+Vec3{static_cast<float>(x+width*.5),static_cast<float>(y+height*.5),.5F},{static_cast<float>(width),static_cast<float>(height),1},color,"probability_piece");};
      piece(left,0,0,prior,lh,gold);piece(left,0,lh,prior,1-lh,teal);piece(left,prior,0,1-prior,lo,coral);piece(left,prior,lo,1-prior,1-lo,muted);
      const auto cage=[&](Vec3 origin){for(unsigned i=0;i<8;++i)for(unsigned bit=0;bit<3;++bit)if(!(i&(1U<<bit))){const auto p=[&](unsigned k){return origin+Vec3{k&1?1.0F:0.0F,k&2?1.0F:0.0F,k&4?1.0F:0.0F};};b.rod(p(i),p(i|(1U<<bit)),white,.008F,"unit_probability_cube");}};cage(left);b.label(level==3?"Joint: full evidence sequence":"Joint model",left+Vec3{.5F,1.2F,.5F});
      if(level>0&&result.defined){b.arrow({-.55F,0,0},{.5F,0,0},violet,"conditioning_rescale");piece(right,0,0,result.posterior,1,gold);piece(right,result.posterior,0,result.posteriorOther,1,coral);cage(right);b.label("Conditioned: total = 1",right+Vec3{.5F,1.2F,.5F});}
      if(level>0&&!result.defined)b.label("Zero-probability evidence",right+Vec3{.5F,.5F,.5F},coral);
      b.table("Hypotheses and selected evidence",{"prior","likelihood","joint","posterior"},level>0&&result.defined?4:3);b.row("H",{prior,lh,result.h,result.posterior});b.row("not H",{1-prior,lo,result.other,result.posteriorOther});
      b.metric("Prior P(H)",prior);b.metric("Evidence probability",result.evidence);b.metric("Joint P(H,evidence)",result.h);b.metric("Joint P(not H,evidence)",result.other);b.metric("Joint total mass",result.h+prior*(1-lh)+result.other+(1-prior)*(1-lo));
      if(level>0){b.metric("Conditioning defined",result.defined?1:0);if(result.defined){b.metric("Posterior P(H)",result.posterior);b.metric("Posterior shift",result.posterior-prior);}}
      if(level==2){const auto ratio=[&](std::string_view value,std::string_view infinite,std::string_view undefined,double numerator,double denominator){if(denominator>0)b.metric(value,numerator/denominator);else b.metric(numerator>0?infinite:undefined,1);};ratio("Prior odds","Prior odds infinite","Prior odds undefined",prior,1-prior);ratio("Likelihood ratio","Likelihood ratio infinite","Likelihood ratio undefined",lh,lo);ratio("Posterior odds","Posterior odds infinite","Posterior odds undefined",result.h,result.other);}
      if(level==3){b.metric("Evidence outcomes",yes+no);auto& plot=b.plot("Posterior through ordered evidence");auto& line=plot.series[plot.seriesCount++];line.name="P(H | prefix)";line.color=gold;for(unsigned step=0;step<=yes+no;++step){const unsigned positives=std::min(step,yes),negatives=step-positives;const auto updated=bayesUpdate(prior,evidenceLikelihood(hit,positives,negatives),evidenceLikelihood(falseHit,positives,negatives));if(!updated.defined)break;line.points[line.count++]={static_cast<double>(step),updated.posterior};}if(result.defined){plot.hasMarker=true;plot.marker={static_cast<double>(yes+no),result.posterior};}}
      break;
    }
    case MathObjectKind::Covariance: {
      const unsigned level=snapshot_.level,component=static_cast<unsigned>(parameter(MathParameter::CloudComponent));const Triple scales{parameter(MathParameter::CloudX),parameter(MathParameter::CloudY),parameter(MathParameter::CloudZ)},centre{parameter(MathParameter::CloudMeanX),parameter(MathParameter::CloudMeanY),parameter(MathParameter::CloudMeanZ)};
      const auto transform=cloudTransform(scales,level?parameter(MathParameter::CloudShear):0,parameter(MathParameter::CloudYaw)*pi/180,parameter(MathParameter::CloudPitch)*pi/180);const auto decomposition=svd(transform);unsigned rank=0;for(double sigma:decomposition.sigma)rank+=sigma>0;const bool fullRank=rank==3;std::array<Triple,8> points{},centered{},whitened{};
      for(unsigned i=0;i<8;++i){const Triple base{i&1?1.0:-1.0,i&2?1.0:-1.0,i&4?1.0:-1.0};centered[i]=tripleTransform(transform,base);points[i]=tripleAdd(centre,centered[i]);}const auto moments=cloudMoments(points);Matrix whitening{};CloudMoments whitenedMoments;
      if(level==3&&fullRank){const double amount=parameter(MathParameter::CloudWhiten);for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)whitening[3*i+j]=((1-amount)+amount/decomposition.sigma[i])*decomposition.u[3*j+i];for(unsigned i=0;i<8;++i)whitened[i]=tripleTransform(whitening,centered[i]);whitenedMoments=cloudMoments(whitened);}
      double extent=1;for(unsigned i=0;i<8;++i){const auto p=level==3?centered[i]:points[i];for(double v:p)extent=std::max(extent,std::fabs(v));if(level==3&&fullRank)for(double v:whitened[i])extent=std::max(extent,std::fabs(v));}const float scale=static_cast<float>((level==3?.78:1.5)/extent);const Vec3 offset=level==3?Vec3{-1.3F,0,0}:Vec3{},right{1.3F,0,0};const auto scene=[&](const Triple& p){return offset+tripleScene(p)*scale;};const Triple zero{};const auto origin=scene(level==3?zero:centre);
      for(unsigned i=0;i<8;++i){const auto position=scene(level==3?centered[i]:points[i]);b.ball(position,.055F,teal,"cloud_point");if(level==0)b.label(cloudLabels[i],position+Vec3{0,.08F,.09F});if(level==2){Triple axis{};for(unsigned j=0;j<3;++j)axis[j]=decomposition.u[3*j+component];const double projection=tripleDot(centered[i],axis);Triple projected=centre;for(unsigned j=0;j<3;++j)projected[j]+=axis[j]*projection;const auto to=scene(projected);b.ball(to,.038F,blue,"principal_projection");b.rod(position,to,muted,.006F,"projection_residual");}if(level==3&&fullRank)b.ball(right+tripleScene(whitened[i])*scale,.055F,blue,"whitened_point");}
      b.ball(origin,.075F,gold,"cloud_mean");
      const auto ellipsoid=[&](Vec3 anchor,const Matrix& axes,const Triple& radii){for(unsigned plane=0;plane<3;++plane)for(unsigned step=0;step<24;++step){const auto p=[&](double angle){Triple local{};local[plane]=radii[plane]*std::cos(angle);local[(plane+1)%3]=radii[(plane+1)%3]*std::sin(angle);return anchor+tripleScene(tripleTransform(axes,local))*scale;};b.rod(p(2*pi*step/24),p(2*pi*(step+1)/24),muted,.007F,"covariance_ellipse");}};
      if(level>=1)ellipsoid(origin,decomposition.u,decomposition.sigma);
      if(level==2)for(unsigned i=0;i<3;++i){Triple axis{};for(unsigned j=0;j<3;++j)axis[j]=decomposition.u[3*j+i]*decomposition.sigma[i];b.arrow(origin,origin+tripleScene(axis)*scale,i==component?gold:violet,"principal_axis");}
      if(level==3){b.label("Centered input",offset+Vec3{0,-1.2F,0});if(fullRank){Triple radii{};for(unsigned i=0;i<3;++i)radii[i]=std::sqrt(std::max(0.0,whitenedMoments.covariance[4*i]));ellipsoid(right,identity,radii);b.label("Principal / whitened coordinates",right+Vec3{0,-1.2F,0},blue);}else b.label("Whitening undefined: singular covariance",right,coral);}
      b.table(level==3?"Original and transformed coordinates":"Eight equally weighted points",{"x","y","z","weight"},4);for(unsigned i=0;i<8;++i)b.row(cloudLabels[i],{points[i][0],points[i][1],points[i][2],.125});b.row("Mean (aggregate)",{moments.mean[0],moments.mean[1],moments.mean[2],0});if(level==3&&fullRank)for(unsigned i=0;i<8;++i)b.row(whitenLabels[i],{whitened[i][0],whitened[i][1],whitened[i][2],.125});
      b.metric("Mean x",moments.mean[0]);b.metric("Mean y",moments.mean[1]);b.metric("Mean z",moments.mean[2]);b.metric("Total variance",moments.covariance[0]+moments.covariance[4]+moments.covariance[8]);b.metric("Population points",8);
      if(level>=1)b.matrix("Population covariance (divisor 8)",moments.covariance);
      if(level==1){b.metric("Variance x",moments.covariance[0]);b.metric("Variance y",moments.covariance[4]);b.metric("Variance z",moments.covariance[8]);b.metric("Covariance xy",moments.covariance[1]);const double denominator=std::sqrt(moments.covariance[0]*moments.covariance[4]);b.metric("Correlation defined",denominator>1e-12?1:0);if(denominator>1e-12)b.metric("Correlation xy",moments.covariance[1]/denominator);}
      if(level>=2){b.metric("Covariance rank",rank);if(level==2){Matrix eigenvalues{};for(unsigned i=0;i<3;++i)eigenvalues[4*i]=decomposition.sigma[i]*decomposition.sigma[i];b.matrix("Principal directions Q",decomposition.u);b.matrix("Principal variances",eigenvalues);b.metric("Largest variance",eigenvalues[0]);b.metric("Middle variance",eigenvalues[4]);b.metric("Smallest variance",eigenvalues[8]);b.metric("Selected projected variance",eigenvalues[4*component]);}
        else {b.metric("Whitening defined",fullRank?1:0);if(fullRank){Matrix error=whitenedMoments.covariance;for(unsigned i=0;i<3;++i)error[4*i]-=1;b.metric("Identity covariance error",frobenius(error));b.metric("Whitened total variance",whitenedMoments.covariance[0]+whitenedMoments.covariance[4]+whitenedMoments.covariance[8]);b.matrix("Transformed covariance",whitenedMoments.covariance);b.matrix("Coordinate transform",whitening);}}}
      break;
    }
    case MathObjectKind::Spherical: {
      const unsigned level=snapshot_.level,a=level?static_cast<unsigned>(parameter(MathParameter::SphereMode)):2,other=level>=2?static_cast<unsigned>(parameter(MathParameter::SphereSecond)):a;
      const double theta=parameter(MathParameter::SphereTheta)*pi/180,phi=parameter(MathParameter::SpherePhi)*pi/180,c=level>=2?parameter(MathParameter::SphereMix):0,t=level==3?parameter(MathParameter::SphereHeat):0;
      const auto ma=sphereMode(a),mb=sphereMode(other);const double la=ma.degree*(ma.degree+1),lb=mb.degree*(mb.degree+1),ca=std::exp(-la*t),cb=c*std::exp(-lb*t);
      const auto f=[&](double polar,double azimuth){return ca*sphereHarmonic(a,polar,azimuth)+cb*sphereHarmonic(other,polar,azimuth);};
      const auto point=[&](double polar,double azimuth){const auto u=sphereDirection(polar,azimuth);const double radius=level?.3+1.4*std::fabs(f(polar,azimuth)):1;return tripleScene(u)*static_cast<float>(radius);};
      auto& surface=snapshot_.surface;surface.rows=surface.columns=MathSurfacePatch::kResolution;
      for(unsigned i=0;i<surface.rows;++i)for(unsigned j=0;j<surface.columns;++j){const double polar=pi*i/(surface.rows-1),azimuth=2*pi*j/(surface.columns-1),v=f(polar,azimuth);const auto u=tripleScene(sphereDirection(polar,azimuth));Vec3 normal=u;
        if(i>0&&i+1<surface.rows){const auto du=point(polar+1e-4,azimuth)-point(polar-1e-4,azimuth),dv=point(polar,azimuth+1e-4)-point(polar,azimuth-1e-4);const auto crossProduct=cross(du,dv);if(length(crossProduct)>1e-12F)normal=normalized(crossProduct);}
        surface.vertices[i*surface.columns+j]={point(polar,azimuth),normal,level?(std::fabs(v)<1e-10?muted:v>=0?teal:coral):blue};
      }
      const auto direction=sphereDirection(theta,phi);const auto probe=point(theta,phi);b.ball(probe,.065F,gold,"sphere_probe");b.arrow({},probe,gold,"sphere_direction");
      b.rod({-1.6F,0,0},{1.6F,0,0},muted);b.rod({0,-1.6F,0},{0,1.6F,0},muted);b.rod({0,0,-1.6F},{0,0,1.6F},muted);b.label("z / north",{0,0,1.75F});
      auto& meridian=b.plot(level?"Function along the selected meridian":"Coordinates along the selected meridian",MathParameter::SphereTheta);
      b.curve(meridian,level?"f(theta, phi)":"z = cos(theta)",teal,0,180,[&](double degrees){return level?f(degrees*pi/180,phi):std::cos(degrees*pi/180);});meridian.hasMarker=true;meridian.marker={theta*180/pi,level?f(theta,phi):direction[2]};
      auto& latitude=b.plot(level?"Function along the selected latitude":"Coordinates along the selected latitude",MathParameter::SpherePhi);
      b.curve(latitude,level?"f(theta, phi)":"y = sin(theta) sin(phi)",coral,0,360,[&](double degrees){return level?f(theta,degrees*pi/180):std::sin(theta)*std::sin(degrees*pi/180);});latitude.hasMarker=true;latitude.marker={phi*180/pi,level?f(theta,phi):direction[1]};
      if(level==0){b.metric("Direction x",direction[0]);b.metric("Direction y",direction[1]);b.metric("Direction z",direction[2]);b.metric("Squared radius",tripleDot(direction,direction));b.label("Unit sphere",{0,-1.5F,-1.5F},blue);}
      else {
        const double crossInner=sphereInner(a,other),same=a==other?1:0,energy=ca*ca+cb*cb+2*ca*cb*same,numeric=ca*ca*sphereInner(a,a)+cb*cb*sphereInner(other,other)+2*ca*cb*crossInner,initial=1+c*c+2*c*same;
        b.metric("Degree",ma.degree);b.metric("Order",ma.order);b.metric("Probe value",f(theta,phi));b.metric("Energy",energy);b.metric("Integrated energy",numeric);b.metric("Energy error",std::fabs(energy-numeric));
        b.table("Real modes: coefficients and projections",{"degree","order","coefficient","projection"},4);
        b.row("First mode",{static_cast<double>(ma.degree),static_cast<double>(ma.order),ca,ca*sphereInner(a,a)+cb*crossInner});
        if(level>=2){b.row("Second mode",{static_cast<double>(mb.degree),static_cast<double>(mb.order),cb,cb*sphereInner(other,other)+ca*crossInner});b.metric("Cross inner product",crossInner);b.metric("Initial energy",initial);}
        if(level==3){b.metric("Heat time",t);b.metric("First eigenvalue",-la);b.metric("Laplacian residual",sphereLaplaceError(a));b.metric("Energy lost",initial-energy);
          auto& heat=b.plot("Energy during spherical heat flow",MathParameter::SphereHeat);b.curve(heat,"Integral f(t)^2",gold,0,1,[&](double time){const double x=std::exp(-la*time),y=c*std::exp(-lb*time);return x*x+y*y+2*x*y*same;});heat.hasMarker=true;heat.marker={t,energy};}
        b.label("Teal + / coral - / gold probe",{0,-1.5F,-1.5F});
      }
      break;
    }
    case MathObjectKind::Quadratic: {
      const unsigned level=snapshot_.level;const Triple lambda{parameter(MathParameter::QuadLambdaX),parameter(MathParameter::QuadLambdaY),parameter(MathParameter::QuadLambdaZ)},x{parameter(MathParameter::QuadX),parameter(MathParameter::QuadY),parameter(MathParameter::QuadZ)};
      const Matrix q=cloudTransform({1,1,1},0,level?parameter(MathParameter::QuadYaw)*pi/180:0,level?parameter(MathParameter::QuadPitch)*pi/180:0),d{lambda[0],0,0,0,lambda[1],0,0,0,lambda[2]},a=multiply(multiply(q,d),transpose(q));
      const auto evaluate=[&](const Triple& v){return tripleDot(v,tripleTransform(a,v));};const auto y=tripleTransform(transpose(q),x);const double value=evaluate(x),sum=lambda[0]*y[0]*y[0]+lambda[1]*y[1]*y[1]+lambda[2]*y[2]*y[2],norm=tripleDot(x,x);
      unsigned positive=0,negative=0,zero=0;for(double l:lambda){positive+=l>0;negative+=l<0;zero+=l==0;}const double lo=*std::min_element(lambda.begin(),lambda.end()),hi=*std::max_element(lambda.begin(),lambda.end());
      b.matrix("Symmetric matrix A",a);if(level>=1){b.matrix("Orthonormal directions Q",q);b.matrix("Principal values D",d);}
      b.table("Principal-coordinate evaluation",{"coordinate y","lambda","lambda*y^2","world x"},4);constexpr std::array<std::string_view,3> axes{"Axis 1","Axis 2","Axis 3"};
      for(unsigned i=0;i<3;++i)b.row(axes[i],{y[i],lambda[i],lambda[i]*y[i]*y[i],x[i]});
      auto& surface=snapshot_.surface;surface.rows=surface.columns=MathSurfacePatch::kResolution;
      for(unsigned i=0;i<surface.rows;++i)for(unsigned j=0;j<surface.columns;++j){Triple v{};Vec3 position,normal;double height;
        if(level==3){v=sphereDirection(pi*i/(surface.rows-1),2*pi*j/(surface.columns-1));height=evaluate(v);position=normal=tripleScene(v);}
        else {v={-2+4.0*j/(surface.columns-1),-2+4.0*i/(surface.rows-1),x[2]};height=evaluate(v);const auto gradient=tripleTransform(a,v);position={static_cast<float>(v[0]),static_cast<float>(v[1]),static_cast<float>(.15*height)};normal=normalized(Vec3{static_cast<float>(-.3*gradient[0]),static_cast<float>(-.3*gradient[1]),1});}
        const float amount=static_cast<float>(std::clamp(std::fabs(height)/(level==3?2:8),0.0,1.0));surface.vertices[i*surface.columns+j]={position,normal,muted*(1-amount)+(height>=0?teal:coral)*amount};
      }
      for(unsigned i=0;i<3;++i){const Vec3 axis{static_cast<float>(q[i]),static_cast<float>(q[3+i]),static_cast<float>(q[6+i])};const Vec3 color=i==0?teal:i==1?coral:blue;b.arrow({},axis*1.65F,color,"principal_direction");b.label(axes[i],axis*1.8F,color);}
      if(level<3){const Vec3 probe{static_cast<float>(x[0]),static_cast<float>(x[1]),static_cast<float>(.15*value)};b.ball(probe,.07F,gold,"quadratic_graph_probe");b.rod({probe.x,probe.y,0},probe,gold);b.label("Height = 0.15 q(x,y,z0)",{-1.8F,-2.2F,0});}
      else {if(norm>0)b.ball(tripleScene(x)*static_cast<float>(1/std::sqrt(norm)),.07F,gold,"rayleigh_probe");else b.label("Rayleigh quotient undefined at x=0",{0,-1.6F,0},coral);b.label("Unit sphere colored by q",{0,-1.6F,-.4F});}
      b.metric("Quadratic value",value);b.metric("Principal sum",sum);b.metric("Coordinate identity error",std::fabs(value-sum));b.metric("Trace",lambda[0]+lambda[1]+lambda[2]);b.metric("Determinant",lambda[0]*lambda[1]*lambda[2]);
      if(level>=2){b.metric("Positive directions",positive);b.metric("Negative directions",negative);b.metric("Null directions",zero);
        b.label(positive&&negative?"Indefinite":positive==3?"Positive definite":negative==3?"Negative definite":positive?"Positive semidefinite":negative?"Negative semidefinite":"Zero form",{-1.8F,-2.2F,.4F},gold);}
      if(level==2){auto& contours=snapshot_.contours;contours.active=true;contours.constrained=false;contours.point={x[0],x[1]};const auto ax=tripleTransform(a,x);contours.gradient={2*ax[0],2*ax[1]};
        // Marching squares on q(x,y,z0)=0; an identically zero cell has no isolated contour.
        for(unsigned i=0;i<20;++i)for(unsigned j=0;j<20;++j){const double u=-2+.2*j,v=-2+.2*i;const std::array<MathPlotPoint,4> corners{{{u,v},{u+.2,v},{u+.2,v+.2},{u,v+.2}}};std::array<double,4> heights{};for(unsigned k=0;k<4;++k)heights[k]=evaluate({corners[k].x,corners[k].y,x[2]});std::array<MathPlotPoint,4> cuts{};unsigned count=0;
          for(unsigned k=0;k<4;++k){const unsigned next=(k+1)%4;if((heights[k]<0)!=(heights[next]<0)){const double fraction=heights[k]/(heights[k]-heights[next]);cuts[count++]={corners[k].x+fraction*(corners[next].x-corners[k].x),corners[k].y+fraction*(corners[next].y-corners[k].y)};}}
          for(unsigned k=0;k+1<count;k+=2){if(contours.count==contours.segments.size())throw std::logic_error("quadratic contour capacity");contours.segments[contours.count++]={cuts[k],cuts[k+1],0};}
        }
        b.metric("Entire section zero",std::fabs(a[0])+std::fabs(a[1])+std::fabs(a[4])+std::fabs(a[2]*x[2])+std::fabs(a[5]*x[2])+std::fabs(a[8]*x[2]*x[2])<1e-12?1:0);
      }
      if(level==3){b.metric("Rayleigh defined",norm>0?1:0);b.metric("Smallest eigenvalue",lo);b.metric("Largest eigenvalue",hi);if(norm>0)b.metric("Rayleigh quotient",value/norm);}
      auto& plot=b.plot(level==3?"Rayleigh quotient around principal great circle":"Quadratic section at the probe y and z",level==3?MathParameter::Count:MathParameter::QuadX);
      b.curve(plot,level==3?"R(Q*(cos t, sin t, 0))":"q(x,y0,z0)",teal,level==3?0:-2,level==3?360:2,[&](double input){return level==3?lambda[0]*std::pow(std::cos(input*pi/180),2)+lambda[1]*std::pow(std::sin(input*pi/180),2):evaluate({input,x[1],x[2]});});
      if(level<3){plot.hasMarker=true;plot.marker={x[0],value};}
      break;
    }
    case MathObjectKind::Roots: {
      const unsigned level=snapshot_.level,n=static_cast<unsigned>(parameter(MathParameter::RootN)),probe=static_cast<unsigned>(parameter(MathParameter::RootIndex))%n,k=static_cast<unsigned>(parameter(MathParameter::RootMultiplier))%n,power=static_cast<unsigned>(parameter(MathParameter::RootPower)),candidate=static_cast<unsigned>(parameter(MathParameter::RootAutomorphism))%n,exponent=level==3?candidate:k,divisor=gcd(exponent,n),cycle=n/gcd(k,n),selected=level==2?k*power%n:exponent*probe%n;
      const auto root=[&](unsigned index,float z){const double angle=2*pi*index/n;return Vec3{static_cast<float>(1.3*std::cos(angle)),static_cast<float>(1.3*std::sin(angle)),z};};
      const bool mapped=level==1||level==3;const float bottom=level?-.7F:0;unsigned primitiveCount=0,fixed=0;double rootResidual=0,polynomialResidual=0;const auto polynomial=cyclotomic(n);
      b.table(level==0?"Roots and multiplicative orders":level==2?"Powers of the subgroup generator":"Exponent action on roots",level==0?std::array<std::string_view,4>{"exponent","real","imaginary","order"}:std::array<std::string_view,4>{"exponent","image","primitive","order"},4);
      for(unsigned i=0;i<n;++i){const bool primitive=gcd(i,n)==1;primitiveCount+=primitive;fixed+=exponent*i%n==i;const Vec3 color=i==probe?gold:primitive?teal:muted;
        b.ball(root(i,bottom),.065F,color,"root_domain");b.rod(root(i,bottom),root((i+1)%n,bottom),muted,.012F,"root_circle");b.label(residueLabels[i],root(i,bottom)+Vec3{0,0,-.16F},color);
        if(mapped){b.ball(root(i,.7F),.052F,i==selected?gold:primitive?teal:muted,"root_codomain");b.rod(root(i,.7F),root((i+1)%n,.7F),muted,.012F,"root_circle");b.rod(root(i,bottom),root(exponent*i%n,.7F),i==probe?gold:blue,.011F,"root_power_map");}
        const double angle=2*pi*i/n;rootResidual=std::max(rootResidual,std::hypot(std::cos(n*angle)-1,std::sin(n*angle)));if(primitive){const auto eval=evaluateCyclotomic(polynomial,angle);polynomialResidual=std::max(polynomialResidual,std::hypot(eval[0],eval[1]));}
        if(level==0)b.row(residueLabels[i],{static_cast<double>(i),std::cos(angle),std::sin(angle),static_cast<double>(n/gcd(i,n))});
        else if(level!=2)b.row(residueLabels[i],{static_cast<double>(i),static_cast<double>(exponent*i%n),primitive?1.0:0.0,static_cast<double>(n/gcd(i,n))});
      }
      b.metric("Number of roots",n);b.metric("Probe exponent",probe);b.metric("Probe order",n/gcd(probe,n));b.metric("Primitive roots",primitiveCount);b.metric("Root equation residual",rootResidual);
      if(mapped){b.metric("Map exponent",exponent);b.metric("Distinct images",n/divisor);b.metric("Preimages per image",divisor);b.metric("Fixed roots",fixed);b.label(level==3&&divisor==1?"Cyclotomic field automorphism":"Complex power map",{-1.2F,-1.7F,0},blue);}
      if(level==2){for(unsigned step=0;step<=cycle;++step){const unsigned index=k*step%n;const float z=-.55F+1.3F*step/cycle;const Vec3 current=root(index,z);b.ball(current,.06F,step==power%cycle?gold:teal,"root_subgroup_step");if(step)b.rod(root(k*(step-1)%n,-.55F+1.3F*(step-1)/cycle),current,blue,.014F,"root_subgroup_path");b.row(countLabels[step],{static_cast<double>(step),static_cast<double>(index),gcd(index,n)==1?1.0:0.0,static_cast<double>(n/gcd(index,n))});}
        b.metric("Generator exponent",k);b.metric("Subgroup order",cycle);b.metric("Selected power",power);b.metric("Power result exponent",selected);b.label("Height records the power step",{-1.2F,-1.7F,0});}
      if(level==3){b.metric("Automorphism defined",divisor==1?1:0);b.metric("Automorphism order",rootUnitOrder(candidate,n));b.metric("Cyclotomic residual",polynomialResidual);if(divisor!=1)b.label("Nonunit: no field automorphism",{-1.2F,-1.7F,.4F},coral);
        auto& plot=b.plot("Phi_n along the unit circle");b.curve(plot,"Real part",teal,0,360,[&](double degrees){return evaluateCyclotomic(polynomial,degrees*pi/180)[0];});b.curve(plot,"Imaginary part",coral,0,360,[&](double degrees){return evaluateCyclotomic(polynomial,degrees*pi/180)[1];});
        auto& coefficients=b.plot("Exact integer coefficients of Phi_n");auto& series=coefficients.series[coefficients.seriesCount++];series.name="Coefficient of x^j";series.color=gold;series.stems=true;for(unsigned j=0;j<=polynomial.degree;++j)series.points[series.count++]={static_cast<double>(j),static_cast<double>(polynomial.coefficients[j])};
      }
      else {auto& plot=b.plot("Complex roots: real and imaginary coordinates");plot.equalAspect=true;auto& points=plot.series[plot.seriesCount++];points.name="Unit-circle roots";points.color=teal;for(unsigned j=0;j<=n;++j)points.points[points.count++]={std::cos(2*pi*j/n),std::sin(2*pi*j/n)};plot.hasMarker=true;const unsigned selectedPoint=level?selected:probe;plot.marker={std::cos(2*pi*selectedPoint/n),std::sin(2*pi*selectedPoint/n)};}
      break;
    }
    case MathObjectKind::Psd: {
      const unsigned level=snapshot_.level;
      const ConeMatrix input{parameter(MathParameter::PsdA),parameter(MathParameter::PsdB),parameter(MathParameter::PsdC)};
      ConeMatrix active=input,other{};double mixture=0,scale=1;
      const double slice=parameter(MathParameter::PsdSlice),costAngle=parameter(MathParameter::PsdCostAngle)*pi/180;
      const double costX=std::cos(costAngle),costY=std::sin(costAngle);
      if(level==2) {
        const double theta=parameter(MathParameter::PsdOtherAngle)*pi/180,u=std::cos(theta),v=std::sin(theta);
        other={2*u*u,2*u*v,2*v*v};mixture=parameter(MathParameter::PsdMix);scale=parameter(MathParameter::PsdRayScale);
        active={scale*((1-mixture)*input.a+mixture*other.a),scale*((1-mixture)*input.b+mixture*other.b),scale*((1-mixture)*input.c+mixture*other.c)};
      }
      if(level==3)active={slice*(1+costX),slice*costY,slice*(1-costX)};
      const auto e=coneSpectrum(active);const auto point=conePosition(active);
      const auto stateColor=e.psd?(e.rank<2?gold:teal):coral;
      for(float height:{.6F,1.5F,3.0F})b.scaled(MathShape::Ring,{0,0,height},{height,height,height},muted,"psd_cone_ring");
      for(unsigned i=0;i<8;++i){const double angle=2*pi*i/8;b.rod({},{static_cast<float>(3*std::cos(angle)),static_cast<float>(3*std::sin(angle)),3},muted,.012F,"psd_cone_ray");}
      b.arrow({-2.2F,0,0},{2.3F,0,0},blue,"psd_x_axis");b.arrow({0,-2.2F,0},{0,2.3F,0},violet,"psd_y_axis");b.arrow({0,0,-2.2F},{0,0,3.4F},white,"psd_t_axis");
      b.label("x=(a-c)/2",{2.3F,0,-.2F},blue);b.label("y=b",{0,2.4F,0},violet);b.label("t=(a+c)/2",{0,0,3.6F});
      b.ball({},.045F,white,"psd_zero");b.ball(point,.105F,stateColor,"psd_matrix_point");
      b.rod({},point,stateColor,.018F,"psd_selected_ray");
      b.label(e.psd?(e.rank==0?"Zero matrix":e.rank==1?"PSD boundary: rank one":"Positive definite"):e.hi< -e.tolerance?"Negative definite":e.lo< -e.tolerance&&e.hi>e.tolerance?"Indefinite":"Negative semidefinite",point+Vec3{.14F,0,.2F},stateColor);
      b.matrix2(level==2?"Mixture M":level==3?"Maximizing matrix A":"Symmetric matrix A",{active.a,active.b,active.b,active.c});
      b.metric("Trace",active.a+active.c);b.metric("Determinant",active.a*active.c-active.b*active.b);
      b.metric("Smallest eigenvalue",e.lo);b.metric("Largest eigenvalue",e.hi);b.metric("PSD",e.psd?1:0);b.metric("Rank",e.rank);
      b.table("Matrix coefficients and principal minors",{"a","b","c","determinant"},4);
      b.row(level==2?"M":level==3?"Optimum":"A",{active.a,active.b,active.c,active.a*active.c-active.b*active.b});
      if(level==0) {
        auto& plot=b.plot("Smallest eigenvalue as b changes",MathParameter::PsdB);
        b.curve(plot,"Smallest eigenvalue",teal,-2,2,[&](double v){return coneSpectrum({input.a,v,input.c}).lo;});
        b.curve(plot,"PSD threshold",muted,-2,2,[](double){return 0.0;});plot.hasMarker=true;plot.marker={input.b,e.lo};
      }
      if(level==1) {
        const Vec3 offset{4.6F,0,0};const double theta=.5*std::atan2(2*input.b,input.a-input.c),c=std::cos(theta),d=std::sin(theta);
        b.matrix2("Eigenvector columns: max, min",{c,-d,d,c});b.matrix2("Eigenvalues: max, min",{e.hi,0,0,e.lo});
        auto& patch=snapshot_.surface;patch.rows=patch.columns=MathSurfacePatch::kResolution;
        for(unsigned i=0;i<patch.rows;++i)for(unsigned j=0;j<patch.columns;++j) {
          const double u=-1.2+2.4*j/(patch.columns-1),v=-1.2+2.4*i/(patch.rows-1),q=quadratic2(input,u,v);
          const Vec3 pos=offset+Vec3{static_cast<float>(u),static_cast<float>(v),static_cast<float>(.3*q)};
          const Vec3 normal=normalized(Vec3{static_cast<float>(-.6*(input.a*u+input.b*v)),static_cast<float>(-.6*(input.b*u+input.c*v)),1});
          const float amount=static_cast<float>(std::clamp(std::fabs(q)/4,0.0,1.0));patch.vertices[i*patch.columns+j]={pos,normal,muted*(1-amount)+(q>=0?teal:coral)*amount};
        }
        const double angle=parameter(MathParameter::PsdProbeAngle)*pi/180,u=std::cos(angle),v=std::sin(angle),q=quadratic2(input,u,v);
        const Vec3 probe=offset+Vec3{static_cast<float>(u),static_cast<float>(v),static_cast<float>(.3*q)};
        b.ball(probe,.075F,gold,"psd_quadratic_probe");b.rod(offset+Vec3{static_cast<float>(u),static_cast<float>(v),0},probe,gold,.018F,"psd_quadratic_height");
        b.arrow(offset,offset+Vec3{static_cast<float>(c),static_cast<float>(d),0},blue,"psd_max_eigenvector");
        b.arrow(offset,offset+Vec3{static_cast<float>(-d),static_cast<float>(c),0},violet,"psd_min_eigenvector");
        b.label("Height = 0.3 q(u,v)",offset+Vec3{-1,-1.6F,0});b.metric("Probe quadratic value",q);
        auto& plot=b.plot("Quadratic value around the unit circle",MathParameter::PsdProbeAngle);
        b.curve(plot,"q(cos(theta),sin(theta))",teal,0,180,[&](double degrees){return quadratic2(input,std::cos(degrees*pi/180),std::sin(degrees*pi/180));});
        b.curve(plot,"Smallest eigenvalue",violet,0,180,[&](double){return e.lo;});b.curve(plot,"Largest eigenvalue",blue,0,180,[&](double){return e.hi;});plot.hasMarker=true;plot.marker={parameter(MathParameter::PsdProbeAngle),q};
      }
      if(level==2) {
        const auto a=conePosition(input),c=conePosition(other);const auto unscaled=a*static_cast<float>(1-mixture)+c*static_cast<float>(mixture);
        b.ball(a,.085F,blue,"psd_endpoint_a");b.ball(c,.085F,violet,"psd_endpoint_b");b.rod(a,c,blue,.02F,"psd_convex_segment");
        b.ball(unscaled,.065F,white,"psd_unscaled_mixture");b.label("A",a+Vec3{0,0,.2F},blue);b.label("B",c+Vec3{0,0,.2F},violet);
        b.matrix2("Endpoint A",{input.a,input.b,input.b,input.c});b.matrix2("Rank-one endpoint B",{other.a,other.b,other.b,other.c});
        const auto ae=coneSpectrum(input);b.metric("Endpoint A PSD",ae.psd?1:0);b.metric("Endpoint A rank",ae.rank);b.metric("Mixture fraction",mixture);b.metric("Ray scale",scale);
        b.row("A",{input.a,input.b,input.c,input.a*input.c-input.b*input.b});b.row("B",{other.a,other.b,other.c,other.a*other.c-other.b*other.b});
        if(!ae.psd)b.label("A is outside: convexity premise fails",{-2,-2.4F,-.4F},coral);
        auto& plot=b.plot("Smallest eigenvalue along the scaled mixture",MathParameter::PsdMix);
        b.curve(plot,"lambda_min(M)",teal,0,1,[&](double f){return coneSpectrum({scale*((1-f)*input.a+f*other.a),scale*((1-f)*input.b+f*other.b),scale*((1-f)*input.c+f*other.c)}).lo;});
        b.curve(plot,"PSD threshold",muted,0,1,[](double){return 0.0;});plot.hasMarker=true;plot.marker={mixture,e.lo};
      }
      if(level==3) {
        const float t=static_cast<float>(slice);const Vec3 center{0,0,t},direction{static_cast<float>(costX),static_cast<float>(costY),0},tangent{-direction.y,direction.x,0};
        const double fraction=parameter(MathParameter::PsdObjective),target=slice*fraction;
        b.planeFrame(center,{2.2F,0,0},{0,2.2F,0},blue,"psd_trace_plane");
        if(slice>0) {
          b.scaled(MathShape::Ring,center,{t,t,1},white,"psd_feasible_slice");
          for(unsigned i=0;i<8;++i){const double angle=2*pi*i/8;b.rod(center,center+Vec3{static_cast<float>(slice*std::cos(angle)),static_cast<float>(slice*std::sin(angle)),0},muted,.009F,"psd_feasible_disk");}
        } else b.ball({},.09F,white,"psd_singleton_slice");
        const Vec3 planeCenter=center+direction*static_cast<float>(target);
        b.planeFrame(planeCenter,tangent*2.2F,{0,0,.7F},violet,"psd_objective_plane");
        b.arrow(center,center+direction*.7F,gold,"psd_objective_direction");
        if(std::fabs(fraction)<=1&&slice>0) {
          const float half=static_cast<float>(slice*std::sqrt(std::max(0.0,1-fraction*fraction)));
          b.rod(planeCenter-tangent*half,planeCenter+tangent*half,violet,.024F,"psd_objective_chord");
        }
        b.metric("Maximum objective",slice);b.metric("Plane objective",target);b.metric("Objective gap",slice-target);b.metric("Plane intersects disk",slice==0||std::fabs(fraction)<=1?1:0);
        b.label("Fixed trace: disk in the cone",{-2,-2.4F,t},blue);b.label("Maximum",point+Vec3{.1F,0,-.25F},gold);
        auto& plot=b.plot("Best objective versus moving plane",MathParameter::PsdObjective);
        b.curve(plot,"Plane value",violet,-1.25,1.25,[&](double f){return slice*f;});b.curve(plot,"Maximum",gold,-1.25,1.25,[&](double){return slice;});plot.hasMarker=true;plot.marker={fraction,target};
      }
      break;
    }
    case MathObjectKind::Norm: {
      const unsigned level=snapshot_.level;const double p=parameter(MathParameter::NormP);const bool infinity=parameter(MathParameter::NormInfinity)!=0;
      const Triple x{parameter(MathParameter::NormX),parameter(MathParameter::NormY),parameter(MathParameter::NormZ)};
      const double value=normLength(x,p,infinity),maximum=normLength(x,1,true);const Vec3 origin{};
      if(level==1||parameter(MathParameter::NormWire)!=0)b.normWire(origin,p,infinity,teal,"norm_open_boundary");
      else if(infinity)b.scaled(MathShape::Box,origin,{2,2,2},teal,"norm_exact_cube");
      else {
        auto& patch=snapshot_.surface;patch.rows=patch.columns=MathSurfacePatch::kResolution;
        for(unsigned i=0;i<patch.rows;++i)for(unsigned j=0;j<patch.columns;++j) {
          auto direction=sphereDirection(pi*i/(patch.rows-1),2*pi*j/(patch.columns-1));for(auto& v:direction)if(std::fabs(v)<1e-14)v=0;
          const auto point=normBoundary(direction,p,false);Triple gradient{};
          for(unsigned axis=0;axis<3;++axis)gradient[axis]=((point[axis]>0)-(point[axis]<0))*std::pow(std::fabs(point[axis]),p-1);
          const Vec3 normal=normalized(tripleScene(gradient));const float shade=static_cast<float>(.2+.5*std::fabs(point[2]));
          patch.vertices[i*patch.columns+j]={tripleScene(point),normal,teal*(1-shade)+blue*shade};
        }
      }
      b.label(infinity?"Exact infinity-norm unit cube":p==1?"Unit octahedron":p==2?"Euclidean unit sphere":"Finite-p unit boundary",{-1.2F,-1.45F,0},teal);
      b.metric("Infinity norm selected",infinity?1:0);if(!infinity)b.metric("Exponent p",p);
      b.metric("Vector norm",value);b.metric("Max norm",maximum);b.metric("Normalization defined",value>0?1:0);
      const auto normalizedPoint=normBoundary(x,p,infinity);if(value>0)b.metric("Unit boundary error",std::fabs(normLength(normalizedPoint,p,infinity)-1));
      b.table(level==3?"Duality uses the vector controls as w":"One vector, several distance rules",{"1-norm","2-norm","selected norm","max norm"},4);
      const auto addNormRow=[&](std::string_view label,const Triple& v){b.row(label,{normLength(v,1),normLength(v,2),normLength(v,p,infinity),normLength(v,1,true)});};
      addNormRow(level==3?"w":"x",x);
      if(level<3) {
        b.arrow(origin,tripleScene(x),gold,"norm_vector_x");b.ball(tripleScene(x),.065F,gold,"norm_probe");b.label("x",tripleScene(x)+Vec3{.12F,0,.12F},gold);
        if(value>0)b.ball(tripleScene(normalizedPoint),.055F,blue,"norm_normalized_probe");
      }
      if(level==0) {
        auto& plot=b.plot("Unit boundary in the xy plane");plot.equalAspect=true;auto& line=plot.series[plot.seriesCount++];line.name="Norm one";line.color=teal;
        for(unsigned i=0;i<=128;++i){const double t=2*pi*i/128;const auto v=normBoundary({std::cos(t),std::sin(t),0},p,infinity);line.points[line.count++]={v[0],v[1]};}
      }
      if(level==1) {
        const Triple y{parameter(MathParameter::NormOtherX),parameter(MathParameter::NormOtherY),parameter(MathParameter::NormOtherZ)},sum{x[0]+y[0],x[1]+y[1],x[2]+y[2]};
        const double second=normLength(y,p,infinity),total=normLength(sum,p,infinity);
        b.arrow(tripleScene(x),tripleScene(sum),violet,"norm_translated_y");b.arrow(origin,tripleScene(sum),teal,"norm_sum");
        b.ball(tripleScene(sum),.075F,teal,"norm_sum_tip");b.rod(origin,tripleScene(y),muted,.012F,"norm_y_at_origin");b.rod(tripleScene(y),tripleScene(sum),muted,.012F,"norm_parallelogram");
        b.label("x+y",tripleScene(sum)+Vec3{.15F,0,.1F},teal);
        b.metric("Other vector norm",second);b.metric("Sum norm",total);b.metric("Triangle slack",value+second-total);addNormRow("y",y);addNormRow("x+y",sum);
        auto& plot=b.plot("Triangle inequality across finite p",infinity?MathParameter::Count:MathParameter::NormP);
        b.curve(plot,"Norm of sum",teal,1,32,[&](double e){return normLength(sum,e);});b.curve(plot,"Sum of norms",violet,1,32,[&](double e){return normLength(x,e)+normLength(y,e);});
        if(!infinity){plot.hasMarker=true;plot.marker={p,total};}
      }
      if(level==2) {
        b.wireBox({},1,white,"norm_limit_cube");b.metric("Upper bound",infinity?maximum:std::pow(3.0,1/p)*maximum);b.metric("Limit gap",value-maximum);
        auto& plot=b.plot("Fixed vector squeezed towards its max norm",infinity?MathParameter::Count:MathParameter::NormP);
        b.curve(plot,"Finite p-norm",teal,1,32,[&](double e){return normLength(x,e);});b.curve(plot,"Max norm",gold,1,32,[&](double){return maximum;});b.curve(plot,"3^(1/p) times max",violet,1,32,[&](double e){return std::pow(3.0,1/e)*maximum;});
        if(!infinity){plot.hasMarker=true;plot.marker={p,value};}
      }
      if(level==3) {
        const bool dualInfinity=!infinity&&p==1;const double dualP=infinity?1:dualInfinity?1:p/(p-1);
        const Vec3 dualCenter{3.5F,0,0};const double h=normLength(x,dualP,dualInfinity),fraction=parameter(MathParameter::NormSupport);
        b.normWire(dualCenter,dualP,dualInfinity,violet,"norm_dual_boundary");
        b.label(dualInfinity?"Dual: infinity-norm cube":dualP==1?"Dual: 1-norm octahedron":dualP==2?"Dual: Euclidean sphere":"Dual: q=p/(p-1)",dualCenter+Vec3{-1.2F,-1.45F,0},violet);
        b.metric("Support value",h);b.metric("Support defined",h>0?1:0);b.metric("Plane fraction",fraction);
        if(h>0) {
          const auto contact=normSupport(x,p,infinity);const Vec3 point=tripleScene(contact),w=tripleScene(x),normal=normalized(w);
          const auto u=normalized(cross(std::fabs(normal.z)>.9F?Vec3{0,1,0}:Vec3{0,0,1},normal)),v=cross(normal,u);
          const auto planeCenter=normal*static_cast<float>(h*fraction/normLength(x,2));
          b.planeFrame(planeCenter,u*1.45F,v*1.45F,gold,"norm_support_plane");b.arrow(point,point+normal*.65F,gold,"norm_support_normal");b.ball(point,.075F,gold,"norm_support_contact");
          Triple dualPoint=x;for(auto& coordinate:dualPoint)coordinate/=h;
          b.ball(dualCenter+tripleScene(dualPoint),.075F,gold,"norm_dual_contact");
          b.metric("Contact norm",normLength(contact,p,infinity));b.metric("Pairing w dot contact",tripleDot(x,contact));b.metric("Dual point norm",normLength(dualPoint,dualP,dualInfinity));
          b.label("Maximizing x",point+Vec3{.14F,0,.15F},gold);b.label("w / dual norm",dualCenter+tripleScene(dualPoint)+Vec3{0,0,.18F},gold);
          auto& plot=b.plot("Support value and movable plane",MathParameter::NormSupport);
          b.curve(plot,"Plane value",gold,0,1.4,[&](double f){return h*f;});b.curve(plot,"Dual norm: maximum",violet,0,1.4,[&](double){return h;});plot.hasMarker=true;plot.marker={fraction,h*fraction};
        } else b.label("w=0: no supporting direction",{0,-1.8F,0},coral);
      }
      break;
    }
    case MathObjectKind::Curve: {
      const unsigned level=snapshot_.level;
      CubicBezier curve;for(unsigned i=0;i<4;++i)for(unsigned j=0;j<3;++j)curve.controls[i][j]=parameter(curveCoordinates[i][j]);
      const auto arc=analyzeBezier(curve);const double progress=parameter(MathParameter::CurveProgress);
      const bool distance=level>=2&&parameter(MathParameter::CurveTravel)==1;
      const double t=distance?bezierParameterAtFraction(arc,progress):progress;
      const auto sample=sampleBezier(curve,t);
      const bool sweepAvailable=arc.regular&&arc.length()>0;
      const bool guides=level!=3||parameter(MathParameter::CurveGuides)==1||!sweepAvailable;
      const Vec3 left=level==2?Vec3{-2.6F,0,0}:Vec3{},right{2.6F,0,0};
      const auto position=[&](double u){return tripleScene(sampleBezier(curve,u).position);};
      snapshot_.curve.active=guides;snapshot_.curve.selected=static_cast<unsigned>(parameter(MathParameter::CurveControl));
      constexpr std::array<std::string_view,4> controlNames{"P0","P1","P2","P3"};
      for(unsigned i=0;i<4;++i) {
        const auto p=left+tripleScene(curve.controls[i]);snapshot_.curve.controls[i]=p;
        if(guides){const auto color=i==snapshot_.curve.selected?gold:muted;b.ball(p,.07F,color,"curve_control");b.label(controlNames[i],p+Vec3{0,0,.16F},color);}
        if(guides&&i>0)b.rod(left+tripleScene(curve.controls[i-1]),p,muted,.012F,"curve_control_polygon");
      }
      const auto drawCurve=[&](Vec3 offset,unsigned segments,Vec3 color){for(unsigned i=0;i<segments;++i)b.rod(offset+position(static_cast<double>(i)/segments),offset+position(static_cast<double>(i+1)/segments),color,.02F,"bezier_curve");};
      if(guides&&level!=2)drawCurve({},64,teal);
      if(guides&&level!=2)b.ball(position(t),.09F,gold,"curve_probe");
      b.metric("Parameter t",t);b.metric("Progress",progress);b.metric("Arc length estimate",arc.length());
      b.metric("Length bound width",arc.bounds.upper-arc.bounds.lower);b.metric("Speed",sample.speed);b.metric("Tangent defined",sample.regular?1:0);
      if(level==0) {
        const auto construction=constructBezier(curve,t);
        for(unsigned i=0;i<3;++i){b.ball(tripleScene(construction.first[i]),.06F,blue,"curve_first_interpolation");if(i)b.rod(tripleScene(construction.first[i-1]),tripleScene(construction.first[i]),blue,.014F,"curve_first_polygon");}
        for(unsigned i=0;i<2;++i)b.ball(tripleScene(construction.second[i]),.07F,violet,"curve_second_interpolation");
        b.rod(tripleScene(construction.second[0]),tripleScene(construction.second[1]),violet,.018F,"curve_second_polygon");
        for(const auto edge:{std::array<unsigned,2>{0,2},{0,3},{1,3}})b.rod(tripleScene(curve.controls[edge[0]]),tripleScene(curve.controls[edge[1]]),muted,.008F,"curve_control_hull");
        // Compute the certificate in double precision, independently of scene coordinates.
        Triple da{},db{},dc{};for(unsigned j=0;j<3;++j){da[j]=curve.controls[1][j]-curve.controls[0][j];db[j]=curve.controls[2][j]-curve.controls[0][j];dc[j]=curve.controls[3][j]-curve.controls[0][j];}
        const double determinant=da[0]*(db[1]*dc[2]-db[2]*dc[1])-da[1]*(db[0]*dc[2]-db[2]*dc[0])+da[2]*(db[0]*dc[1]-db[1]*dc[0]);
        const std::array<double,4> weights{(1-t)*(1-t)*(1-t),3*t*(1-t)*(1-t),3*t*t*(1-t),t*t*t};
        b.metric("Control tetrahedron volume",std::fabs(determinant)/6);b.metric("Weight sum",weights[0]+weights[1]+weights[2]+weights[3]);b.metric("Smallest weight",*std::min_element(weights.begin(),weights.end()));
        b.table("Controls and Bernstein weights",{"x","y","z","weight"},4);for(unsigned i=0;i<4;++i)b.row(controlNames[i],{curve.controls[i][0],curve.controls[i][1],curve.controls[i][2],weights[i]});
        auto& endpoints=b.plot("Endpoint influence",MathParameter::CurveProgress);
        b.curve(endpoints,"P0 weight",blue,0,1,[](double u){return (1-u)*(1-u)*(1-u);});b.curve(endpoints,"P3 weight",teal,0,1,[](double u){return u*u*u;});endpoints.hasMarker=true;endpoints.marker={t,weights[0]};
        auto& handles=b.plot("Handle influence",MathParameter::CurveProgress);
        b.curve(handles,"P1 weight",violet,0,1,[](double u){return 3*u*(1-u)*(1-u);});b.curve(handles,"P2 weight",coral,0,1,[](double u){return 3*u*u*(1-u);});handles.hasMarker=true;handles.marker={t,weights[1]};
      }
      if(level==1) {
        b.metric("Curvature defined",sample.regular?1:0);
        if(sample.regular) {
          b.metric("Curvature",sample.curvature);const auto tangent=tripleScene(sample.tangent);b.arrow(position(t),position(t)+tangent*.7F,gold,"curve_tangent");
          Triple bend{};const double along=tripleDot(sample.second,sample.tangent);for(unsigned j=0;j<3;++j)bend[j]=sample.second[j]-along*sample.tangent[j];
          if(normLength(bend,2)>1e-10){const auto normal=normalized(tripleScene(bend));b.arrow(position(t),position(t)+normal*.55F,coral,"curve_curvature_normal");}
        } else b.label("Zero speed: tangent and curvature undefined",position(t)+Vec3{0,-.45F,0},coral);
        b.table("Curve derivatives",{"x","y","z",{}},3);b.row("r(t)",{sample.position[0],sample.position[1],sample.position[2]});b.row("r'(t)",{sample.first[0],sample.first[1],sample.first[2]});b.row("r''(t)",{sample.second[0],sample.second[1],sample.second[2]});if(sample.regular)b.row("Unit tangent",{sample.tangent[0],sample.tangent[1],sample.tangent[2]});
        auto& derivative=b.plot("Velocity components",MathParameter::CurveProgress);for(unsigned j=0;j<3;++j)b.curve(derivative,std::array<std::string_view,3>{"dx/dt","dy/dt","dz/dt"}[j],std::array{coral,teal,blue}[j],0,1,[&](double u){return sampleBezier(curve,u).first[j];});derivative.hasMarker=true;derivative.marker={t,sample.first[0]};
        auto& speed=b.plot("Speed along the curve",MathParameter::CurveProgress);b.curve(speed,"Speed",gold,0,1,[&](double u){return sampleBezier(curve,u).speed;});speed.hasMarker=true;speed.marker={t,sample.speed};
      }
      if(level==2) {
        drawCurve(left,48,blue);drawCurve(right,48,teal);
        b.label("Equal parameter steps",left+Vec3{-1.6F,-2.35F,0},blue);b.label("Equal distance steps",right+Vec3{-1.6F,-2.35F,0},teal);
        std::array<double,9> parameters{},equalArc{},uniformArc{};
        for(unsigned i=0;i<=8;++i) {
          const double f=i/8.0;parameters[i]=bezierParameterAtFraction(arc,f);equalArc[i]=bezierArcAt(arc,parameters[i]);uniformArc[i]=bezierArcAt(arc,f);
          b.ball(left+position(f),.055F,blue,"curve_parameter_marker");b.ball(right+position(parameters[i]),.055F,teal,"curve_distance_marker");
        }
        b.ball(left+position(progress),.095F,distance?muted:gold,"curve_parameter_probe");b.ball(right+position(bezierParameterAtFraction(arc,progress)),.095F,distance?gold:muted,"curve_distance_probe");
        b.table("Eight intervals: distance along the curve",{"progress","distance t","parameter ds","distance ds"},4);
        double uniformMin=arc.length(),uniformMax=0,equalMin=arc.length(),equalMax=0;
        for(unsigned i=1;i<=8;++i){const double u=uniformArc[i]-uniformArc[i-1],e=equalArc[i]-equalArc[i-1];uniformMin=std::min(uniformMin,u);uniformMax=std::max(uniformMax,u);equalMin=std::min(equalMin,e);equalMax=std::max(equalMax,e);b.row(std::array<std::string_view,8>{"1","2","3","4","5","6","7","8"}[i-1],{i/8.0,parameters[i],u,e});}
        b.metric("Distance travel defined",arc.length()>0?1:0);b.metric("Distance travel",distance?1:0);
        b.metric("Parameter distance spread",arc.length()>0?8*(uniformMax-uniformMin)/arc.length():0);b.metric("Equal distance spread",arc.length()>0?8*(equalMax-equalMin)/arc.length():0);
        auto& travel=b.plot("Parameter selected by each travel rule",MathParameter::CurveProgress);
        b.curve(travel,"Parameter travel",blue,0,1,[](double f){return f;});if(arc.length()>0)b.curve(travel,"Distance travel",teal,0,1,[&](double f){return bezierParameterAtFraction(arc,f);});travel.hasMarker=true;travel.marker={progress,t};
        auto& lengths=b.plot("Distance in each of eight intervals");
        b.curve(lengths,"Equal parameter steps",blue,1,8,[&](double x){const auto i=static_cast<unsigned>(std::round(x));return uniformArc[i]-uniformArc[i-1];});b.curve(lengths,"Equal distance steps",teal,1,8,[&](double x){const auto i=static_cast<unsigned>(std::round(x));return equalArc[i]-equalArc[i-1];});
        if(arc.length()==0)b.label("Constant curve: distance travel undefined",{-.9F,1,0},coral);
      }
      if(level==3) {
        const unsigned profile=static_cast<unsigned>(parameter(MathParameter::CurveProfile));const double exponent=profile==2?parameter(MathParameter::CurveNormP):2;
        const double radius=parameter(MathParameter::CurveRadius),aspect=parameter(MathParameter::CurveAspect),endScale=parameter(MathParameter::CurveEndScale),twist=parameter(MathParameter::CurveTwist);
        b.metric("Sweep available",sweepAvailable?1:0);b.metric("Start radius",radius);b.metric("End radius",radius*endScale);b.metric("Twist degrees",twist);b.metric("Section aspect",aspect);
        const auto profileAt=[&](unsigned j){const double angle=2*pi*(j%16)/16;return normBoundary({std::cos(angle),std::sin(angle),0},exponent,profile==1);};
        if(sweepAvailable) {
          constexpr unsigned rings=21,columns=17;
          std::array<Vec3,rings> centers{},tangents{},normals{},binormals{};
          auto frame=seedBezierFrame(sampleBezier(curve,0).tangent);unsigned nextStep=1;
          auto& patch=snapshot_.surface;patch.rows=rings+4;patch.columns=columns;
          for(unsigned i=0;i<rings;++i) {
            const double s=static_cast<double>(i)/(rings-1),u=bezierParameterAtFraction(arc,s);
            // Fixed parameter anchors make the orientation independent of the
            // probe, profile size and playback. Transport through each anchor.
            while(nextStep<BezierArcTable::kKnots&&static_cast<double>(nextStep)/(BezierArcTable::kKnots-1)<u){frame=transportBezierFrame(frame,sampleBezier(curve,static_cast<double>(nextStep)/(BezierArcTable::kKnots-1)).tangent);++nextStep;}
            const auto point=sampleBezier(curve,u);frame=transportBezierFrame(frame,point.tangent);
            const double angle=twist*pi/180*s;Triple normal{},binormal{};
            for(unsigned k=0;k<3;++k){normal[k]=std::cos(angle)*frame.normal[k]+std::sin(angle)*frame.binormal[k];binormal[k]=-std::sin(angle)*frame.normal[k]+std::cos(angle)*frame.binormal[k];}
            centers[i]=tripleScene(point.position);tangents[i]=tripleScene(frame.tangent);normals[i]=tripleScene(normal);binormals[i]=tripleScene(binormal);
            const double size=radius*((1-s)+s*endScale);const Vec3 color=teal*static_cast<float>(1-.45*s)+blue*static_cast<float>(.45*s);
            for(unsigned j=0;j<columns;++j){const auto uv=profileAt(j);auto& vertex=patch.vertices[(i+2)*columns+j];vertex.position=centers[i]+normals[i]*static_cast<float>(size*uv[0])+binormals[i]*static_cast<float>(size*aspect*uv[1]);vertex.color=color;}
          }
          for(unsigned i=0;i<rings;++i)for(unsigned j=0;j<columns;++j) {
            const unsigned k=j%16;const auto& previous=patch.vertices[(i+2)*columns+(k+15)%16].position;const auto& next=patch.vertices[(i+2)*columns+(k+1)%16].position;
            const auto forward=patch.vertices[(std::min(i+1,rings-1)+2)*columns+k].position-patch.vertices[((i>0?i-1:0)+2)*columns+k].position;
            const auto uv=profileAt(k);const auto radial=normalized(normals[i]*static_cast<float>(uv[0])+binormals[i]*static_cast<float>(uv[1]/aspect));
            auto normal=cross(next-previous,forward);if(length(normal)<1e-7F)normal=radial;else normal=normalized(normal);if(dot(normal,radial)<0)normal=normal*-1;
            patch.vertices[(i+2)*columns+j].normal=normal;
          }
          // Duplicate the rim for a hard cap normal. The centre rows collapse
          // triangles deliberately, just as the sphere primitive's poles do.
          for(unsigned j=0;j<columns;++j){const auto start=patch.vertices[2*columns+j],end=patch.vertices[(rings+1)*columns+j];patch.vertices[j]={centers.front(),tangents.front()*-1,teal};patch.vertices[columns+j]={start.position,tangents.front()*-1,teal};patch.vertices[(rings+2)*columns+j]={end.position,tangents.back(),blue};patch.vertices[(rings+3)*columns+j]={centers.back(),tangents.back(),blue};}
        } else b.label("Sweep paused: move controls to remove a zero tangent",{-.9F,-1,0},coral);
        auto& section=b.plot("Cross-section: start and end");
        const auto outline=[&](std::string_view name,Vec3 color,double scale){auto& series=section.series[section.seriesCount++];series.name=name;series.color=color;series.count=65;for(unsigned j=0;j<series.count;++j){const double angle=2*pi*(j%64)/64;const auto uv=normBoundary({std::cos(angle),std::sin(angle),0},exponent,profile==1);series.points[j]={scale*uv[0],scale*aspect*uv[1]};}};
        outline("Start profile",teal,radius);outline("End profile (before twist)",gold,radius*endScale);
      }
      break;
    }
    case MathObjectKind::Lathe: {
      LatheInput input;
      for(unsigned i=0;i<7;++i)input.radii[i]=parameter(static_cast<MathParameter>(index(MathParameter::LatheR0)+i));
      for(unsigned i=1;i<6;++i)input.heights[i]=parameter(static_cast<MathParameter>(index(MathParameter::LatheH1)+i-1));
      input.height=parameter(MathParameter::LatheHeight);input.hollow=parameter(MathParameter::LatheHollow)==1;input.wall=parameter(MathParameter::LatheWall);input.floor=parameter(MathParameter::LatheFloor);
      const auto profile=prepareLathe(input);const unsigned level=snapshot_.level,n=static_cast<unsigned>(parameter(MathParameter::LatheSlices));
      const double probe=parameter(MathParameter::LatheProbe),turn=level==0?1:parameter(MathParameter::LatheTurn)/360;
      const auto measure=measureLathe(profile,turn);const unsigned selected=std::min(n-1,static_cast<unsigned>(probe*n));
      LatheDisplay display{turn,parameter(MathParameter::LatheCut)/100};
      if(level==3){display.bandFrom=static_cast<double>(selected)/n;display.bandTo=static_cast<double>(selected+1)/n;}
      const bool shells=parameter(MathParameter::LatheMethod)==1;
      const bool guides=level==0||level==2||parameter(MathParameter::LatheGuides)==1||measure.volume==0;
      if(level==1||level==3)buildLatheSurface(profile,display,snapshot_.solid);
      if(level==2)buildLatheElement(profile,display,n,shells,selected,snapshot_.solid);
      const auto at=[&](double u,double angle=0,bool inside=false){const auto p=sampleLathe(profile,u);const double r=inside?p.inner:p.radius;return Vec3{static_cast<float>(r*std::cos(angle)),static_cast<float>(input.height*(u-.5)),static_cast<float>(r*std::sin(angle))};};
      const auto wire=[&](double angle,unsigned steps,bool inside,Vec3 color){for(unsigned i=0;i<steps;++i){const double a=static_cast<double>(i)/steps,c=static_cast<double>(i+1)/steps;if(inside&&sampleLathe(profile,(a+c)/2).inner==0)continue;const double lo=inside?std::max(a,input.floor):a;if(lo>=c)continue;b.rod(at(lo,angle,inside),at(c,angle,inside),color,.013F,inside?"lathe_inner_profile":"lathe_outer_profile");}};
      snapshot_.curve.active=guides;snapshot_.curve.count=7;snapshot_.curve.selectionParameter=MathParameter::LatheControl;snapshot_.curve.selected=static_cast<unsigned>(parameter(MathParameter::LatheControl));
      constexpr std::array<std::string_view,7> names{"P0","P1","P2","P3","P4","P5","P6"};
      for(unsigned i=0;i<7;++i){const auto point=at(input.heights[i]);snapshot_.curve.controls[i]=point;if(guides){const auto color=i==snapshot_.curve.selected?gold:blue;b.ball(point,.06F,color,"lathe_control");b.label(names[i],point+Vec3{.12F,0,0},color);}}
      if(guides){b.rod({0,static_cast<float>(-input.height/2-.15),0},{0,static_cast<float>(input.height/2+.15),0},muted,.01F,"lathe_axis");wire(0,level==2?24:36,false,blue);}
      if(level==0){wire(pi,36,false,muted);if(input.hollow)wire(0,36,true,violet);}
      if(level==2){wire(pi,24,false,blue);if(input.hollow){wire(0,24,true,violet);wire(pi,24,true,violet);}}
      const auto point=sampleLathe(profile,probe);b.metric("Height",input.height);if(level<3)b.metric("Maximum radius",profile.maximumRadius);
      if(level==0){b.metric("Selected radius",input.radii[snapshot_.curve.selected]);b.metric("Selected height fraction",input.heights[snapshot_.curve.selected]);b.metric("Interior bulge",*std::max_element(input.radii.begin()+1,input.radii.end()-1)-std::max(input.radii.front(),input.radii.back()));b.metric("Full-turn material volume",measure.volume);b.metric("Cavity volume",measure.voidVolume);
        b.table("Profile points",{"height fraction","height","outer radius","inner radius"},4);for(unsigned i=0;i<7;++i)b.row(names[i],{input.heights[i],input.height*input.heights[i],input.radii[i],sampleLathe(profile,input.heights[i]).inner});
      }
      if(level==0||level==1){auto& graph=b.plot("Outer and inner radius versus height fraction",MathParameter::LatheProbe);b.curve(graph,"Outer radius",blue,0,1,[&](double u){return sampleLathe(profile,u).radius;});if(input.hollow)b.curve(graph,"Inner radius",violet,0,1,[&](double u){return sampleLathe(profile,u).inner;});graph.hasMarker=true;graph.marker={probe,point.radius};}
      if(level==1){const auto full=measureLathe(profile);b.metric("Revolution degrees",360*turn);b.metric("Material volume",measure.volume);b.metric("Full-turn volume",full.volume);b.metric("Cavity volume",measure.voidVolume);b.metric("Angular volume identity error",std::fabs(measure.volume-turn*full.volume));
        if(guides){const double theta=2*pi*turn;wire(theta,24,false,gold);for(unsigned i=0;i<16;++i)b.rod(at(probe,theta*i/16),at(probe,theta*(i+1)/16),teal,.014F,"lathe_rotation_path");b.rod({0,static_cast<float>(input.height*(probe-.5)),0},at(probe,theta),gold,.015F,"lathe_generating_radius");}
        auto& volume=b.plot("Volume grows with the revolution angle",MathParameter::LatheTurn);b.curve(volume,"Material volume",teal,0,360,[&](double degrees){return degrees/360*full.volume;});volume.hasMarker=true;volume.marker={360*turn,measure.volume};
      }
      if(level==2){const auto approximation=approximateLatheVolume(profile,n,shells,turn);b.metric("Reference volume",measure.volume);b.metric("Midpoint volume",approximation.volume);b.metric("Relative volume error",measure.volume>0?std::fabs(approximation.volume-measure.volume)/measure.volume:0);b.metric("Subdivisions",n);b.metric("Highlighted element",selected+1);b.metric("Element volume",approximation.elements[selected].volume);
        if(shells)b.metric("Highlighted shell intervals",latheShell(profile,approximation.elements[selected].center).count);
        b.table(shells?"All midpoint shells":"All midpoint disks / washers",{shells?"radius":"height",shells?"dr":"dh",shells?"total height":"section area","volume"},4);
        constexpr std::array<std::string_view,32> rows{"1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16","17","18","19","20","21","22","23","24","25","26","27","28","29","30","31","32"};
        for(unsigned i=0;i<n;++i){const auto& e=approximation.elements[i];b.row(rows[i],{e.center,e.step,e.section,e.volume});}
        auto& density=b.plot(shells?"Shell integrand versus radius fraction":"Washer integrand versus height fraction",MathParameter::LatheProbe);
        const auto integrand=[&](double u){if(shells){const double r=u*profile.maximumRadius;return turn*2*pi*r*latheShell(profile,r).height*profile.maximumRadius;}const auto q=sampleLathe(profile,u);return turn*pi*(q.radius*q.radius-q.inner*q.inner)*input.height;};
        b.curve(density,"Volume density",teal,0,1,integrand);density.hasMarker=true;density.marker={probe,integrand(probe)};
        auto& convergence=b.plot("Midpoint sums as subdivisions increase",MathParameter::LatheSlices);auto& estimates=convergence.series[convergence.seriesCount++];estimates.name="Midpoint sum";estimates.color=gold;estimates.count=29;for(unsigned i=4;i<=32;++i)estimates.points[i-4]={static_cast<double>(i),approximateLatheVolume(profile,i,shells,turn).volume};b.curve(convergence,"Reference",teal,4,32,[&](double){return measure.volume;});convergence.hasMarker=true;convergence.marker={static_cast<double>(n),approximation.volume};
        b.label(shells?"Highlighted shell: all its height intervals":"Highlighted disk / washer",{-.9F,static_cast<float>(-input.height/2-.3),0},gold);
      }
      if(level==3){const double estimate=approximateLatheArea(profile,n,turn);const bool regular=point.radius>1e-10;const auto normal=normalized(Vec3{1,static_cast<float>(-point.slope),0}),meridian=normalized(Vec3{static_cast<float>(point.slope),1,0});const double residual=std::fabs(dot(normal,meridian));
        b.metric("Outer lateral area",measure.outerArea);b.metric("Inner lateral area",measure.innerArea);b.metric("Base / rim / floor area",measure.closureArea);b.metric("Physical radial faces",measure.cutArea);b.metric("Boundary area reference",measure.area);b.metric("Frustum boundary area",estimate);b.metric("Relative area error",measure.area>0?std::fabs(estimate-measure.area)/measure.area:0);b.metric("Normal defined",regular?1:0);b.metric("Subdivisions",n);if(regular)b.metric("Normal orthogonality error",residual);
        if(guides&&regular){const auto origin=at(probe);b.arrow(origin,origin+normal*.45F,gold,"lathe_normal");b.arrow(origin,origin+meridian*.45F,coral,"lathe_meridian_tangent");b.arrow(origin,origin+Vec3{0,0,.45F},violet,"lathe_circle_tangent");}
        if(!regular)b.label("On the axis: parameter normal undefined",at(probe)+Vec3{.12F,0,0},coral);
        auto& density=b.plot("Lateral area per unit height fraction",MathParameter::LatheProbe);b.curve(density,"Outer",teal,0,1,[&](double u){const auto q=sampleLathe(profile,u);return turn*2*pi*input.height*q.radius*std::hypot(1,q.slope);});if(input.hollow)b.curve(density,"Inner",violet,0,1,[&](double u){const auto q=sampleLathe(profile,u);return turn*2*pi*input.height*q.inner*std::hypot(1,q.slope);});density.hasMarker=true;density.marker={probe,turn*2*pi*input.height*point.radius*std::hypot(1,point.slope)};
        auto& convergence=b.plot("Frustum bands approach the boundary area",MathParameter::LatheSlices);auto& estimates=convergence.series[convergence.seriesCount++];estimates.name="Frustum area";estimates.color=gold;estimates.count=5;constexpr std::array<unsigned,5> counts{4,8,16,24,32};for(unsigned i=0;i<counts.size();++i)estimates.points[i]={static_cast<double>(counts[i]),approximateLatheArea(profile,counts[i],turn)};b.curve(convergence,"Reference",teal,4,32,[&](double){return measure.area;});convergence.hasMarker=true;convergence.marker={static_cast<double>(n),estimate};
      }
      if(measure.volume==0&&level>0)b.label("No material volume at this setting",{-.8F,static_cast<float>(input.height/2+.3),0},coral);
      break;
    }
    case MathObjectKind::Boolean: {
      const auto v=[&](MathParameter p){return parameter(p);};BooleanInput input;
      input.a.shape=static_cast<SolidShape>(static_cast<unsigned>(v(MathParameter::BooleanShapeA)));input.a.size=v(MathParameter::BooleanSizeA);
      input.b.shape=static_cast<SolidShape>(static_cast<unsigned>(v(MathParameter::BooleanShapeB)));input.b.size=v(MathParameter::BooleanSizeB);input.b.center={v(MathParameter::BooleanX),v(MathParameter::BooleanY),v(MathParameter::BooleanZ)};input.b.yaw=v(MathParameter::BooleanYaw);input.b.pitch=v(MathParameter::BooleanPitch);
      input.operation=static_cast<SolidOperation>(static_cast<unsigned>(v(MathParameter::BooleanOperation)));input.blend=v(MathParameter::BooleanBlend);
      const auto solid=prepareBoolean(input);const unsigned level=snapshot_.level,n=static_cast<unsigned>(v(MathParameter::BooleanResolution));
      const SolidPoint probe{v(MathParameter::BooleanProbeX),v(MathParameter::BooleanProbeY),v(MathParameter::BooleanProbeZ)};
      const auto a=samplePrimitive(solid.a,probe),other=samplePrimitive(solid.b,probe),result=sampleBoolean(solid,probe);const bool boundary=std::fabs(a.value)<1e-9||std::fabs(other.value)<1e-9;
      const auto surface=probeBooleanSurface(solid,probe);const bool section=v(MathParameter::BooleanSection)==1;
      const auto mesh=buildBooleanSurface(solid,{n,section,probe[2]},snapshot_.solid);
      const bool guides=v(MathParameter::BooleanGuides)==1||!snapshot_.solid.indexCount;
      const auto outline=[&](const PreparedPrimitive& p,Vec3 color){
        const auto world=[&](SolidPoint q){SolidPoint point=p.input.center;for(unsigned j=0;j<3;++j)for(unsigned k=0;k<3;++k)point[j]+=p.input.size*p.axes[k][j]*q[k];return tripleScene(point);};
        const auto line=[&](SolidPoint x,SolidPoint y){b.rod(world(x),world(y),color,.009F,"boolean_input_outline");};
        switch(p.input.shape){
          case SolidShape::Box:{const auto corner=[](unsigned i){return SolidPoint{(i&1)?1.2:-1.2,(i&2)?1.1:-1.1,(i&4)?.7:-.7};};for(unsigned i=0;i<8;++i)for(unsigned bit:{1U,2U,4U})if(!(i&bit))line(corner(i),corner(i|bit));break;}
          case SolidShape::Sphere:for(unsigned axis=0;axis<3;++axis)for(unsigned j=0;j<16;++j){SolidPoint x{},y{};x[(axis+1)%3]=std::cos(2*pi*j/16);x[(axis+2)%3]=std::sin(2*pi*j/16);y[(axis+1)%3]=std::cos(2*pi*(j+1)/16);y[(axis+2)%3]=std::sin(2*pi*(j+1)/16);line(x,y);}break;
          case SolidShape::Cylinder:for(double z:{-3.0,3.0})for(unsigned j=0;j<16;++j)line({std::cos(2*pi*j/16),std::sin(2*pi*j/16),z},{std::cos(2*pi*(j+1)/16),std::sin(2*pi*(j+1)/16),z});for(unsigned j=0;j<4;++j)line({std::cos(pi*j/2),std::sin(pi*j/2),-3},{std::cos(pi*j/2),std::sin(pi*j/2),3});break;
          case SolidShape::Arch:for(double z:{-3.0,3.0}){for(unsigned j=0;j<16;++j)line({std::cos(pi*j/16),.2+std::sin(pi*j/16),z},{std::cos(pi*(j+1)/16),.2+std::sin(pi*(j+1)/16),z});line({-1,.2,z},{-1,-2,z});line({-1,-2,z},{1,-2,z});line({1,-2,z},{1,.2,z});}for(double x:{-1.0,1.0})for(double y:{-2.0,.2})line({x,y,-3},{x,y,3});break;
          case SolidShape::Count:break;
        }
      };
      if(guides){outline(solid.a,teal);outline(solid.b,coral);const auto point=tripleScene(probe);b.ball(point,.055F,gold,"boolean_probe");b.label("Probe",point+Vec3{.1F,.13F,0},gold);
        b.rod({static_cast<float>(solid.bounds.minimum[0]),point.y,point.z},{static_cast<float>(solid.bounds.maximum[0]),point.y,point.z},muted,.006F,"boolean_probe_line");
        if(level==2&&surface.found){const auto at=tripleScene(surface.position);b.ball(at,.05F,violet,"boolean_surface_probe");if(surface.sample.regular){const auto normal=normalized(tripleScene(surface.sample.gradient));b.arrow(at,at+normal*.4F,gold,"boolean_surface_normal");}else b.label("Normal undefined here",at+Vec3{0,.2F,0},coral);}
        if(level==3){SolidPoint lower{},upper{};for(unsigned j=0;j<3;++j){const double step=(solid.bounds.maximum[j]-solid.bounds.minimum[j])/n;const auto cell=static_cast<unsigned>(std::clamp(std::floor((probe[j]-solid.bounds.minimum[j])/step),0.0,static_cast<double>(n-1)));lower[j]=solid.bounds.minimum[j]+cell*step;upper[j]=lower[j]+step;}const auto corner=[&](unsigned i){return Vec3{static_cast<float>((i&1)?upper[0]:lower[0]),static_cast<float>((i&2)?upper[1]:lower[1]),static_cast<float>((i&4)?upper[2]:lower[2])};};for(unsigned i=0;i<8;++i)for(unsigned bit:{1U,2U,4U})if(!(i&bit))b.rod(corner(i),corner(i|bit),gold,.007F,"boolean_sample_cell");}
        if(parameterAvailable(MathParameter::BooleanFit)&&v(MathParameter::BooleanFit)==1){const double radius=input.b.size-v(MathParameter::BooleanClearance);const Vec3 center{static_cast<float>(solid.a.bounds.maximum[0]+radius+.45),static_cast<float>(input.b.center[1]),static_cast<float>(input.b.center[2])};b.ball(center,static_cast<float>(radius),blue,"boolean_matching_ball");b.label("Matching ball (preview)",center+Vec3{0,static_cast<float>(radius+.15),0},blue);}
      }
      if(!snapshot_.solid.indexCount)b.label("No sampled material in this view",{0,2.2F,0},gold);
      if(mesh.reduced)b.label("Mesh budget: resolution reduced",{0,2.5F,0},gold);
      b.metric("Field A",a.value);b.metric("Field B",other.value);b.metric("Result field",result.value);
      b.metric("Probe state (-1=in,0=edge,1=out)",std::fabs(result.value)<1e-9?0:result.value<0?-1:1);
      if(level<3){b.metric("Requested mesh cells",n);b.metric("Actual mesh cells",mesh.cells);}
      if(level==0){b.metric("Input boundary at probe",boundary);b.metric("Result gradient defined",result.regular);b.table("Probe classification: -1 interior, 0 boundary, 1 exterior",{"field","state",{},{}},2);const auto row=[&](std::string_view name,SolidSample q){b.row(name,{q.value,std::fabs(q.value)<1e-9?0.0:q.value<0?-1.0:1.0});};row("A",a);row("B",other);row("Result",result);}
      if(level==1){b.metric("Input boundary at probe",boundary);b.metric("Hard Boolean result",booleanTruth(input.operation,a.value<0,other.value<0));b.metric("Result interior",result.value<0);b.metric("Blend added material",input.operation==SolidOperation::SmoothUnion&&a.value>0&&other.value>0&&result.value<0);
        b.table(input.operation==SolidOperation::SmoothUnion?"Hard union truth table (blend baseline)":"Boolean truth table (interior points)",{"A","B","result","Probe match"},4);constexpr std::array<std::string_view,4> names{"Neither","B only","A only","Both"};for(unsigned i=0;i<4;++i){const bool ai=(i&2)!=0,bi=(i&1)!=0;b.row(names[i],{static_cast<double>(ai),static_cast<double>(bi),static_cast<double>(booleanTruth(input.operation,ai,bi)),static_cast<double>(!boundary&&ai==(a.value<0)&&bi==(other.value<0))});}}
      if(level==2){b.metric("Blend added material",input.operation==SolidOperation::SmoothUnion&&a.value>0&&other.value>0&&result.value<0);b.metric("Surface crossing found",surface.found);b.metric("Surface normal defined",surface.found&&surface.sample.regular);if(surface.found){b.metric("Surface crossing residual",std::fabs(surface.sample.value));b.metric("Surface gradient magnitude",length(tripleScene(surface.sample.gradient)));}
        b.table("Gradients at probe; last row at the surface crossing",{"dF/dx","dF/dy","dF/dz","defined"},4);const auto row=[&](std::string_view name,SolidSample q){b.row(name,{q.gradient[0],q.gradient[1],q.gradient[2],static_cast<double>(q.regular)});};row("A",a);row("B",other);row("Result",result);if(surface.found)row("Surface crossing",surface.sample);}
      if(level<3){auto& plot=b.plot("Defining fields along the probe's x line",MathParameter::BooleanProbeX);const double left=std::max(-3.0,solid.bounds.minimum[0]),right=std::min(3.0,solid.bounds.maximum[0]);b.curve(plot,"A",teal,left,right,[&](double x){return samplePrimitive(solid.a,{x,probe[1],probe[2]}).value;});b.curve(plot,"B",coral,left,right,[&](double x){return samplePrimitive(solid.b,{x,probe[1],probe[2]}).value;});b.curve(plot,"Result",gold,left,right,[&](double x){return sampleBoolean(solid,{x,probe[1],probe[2]}).value;});plot.hasMarker=true;plot.marker={probe[0],result.value};}
      if(level==3){constexpr std::array<unsigned,7> counts{12,16,20,24,28,48,64};std::array<SolidVolume,7> estimates{};for(unsigned i=0;i<counts.size();++i)estimates[i]=measureBooleanVolume(solid,counts[i]);const double fine=estimates.back().volume,change=std::fabs(fine-estimates[5].volume);const auto selected=measureBooleanVolume(solid,n);
        b.metric("Midpoint full volume",selected.volume);b.metric("64-cell volume estimate",fine);b.metric("48/64 relative change",fine>0?change/fine:change==0?0:1);b.metric("Cell volume",selected.cellVolume);b.metric("Requested mesh cells",n);b.metric("Actual mesh cells",mesh.cells);b.metric("Visible mesh volume",mesh.volume);b.metric("Mesh field residual",mesh.maximumResidual);
        b.table("Midpoint volume comparison (full solid)",{"cells/axis","occupied","volume","delta vs 64"},4);constexpr std::array<std::string_view,7> names{"12 cells","16 cells","20 cells","24 cells","28 cells","48 cells","64 cells"};for(unsigned i=0;i<counts.size();++i)b.row(names[i],{static_cast<double>(counts[i]),static_cast<double>(estimates[i].inside),estimates[i].volume,estimates[i].volume-fine});
        auto& plot=b.plot("Volume estimates as sampling increases");auto& series=plot.series[plot.seriesCount++];series.name="Midpoint volume";series.color=gold;series.count=counts.size();for(unsigned i=0;i<counts.size();++i)series.points[i]={static_cast<double>(counts[i]),estimates[i].volume};b.curve(plot,"64-cell comparison",teal,12,64,[&](double){return fine;});plot.hasMarker=true;plot.marker={static_cast<double>(n),selected.volume};
      }
      break;
    }
    case MathObjectKind::Patch: {
      BicubicPatch patch;
      for(unsigned i=0;i<16;++i)for(unsigned k=0;k<3;++k)patch.controls[i][k]=parameter(static_cast<MathParameter>(index(MathParameter::PatchP00X)+3*i+k));
      const unsigned level=snapshot_.level,selected=static_cast<unsigned>(parameter(MathParameter::PatchControl)),n=static_cast<unsigned>(parameter(MathParameter::PatchResolution));
      const double u=parameter(MathParameter::PatchU),v=parameter(MathParameter::PatchV);const auto probe=samplePatch(patch,u,v);
      constexpr std::array modes{PatchColour::Influence,PatchColour::Material,PatchColour::Gaussian,PatchColour::AreaDensity};
      buildPatchSurface(patch,{n,selected,modes[level]},snapshot_.solid);const auto mesh=measurePatchMesh(patch,n);
      const bool guides=parameter(MathParameter::PatchGuides)==1||!snapshot_.solid.indexCount;
      constexpr std::array<std::string_view,16> names{"P00","P01","P02","P03","P10","P11","P12","P13","P20","P21","P22","P23","P30","P31","P32","P33"};
      const auto at=[&](double a,double c){return tripleScene(samplePatch(patch,a,c).position);};
      const auto point=tripleScene(probe.position),normal=tripleScene(probe.normal);
      snapshot_.curve.active=guides;snapshot_.curve.count=16;snapshot_.curve.selected=selected;snapshot_.curve.selectionParameter=MathParameter::PatchControl;
      for(unsigned i=0;i<16;++i)snapshot_.curve.controls[i]=tripleScene(patch.controls[i]);
      if(guides) {
        for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j){const unsigned k=4*i+j;const auto p=tripleScene(patch.controls[k]);
          if(i<3)b.rod(p,tripleScene(patch.controls[k+4]),muted,.009F,"patch_control_u");
          if(j<3)b.rod(p,tripleScene(patch.controls[k+1]),muted,.009F,"patch_control_v");
          b.ball(p,k==selected?.065F:.04F,k==selected?gold:white,"patch_control_point");b.label(names[k],p+Vec3{0,.11F,0},k==selected?gold:white);
        }
        for(unsigned i=0;i<16;++i){const double a=static_cast<double>(i)/16,c=static_cast<double>(i+1)/16;
          b.rod(at(a,v),at(c,v),coral,.012F,"patch_u_section");b.rod(at(u,a),at(u,c),blue,.012F,"patch_v_section");
        }
        b.ball(point,.05F,gold,"patch_probe");b.label("S(u,v)",point+Vec3{0,.15F,0},gold);
        if(level>=1&&probe.regular) {
          const auto du=normalized(tripleScene(probe.du)),dv=normalized(tripleScene(probe.dv));
          b.arrow(point,point+du*.6F,coral,"patch_u_tangent");b.arrow(point,point+dv*.6F,blue,"patch_v_tangent");b.arrow(point,point+normal*.65F,gold,"patch_normal");
          b.label("u",point+du*.7F,coral);b.label("v",point+dv*.7F,blue);b.label("N",point+normal*.78F,gold);
          b.planeFrame(point,du*.32F,normalized(cross(normal,du))*.32F,white,"patch_tangent_plane");
        }
        if(level==3) {
          const unsigned i=std::min(n-1,static_cast<unsigned>(u*n)),j=std::min(n-1,static_cast<unsigned>(v*n));
          const double a=static_cast<double>(i)/n,c=static_cast<double>(i+1)/n,d=static_cast<double>(j)/n,e=static_cast<double>(j+1)/n;
          const std::array corners{at(a,d),at(c,d),at(c,e),at(a,e)};
          for(unsigned k=0;k<4;++k)b.rod(corners[k],corners[(k+1)%4],gold,.022F,"patch_area_cell");b.rod(corners[0],corners[2],gold,.012F,"patch_area_diagonal");
        }
      }
      if(!probe.regular)b.label("Normal / curvature undefined",point+Vec3{0,.32F,0},gold);
      const auto vectorRow=[&](std::string_view label,BezierPoint a){b.row(label,{a[0],a[1],a[2],std::hypot(a[0],a[1],a[2])});};
      if(level==0) {
        double weightSum=0;for(double weight:probe.weights)weightSum+=weight;
        b.metric("Selected basis weight",probe.weights[selected]);b.metric("Weight sum",weightSum);b.metric("Triangulated area",mesh.area,"units^2");
        b.table("Control positions and influence at the UV probe",{"x","y","z","weight"},4);
        for(unsigned i=0;i<16;++i)b.row(names[i],{patch.controls[i][0],patch.controls[i][1],patch.controls[i][2],probe.weights[i]});
        auto& plot=b.plot("Selected control influence along u",MathParameter::PatchU);b.curve(plot,"Bernstein weight",gold,0,1,[&](double t){return samplePatch(patch,t,v).weights[selected];});plot.hasMarker=true;plot.marker={u,probe.weights[selected]};
      }
      if(level==1) {
        b.metric("Area density",probe.jacobian);b.metric("Regular probe",probe.regular);
        if(probe.regular){b.metric("Normal y",probe.normal[1]);b.metric("Normal dot S_u",probe.normal[0]*probe.du[0]+probe.normal[1]*probe.du[1]+probe.normal[2]*probe.du[2]);b.metric("Normal dot S_v",probe.normal[0]*probe.dv[0]+probe.normal[1]*probe.dv[1]+probe.normal[2]*probe.dv[2]);b.metric("Tangent angle",std::acos(std::clamp(probe.F/std::sqrt(probe.E*probe.G),-1.0,1.0))*180/pi,"degrees");}
        b.table(probe.regular?"Partial derivatives and oriented normal":"Partial derivatives; normal is undefined",{"x","y","z","length"},4);
        vectorRow("S(u,v)",probe.position);vectorRow("S_u",probe.du);vectorRow("S_v",probe.dv);if(probe.regular)vectorRow("Unit normal",probe.normal);
        auto& plot=b.plot("Area density along u",MathParameter::PatchU);b.curve(plot,"|S_u cross S_v|",gold,0,1,[&](double t){return samplePatch(patch,t,v).jacobian;});plot.hasMarker=true;plot.marker={u,probe.jacobian};
      }
      if(level<=1) {
        auto& plot=b.plot("Surface height along v",MathParameter::PatchV);b.curve(plot,"World y at fixed u",blue,0,1,[&](double t){return samplePatch(patch,u,t).position[1];});plot.hasMarker=true;plot.marker={v,probe.position[1]};
      }
      if(level==2) {
        if(probe.regular){b.metric("Gaussian curvature",probe.gaussian,"1/units^2");b.metric("Mean curvature",probe.mean,"1/units");}
        b.metric("Regular probe",probe.regular);b.metric("Area density",probe.jacobian);
        if(probe.regular){b.metric("Principal curvature min",probe.principalMin,"1/units");b.metric("Principal curvature max",probe.principalMax,"1/units");}
        b.matrix2("First fundamental form",{probe.E,probe.F,probe.F,probe.G});
        if(probe.regular)b.matrix2("Second fundamental form",{probe.e,probe.f,probe.f,probe.g});
        b.table(probe.regular?"Surface derivatives at the probe":"Singular probe: curvature and normal unavailable",{"x","y","z","length"},4);
        vectorRow("S_u",probe.du);vectorRow("S_v",probe.dv);vectorRow("S_uu",probe.duu);vectorRow("S_uv",probe.duv);vectorRow("S_vv",probe.dvv);
        std::array<PatchSample,129> line{};bool regularLine=probe.regular;for(unsigned i=0;i<line.size();++i){line[i]=samplePatch(patch,static_cast<double>(i)/(line.size()-1),v);regularLine=regularLine&&line[i].regular;}
        b.metric("Curvature trace available",regularLine);
        if(regularLine)for(unsigned view=0;view<2;++view){auto& plot=b.plot(view?"Mean curvature along u":"Gaussian curvature along u",MathParameter::PatchU);auto& series=plot.series[plot.seriesCount++];series.name=view?"H":"K";series.color=view?teal:coral;series.count=line.size();
          for(unsigned i=0;i<line.size();++i)series.points[i]={static_cast<double>(i)/(line.size()-1),view?line[i].mean:line[i].gaussian};plot.hasMarker=probe.regular;plot.marker={u,view?probe.mean:probe.gaussian};
        }
      }
      if(level==3) {
        const double coarse=integratePatchArea(patch,8),fine=integratePatchArea(patch,16);
        const bool relative=fine>1e-12;const auto percentage=[&](double a){return relative?100*std::fabs(a-fine)/fine:std::fabs(a-fine);};
        b.metric("Triangulated area",mesh.area,"units^2");b.metric("Quadrature area",fine,"units^2");b.metric(relative?"Mesh difference":"Absolute mesh difference",percentage(mesh.area),relative?"%":"units^2");b.metric(relative?"Quadrature change":"Absolute quadrature change",percentage(coarse),relative?"%":"units^2");
        b.metric("Area density",probe.jacobian);b.metric("Mesh subdivisions",n);b.metric("Mesh triangles",snapshot_.solid.indexCount/3);b.metric("Skipped triangles",mesh.skipped);
        b.table("Area convergence; quadrature is an estimate",{"cells/axis","triangle area",relative?"vs quadrature %":"absolute difference","triangles"},4);
        auto& plot=b.plot("Area as mesh resolution increases",MathParameter::PatchResolution);auto& line=plot.series[plot.seriesCount++];line.name="Triangulated area";line.color=gold;line.count=8;
        constexpr std::array<std::string_view,8> labels{"4 cells","8 cells","12 cells","16 cells","20 cells","24 cells","28 cells","32 cells"};
        for(unsigned i=0;i<8;++i){const unsigned cells=4*(i+1);const auto estimate=measurePatchMesh(patch,cells);b.row(labels[i],{static_cast<double>(cells),estimate.area,percentage(estimate.area),static_cast<double>(estimate.triangles)});line.points[i]={static_cast<double>(cells),estimate.area};}
        b.curve(plot,"Gauss comparison",teal,4,32,[&](double){return fine;});plot.hasMarker=true;plot.marker={static_cast<double>(n),mesh.area};
      }
      break;
    }
    case MathObjectKind::Membrane: {
      MembraneInput input;input.width=parameter(MathParameter::MembraneWidth);input.depth=parameter(MathParameter::MembraneDepth);input.tension=parameter(MathParameter::MembraneTension);input.density=parameter(MathParameter::MembraneDensity);input.damping=parameter(MathParameter::MembraneDamping);
      for(unsigned i=0;i<4;++i){const unsigned first=index(MathParameter::MembraneM0)+4*i;input.modes[i]={static_cast<unsigned>(parameters_[first]),static_cast<unsigned>(parameters_[first+1]),parameters_[first+2],parameters_[first+3]};}
      const double time=parameter(MathParameter::MembraneTime),u=parameter(MathParameter::MembraneU),v=parameter(MathParameter::MembraneV);
      const unsigned level=snapshot_.level,selected=static_cast<unsigned>(parameter(MathParameter::MembraneSlot)),n=static_cast<unsigned>(parameter(MathParameter::MembraneResolution));
      const bool isolated=parameter(MathParameter::MembraneView)==1,guides=parameter(MathParameter::MembraneGuides)==1;
      const auto state=prepareMembrane(input,time);const auto probe=sampleMembrane(state,u,v);const auto& slot=state.slots[selected];
      const auto colour=level==1?MembraneColour::SelectedBasis:level==3?MembraneColour::Energy:MembraneColour::Displacement;
      buildMembraneSurface(state,{n,selected,isolated,colour},snapshot_.solid);
      auto shownInput=input;if(isolated)for(unsigned i=0;i<4;++i)if(i!=selected)shownInput.modes[i].displacement=shownInput.modes[i].velocity=0;
      const auto shown=prepareMembrane(shownInput,time);const auto shownProbe=sampleMembrane(shown,u,v);
      const auto at=[&](double a,double c){return Vec3{static_cast<float>(input.width*(a-.5)),static_cast<float>(sampleMembrane(shown,a,c).displacement),static_cast<float>(input.depth*(.5-c))};};
      const auto rest=[&](double a,double c){return Vec3{static_cast<float>(input.width*(a-.5)),0,static_cast<float>(input.depth*(.5-c))};};
      const auto point=at(u,v);
      if(guides){
        const std::array corners{rest(0,0),rest(1,0),rest(1,1),rest(0,1)};for(unsigned i=0;i<4;++i)b.rod(corners[i],corners[(i+1)%4],white,.025F,"membrane_fixed_edge");
        for(unsigned i=0;i<32;++i){const double a=i/32.,c=(i+1)/32.;b.rod(at(a,v),at(c,v),coral,.009F,"membrane_u_section");b.rod(at(u,a),at(u,c),blue,.009F,"membrane_v_section");}
        b.rod(rest(u,v),point,gold,.012F,"membrane_displacement");b.ball(point,.05F,gold,"membrane_probe");b.label(isolated?"Selected slot probe":"Combined probe",point+Vec3{0,.16F,0},gold);
        if(level==0)b.arrow(point,point+Vec3{0,static_cast<float>(.55*std::tanh(shownProbe.velocity)),0},teal,"membrane_velocity_direction");
        if(level==1){
          for(unsigned k=1;k<slot.m;++k)b.rod(rest(static_cast<double>(k)/slot.m,0)+Vec3{0,-.035F,0},rest(static_cast<double>(k)/slot.m,1)+Vec3{0,-.035F,0},gold,.017F,"membrane_slot_node_u");
          for(unsigned k=1;k<slot.n;++k)b.rod(rest(0,static_cast<double>(k)/slot.n)+Vec3{0,-.035F,0},rest(1,static_cast<double>(k)/slot.n)+Vec3{0,-.035F,0},gold,.017F,"membrane_slot_node_v");
          b.label("Selected slot nodes: rest-plane reference",rest(.5,1)+Vec3{0,-.14F,0},gold);
        }
      }
      constexpr std::array<std::string_view,4> names{"Slot 1","Slot 2","Slot 3","Slot 4"},pairs{"Pair 1","Pair 2","Pair 3","Pair 4"};
      if(level==0){
        b.metric("Combined displacement",probe.displacement);b.metric("Combined velocity",probe.velocity);b.metric("Time",time,"s");b.metric("Wave speed",state.waveSpeed);b.metric("Initial energy",state.initialEnergy);
        b.table("Slot contributions at the probe",{"m","n","height","velocity"},4);for(unsigned i=0;i<4;++i)b.row(names[i],{static_cast<double>(state.slots[i].m),static_cast<double>(state.slots[i].n),probe.contributions[i],probe.velocities[i]});
      }
      if(level==1){
        b.metric("Selected spatial weight",probe.weights[selected]);b.metric("Natural frequency",slot.omega/(2*pi),"Hz");b.metric("Internal nodal lines",slot.m+slot.n-2);b.metric("Selected excitation",std::hypot(slot.initialDisplacement,slot.initialVelocity));b.metric("Damping ratio",input.damping/slot.omega);
        b.table("Mode slots: undamped natural frequencies",{"m","n","natural Hz","q(t)"},4);for(unsigned i=0;i<4;++i){const auto& mode=state.slots[i];b.row(names[i],{static_cast<double>(mode.m),static_cast<double>(mode.n),mode.omega/(2*pi),mode.q});}
        auto& horizontal=b.plot("Selected spatial basis along u",MathParameter::MembraneU);b.curve(horizontal,"sin(m*pi*u) sin(n*pi*v)",coral,0,1,[&](double a){return sampleMembrane(state,a,v).weights[selected];});horizontal.hasMarker=true;horizontal.marker={u,probe.weights[selected]};
        auto& vertical=b.plot("Selected spatial basis along v",MathParameter::MembraneV);b.curve(vertical,"sin(m*pi*u) sin(n*pi*v)",blue,0,1,[&](double c){return sampleMembrane(state,u,c).weights[selected];});vertical.hasMarker=true;vertical.marker={v,probe.weights[selected]};
      }
      if(level==2){
        b.metric("Combined displacement",probe.displacement);b.metric("Cancellation magnitude",std::max(0.0,probe.absoluteContributions-std::fabs(probe.displacement)));b.metric("Distinct active modes",state.activeModes);b.metric("Absolute pair contributions",probe.absoluteContributions);b.metric("Time",time,"s");
        b.table("Slot contributions; equal pairs combine coherently",{"m","n","height","velocity"},4);for(unsigned i=0;i<4;++i)b.row(names[i],{static_cast<double>(state.slots[i].m),static_cast<double>(state.slots[i].n),probe.contributions[i],probe.velocities[i]});
      }
      if(level==3){
        const double total=state.kinetic+state.potential;
        b.metric("Kinetic energy",state.kinetic);b.metric("Strain energy",state.potential);b.metric("Total energy",total);b.metric("Initial energy",state.initialEnergy);
        if(state.initialEnergy>1e-12)b.metric("Energy retained",100*total/state.initialEnergy,"%");
        b.metric("Energy loss rate",state.lossRate);b.metric("Energy lost",std::max(0.0,state.initialEnergy-total));b.metric("Time",time,"s");
        b.table("Integrated energy by distinct mode pair",{"m","n","kinetic","strain"},4);for(unsigned i=0;i<state.combinedCount;++i){const auto& mode=state.combined[i];b.row(pairs[i],{static_cast<double>(mode.m),static_cast<double>(mode.n),mode.kinetic,mode.potential});}
      }
      b.metric("Displayed probe height",shownProbe.displacement);b.metric("Selected slot only",isolated);
      // At most four cycles of the fastest natural mode: energy has twice the
      // oscillation frequency and still receives at least 16 samples per cycle.
      const double window=std::min(12.0,8*pi/state.maxOmega),start=std::clamp(time-window/2,0.0,12-window),end=std::min(12.0,start+window);
      if(level!=1){
        auto& plot=b.plot(level==3?"Combined energy near the selected time":"Combined and component motion near the selected time",MathParameter::MembraneTime);
        if(level==3){
          b.curve(plot,"Kinetic",coral,start,end,[&](double t){return prepareMembrane(input,t).kinetic;});b.curve(plot,"Strain",blue,start,end,[&](double t){return prepareMembrane(input,t).potential;});b.curve(plot,"Total",gold,start,end,[&](double t){const auto s=prepareMembrane(input,t);return s.kinetic+s.potential;});plot.marker={time,state.kinetic+state.potential};
        }else{
          b.curve(plot,"Combined",gold,start,end,[&](double t){return sampleMembrane(prepareMembrane(input,t),u,v).displacement;});b.curve(plot,"Selected slot",coral,start,end,[&](double t){return sampleMembrane(prepareMembrane(input,t),u,v).contributions[selected];});b.curve(plot,"Other slots",blue,start,end,[&](double t){const auto p=sampleMembrane(prepareMembrane(input,t),u,v);return p.displacement-p.contributions[selected];});plot.marker={time,probe.displacement};
        }plot.hasMarker=true;
      }
      if(level==0||level==2){
        auto& plot=b.plot("Combined and component heights along u",MathParameter::MembraneU);
        b.curve(plot,"Combined",gold,0,1,[&](double a){return sampleMembrane(state,a,v).displacement;});b.curve(plot,"Selected slot",coral,0,1,[&](double a){return sampleMembrane(state,a,v).contributions[selected];});b.curve(plot,"Other slots",blue,0,1,[&](double a){const auto p=sampleMembrane(state,a,v);return p.displacement-p.contributions[selected];});plot.hasMarker=true;plot.marker={u,probe.displacement};
      }
      break;
    }
    case MathObjectKind::Rigid: {
      RigidInput input;input.shape=static_cast<RigidShape>(parameter(MathParameter::RigidShape));input.dimensions={parameter(MathParameter::RigidWidth),parameter(MathParameter::RigidHeight),parameter(MathParameter::RigidDepth)};input.mass=parameter(MathParameter::RigidMass);input.balance=parameter(MathParameter::RigidBalance);
      input.rotationDegrees={parameter(MathParameter::RigidRotX),parameter(MathParameter::RigidRotY),parameter(MathParameter::RigidRotZ)};input.omega={parameter(MathParameter::RigidSpinX),parameter(MathParameter::RigidSpinY),parameter(MathParameter::RigidSpinZ)};
      rigidMotion_.configure(input);const auto& body=rigidMotion_.body();const double time=parameter(MathParameter::RigidTime);const auto state=rigidMotion_.at(time),initial=rigidMotion_.at(0);
      const unsigned level=snapshot_.level,axis=static_cast<unsigned>(parameter(MathParameter::RigidAxis));const bool guides=parameter(MathParameter::RigidGuides)==1;
      const float radius=static_cast<float>(body.radius),axisLength=radius*1.16F;
      const auto place=[&](const RigidVector& p){return tripleScene(rigidRotate(state.orientation,p));};
      constexpr std::array<Vec3,3> axisColors{coral,teal,blue};
      constexpr std::array<std::array<Vec3,3>,4> materials{{{teal,coral,white},{teal,coral,muted},{teal,coral,Vec3{.9F,.86F,.7F}},{blue,teal,gold}}};
      for(unsigned i=0;i<body.count;++i){const auto& p=body.parts[i];auto d=p.dimensions;const bool cylinder=p.primitive==RigidPrimitive::Cylinder;if(cylinder){d[0]*=.5;d[2]*=.5;}
        b.part(cylinder?MathShape::Disk:MathShape::Box,place(p.center),place({d[0],0,0}),place({0,d[1],0}),place({0,0,d[2]}),materials[static_cast<unsigned>(input.shape)][p.material],"rigid_mass_component");
      }
      // Thin surface markings identify orientation but do not enter the mass model.
      if(input.shape==RigidShape::Flywheel){const double y=input.dimensions[1]/2+.004;b.rod(place({0,y,0}),place({input.dimensions[0]*.46,y,0}),gold,.012F,"rigid_surface_mark");}
      if(input.shape==RigidShape::Satellite)for(unsigned part=1;part<=2;++part){const auto& p=body.parts[part];for(unsigned j=1;j<4;++j){const double z=p.dimensions[2]*(j/4.-.5),y=p.center[1]+p.dimensions[1]/2+.003;b.rod(place({p.center[0]-.46*p.dimensions[0],y,z}),place({p.center[0]+.46*p.dimensions[0],y,z}),white,.006F,"rigid_panel_mark");}}
      RigidVector tracked{};tracked[axis]=1;const auto direction=rigidRotate(state.orientation,tracked),releaseDirection=rigidRotate(initial.orientation,tracked);
      const double momentum=rigidMagnitude(initial.worldMomentum);
      const auto alignment=[&](const RigidState& s){return momentum>0?std::clamp(tripleDot(rigidRotate(s.orientation,tracked),initial.worldMomentum)/momentum,-1.,1.):0.;};
      const double releaseAlignment=tripleDot(direction,releaseDirection);
      if(guides){
        constexpr std::array<std::string_view,3> names{"Body X","Body Y","Body Z"},worldNames{"World X","World Y","World Z"};
        for(unsigned i=0;i<3;++i){RigidVector e{};e[i]=axisLength;const auto end=place(e);b.arrow({},end,axisColors[i],"rigid_body_axis");b.label(names[i],end*1.1F,axisColors[i]);const auto fixed=tripleScene(e)*1.1F;b.rod({},fixed,muted,.009F,"rigid_world_axis");b.label(worldNames[i],fixed*1.1F,muted);}
        const auto tip=tripleScene(direction)*axisLength;b.ball(tip,.045F,gold,"rigid_tracked_tip");b.ball({},.04F,gold,"rigid_center_of_mass");
        if(level==1){const auto origin=place({-body.center[0],-body.center[1],-body.center[2]});b.ball(origin,.032F,muted,"rigid_assembly_origin");b.rod({},origin,gold,.014F,"rigid_com_offset");for(unsigned i=0;i<body.count;++i)b.ball(place(body.parts[i].center),static_cast<float>(.035+.045*std::cbrt(body.parts[i].mass/body.mass)),gold,"rigid_component_com");}
        if(level>=2){const auto arrow=[&](const RigidVector& v,Vec3 color,std::string_view role){const double magnitude=rigidMagnitude(v);if(magnitude>0)b.arrow({},tripleScene(v)*static_cast<float>(1.38*radius/magnitude),color,role);};arrow(state.worldMomentum,gold,"rigid_world_momentum");arrow(state.worldOmega,teal,"rigid_world_velocity");}
        const double history=std::min(time,4*pi/std::max(rigidMotion_.speedBound(),1e-12)),start=time-history;
        auto previous=tripleScene(rigidRotate(rigidMotion_.at(start).orientation,tracked))*axisLength;
        for(unsigned i=1;i<=64;++i){const auto p=tripleScene(rigidRotate(rigidMotion_.at(start+history*i/64).orientation,tracked))*axisLength;b.rod(previous,p,gold,.009F,"rigid_axis_trail");previous=p;}
      }
      b.metric("Time",time,"s");b.metric("Angular speed",rigidMagnitude(state.bodyOmega),"rad/s");
      if(level==0){
        double orthogonalError=0;for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){double dot=0;for(unsigned k=0;k<3;++k)dot+=state.rotation[3*k+i]*state.rotation[3*k+j];orthogonalError=std::max(orthogonalError,std::fabs(dot-(i==j)));}
        b.metric("Release-axis alignment",releaseAlignment);b.metric("Orthogonality error",orthogonalError);b.metric("Rotation determinant",determinant(state.rotation));
        b.matrix("R: body to world",state.rotation);b.table("Unit quaternion: body to world",{"w","x","y","z"},4);b.row("q(t)",state.orientation);b.row("q(0)",initial.orientation);
      }
      if(level==1){
        b.metric("Total mass",body.mass,"kg");b.metric("COM offset",rigidMagnitude(body.center),"m");b.metric("Inertia X",body.inertia[0],"kg m^2");b.metric("Inertia Y",body.inertia[1],"kg m^2");b.metric("Inertia Z",body.inertia[2],"kg m^2");
        b.matrix("I_body about COM",{body.inertia[0],0,0,0,body.inertia[1],0,0,0,body.inertia[2]});b.matrix("I_world = R I_body R^T",state.worldInertia);
        b.table("Component mass and position relative to COM",{"mass kg","x m","y m","z m"},4);constexpr std::array<std::string_view,5> names{"Part 1","Part 2","Part 3","Part 4","Part 5"};for(unsigned i=0;i<body.count;++i){const auto& p=body.parts[i];b.row(names[i],{p.mass,p.center[0],p.center[1],p.center[2]});}b.row("Original COM offset",{body.mass,body.center[0],body.center[1],body.center[2]});
      }
      if(level==2){
        const double omega=rigidMagnitude(state.bodyOmega),l=rigidMagnitude(state.bodyMomentum),angle=omega*l>0?std::acos(std::clamp(tripleDot(state.bodyOmega,state.bodyMomentum)/(omega*l),-1.,1.))*180/pi:0;
        b.metric("Velocity-momentum angle",angle,"deg");b.metric("Momentum magnitude",l,"kg m^2/s");b.metric("Rotational energy",state.energy,"J");b.metric("Relative momentum error",state.momentumError);
        b.table("Angular vectors: arrows show direction, table retains magnitude",{"x","y","z","magnitude"},4);
        const auto row=[&](std::string_view name,const RigidVector& v){b.row(name,{v[0],v[1],v[2],rigidMagnitude(v)});};row("omega body (rad/s)",state.bodyOmega);row("omega world (rad/s)",state.worldOmega);row("L body (kg m^2/s)",state.bodyMomentum);row("L world (kg m^2/s)",state.worldMomentum);
      }
      if(level==3){
        b.metric("Rotational energy",state.energy,"J");b.metric("Momentum magnitude",rigidMagnitude(state.worldMomentum),"kg m^2/s");b.metric("Relative energy error",state.energyError);b.metric("Relative momentum error",state.momentumError);
        b.metric("Distinct principal moments",body.distinctMoments);b.metric("Tracked intermediate axis",body.distinctMoments&&body.order[1]==axis);b.metric("Initial momentum alignment",alignment(initial));b.metric("Momentum alignment",alignment(state));
        b.table(body.distinctMoments?"Principal moments: min/max stable, intermediate unstable":"Repeated moments: no unique intermediate axis",{"I (kg m^2)","omega0","omega(t)","L body"},4);
        constexpr std::array<std::string_view,3> names{"Body X","Body Y","Body Z"};for(unsigned rank=0;rank<3;++rank){const auto i=body.order[rank];b.row(names[i],{body.inertia[i],input.omega[i],state.bodyOmega[i],state.bodyMomentum[i]});}
      }
      if(level!=1){
        // Fixed sample count, at most four rotations under the energy-based speed
        // bound. Random access shares checkpoints with the displayed state.
        const double window=std::min(12.,8*pi/std::max(rigidMotion_.speedBound(),1e-12)),start=std::clamp(time-window/2,0.,12-window);
        std::array<RigidState,RigidMotion::samples> trace;for(unsigned i=0;i<trace.size();++i)trace[i]=rigidMotion_.at(start+window*i/(trace.size()-1));
        const auto line=[&](MathPlot& p,std::string_view name,Vec3 color,auto value){auto& c=p.series[p.seriesCount++];c={};c.name=name;c.color=color;c.count=trace.size();for(unsigned i=0;i<trace.size();++i)c.points[i]={trace[i].time,value(trace[i])};};
        auto& plot=b.plot(level==0?"Tracked axis in world coordinates":level==2?"Angular velocity in body coordinates":"Tracked axis alignment with initial world momentum",MathParameter::RigidTime);
        if(level==3){line(plot,momentum>0?"Axis dot L0/|L0|":"At rest: alignment undefined",gold,alignment);plot.hasMarker=true;plot.marker={time,alignment(state)};
          auto& errors=b.plot("Relative conservation errors (zero at exact rest)",MathParameter::RigidTime);line(errors,"Energy (signed)",coral,[](const RigidState& s){return s.energyError;});line(errors,"World momentum vector",blue,[](const RigidState& s){return s.momentumError;});
        }else{constexpr std::array<std::string_view,3> names{"X","Y","Z"};for(unsigned i=0;i<3;++i)line(plot,names[i],axisColors[i],[&](const RigidState& s){return level==0?rigidRotate(s.orientation,tracked)[i]:s.bodyOmega[i];});}
      }
      break;
    }
    case MathObjectKind::Count:throw std::logic_error("invalid math object state");
  }
}
} // namespace paths
