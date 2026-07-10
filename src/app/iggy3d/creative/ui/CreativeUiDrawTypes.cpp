#include "app/iggy3d/creative/ui/CreativeUiDrawTypes.hpp"

#include <array>
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
  tones[static_cast<std::size_t>(CreativeUiTone::Status)] =
      {0.50F, 0.56F, 0.53F, 1.0F};
  return theme;
}

CreativeUiTheme makeJournalTheme() {
  CreativeUiTheme theme;
  auto& tones = theme.tones;
  tones[static_cast<std::size_t>(CreativeUiTone::Surface)] =
      {0.169F, 0.125F, 0.094F, 1.0F};
  tones[static_cast<std::size_t>(CreativeUiTone::SurfaceRaised)] =
      {0.925F, 0.890F, 0.804F, 1.0F};
  tones[static_cast<std::size_t>(CreativeUiTone::TextPrimary)] =
      {0.247F, 0.224F, 0.180F, 1.0F};
  tones[static_cast<std::size_t>(CreativeUiTone::TextMuted)] =
      {0.420F, 0.384F, 0.322F, 1.0F};
  tones[static_cast<std::size_t>(CreativeUiTone::Accent)] =
      {0.725F, 0.541F, 0.235F, 1.0F};
  tones[static_cast<std::size_t>(CreativeUiTone::Selected)] =
      {0.851F, 0.780F, 0.627F, 1.0F};
  tones[static_cast<std::size_t>(CreativeUiTone::Disabled)] =
      {0.604F, 0.565F, 0.471F, 1.0F};
  tones[static_cast<std::size_t>(CreativeUiTone::Border)] =
      {0.761F, 0.706F, 0.549F, 1.0F};
  tones[static_cast<std::size_t>(CreativeUiTone::Status)] =
      {0.478F, 0.427F, 0.329F, 1.0F};
  return theme;
}

}  // namespace

const CreativeUiTheme& creativeUiTheme(CreativeUiThemeId id) {
  static const CreativeUiTheme kSystem = makeSystemTheme();
  static const CreativeUiTheme kJournal = makeJournalTheme();
  return id == CreativeUiThemeId::Journal ? kJournal : kSystem;
}

CreativeUiColor creativeUiToneColor(CreativeUiTone tone,
                                    const CreativeUiTheme& theme) {
  return theme.tones[static_cast<std::size_t>(tone)];
}

}  // namespace iggy3d
