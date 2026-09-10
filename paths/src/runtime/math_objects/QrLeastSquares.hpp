#pragma once
#include <array>

namespace paths {
using QrVector=std::array<double,3>;
using QrColumns=std::array<QrVector,2>;
using QrCoefficients=std::array<double,2>;
struct QrAnalysis {
  std::array<unsigned,2> order{0,1}; // A Pi = Q R; pivot order, ties retain input order.
  QrColumns q{}; // Active orthonormal columns only; unused columns are zero.
  std::array<double,4> r{}; // Row-major 2x2 upper triangular R.
  QrVector removed{},remainder{},projected{},residual{};
  QrCoefficients solution{}; // Minimum coefficient norm for the numerical-rank model.
  std::array<QrCoefficients,2> nullBasis{}; // First (2-rank) columns are active.
  unsigned rank=0;
  double tolerance=0,reconstructionError=0,orthogonalityError=0;
  double residualSquared=0,normalResidual=0;
};
struct QrTrial {QrVector fitted{},residual{};double squaredError=0,normalResidual=0;};
// Fixed real 3x2 model. Entries of A and b in [-4,4]. max(abs(A)) is zero
// or >=1e-100. Column pivoting and twice-orthogonalized Gram-Schmidt use
// relative rank threshold 1e-10 of the leading pivot norm. No normal equations.
// Dependent columns are truncated explicitly; reconstruction error measures
// relative Frobenius change. The rank-one solve minimizes coefficient norm,
// and the zero-matrix solution is zero. Fixed storage and bounded work.
QrAnalysis analyzeQr(const QrColumns&,const QrVector&);
// Exact input A used for trial residuals, even when the analysis truncates rank.
// Finite trial coefficients have magnitude <=1e120 (keeps squared error finite).
QrTrial evaluateQrTrial(const QrColumns&,const QrVector&,const QrCoefficients&);
}
