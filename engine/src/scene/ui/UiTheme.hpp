#pragma once

#include <string>

namespace iggy::ui {

struct UiThemeInputs {
	std::string base = "#101418";
	std::string surface = "#171d24";
	std::string accent = "#8fb4d8";
	std::string text = "#dce5ee";
	std::string uiFont = "Avenir Next";
	std::string codeFont = "Menlo";
	int uiFontSize = 12;
	int codeFontSize = 13;
};

struct UiThemeTokens {
	std::string base;
	std::string surface;
	std::string surfaceRaised;
	std::string workspaceBody;
	std::string control;
	std::string controlHover;
	std::string selected;
	std::string rowSelected;
	std::string text;
	std::string textMuted;
	std::string textFaint;
	std::string borderMajor;
	std::string borderMinor;
	std::string borderFocus;
	std::string accent;
	std::string accentSoft;
	std::string success;
	std::string warning;
	std::string danger;
	std::string pending;
	std::string disabled;
	std::string trafficClose;
	std::string trafficCloseEdge;
	std::string trafficMinimize;
	std::string trafficMinimizeEdge;
	std::string trafficZoom;
	std::string trafficZoomEdge;
	std::string uiFont;
	std::string codeFont;
	int fontSizeXs = 10;
	int fontSizeSm = 11;
	int fontSizeBody = 12;
	int fontSizeTitle = 13;
	int fontSizeEditor = 13;
};

[[nodiscard]] std::string mixUiHexColor(std::string a, std::string b, double ratio);
[[nodiscard]] UiThemeTokens deriveUiThemeTokens(const UiThemeInputs &inputs = {});

} // namespace iggy::ui
