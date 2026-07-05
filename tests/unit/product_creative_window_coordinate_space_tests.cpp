#include "app/iggy3d/creative/bridge/WindowCoordinateSpace.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool highDpiLogicalSizeChoosesVirtualSpaceAndCarriesDrawable() {
  const iggy3d::ProductCreativeWindowCoordinateSpace space =
      iggy3d::resolveProductCreativeWindowCoordinateSpace(
          iggy3d::ProductCreativeWindowCoordinateSpaceRequest{
              1280,
              720,
              2560,
              1440,
              800,
              600,
              1280,
              720});

  return expect(space.virtualWidth == 1280U, "logical virtual width") &&
         expect(space.virtualHeight == 720U, "logical virtual height") &&
         expect(space.drawableWidth == 2560U, "drawable width carried") &&
         expect(space.drawableHeight == 1440U, "drawable height carried") &&
         expect(space.usedLogical, "logical flag") &&
         expect(!space.usedFallback, "fallback flag false") &&
         expect(!space.usedGuard, "guard flag false") &&
         expect(space.status == "creative_coordinate_space_logical",
                "logical status");
}

bool missingLogicalChoosesFallback() {
  const iggy3d::ProductCreativeWindowCoordinateSpace space =
      iggy3d::resolveProductCreativeWindowCoordinateSpace(
          iggy3d::ProductCreativeWindowCoordinateSpaceRequest{
              0,
              0,
              2560,
              1440,
              1366,
              768,
              1280,
              720});

  return expect(space.virtualWidth == 1366U, "fallback virtual width") &&
         expect(space.virtualHeight == 768U, "fallback virtual height") &&
         expect(space.drawableWidth == 2560U, "fallback drawable width") &&
         expect(space.drawableHeight == 1440U, "fallback drawable height") &&
         expect(!space.usedLogical, "logical flag false") &&
         expect(space.usedFallback, "fallback flag") &&
         expect(!space.usedGuard, "guard flag false") &&
         expect(space.reasonCode == "creative_coordinate_space_fallback",
                "fallback reason");
}

bool missingLogicalAndFallbackChoosesGuard() {
  const iggy3d::ProductCreativeWindowCoordinateSpace space =
      iggy3d::resolveProductCreativeWindowCoordinateSpace(
          iggy3d::ProductCreativeWindowCoordinateSpaceRequest{
              0,
              0,
              2560,
              1440,
              0,
              0,
              1280,
              720});

  return expect(space.virtualWidth == 1280U, "guard virtual width") &&
         expect(space.virtualHeight == 720U, "guard virtual height") &&
         expect(space.drawableWidth == 2560U, "guard drawable width") &&
         expect(space.drawableHeight == 1440U, "guard drawable height") &&
         expect(!space.usedLogical, "guard logical flag false") &&
         expect(!space.usedFallback, "guard fallback flag false") &&
         expect(space.usedGuard, "guard flag") &&
         expect(space.status == "creative_coordinate_space_guard",
                "guard status");
}

}  // namespace

int main() {
  const bool ok = highDpiLogicalSizeChoosesVirtualSpaceAndCarriesDrawable() &&
                  missingLogicalChoosesFallback() &&
                  missingLogicalAndFallbackChoosesGuard();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
