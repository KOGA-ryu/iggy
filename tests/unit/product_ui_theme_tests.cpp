#include "app/iggy3d/menu/DrawList.hpp"

#include <array>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    return false;
  }
  return true;
}

constexpr std::array<iggy3d::ProductUiTone, 9> kAllTones{
    iggy3d::ProductUiTone::Surface,     iggy3d::ProductUiTone::SurfaceRaised,
    iggy3d::ProductUiTone::TextPrimary, iggy3d::ProductUiTone::TextMuted,
    iggy3d::ProductUiTone::Accent,      iggy3d::ProductUiTone::Selected,
    iggy3d::ProductUiTone::Disabled,    iggy3d::ProductUiTone::Border,
    iggy3d::ProductUiTone::Status};

// The System theme must resolve every tone to exactly the long-standing default
// colour, so no existing surface (the starter menu) shifts a pixel.
bool systemThemeMatchesTheDefaultPalette() {
  const iggy3d::ProductUiTheme& system =
      iggy3d::productUiTheme(iggy3d::ProductUiThemeId::System);
  bool ok = true;
  for (const iggy3d::ProductUiTone tone : kAllTones) {
    const iggy3d::ProductUiColor themed =
        iggy3d::productUiToneColor(tone, system);
    const iggy3d::ProductUiColor legacy = iggy3d::productUiToneColor(tone);
    ok &= expect(themed.r == legacy.r && themed.g == legacy.g &&
                     themed.b == legacy.b && themed.a == legacy.a,
                 "system theme equals the default tone colour");
  }
  return ok;
}

// The Journal theme inverts the light/dark relationship: light cream page, dark
// graphite ink — an aged notebook, from the same tones the System shell draws dark.
bool journalThemeIsAnAgedNotebookPalette() {
  const iggy3d::ProductUiTheme& system =
      iggy3d::productUiTheme(iggy3d::ProductUiThemeId::System);
  const iggy3d::ProductUiTheme& journal =
      iggy3d::productUiTheme(iggy3d::ProductUiThemeId::Journal);

  const iggy3d::ProductUiColor journalPage =
      iggy3d::productUiToneColor(iggy3d::ProductUiTone::SurfaceRaised, journal);
  const iggy3d::ProductUiColor systemPage =
      iggy3d::productUiToneColor(iggy3d::ProductUiTone::SurfaceRaised, system);
  const iggy3d::ProductUiColor journalInk =
      iggy3d::productUiToneColor(iggy3d::ProductUiTone::TextPrimary, journal);
  const iggy3d::ProductUiColor systemInk =
      iggy3d::productUiToneColor(iggy3d::ProductUiTone::TextPrimary, system);

  bool ok = true;
  ok &= expect(journalPage.r > 0.8F && journalPage.g > 0.8F,
               "journal page (SurfaceRaised) is light cream");
  ok &= expect(systemPage.r < 0.3F, "system surface is dark");
  ok &= expect(journalInk.r < 0.35F, "journal ink (TextPrimary) is dark graphite");
  ok &= expect(systemInk.r > 0.8F, "system text is light");
  for (const iggy3d::ProductUiTone tone : kAllTones) {
    ok &= expect(iggy3d::productUiToneColor(tone, journal).a == 1.0F,
                 "journal tone is opaque");
  }
  return ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok &= systemThemeMatchesTheDefaultPalette();
  ok &= journalThemeIsAnAgedNotebookPalette();
  if (!ok) {
    return 1;
  }
  std::cout << "product_ui_theme_tests=pass\n";
  return 0;
}
