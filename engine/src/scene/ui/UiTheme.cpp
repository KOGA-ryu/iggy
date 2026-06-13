#include "scene/ui/UiTheme.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string_view>

namespace iggy::ui {
namespace {

bool ParseHex(std::string_view value, std::array<int, 3> &rgb)
{
	if (!value.empty() && value.front() == '#')
		value.remove_prefix(1);
	if (value.size() != 6)
		return false;
	if (!std::all_of(value.begin(), value.end(), [](char character) {
			return std::isxdigit(static_cast<unsigned char>(character)) != 0;
		}))
		return false;

	int packed = 0;
	std::istringstream stream { std::string { value } };
	stream >> std::hex >> packed;
	if (stream.fail())
		return false;

	rgb = { (packed >> 16) & 0xff, (packed >> 8) & 0xff, packed & 0xff };
	return true;
}

std::string ToHex(int r, int g, int b)
{
	const auto clampByte = [](int value) {
		return std::clamp(value, 0, 255);
	};

	std::ostringstream stream;
	stream << '#'
		   << std::hex << std::setfill('0') << std::nouppercase
		   << std::setw(2) << clampByte(r)
		   << std::setw(2) << clampByte(g)
		   << std::setw(2) << clampByte(b);
	return stream.str();
}

} // namespace

std::string mixUiHexColor(std::string a, std::string b, double ratio)
{
	std::array<int, 3> left { 0, 0, 0 };
	std::array<int, 3> right { 0, 0, 0 };
	(void)ParseHex(a, left);
	(void)ParseHex(b, right);

	const double amount = std::clamp(ratio, 0.0, 1.0);
	const auto channel = [&](int index) {
		return static_cast<int>(std::floor(left[index] + (right[index] - left[index]) * amount + 0.5));
	};

	return ToHex(channel(0), channel(1), channel(2));
}

UiThemeTokens deriveUiThemeTokens(const UiThemeInputs &inputs)
{
	UiThemeTokens tokens;
	tokens.base = inputs.base;
	tokens.surface = inputs.surface;
	tokens.accent = inputs.accent;
	tokens.text = inputs.text;

	tokens.surfaceRaised = mixUiHexColor(inputs.surface, inputs.text, 0.08);
	tokens.workspaceBody = mixUiHexColor(inputs.surface, inputs.text, 0.06);
	tokens.control = mixUiHexColor(inputs.surface, inputs.text, 0.08);
	tokens.controlHover = mixUiHexColor(inputs.surface, inputs.text, 0.10);
	tokens.selected = mixUiHexColor(inputs.surface, inputs.accent, 0.22);
	tokens.rowSelected = mixUiHexColor(inputs.base, inputs.accent, 0.14);
	tokens.textMuted = mixUiHexColor(inputs.surface, inputs.text, 0.62);
	tokens.textFaint = mixUiHexColor(inputs.surface, inputs.text, 0.38);
	tokens.borderMajor = mixUiHexColor(inputs.surface, inputs.text, 0.12);
	tokens.borderMinor = mixUiHexColor(inputs.surface, inputs.text, 0.055);
	tokens.borderFocus = mixUiHexColor(inputs.surface, inputs.accent, 0.36);
	tokens.accentSoft = mixUiHexColor(inputs.surface, inputs.accent, 0.26);
	tokens.pending = mixUiHexColor(inputs.surface, inputs.accent, 0.62);
	tokens.disabled = mixUiHexColor(inputs.surface, inputs.text, 0.30);

	tokens.success = "#91c89b";
	tokens.warning = "#d5bb78";
	tokens.danger = "#d98b8b";
	tokens.trafficClose = "#ff5f57";
	tokens.trafficMinimize = "#ffbd2e";
	tokens.trafficZoom = "#28c840";
	tokens.trafficCloseEdge = mixUiHexColor(tokens.trafficClose, "#000000", 0.2);
	tokens.trafficMinimizeEdge = mixUiHexColor(tokens.trafficMinimize, "#000000", 0.2);
	tokens.trafficZoomEdge = mixUiHexColor(tokens.trafficZoom, "#000000", 0.2);

	tokens.uiFont = inputs.uiFont;
	tokens.codeFont = inputs.codeFont;
	tokens.fontSizeBody = std::clamp(inputs.uiFontSize, 9, 28);
	tokens.fontSizeSm = std::clamp(tokens.fontSizeBody - 1, 8, 27);
	tokens.fontSizeXs = std::clamp(tokens.fontSizeBody - 2, 8, 26);
	tokens.fontSizeTitle = std::clamp(tokens.fontSizeBody + 1, 10, 29);
	tokens.fontSizeEditor = std::clamp(inputs.codeFontSize, 9, 28);
	return tokens;
}

} // namespace iggy::ui
