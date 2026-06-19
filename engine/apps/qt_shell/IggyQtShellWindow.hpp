#pragma once

#include <QMainWindow>
#include <QPoint>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

#include "runtime/RuntimeGameplayAuthoringPreviewModel.hpp"
#include "runtime/RuntimeGameplayProductFrameRequest.hpp"
#include "runtime/RuntimeGameplayProductInputAccumulator.hpp"
#include "runtime/RuntimeGameplayProductInputFrameTargetContext.hpp"
#include "runtime/RuntimeGameplayProductLoop.hpp"
#include "runtime/RuntimeGameplayProductPlayMode.hpp"
#include "runtime/RuntimeGameplayProductScenarioLoader.hpp"
#include "scene/camera/CameraState.hpp"
#include "runtime/RuntimeGameplayTomlScenarioFacade.hpp"
#include "scene/ui/UiFeatureContext.hpp"
#include "scene/ui/UiRuntimeWorkspaceModel.hpp"
#include "scene/ui/UiSettingsState.hpp"
#include "scene/ui/UiToolInventory.hpp"

class QEvent;
class QFrame;
class QKeyEvent;
class QMouseEvent;
class QPushButton;
class QTimer;

namespace iggy::qt_shell {

struct IggyQtShellPreviewOptions {
	bool enabled = false;
	std::filesystem::path path;
	runtime::RuntimeGameplayTomlScenarioFacadeConfig config;
};

struct IggyQtShellPlayOptions {
	bool enabled = false;
	std::filesystem::path path;
};

struct IggyQtShellLaunchOptions {
	IggyQtShellPreviewOptions preview;
	IggyQtShellPlayOptions play;
};

class IggyQtShellWindow final : public QMainWindow {
public:
	explicit IggyQtShellWindow(IggyQtShellLaunchOptions launchOptions = {});

protected:
	bool eventFilter(QObject *watched, QEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	void keyReleaseEvent(QKeyEvent *event) override;

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
	[[nodiscard]] bool productPlayInputFocusEnabled() const;
	[[nodiscard]] bool productPlayInputFocusAvailable() const;
	void setProductPlayInputFocus(bool enabled);
	void toggleProductPlayInputFocus();
	[[nodiscard]] bool productManualStepAvailable() const;
	[[nodiscard]] bool productFramePumpAvailable() const;
	[[nodiscard]] bool productFramePumpEnabled() const;
	void setProductFramePumpEnabled(bool enabled);
	[[nodiscard]] runtime::RuntimeGameplayProductPresentationCameraConfig
	productPresentationCameraConfig() const;
	void runProductManualStep();
	void runProductFrameRequestOnce();
	void clearProductInputAccumulator();
	[[nodiscard]] std::optional<runtime::RuntimeGameplayProductInputControl2D>
	mapQtKeyToProductControl(int key) const;
	bool recordProductKeyEvent(
		QKeyEvent &event,
		runtime::RuntimeGameplayProductInputEventKind kind);
	bool recordProductViewportPrimaryTilePress(QMouseEvent &event);

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
	[[nodiscard]] QWidget *buildProductPlayModePanelContent();
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
	runtime::RuntimeGameplayProductScenarioLoadResult productLoad_;
	runtime::RuntimeGameplayProductLoopBuildResult productLoopBuild_;
	runtime::RuntimeGameplayProductPlayModeBuildResult productPlayBuild_;
	runtime::RuntimeGameplayProductPlayModeState productPlayState_;
	runtime::RuntimeGameplayProductInputAccumulatorState productInputAccumulator_;
	runtime::RuntimeGameplayProductPlayModeFrameResult latestProductPlayModeFrame_;
	bool hasLatestProductPlayModeFrame_ = false;
	runtime::RuntimeGameplayProductInputFrameTargetContextResult latestProductInputFrameTargetContext_;
	bool hasLatestProductInputFrameTargetContext_ = false;
	CameraState productPresentationCamera_;
	bool hasProductPresentationCamera_ = false;
	bool hasProductPlayMode_ = false;
	QWidget *root_ = nullptr;
	QVBoxLayout *rootLayout_ = nullptr;
	QWidget *body_ = nullptr;
	QWidget *statusBar_ = nullptr;
	QWidget *settingsWindow_ = nullptr;
	QWidget *settingsWindowContent_ = nullptr;
	QWidget *chrome_ = nullptr;
	QFrame *productViewport_ = nullptr;
	bool draggingChrome_ = false;
	QPoint chromeDragOffset_;
	QTimer *productFramePumpTimer_ = nullptr;
};

} // namespace iggy::qt_shell
