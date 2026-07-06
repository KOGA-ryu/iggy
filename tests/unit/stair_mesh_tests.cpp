#include "core/geom/StairMesh.hpp"

#include "core/math/Aabb3.hpp"
#include "core/math/Vec3.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
using iggy3d::generateStairs;
using iggy3d::StairMesh;
using iggy3d::StairSpec;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float a, float b, float tol = 1e-4F) { return std::fabs(a - b) <= tol; }

StairSpec spec(float width, float rise, float run, std::uint32_t steps) {
  StairSpec s;
  s.width = width;
  s.totalRise = rise;
  s.totalRun = run;
  s.stepCount = steps;
  return s;
}

bool generatesRequestedSteps() {
  const StairMesh m = generateStairs(spec(2.0F, 4.0F, 8.0F, 4U));
  return expect(m.ok, "valid spec accepted") &&
         expect(m.reasonCode == "stair_ok", "ok reason code") &&
         expect(m.steps.size() == 4U, "four step boxes") &&
         expect(m.stepCount == 4U, "receipt step count") &&
         expect(near(m.stepRise, 1.0F), "step rise = 4/4") &&
         expect(near(m.stepRun, 2.0F), "step run = 8/4");
}

bool stepsRiseAndTileWithoutGaps() {
  const StairMesh m = generateStairs(spec(2.0F, 4.0F, 8.0F, 4U));
  bool ok = true;
  for (std::size_t i = 0; i < m.steps.size(); ++i) {
    // Every step is solid from the floor.
    ok = ok && expect(near(m.steps[i].min.y, 0.0F), "step floors at y=0");
    // Tread height rises one stepRise per step.
    ok = ok && expect(near(m.steps[i].max.y, static_cast<float>(i + 1) * m.stepRise),
                      "tread height rises");
    // Run slices are contiguous: this step's far edge meets the next step's near edge.
    if (i + 1 < m.steps.size()) {
      ok = ok && expect(near(m.steps[i].max.z, m.steps[i + 1].min.z),
                        "run slices tile without gap or overlap");
    }
  }
  return ok;
}

bool spansFullDimensions() {
  const StairMesh m = generateStairs(spec(2.0F, 4.0F, 8.0F, 4U));
  const iggy3d::Aabb3 first = m.steps.front();
  const iggy3d::Aabb3 last = m.steps.back();
  return expect(near(first.min.x, 0.0F) && near(first.min.y, 0.0F) &&
                    near(first.min.z, 0.0F),
                "first step starts at the origin") &&
         expect(near(last.max.x, 2.0F), "width spans X") &&
         expect(near(last.max.y, 4.0F), "top tread reaches totalRise") &&
         expect(near(last.max.z, 8.0F), "last step reaches totalRun");
}

bool singleStepIsOneBox() {
  const StairMesh m = generateStairs(spec(1.5F, 0.25F, 0.3F, 1U));
  return expect(m.ok && m.steps.size() == 1U, "one step -> one box") &&
         expect(near(m.steps[0].max.y, 0.25F) && near(m.steps[0].max.z, 0.3F),
                "single box spans the whole spec");
}

bool invalidDimensionsRejected() {
  const StairMesh zeroWidth = generateStairs(spec(0.0F, 4.0F, 8.0F, 4U));
  const StairMesh negRise = generateStairs(spec(2.0F, -1.0F, 8.0F, 4U));
  const float nan = std::numeric_limits<float>::quiet_NaN();
  const StairMesh nanRun = generateStairs(spec(2.0F, 4.0F, nan, 4U));
  return expect(!zeroWidth.ok && zeroWidth.steps.empty(), "zero width rejected") &&
         expect(!negRise.ok, "negative rise rejected") &&
         expect(!nanRun.ok, "non-finite run rejected") &&
         expect(zeroWidth.reasonCode == "stair_invalid_dimensions", "dimension reason code");
}

bool invalidStepCountRejected() {
  const StairMesh zero = generateStairs(spec(2.0F, 4.0F, 8.0F, 0U));
  const StairMesh tooMany = generateStairs(spec(2.0F, 4.0F, 8.0F, iggy3d::kMaxStairSteps + 1U));
  return expect(!zero.ok && zero.steps.empty(), "zero steps rejected") &&
         expect(!tooMany.ok, "over-cap step count rejected") &&
         expect(zero.reasonCode == "stair_invalid_step_count", "step-count reason code");
}

bool capIsInclusive() {
  const StairMesh atCap = generateStairs(spec(2.0F, 4.0F, 8.0F, iggy3d::kMaxStairSteps));
  return expect(atCap.ok && atCap.steps.size() == iggy3d::kMaxStairSteps,
                "exactly kMaxStairSteps is allowed");
}

}  // namespace

int main() {
  const bool ok = generatesRequestedSteps() && stepsRiseAndTileWithoutGaps() &&
                  spansFullDimensions() && singleStepIsOneBox() &&
                  invalidDimensionsRejected() && invalidStepCountRejected() &&
                  capIsInclusive();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
