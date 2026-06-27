#include "app/iggy3d/ProductInteractionMode.hpp"

#include <array>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::ProductControllerModeChordSample fullChord() {
  return {
      true,
      true,
      true,
      true,
  };
}

bool namesAreStable() {
  struct ModeNameCase {
    iggy3d::ProductInteractionMode mode;
    std::string_view name;
  };
  constexpr std::array modeCases{
      ModeNameCase{iggy3d::ProductInteractionMode::Player, "player"},
      ModeNameCase{iggy3d::ProductInteractionMode::Creative, "creative"},
  };

  struct SurfaceNameCase {
    iggy3d::ProductInputSurface surface;
    std::string_view name;
  };
  constexpr std::array surfaceCases{
      SurfaceNameCase{iggy3d::ProductInputSurface::None, "none"},
      SurfaceNameCase{iggy3d::ProductInputSurface::Starter, "starter"},
      SurfaceNameCase{iggy3d::ProductInputSurface::Pause, "pause"},
      SurfaceNameCase{iggy3d::ProductInputSurface::Gameplay, "gameplay"},
      SurfaceNameCase{iggy3d::ProductInputSurface::RoomEditor, "room_editor"},
      SurfaceNameCase{iggy3d::ProductInputSurface::Settings, "settings"},
      SurfaceNameCase{iggy3d::ProductInputSurface::DevTools, "dev_tools"},
      SurfaceNameCase{iggy3d::ProductInputSurface::SaveBrowser, "save_browser"},
      SurfaceNameCase{iggy3d::ProductInputSurface::WorldSetup, "world_setup"},
  };

  bool ok = true;
  for (const ModeNameCase row : modeCases) {
    ok = expect(iggy3d::productInteractionModeName(row.mode) == row.name,
                "interaction mode name is stable") &&
         ok;
  }
  for (const SurfaceNameCase row : surfaceCases) {
    ok = expect(iggy3d::productInputSurfaceName(row.surface) == row.name,
                "input surface name is stable") &&
         ok;
  }
  return ok;
}

bool fullChordTogglesOnGameplaySurfaces() {
  const iggy3d::ProductInteractionModeToggleResult playerToCreative =
      iggy3d::applyProductInteractionModeToggle(
          {iggy3d::ProductInteractionMode::Player,
           iggy3d::ProductInputSurface::Gameplay,
           fullChord(),
           {}});
  const iggy3d::ProductInteractionModeToggleResult creativeToPlayer =
      iggy3d::applyProductInteractionModeToggle(
          {iggy3d::ProductInteractionMode::Creative,
           iggy3d::ProductInputSurface::RoomEditor,
           fullChord(),
           {}});

  return expect(playerToCreative.toggleRequested,
                "gameplay chord requests toggle") &&
         expect(playerToCreative.toggleAccepted,
                "gameplay chord accepts toggle") &&
         expect(playerToCreative.mode ==
                    iggy3d::ProductInteractionMode::Creative,
                "gameplay chord toggles player to creative") &&
         expect(playerToCreative.status == "interaction_mode_toggled",
                "gameplay toggle status") &&
         expect(creativeToPlayer.toggleAccepted,
                "room editor chord accepts toggle") &&
         expect(creativeToPlayer.mode == iggy3d::ProductInteractionMode::Player,
                "room editor chord toggles creative to player") &&
         expect(creativeToPlayer.reasonCode == "interaction_mode_toggled",
                "room editor toggle reason");
}

