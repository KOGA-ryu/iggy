#pragma once
#include <array>

namespace paths {
using PolarMatrix=std::array<double,9>; // Row major; column vectors.
inline constexpr unsigned polarMaxSteps=32;
struct PolarAnalysis {
  PolarMatrix w{},p{},alternateW{};
  std::array<double,3> singular{}; // Descending; values below rank tolerance become zero.
  unsigned rank=0;
  double rankTolerance=0,detA=0,detW=0,reconstructionError=0,orthogonalityError=0;
  bool iterationAvailable=false;
  std::array<PolarMatrix,polarMaxSteps+1> iterates{};
  std::array<std::array<double,3>,polarMaxSteps+1> iterationSingular{};
  std::array<double,polarMaxSteps+1> iterationError{}; // Frobenius ||X_k^T X_k-I||.
};
// Finite entries in [-4,4]. One-sided 3x3 Jacobi SVD, at most 32 sweeps,
// with rank tolerance 1e-12*sigma_max and deterministic null-space completion.
// Reconstruction error is relative Frobenius error (zero for A=0).
// W is orthogonal, possibly a reflection; P is symmetric PSD. At numerical
// rank loss alternateW reverses one null direction, with the same product WP.
// Higham's unscaled inverse-transpose iteration is evaluated in the SVD basis
// using d_next=(d+1/d)/2. Available only at rank 3 and sigma_min >= 1e-8;
// no inverse or fabricated trace is produced otherwise. Fixed storage, no heap.
PolarAnalysis analyzePolar(const PolarMatrix&);
}
