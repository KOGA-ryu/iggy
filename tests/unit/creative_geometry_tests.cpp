#include "app/iggy3d/creative/Geometry.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>
#include <type_traits>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool vectorValidationAndExactEqualityAreExplicit() {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double infinity = std::numeric_limits<double>::infinity();

  return expect(cr::isFiniteCreativeVec3({1.0, -2.0, 3.0}),
                "finite vector is accepted") &&
         expect(!cr::isFiniteCreativeVec3({nan, 0.0, 0.0}) &&
                    !cr::isFiniteCreativeVec3({0.0, infinity, 0.0}),
                "NaN and infinity are rejected") &&
         expect(cr::creativeVec3ExactlyEqual({-0.0, 2.0, 3.0},
                                             {0.0, 2.0, 3.0}),
                "exact equality follows numeric signed-zero equality") &&
         expect(!cr::creativeVec3ExactlyEqual({1.0, 2.0, 3.0},
                                              {1.0, 2.0, 3.0000001}),
                "exact equality does not introduce tolerance") &&
         expect(cr::isPositiveCreativeVec3({1.0, 2.0, 3.0}) &&
                    !cr::isPositiveCreativeVec3({1.0, 0.0, 3.0}) &&
                    !cr::isPositiveCreativeVec3({1.0, infinity, 3.0}),
                "positive-vector policy requires finite positive axes");
}

bool boundsMetricsOwnValidationAndStableArithmetic() {
  const cr::CreativeBoundsMetrics ordinary =
      cr::measureCreativeBounds({{-4.0, 2.0, -8.0}, {6.0, 6.0, 2.0}});
  const cr::CreativeBoundsMetrics zero =
      cr::measureCreativeBounds({{3.0, 3.0, 3.0}, {3.0, 3.0, 3.0}});
  const cr::CreativeBoundsMetrics reversed =
      cr::measureCreativeBounds({{1.0, 0.0, 0.0}, {-1.0, 2.0, 3.0}});
  const cr::CreativeBoundsMetrics nonFinite = cr::measureCreativeBounds(
      {{0.0, 0.0, 0.0},
       {std::numeric_limits<double>::infinity(), 1.0, 1.0}});
  const double maximum = std::numeric_limits<double>::max();
  const cr::CreativeBoundsMetrics overflow =
      cr::measureCreativeBounds({{-maximum, 0.0, 0.0},
                                 {maximum, 1.0, 1.0}});

  return expect(ordinary.valid &&
                    ordinary.status == cr::CreativeGeometryStatus::Valid &&
                    cr::creativeVec3ExactlyEqual(ordinary.size,
                                                 {10.0, 4.0, 10.0}) &&
                    cr::creativeVec3ExactlyEqual(ordinary.center,
                                                 {1.0, 4.0, -3.0}),
                "ordinary bounds produce stable size and center") &&
         expect(zero.valid &&
                    cr::creativeVec3ExactlyEqual(zero.size, {}),
                "zero-sized bounds remain valid geometry") &&
         expect(!reversed.valid &&
                    reversed.status ==
                        cr::CreativeGeometryStatus::ReversedBounds,
                "reversed bounds are distinguished") &&
         expect(!nonFinite.valid &&
                    nonFinite.status ==
                        cr::CreativeGeometryStatus::NonFiniteBounds,
                "non-finite bounds are distinguished") &&
         expect(!overflow.valid &&
                    overflow.status ==
                        cr::CreativeGeometryStatus::ArithmeticOverflow,
                "finite bounds arithmetic overflow fails closed") &&
         expect(cr::creativeBoundsExactlyEqual(
                    {{-0.0, 1.0, 2.0}, {3.0, 4.0, 5.0}},
                    {{0.0, 1.0, 2.0}, {3.0, 4.0, 5.0}}),
                "bounds exact equality preserves signed-zero semantics");
}

bool coreConversionReportsEveryNarrowingFailure() {
  const double maximumFloat =
      static_cast<double>(std::numeric_limits<float>::max());
  const cr::CreativeCoreVec3Conversion boundary =
      cr::creativeVec3ToCoreChecked({maximumFloat, -maximumFloat, -0.0});
  const cr::CreativeCoreVec3Conversion overflow =
      cr::creativeVec3ToCoreChecked(
          {std::nextafter(maximumFloat,
                          std::numeric_limits<double>::infinity()),
           0.0, 0.0});
  const cr::CreativeCoreVec3Conversion invalid =
      cr::creativeVec3ToCoreChecked(
          {0.0, std::numeric_limits<double>::quiet_NaN(), 0.0});
  const cr::CreativeVec3 widened =
      cr::creativeVec3FromCore({1.25F, -2.5F, 4.0F});

  return expect(boundary.converted &&
                    boundary.status == cr::CreativeGeometryStatus::Valid &&
                    boundary.value.x == std::numeric_limits<float>::max() &&
                    boundary.value.y == -std::numeric_limits<float>::max() &&
                    std::signbit(boundary.value.z),
                "float boundary and negative zero convert exactly") &&
         expect(!overflow.converted &&
                    overflow.status ==
                        cr::CreativeGeometryStatus::OutsideCoreFloatRange,
                "out-of-range double does not silently narrow") &&
         expect(!invalid.converted &&
                    invalid.status ==
                        cr::CreativeGeometryStatus::NonFiniteVector,
                "non-finite double conversion fails distinctly") &&
         expect(cr::creativeVec3ExactlyEqual(widened,
                                             {1.25, -2.5, 4.0}),
                "core values widen without arithmetic drift");
}

}  // namespace

static_assert(std::is_trivially_copyable_v<cr::CreativeBoundsMetrics>);
static_assert(std::is_trivially_copyable_v<cr::CreativeCoreVec3Conversion>);

int main() {
  bool ok = true;
  ok = vectorValidationAndExactEqualityAreExplicit() && ok;
  ok = boundsMetricsOwnValidationAndStableArithmetic() && ok;
  ok = coreConversionReportsEveryNarrowingFailure() && ok;
  return ok ? 0 : 1;
}