bool chordIsLatchedUntilReleased() {
  iggy3d::ProductControllerModeChordState state;
  const iggy3d::ProductInteractionModeToggleResult first =
      iggy3d::applyProductInteractionModeToggle(
          {iggy3d::ProductInteractionMode::Player,
           iggy3d::ProductInputSurface::Gameplay,
           fullChord(),
           state});
  state = first.chordState;
  const iggy3d::ProductInteractionModeToggleResult held =
      iggy3d::applyProductInteractionModeToggle(
          {first.mode,
           iggy3d::ProductInputSurface::Gameplay,
           fullChord(),
           state});
  state = held.chordState;
  const iggy3d::ProductInteractionModeToggleResult released =
      iggy3d::applyProductInteractionModeToggle(
          {held.mode,
           iggy3d::ProductInputSurface::Gameplay,
           {},
           state});
  state = released.chordState;
  const iggy3d::ProductInteractionModeToggleResult pressedAgain =
      iggy3d::applyProductInteractionModeToggle(
          {released.mode,
           iggy3d::ProductInputSurface::Gameplay,
           fullChord(),
           state});

  return expect(first.toggleAccepted, "first chord toggles") &&
         expect(first.mode == iggy3d::ProductInteractionMode::Creative,
                "first chord mode") &&
         expect(held.toggleRequested, "held chord still requested") &&
         expect(!held.toggleAccepted, "held chord does not retoggle") &&
         expect(held.mode == iggy3d::ProductInteractionMode::Creative,
                "held chord preserves mode") &&
         expect(held.status == "interaction_mode_chord_held",
                "held chord status") &&
         expect(!released.toggleRequested, "released chord not requested") &&
         expect(!released.toggleAccepted, "released chord not accepted") &&
         expect(released.status == "interaction_mode_chord_partial",
                "released chord status") &&
         expect(pressedAgain.toggleAccepted, "re-pressed chord toggles") &&
         expect(pressedAgain.mode == iggy3d::ProductInteractionMode::Player,
                "re-pressed chord toggles back");
}

bool partialChordDoesNotToggle() {
  constexpr std::array partialSamples{
      iggy3d::ProductControllerModeChordSample{true, true, true, false},
      iggy3d::ProductControllerModeChordSample{true, true, false, true},
      iggy3d::ProductControllerModeChordSample{true, false, true, true},
      iggy3d::ProductControllerModeChordSample{false, true, true, true},
  };

  bool ok = true;
  for (const iggy3d::ProductControllerModeChordSample sample :
       partialSamples) {
    const iggy3d::ProductInteractionModeToggleResult result =
        iggy3d::applyProductInteractionModeToggle(
            {iggy3d::ProductInteractionMode::Player,
             iggy3d::ProductInputSurface::Gameplay,
             sample,
             {}});
    ok = expect(!result.toggleRequested, "partial chord not requested") &&
         expect(!result.toggleAccepted, "partial chord not accepted") &&
         expect(result.mode == iggy3d::ProductInteractionMode::Player,
                "partial chord preserves mode") &&
         expect(result.status == "interaction_mode_chord_partial",
                "partial chord status") &&
         ok;
  }
  return ok;
}

bool blockedSurfacesRejectFullChord() {
  constexpr std::array blockedSurfaces{
      iggy3d::ProductInputSurface::None,
      iggy3d::ProductInputSurface::Starter,
      iggy3d::ProductInputSurface::Pause,
      iggy3d::ProductInputSurface::Settings,
      iggy3d::ProductInputSurface::DevTools,
      iggy3d::ProductInputSurface::SaveBrowser,
      iggy3d::ProductInputSurface::WorldSetup,
  };

  bool ok = true;
  for (const iggy3d::ProductInputSurface surface : blockedSurfaces) {
    const iggy3d::ProductInteractionModeToggleResult result =
        iggy3d::applyProductInteractionModeToggle(
            {iggy3d::ProductInteractionMode::Player,
             surface,
             fullChord(),
             {}});
    ok = expect(result.toggleRequested,
                "blocked full chord still requests toggle") &&
         expect(!result.toggleAccepted, "blocked full chord rejected") &&
         expect(result.mode == iggy3d::ProductInteractionMode::Player,
                "blocked full chord preserves mode") &&
         expect(result.status == "interaction_mode_surface_blocked",
                "blocked surface status") &&
         expect(result.reasonCode == "interaction_mode_surface_blocked",
                "blocked surface reason") &&
         ok;
  }
  return ok;
}

}  // namespace

int main() {
  const bool ok = namesAreStable() && fullChordTogglesOnGameplaySurfaces() &&
                  chordIsLatchedUntilReleased() &&
                  partialChordDoesNotToggle() &&
                  blockedSurfacesRejectFullChord();
  return ok ? 0 : 1;
}
