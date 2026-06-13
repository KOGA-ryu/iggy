#include <cstdlib>

#include "scene/ui/UiTheme.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

void TestMixHexColorUsesRoundedChannelInterpolation()
{
	Expect(iggy::ui::mixUiHexColor("#000000", "#ffffff", 0.5) == "#808080", "ui theme color mix should round half channels");
	Expect(iggy::ui::mixUiHexColor("#000000", "#ffffff", -1.0) == "#000000", "ui theme color mix should clamp low ratio");
	Expect(iggy::ui::mixUiHexColor("#000000", "#ffffff", 2.0) == "#ffffff", "ui theme color mix should clamp high ratio");
	Expect(iggy::ui::mixUiHexColor("not-hex", "#ffffff", 0.5) == "#808080", "ui theme color mix should degrade invalid input to black");
}

void TestDeriveThemePreservesInputsAndDerivesTokens()
{
	const iggy::ui::UiThemeTokens tokens = iggy::ui::deriveUiThemeTokens();

	Expect(tokens.base == "#101418", "ui theme should preserve base input");
	Expect(tokens.surface == "#171d24", "ui theme should preserve surface input");
	Expect(tokens.accent == "#8fb4d8", "ui theme should preserve accent input");
	Expect(tokens.text == "#dce5ee", "ui theme should preserve text input");
	Expect(tokens.surfaceRaised == "#272d34", "ui theme should derive raised surface from surface and text");
	Expect(tokens.selected == "#313e4c", "ui theme should derive selected color from surface and accent");
	Expect(tokens.trafficCloseEdge == "#cc4c46", "ui theme should derive traffic light edge");
	Expect(tokens.fontSizeXs == 10 && tokens.fontSizeSm == 11 && tokens.fontSizeTitle == 13, "ui theme should derive font size ramp");
}

void TestDeriveThemeClampsFontInputs()
{
	iggy::ui::UiThemeInputs inputs;
	inputs.uiFontSize = 99;
	inputs.codeFontSize = 1;

	const iggy::ui::UiThemeTokens tokens = iggy::ui::deriveUiThemeTokens(inputs);

	Expect(tokens.fontSizeBody == 28, "ui theme should clamp high body font size");
	Expect(tokens.fontSizeTitle == 29, "ui theme should derive title size from clamped body");
	Expect(tokens.fontSizeEditor == 9, "ui theme should clamp low editor font size");
}

} // namespace

int main()
{
	TestMixHexColorUsesRoundedChannelInterpolation();
	TestDeriveThemePreservesInputsAndDerivesTokens();
	TestDeriveThemeClampsFontInputs();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
