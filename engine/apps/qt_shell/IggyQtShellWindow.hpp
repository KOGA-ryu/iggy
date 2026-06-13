#pragma once

#include <QMainWindow>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <functional>
#include <string>

#include "scene/ui/UiFeatureContext.hpp"
#include "scene/ui/UiRuntimeWorkspaceModel.hpp"
#include "scene/ui/UiSettingsState.hpp"
#include "scene/ui/UiToolInventory.hpp"

namespace iggy::qt_shell {

class IggyQtShellWindow final : public QMainWindow {
public:
	IggyQtShellWindow();

private:
	void rebuild();
	void rebuildModel();
	void applyTheme();

	[[nodiscard]] QWidget *buildChrome();
	[[nodiscard]] QWidget *buildBody();
	[[nodiscard]] QWidget *buildRail();
	[[nodiscard]] QWidget *buildMainSlot();
	[[nodiscard]] QWidget *buildPanelSlot(ui::UiShellSlot slot, const char *objectName);
	[[nodiscard]] QWidget *buildPanelContent(const ui::UiMountedPanel &panel);
	[[nodiscard]] QWidget *buildToolBelt();
	[[nodiscard]] QWidget *buildPaletteStrip();
	[[nodiscard]] QWidget *buildSettings();
	[[nodiscard]] QWidget *buildSettingsPage(const ResourceId &pageId);
	[[nodiscard]] QWidget *buildThemeSettingsPage();
	[[nodiscard]] QWidget *buildToolBeltSettingsPage();
	[[nodiscard]] QWidget *buildPanelSettingsPage();

	void addThemeColorRow(
		QVBoxLayout *layout,
		const QString &label,
		const std::string &value,
		std::function<void(const std::string &)> apply);
	void addThemeFontRow(
		QVBoxLayout *layout,
		const QString &label,
		const std::string &fontFamily,
		int fontSize,
		std::function<void(const std::string &)> applyFont,
		std::function<void(int)> applySize);

	ui::UiToolInventory inventory_ = ui::defaultUiToolInventory();
	ui::UiSettingsState settings_;
	ui::UiFeatureContext context_;
	ui::UiRuntimeWorkspaceModelInput input_;
	ui::UiRuntimeWorkspaceModel model_;
	bool showSettings_ = false;
};

} // namespace iggy::qt_shell
