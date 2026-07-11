#include "app/iggy3d/creative/input/UiInput.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float actual, float expected) {
  return std::fabs(actual - expected) <= 1.0e-5F;
}

bool wheelProfilesOwnDirectionAndMagnitude() {
  cr::CreativeWheelProfile reversed;
  reversed.polarity = cr::CreativeWheelPolarity::Reversed;
  cr::CreativeWheelProfile unit = reversed;
  unit.stepMode = cr::CreativeWheelStepMode::Unit;
  cr::CreativeWheelProfile invalid;
  invalid.polarity = cr::CreativeWheelPolarity::Count;

  return expect(cr::quantizeCreativeWheelSteps(2.6F) == 3,
                "natural wheel preserves rounded positive magnitude") &&
         expect(cr::quantizeCreativeWheelSteps(-2.6F) == -3,
                "natural wheel preserves rounded negative magnitude") &&
         expect(cr::quantizeCreativeWheelSteps(2.6F, reversed) == -3 &&
                    cr::quantizeCreativeWheelSteps(-2.6F, reversed) == 3,
                "reversed profile owns navigation polarity") &&
         expect(cr::quantizeCreativeWheelSteps(2.6F, unit) == -1,
                "unit profile collapses wheel magnitude") &&
         expect(cr::quantizeCreativeWheelSteps(1.0e-5F) == 0 &&
                    cr::quantizeCreativeWheelSteps(
                        std::numeric_limits<float>::quiet_NaN()) == 0 &&
                    cr::quantizeCreativeWheelSteps(1.0F, invalid) == 0,
                "inactive and invalid wheel samples resolve to zero") &&
         expect(cr::quantizeCreativeWheelSteps(
                    std::numeric_limits<float>::max()) ==
                    std::numeric_limits<std::int32_t>::max(),
                "wheel magnitude saturates before integer conversion");
}

bool pointerMappingOwnsDrawableAndCenteredCoordinates() {
  const cr::CreativeDrawablePointer pointer =
      cr::resolveCreativeDrawablePointer(
          {25.0F, 10.0F, 100U, 50U, 200U, 100U, true, true});
  const cr::CreativeDrawablePointer fallback =
      cr::resolveCreativeDrawablePointer(
          {25.0F, 10.0F, 0U, 0U, 200U, 100U, false, false});
  const cr::CreativeDrawablePointer invalid =
      cr::resolveCreativeDrawablePointer(
          {std::numeric_limits<float>::infinity(), 0.0F,
           100U, 100U, 200U, 200U, true, true});

  return expect(pointer.valid && pointer.moved && pointer.primaryPressed,
                "pointer flags survive canonical mapping") &&
         expect(near(pointer.x, 50.0F) && near(pointer.y, 20.0F),
                "logical pointer scales to drawable coordinates") &&
         expect(near(pointer.centeredX, -50.0F) &&
                    near(pointer.centeredY, 30.0F),
                "centered pointer is right-positive and up-positive") &&
         expect(fallback.valid && near(fallback.x, 25.0F) &&
                    near(fallback.y, 10.0F),
                "missing logical extent preserves prior unit-scale fallback") &&
         expect(!invalid.valid && invalid.moved && invalid.primaryPressed,
                "invalid coordinates retain event flags but reject position");
}

bool wrappedIndexHandlesEdgesAndExtremeSteps() {
  const cr::CreativeWrappedIndexResult backward =
      cr::stepCreativeWrappedIndex(0U, 8U, -1);
  const cr::CreativeWrappedIndexResult forward =
      cr::stepCreativeWrappedIndex(7U, 8U, 1);
  const cr::CreativeWrappedIndexResult normalized =
      cr::stepCreativeWrappedIndex(10U, 8U, 0);
  const cr::CreativeWrappedIndexResult minimum =
      cr::stepCreativeWrappedIndex(
          3U, 5U, std::numeric_limits<std::int64_t>::min());

  return expect(backward.valid && backward.index == 7U && backward.changed,
                "negative step wraps backward") &&
         expect(forward.valid && forward.index == 0U && forward.changed,
                "positive step wraps forward") &&
         expect(normalized.valid && normalized.index == 2U &&
                    normalized.changed,
                "out-of-range current index normalizes deterministically") &&
         expect(minimum.valid && minimum.index == 0U,
                "minimum signed step avoids overflow") &&
         expect(!cr::stepCreativeWrappedIndex(0U, 0U, 1).valid,
                "empty index domain is invalid");
}

bool radialSectorsRunClockwiseFromUp() {
  constexpr std::array directions{
      std::array{0.0F, 1.0F},   std::array{1.0F, 1.0F},
      std::array{1.0F, 0.0F},   std::array{1.0F, -1.0F},
      std::array{0.0F, -1.0F},  std::array{-1.0F, -1.0F},
      std::array{-1.0F, 0.0F},  std::array{-1.0F, 1.0F},
  };
  bool ok = true;
  for (std::size_t index = 0U; index < directions.size(); ++index) {
    const cr::CreativeRadialSectorResult sector =
        cr::resolveCreativeRadialSector(directions[index][0],
                                        directions[index][1], 8U);
    ok = expect(sector.valid && sector.index == index,
                "radial cardinal/diagonal sector ordering") &&
         ok;
  }

  constexpr float kDegreesToRadians = 0.01745329251994329577F;
  const auto sectorAtDegrees = [](float degrees) {
    const float radians = degrees * kDegreesToRadians;
    return cr::resolveCreativeRadialSector(std::sin(radians),
                                           std::cos(radians), 8U);
  };
  return expect(sectorAtDegrees(22.0F).index == 0U &&
                    sectorAtDegrees(23.0F).index == 1U,
                "radial half-sector rounding is deterministic") &&
         expect(!cr::resolveCreativeRadialSector(0.1F, 0.1F, 8U, 0.2F)
                     .valid,
                "radial deadzone rejects short direction") &&
         expect(!cr::resolveCreativeRadialSector(
                     1.0F, 0.0F, cr::kCreativeRadialSectorCapacity + 1U)
                     .valid,
                "radial sector count is bounded") &&
         ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok = wheelProfilesOwnDirectionAndMagnitude() && ok;
  ok = pointerMappingOwnsDrawableAndCenteredCoordinates() && ok;
  ok = wrappedIndexHandlesEdgesAndExtremeSteps() && ok;
  ok = radialSectorsRunClockwiseFromUp() && ok;
  return ok ? 0 : 1;
}
