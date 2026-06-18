#pragma once

#include <QMainWindow>
#include <QPoint>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <filesystem>
#include <functional>
#include <string>

#include "runtime/RuntimeGameplayAuthoringPreviewModel.hpp"
#include "runtime/RuntimeGameplayTomlScenarioFacade.hpp"
#include "scene/ui/UiFeatureContext.hpp"
#include "scene/ui/UiRuntimeWorkspaceModel.hpp"
#include "scene/ui/UiSettingsState.hpp"
#include "scene/ui/UiToolInventory.hpp"

class QEvent;
class QPushButton;

namespace iggy::qt_shell {

struct IggyQtShellPreviewOptions {
	bool enabled = false;
	std::filesystem::path path;
	runtime::RuntimeGameplayTomlScenarioFacadeConfig config;
};

class IggyQtShellWindow final : public QMainWindow {
public:
	explicit IggyQtShellWindow(IggyQtShellPreviewOptions previewOptions = {});

protected:
	bool eventFilter(QObject *watched, QEvent *event) override;

private:
	void buildShell();
	void buildSettingsWindow();
	void rebuildSettingsWindowContent();
	void refreshBody();
	void refreshStatusBar();
	void refreshChrome();
	void refreshAfterModelChange();
	void rebuildModel();
	void applyTheme();
	void openSettingsWindow();

	[[nodiscard]] QWidget *buildChrome();
	[[nodiscard]] QWidget *buildBody();
	[[nodiscard]] QWidget *buildStatusBar();
	[[nodiscard]] QWidget *buildRail();
	[[nodiscard]] QWidget *buildWorkspaceHost();
	[[nodiscard]] QWidget *buildMainSlot();
	[[nodiscard]] QWidget *buildPanelSlot(ui::UiShellSlot slot, const char *objectName);
	[[nodiscard]] QWidget *buildPanelContent(const ui::UiMountedPanel &panel);
	[[nodiscard]] QWidget *buildRuntimeFramePanelContent();
	[[nodiscard]] QWidget *buildInteractionEventsPanelContent();
	[[nodiscard]] QWidget *buildInventoryPanelContent();
	[[nodiscard]] QWidget *buildCollisionPanelContent();
	[[nodiscard]] QWidget *buildAuthoringPreviewPanelContent();
	[[nodiscard]] QWidget *buildUnavailablePanelContent(const ui::UiMountedPanel &panel);
	[[nodiscard]] QPushButton *buildMountedChromePanelButton(const ui::UiMountedChromePanel &panel);
	[[nodiscard]] QWidget *buildToolBelt();
	[[nodiscard]] QWidget *buildPaletteStrip();
	[[nodiscard]] QWidget *buildMountedPalette(const ui::UiMountedPalette &palette);
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
	runtime::RuntimeGameplayAuthoringPreviewModel authoringPreview_;
	bool hasAuthoringPreview_ = false;
	QWidget *root_ = nullptr;
	QVBoxLayout *rootLayout_ = nullptr;
	QWidget *body_ = nullptr;
	QWidget *statusBar_ = nullptr;
	QWidget *settingsWindow_ = nullptr;
	QWidget *settingsWindowContent_ = nullptr;
	QWidget *chrome_ = nullptr;
	bool draggingChrome_ = false;
	QPoint chromeDragOffset_;
};

} // namespace iggy::qt_shell
