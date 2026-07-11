#include "app/iggy3d/creative/ui/UiTheme.hpp"

#include <cstddef>

namespace iggy3d {
namespace {

CreativeUiTheme makeSystemTheme() {
  CreativeUiTheme theme;
  auto& tones = theme.tones;
  tones[static_cast<std::size_t>(CreativeUiTone::Surface)] =
      {0.07F, 0.09F, 0.11F, 1.0F};
  tones[static_cast<std::size_t>(CreativeUiTone::SurfaceRaised)] =
      {0.11F, 0.14F, 0.16F, 1.0F};
  tones[static_cast<std::size_t>(CreativeUiTone::TextPrimary)] =
      {0.90F, 0.93F, 0.84F, 1.0F};
  tones[static_cast<std::size_t>(CreativeUiTone::TextMuted)] =
      {0.64F, 0.70F, 0.67F, 1.0F};
  tones[static_cast<std::size_t>(CreativeUiTone::Accent)] =
      {0.49F, 0.79F, 0.69F, 1.0F};
  tones[static_cast<std::size_t>(CreativeUiTone::Selected)] =
      {0.25F, 0.37F, 0.34F, 1.0F};
  tones[static_cast<std::size_t>(CreativeUiTone::Disabled)] =
      {0.31F, 0.35F, 0.35F, 1.0F};
  tones[static_cast<std::size_t>(CreativeUiTone::Border)] =
      {0.18F, 0.23F, 0.25F, 1.0F};
  return theme;
}

}  // namespace

const CreativeUiTheme& creativeUiTheme() {
  static const CreativeUiTheme kSystem = makeSystemTheme();
  return kSystem;
}

CreativeUiColor creativeUiToneColor(CreativeUiTone tone,
                                    const CreativeUiTheme& theme) {
  const std::size_t index = static_cast<std::size_t>(tone);
  if (index >= theme.tones.size()) {
    return theme.tones[static_cast<std::size_t>(CreativeUiTone::Surface)];
  }
  return theme.tones[index];
}

}  // namespace iggy3d
