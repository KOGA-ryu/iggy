#pragma once

#include <array>
#include <string_view>

namespace paths {
using SimplexPoint = std::array<double,3>;
enum class SimplexFunction : unsigned { Mean, Entropy, NegativeEntropy, Variance, Count };

// Three named outcomes, including equal or unordered values. Probabilities are
// finite, nonnegative, and sum to one within 1e-12. No heap use; O(3) per query.
SimplexPoint simplexPoint(double a,double b);
bool isSimplexPoint(const SimplexPoint&);
SimplexPoint mixSimplex(const SimplexPoint& p,const SimplexPoint& q,double t);
double simplexFunction(SimplexFunction,const SimplexPoint&,const SimplexPoint& outcomes);
// Natural logarithms, with 0*log(0)=0. Returns +infinity iff some p_i>0
// and q_i=0. Infinite results must never be sent to finite scene/plot buffers.
double simplexKl(const SimplexPoint& p,const SimplexPoint& q);
// Negative-entropy supporting plane restricted to sum(p)=1. Requires q>0.
double simplexEntropyTangent(const SimplexPoint& p,const SimplexPoint& q);
std::string_view simplexFunctionName(SimplexFunction);
std::string_view simplexCurvature(SimplexFunction);
}
