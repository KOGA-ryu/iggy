// M-LAB s1 anti-drift: the movement tuning cockpit must cover EVERY float feel field. This test is
// the mechanism that FAILS when someone adds a float to ProductGameplayMovementTuning without either a
// cockpit descriptor or a named exclusion -- so coverage can't silently rot.

#include "app/iggy3d/gameplay/MovementTuning.hpp"

#include <cstddef>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

// The number of float fields, derived from the struct layout (the static_assert beside the struct
// compile-enforces that the layout is padding-free, so this arithmetic is exact).
constexpr std::size_t floatFieldCount() {
  return (sizeof(iggy3d::ProductGameplayMovementTuning) -
          3U * sizeof(std::string_view)) /
         sizeof(float);
}

bool everyFloatFieldIsCoveredOrExcluded() {
  const std::size_t descriptors = iggy3d::kProductGameplayMovementTuningFields.size();
  const std::size_t excluded = iggy3d::kExcludedMovementTuningFields.size();
  return expect(descriptors + excluded == floatFieldCount(),
                "descriptors + named exclusions account for EVERY float field");
}

bool descriptorMemberPointersAreDistinct() {
  const auto& fields = iggy3d::kProductGameplayMovementTuningFields;
  bool ok = true;
  for (std::size_t i = 0; i < fields.size(); ++i) {
    for (std::size_t j = i + 1; j < fields.size(); ++j) {
      // Two descriptors pointing at the same struct member = a duplicated / mis-wired field.
      ok = ok && expect(fields[i].value != fields[j].value,
                        "each descriptor targets a DISTINCT tuning member");
    }
  }
  return ok;
}

bool excludedFieldsAreNotAlsoDescribed() {
  const auto& fields = iggy3d::kProductGameplayMovementTuningFields;
  bool ok = true;
  for (float iggy3d::ProductGameplayMovementTuning::* excluded :
       iggy3d::kExcludedMovementTuningFields) {
    for (const iggy3d::ProductGameplayMovementTuningFieldDescriptor& descriptor : fields) {
      ok = ok && expect(descriptor.value != excluded,
                        "a named-excluded field has no cockpit descriptor");
    }
  }
  return ok;
}

}  // namespace

int main() {
  const bool ok = everyFloatFieldIsCoveredOrExcluded() &&
                  descriptorMemberPointersAreDistinct() &&
                  excludedFieldsAreNotAlsoDescribed();
  return ok ? 0 : 1;
}
