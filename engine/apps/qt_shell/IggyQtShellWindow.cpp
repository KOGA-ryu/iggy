#include "IggyQtShellWindow.hpp"

#include <QButtonGroup>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QEvent>
#include <QFont>
#include <QFontComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSize>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#include "IggyQtShellUi.hpp"
#include "scene/ui/UiTheme.hpp"

namespace iggy::qt_shell {
namespace {

bool shouldShowPanel(
	const ui::UiRuntimeWorkspaceModel &model,
	ui::UiShellSlot slot,
	const ui::UiPanelState &state,
	int windowWidth,
	int windowHeight)
{
	return ui::uiPanelVisibility(slot, state, windowWidth, windowHeight) == ui::UiPanelVisibility::Visible
		&& std::any_of(model.mountedPanels.begin(), model.mountedPanels.end(), [slot](const ui::UiMountedPanel &panel) {
			   return panel.slot == slot && !panel.hidden;
		   });
}

ui::UiPanelState panelStateFor(const ui::UiShellPanelsState &panels, ui::UiShellSlot slot)
{
	switch (slot) {
	case ui::UiShellSlot::Left:
		return panels.left;
	case ui::UiShellSlot::Right:
		return panels.right;
	case ui::UiShellSlot::Bottom:
		return panels.bottom;
	case ui::UiShellSlot::Main:
		break;
	}
	return {};
}

QString swatchStyle(const std::string &hex, const ui::UiThemeTokens &theme)
{
	const QString color = QColor(toQString(hex)).isValid() ? toQString(hex) : QStringLiteral("#000000");
	return QStringLiteral("background: %1; border: 1px solid %2; min-width: 22px; max-width: 22px;")
		.arg(color, toQString(theme.borderMajor));
}

QPixmap panelToggleIcon(ui::UiShellSlot slot, const QColor &frame, const QColor &bar, qreal devicePixelRatio)
{
	QPixmap pixmap(qRound(16 * devicePixelRatio), qRound(14 * devicePixelRatio));
	pixmap.setDevicePixelRatio(devicePixelRatio);
	pixmap.fill(Qt::transparent);

	QPainter painter(&pixmap);
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setPen(frame);
	painter.drawRect(0, 0, 15, 13);

	QRect barRect;
	switch (slot) {
	case ui::UiShellSlot::Left:
		barRect = QRect(1, 1, 5, 12);
		break;
	case ui::UiShellSlot::Right:
		barRect = QRect(10, 1, 5, 12);
		break;
	case ui::UiShellSlot::Bottom:
		barRect = QRect(2, 8, 12, 5);
		break;
	case ui::UiShellSlot::Main:
		break;
	}
	if (!barRect.isNull())
		painter.fillRect(barRect, bar);
	return pixmap;
}

QPushButton *makeTrafficButton(const char *objectName, const QString &tooltip)
{
	auto *button = new QPushButton;
	button->setObjectName(QString::fromUtf8(objectName));
	button->setToolTip(tooltip);
	return button;
}

QPushButton *makePanelToggleButton(const QString &tooltip)
{
	auto *button = new QPushButton;
	button->setObjectName(QStringLiteral("panelToggleButton"));
	button->setToolTip(tooltip);
	button->setCheckable(true);
	return button;
}

QMenu *attachMenu(QPushButton *button)
{
	auto *menu = new QMenu(button);
	button->setMenu(menu);
	return menu;
}

void removeWidgetFromLayout(QLayout *layout, QWidget *widget)
{
	if (layout == nullptr || widget == nullptr)
		return;
	layout->removeWidget(widget);
	widget->deleteLater();
}

} // namespace

IggyQtShellWindow::IggyQtShellWindow()
{
	setWindowFlag(Qt::FramelessWindowHint, true);
	setMinimumSize(520, 420);
	settings_ = ui::defaultUiSettingsState(inventory_);
	input_ = ui::defaultUiRuntimeWorkspaceModelInput(inventory_);
	context_.activeToolId = id("tool:select");
	rebuildModel();
	applyTheme();
	buildShell();
	buildSettingsWindow();
	resize(1180, 760);
	setWindowTitle(QStringLiteral("Iggy Qt Shell"));
}

void IggyQtShellWindow::buildShell()
{
	root_ = new QWidget;
	root_->setObjectName(QStringLiteral("shellRoot"));
	rootLayout_ = new QVBoxLayout(root_);
	rootLayout_->setContentsMargins(0, 0, 0, 0);
	rootLayout_->setSpacing(0);
	chrome_ = buildChrome();
	rootLayout_->addWidget(chrome_);
	body_ = buildBody();
	rootLayout_->addWidget(body_, 1);
	statusBar_ = buildStatusBar();
	rootLayout_->addWidget(statusBar_);
	setCentralWidget(root_);
}

void IggyQtShellWindow::buildSettingsWindow()
{
	if (settingsWindow_ != nullptr)
		return;

	settingsWindow_ = new QWidget(this, Qt::Tool);
	settingsWindow_->setObjectName(QStringLiteral("settingsWindow"));
	settingsWindow_->setAttribute(Qt::WA_StyledBackground, true);
	settingsWindow_->setWindowTitle(QStringLiteral("Settings"));
	auto *layout = new QVBoxLayout(settingsWindow_);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	settingsWindow_->resize(720, 680);
	settingsWindow_->setStyleSheet(shellStyleSheet(ui::deriveUiThemeTokens(settings_.theme)));
	rebuildSettingsWindowContent();
}

void IggyQtShellWindow::rebuildSettingsWindowContent()
{
	if (settingsWindow_ == nullptr || settingsWindow_->layout() == nullptr)
		return;
	removeWidgetFromLayout(settingsWindow_->layout(), settingsWindowContent_);
	settingsWindowContent_ = buildSettings();
	settingsWindow_->layout()->addWidget(settingsWindowContent_);
}

void IggyQtShellWindow::refreshBody()
{
	if (rootLayout_ == nullptr) {
		buildShell();
		return;
	}
	removeWidgetFromLayout(rootLayout_, body_);
	body_ = buildBody();
	rootLayout_->insertWidget(1, body_, 1);
}

void IggyQtShellWindow::refreshStatusBar()
{
	if (rootLayout_ == nullptr)
		return;
	removeWidgetFromLayout(rootLayout_, statusBar_);
	statusBar_ = buildStatusBar();
	rootLayout_->addWidget(statusBar_);
}

void IggyQtShellWindow::refreshAfterModelChange()
{
	rebuildModel();
	refreshBody();
	refreshStatusBar();
	rebuildSettingsWindowContent();
}

void IggyQtShellWindow::rebuildModel()
{
	input_.settings = settings_;
	input_.context = context_;
	input_.windowWidth = width() > 0 ? width() : input_.windowWidth;
	input_.windowHeight = height() > 0 ? height() : input_.windowHeight;
	model_ = ui::buildUiRuntimeWorkspaceModel(input_);
}

void IggyQtShellWindow::applyTheme()
{
	setStyleSheet(shellStyleSheet(ui::deriveUiThemeTokens(settings_.theme)));
	if (settingsWindow_ != nullptr)
		settingsWindow_->setStyleSheet(shellStyleSheet(ui::deriveUiThemeTokens(settings_.theme)));

	for (QPushButton *button : findChildren<QPushButton *>())
		button->setCursor(Qt::PointingHandCursor);
	if (settingsWindow_ != nullptr) {
		for (QPushButton *button : settingsWindow_->findChildren<QPushButton *>())
			button->setCursor(Qt::PointingHandCursor);
	}
}

void IggyQtShellWindow::openSettingsWindow()
{
	if (settingsWindow_ == nullptr)
		buildSettingsWindow();
	settingsWindow_->show();
	settingsWindow_->raise();
	settingsWindow_->activateWindow();
}

bool IggyQtShellWindow::eventFilter(QObject *watched, QEvent *event)
{
	if (watched != chrome_)
		return QMainWindow::eventFilter(watched, event);

	switch (event->type()) {
	case QEvent::MouseButtonPress: {
		auto *mouse = static_cast<QMouseEvent *>(event);
		if (mouse->button() == Qt::LeftButton) {
			draggingChrome_ = true;
			chromeDragOffset_ = mouse->globalPosition().toPoint() - frameGeometry().topLeft();
			event->accept();
			return true;
		}
		break;
	}
	case QEvent::MouseMove: {
		if (draggingChrome_) {
			auto *mouse = static_cast<QMouseEvent *>(event);
			move(mouse->globalPosition().toPoint() - chromeDragOffset_);
			event->accept();
			return true;
		}
		break;
	}
	case QEvent::MouseButtonRelease: {
		auto *mouse = static_cast<QMouseEvent *>(event);
		if (mouse->button() == Qt::LeftButton && draggingChrome_) {
			draggingChrome_ = false;
			event->accept();
			return true;
		}
		break;
	}
	case QEvent::MouseButtonDblClick:
		isMaximized() ? showNormal() : showMaximized();
		event->accept();
		return true;
	default:
		break;
	}
	return QMainWindow::eventFilter(watched, event);
}

QWidget *IggyQtShellWindow::buildChrome()
{
	auto *chrome = makeFrame("topChrome");
	chrome->setFixedHeight(42);
	chrome->installEventFilter(this);
	auto *layout = new QHBoxLayout(chrome);
	layout->setContentsMargins(10, 0, 10, 0);
	layout->setSpacing(6);

	const ui::UiThemeTokens theme = ui::deriveUiThemeTokens(settings_.theme);
	const auto applyToggleIcon = [this, &theme](QPushButton *button, ui::UiShellSlot slot, const ui::UiPanelState &state) {
		const ui::UiPanelVisibility visibility = ui::uiPanelVisibility(slot, state, input_.windowWidth, input_.windowHeight);
		QColor bar = QColor(toQString(theme.accent));
		if (visibility == ui::UiPanelVisibility::Collapsed)
			bar = QColor(toQString(theme.textFaint));
		else if (visibility == ui::UiPanelVisibility::AutoHidden)
			bar = QColor(toQString(theme.warning));
		button->setProperty("panelState", panelVisibilityName(visibility).replace('-', '_'));
		button->setIcon(QIcon(panelToggleIcon(slot, QColor(toQString(theme.textMuted)), bar, devicePixelRatioF())));
		button->setIconSize(QSize(16, 14));
	};

	auto *closeButton = makeTrafficButton("trafficClose", QStringLiteral("Close window"));
	auto *minimizeButton = makeTrafficButton("trafficMinimize", QStringLiteral("Minimize window"));
	auto *zoomButton = makeTrafficButton("trafficZoom", QStringLiteral("Zoom window"));
	auto *leftToggle = makePanelToggleButton(QStringLiteral("Toggle left panel"));
	auto *back = makeChromeButton(QStringLiteral("<"), QStringLiteral("Back"));
	auto *forward = makeChromeButton(QStringLiteral(">"), QStringLiteral("Forward"));
	auto *file = makeChromeButton(QStringLiteral("File"));
	auto *edit = makeChromeButton(QStringLiteral("Edit"));
	auto *view = makeChromeButton(QStringLiteral("View"));
	auto *settings = makeChromeButton(QStringLiteral("Settings"), QStringLiteral("Open settings"));
	auto *bottomToggle = makePanelToggleButton(QStringLiteral("Toggle bottom panel"));
	auto *rightToggle = makePanelToggleButton(QStringLiteral("Toggle right panel"));
	back->setEnabled(false);
	forward->setEnabled(false);
	leftToggle->setChecked(!input_.panels.left.collapsed);
	bottomToggle->setChecked(!input_.panels.bottom.collapsed);
	rightToggle->setChecked(!input_.panels.right.collapsed);
	applyToggleIcon(leftToggle, ui::UiShellSlot::Left, input_.panels.left);
	applyToggleIcon(bottomToggle, ui::UiShellSlot::Bottom, input_.panels.bottom);
	applyToggleIcon(rightToggle, ui::UiShellSlot::Right, input_.panels.right);

	layout->addWidget(closeButton);
	layout->addWidget(minimizeButton);
	layout->addWidget(zoomButton);
	layout->addSpacing(8);
	layout->addWidget(leftToggle);
	layout->addWidget(back);
	layout->addWidget(forward);
	layout->addWidget(file);
	layout->addWidget(edit);
	layout->addWidget(view);
	auto *fileMenu = attachMenu(file);
	fileMenu->addAction(QStringLiteral("New Workspace"))->setEnabled(false);
	fileMenu->addAction(QStringLiteral("Open"))->setEnabled(false);
	fileMenu->addAction(QStringLiteral("Save"))->setEnabled(false);
	auto *editMenu = attachMenu(edit);
	editMenu->addAction(QStringLiteral("Undo"))->setEnabled(false);
	editMenu->addAction(QStringLiteral("Redo"))->setEnabled(false);
	auto *viewMenu = attachMenu(view);
	viewMenu->addAction(QStringLiteral("Full Layout"), this, [this]() {
		input_.panels.left.collapsed = false;
		input_.panels.right.collapsed = false;
		input_.panels.bottom.collapsed = false;
		refreshAfterModelChange();
	});
	viewMenu->addAction(QStringLiteral("Focus Layout"), this, [this]() {
		input_.panels.left.collapsed = true;
		input_.panels.right.collapsed = true;
		input_.panels.bottom.collapsed = true;
		refreshAfterModelChange();
	});
	layout->addSpacing(8);
	for (const ui::UiMountedChromePanel &panel : model_.mountedChromePanels)
		layout->addWidget(makeChromeButton(toQString(panel.label)));
	layout->addStretch(1);
	layout->addWidget(settings);
	layout->addWidget(bottomToggle);
	layout->addWidget(rightToggle);

	connect(closeButton, &QPushButton::clicked, this, [this]() { close(); });
	connect(minimizeButton, &QPushButton::clicked, this, [this]() { showMinimized(); });
	connect(zoomButton, &QPushButton::clicked, this, [this]() {
		isMaximized() ? showNormal() : showMaximized();
	});
	connect(settings, &QPushButton::clicked, this, [this]() {
		openSettingsWindow();
	});
	connect(leftToggle, &QPushButton::clicked, this, [this]() {
		input_.panels.left.collapsed = !input_.panels.left.collapsed;
		refreshAfterModelChange();
	});
	connect(rightToggle, &QPushButton::clicked, this, [this]() {
		input_.panels.right.collapsed = !input_.panels.right.collapsed;
		refreshAfterModelChange();
	});
	connect(bottomToggle, &QPushButton::clicked, this, [this]() {
		input_.panels.bottom.collapsed = !input_.panels.bottom.collapsed;
		refreshAfterModelChange();
	});
	return chrome;
}

QWidget *IggyQtShellWindow::buildStatusBar()
{
	auto *bar = makeFrame("statusBar");
	bar->setFixedHeight(28);
	auto *layout = new QHBoxLayout(bar);
	layout->setContentsMargins(10, 0, 10, 0);
	layout->setSpacing(12);

	QString activeTool = QStringLiteral("No tool");
	for (const ui::UiRuntimeWorkspaceToolView &tool : model_.availableTools) {
		if (tool.active) {
			activeTool = toQString(tool.label);
			break;
		}
	}
	layout->addWidget(makeLabel(QStringLiteral("Tool: %1").arg(activeTool), "statusMode"));
	layout->addWidget(makeLabel(QStringLiteral("Panels: %1").arg(model_.mountedPanels.size()), "statusMode"));
	layout->addStretch(1);
	layout->addWidget(makeLabel(QStringLiteral("Iggy Qt Shell"), "statusFile"));
	return bar;
}

QWidget *IggyQtShellWindow::buildBody()
{
	auto *body = new QWidget;
	auto *layout = new QHBoxLayout(body);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(buildRail());

	auto *mainArea = new QSplitter(Qt::Horizontal);
	mainArea->setChildrenCollapsible(false);
	mainArea->setHandleWidth(2);
	if (shouldShowPanel(model_, ui::UiShellSlot::Left, input_.panels.left, input_.windowWidth, input_.windowHeight))
		mainArea->addWidget(buildPanelSlot(ui::UiShellSlot::Left, "leftPanel"));

	auto *centerColumn = new QSplitter(Qt::Vertical);
	centerColumn->setChildrenCollapsible(false);
	centerColumn->setHandleWidth(2);
	centerColumn->addWidget(buildMainSlot());
	if (shouldShowPanel(model_, ui::UiShellSlot::Bottom, input_.panels.bottom, input_.windowWidth, input_.windowHeight))
		centerColumn->addWidget(buildPanelSlot(ui::UiShellSlot::Bottom, "bottomPanel"));
	mainArea->addWidget(centerColumn);

	if (shouldShowPanel(model_, ui::UiShellSlot::Right, input_.panels.right, input_.windowWidth, input_.windowHeight))
		mainArea->addWidget(buildPanelSlot(ui::UiShellSlot::Right, "rightPanel"));

	layout->addWidget(mainArea, 1);
	return body;
}

QWidget *IggyQtShellWindow::buildRail()
{
	auto *rail = makeFrame("activityRail");
	rail->setFixedWidth(52);
	auto *layout = new QVBoxLayout(rail);
	layout->setContentsMargins(8, 8, 8, 8);
	layout->setSpacing(6);

	auto *runtime = makeRailButton(QStringLiteral("R"), QStringLiteral("Runtime"), true);
	auto *settings = makeRailButton(QStringLiteral("S"), QStringLiteral("Settings"), false);
	layout->addWidget(runtime);
	layout->addWidget(settings);
	layout->addStretch(1);
	auto *add = makeRailButton(QStringLiteral("+"), QStringLiteral("Reserved"), false);
	auto *help = makeRailButton(QStringLiteral("?"), QStringLiteral("Help"), false);
	add->setEnabled(false);
	help->setEnabled(false);
	layout->addWidget(add);
	layout->addWidget(help);

	connect(runtime, &QPushButton::clicked, this, [this]() {
		refreshBody();
	});
	connect(settings, &QPushButton::clicked, this, [this]() {
		openSettingsWindow();
	});
	return rail;
}

QWidget *IggyQtShellWindow::buildMainSlot()
{
	auto *main = makeFrame("mainSlot");
	auto *layout = new QVBoxLayout(main);
	layout->setContentsMargins(14, 12, 14, 12);
	layout->setSpacing(10);

	auto *header = new QWidget;
	auto *headerLayout = new QHBoxLayout(header);
	headerLayout->setContentsMargins(0, 0, 0, 0);
	headerLayout->setSpacing(8);
	headerLayout->addWidget(makeLabel(QStringLiteral("Runtime Workspace"), "panelTitle"));
	headerLayout->addStretch(1);
	layout->addWidget(header);

	auto *stage = makeFrame("canvasStage");
	auto *stageLayout = new QVBoxLayout(stage);
	stageLayout->setContentsMargins(12, 12, 12, 12);
	stageLayout->setSpacing(10);

	auto *toolbar = new QWidget;
	auto *toolbarLayout = new QHBoxLayout(toolbar);
	toolbarLayout->setContentsMargins(0, 0, 0, 0);
	toolbarLayout->setSpacing(6);
	toolbarLayout->addWidget(makeSectionLabel(QStringLiteral("Viewport")));
	toolbarLayout->addStretch(1);
	toolbarLayout->addWidget(makeChromeButton(QStringLiteral("Grid")));
	toolbarLayout->addWidget(makeChromeButton(QStringLiteral("Snap")));
	toolbarLayout->addWidget(makeChromeButton(QStringLiteral("Overlays")));
	stageLayout->addWidget(toolbar);

	auto *well = makeFrame("canvasWell");
	auto *wellLayout = new QVBoxLayout(well);
	wellLayout->setContentsMargins(14, 14, 14, 14);
	wellLayout->setSpacing(8);
	wellLayout->addStretch(1);
	stageLayout->addWidget(well, 1);
	layout->addWidget(stage, 1);

	layout->addWidget(buildPaletteStrip());
	layout->addWidget(buildToolBelt());
	return main;
}

QWidget *IggyQtShellWindow::buildPanelSlot(ui::UiShellSlot slot, const char *objectName)
{
	auto *panel = makeFrame(objectName);
	panel->setMinimumWidth(slot == ui::UiShellSlot::Bottom ? 0 : 240);
	if (slot != ui::UiShellSlot::Bottom)
		panel->setMaximumWidth(380);
	auto *layout = new QVBoxLayout(panel);
	layout->setContentsMargins(12, 10, 12, 10);
	layout->setSpacing(8);

	const ui::UiPanelVisibility visibility = ui::uiPanelVisibility(
		slot,
		panelStateFor(input_.panels, slot),
		input_.windowWidth,
		input_.windowHeight);
	auto *header = new QWidget;
	auto *headerLayout = new QHBoxLayout(header);
	headerLayout->setContentsMargins(0, 0, 0, 0);
	headerLayout->setSpacing(6);
	headerLayout->addWidget(makeLabel(slotName(slot), "panelTitle"));
	headerLayout->addStretch(1);
	headerLayout->addWidget(makeLabel(panelVisibilityName(visibility), "badgeLabel"));
	layout->addWidget(header);

	bool added = false;
	for (const ui::UiMountedPanel &mounted : model_.mountedPanels) {
		if (mounted.slot != slot || mounted.hidden)
			continue;
		layout->addWidget(makeSectionLabel(toQString(mounted.label)));
		layout->addWidget(buildPanelContent(mounted), 1);
		added = true;
	}
	if (!added) {
		layout->addWidget(makeLabel(QStringLiteral("No panel content assigned here."), "mutedText"));
		layout->addStretch(1);
	}
	return panel;
}

QWidget *IggyQtShellWindow::buildPanelContent(const ui::UiMountedPanel &panel)
{
	auto *content = new QWidget;
	auto *layout = new QVBoxLayout(content);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(6);

	if (panel.groupId == id("panel:runtime_frame")) {
		for (const ui::UiRuntimeFrameInspectorRow &row : model_.runtimeInspector.rows) {
			auto *line = new QWidget;
			auto *lineLayout = new QHBoxLayout(line);
			lineLayout->setContentsMargins(0, 2, 0, 2);
			lineLayout->setSpacing(8);
			lineLayout->addWidget(makeLabel(toQString(row.key), "fieldLabel"));
			lineLayout->addStretch(1);
			lineLayout->addWidget(makeLabel(toQString(row.value), "mutedText"));
			layout->addWidget(line);
		}
	} else if (panel.groupId == id("panel:interaction_events")) {
		for (const ui::UiInteractionEventRow &row : model_.interactionEvents.rows) {
			layout->addWidget(makeLabel(
				QStringLiteral("%1 %2").arg(toQString(row.typeLabel), toQString(row.detail)),
				"mutedText"));
		}
	} else if (panel.groupId == id("panel:inventory")) {
		layout->addWidget(makeSectionLabel(QStringLiteral("Inventory")));
		layout->addWidget(makeLabel(QStringLiteral("Empty"), "mutedText"));
	} else if (panel.groupId == id("panel:collision")) {
		layout->addWidget(makeSectionLabel(QStringLiteral("Collision")));
		layout->addWidget(makeLabel(QStringLiteral("No debug draw source"), "mutedText"));
	}
	layout->addStretch(1);
	return makeScrollHost(content);
}

QWidget *IggyQtShellWindow::buildToolBelt()
{
	auto *host = new QWidget;
	host->setObjectName(QStringLiteral("toolBelt"));
	auto *layout = new QHBoxLayout(host);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(6);
	for (const ui::UiRuntimeWorkspaceToolView &tool : model_.availableTools) {
		auto *button = makeToolButton(toQString(tool.label), tool.active);
		button->setObjectName(QStringLiteral("toolChip"));
		connect(button, &QPushButton::clicked, this, [this, tool]() {
			context_.activeToolId = tool.toolId;
			refreshAfterModelChange();
		});
		layout->addWidget(button);
	}
	layout->addStretch(1);
	return host;
}

QWidget *IggyQtShellWindow::buildPaletteStrip()
{
	auto *host = new QWidget;
	auto *layout = new QHBoxLayout(host);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(8);
	for (const ui::UiMountedPalette &palette : model_.mountedPalettes) {
		auto *frame = makeFrame("floatingPalette");
		auto *paletteLayout = new QHBoxLayout(frame);
		paletteLayout->setContentsMargins(8, 6, 8, 6);
		paletteLayout->setSpacing(6);
		paletteLayout->addWidget(makeFrame("paletteGrip"));
		paletteLayout->addWidget(makeLabel(toQString(palette.label), "mutedText"));
		paletteLayout->addWidget(makeLabel(
			QStringLiteral("(%1, %2)").arg(palette.placement.x).arg(palette.placement.y),
			"mutedText"));
		layout->addWidget(frame);
	}
	layout->addStretch(1);
	return host;
}

QWidget *IggyQtShellWindow::buildSettings()
{
	auto *host = new QWidget;
	auto *layout = new QHBoxLayout(host);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	auto *strip = makeFrame("settingsSidebar");
	strip->setFixedWidth(170);
	auto *stripLayout = new QVBoxLayout(strip);
	stripLayout->setContentsMargins(8, 8, 8, 8);
	stripLayout->setSpacing(6);

	auto *stack = new QStackedWidget;
	auto *group = new QButtonGroup(host);
	group->setExclusive(true);
	const std::vector<ui::UiSettingsPageDescriptor> pages = ui::defaultUiSettingsPages();
	for (std::size_t index = 0; index < pages.size(); ++index) {
		QWidget *page = buildSettingsPage(pages[index].id);
		stack->addWidget(page);
		auto *button = makeToolButton(toQString(pages[index].label), settings_.activePageId == pages[index].id);
		group->addButton(button);
		stripLayout->addWidget(button);
		connect(button, &QPushButton::clicked, this, [this, stack, index, pageId = pages[index].id]() {
			settings_.activePageId = pageId;
			stack->setCurrentIndex(static_cast<int>(index));
		});
		if (settings_.activePageId == pages[index].id)
			stack->setCurrentIndex(static_cast<int>(index));
	}
	stripLayout->addStretch(1);

	layout->addWidget(strip);
	layout->addWidget(stack, 1);
	return host;
}

QWidget *IggyQtShellWindow::buildSettingsPage(const ResourceId &pageId)
{
	if (pageId == id("settings:theme"))
		return buildThemeSettingsPage();
	if (pageId == id("settings:tool_belt"))
		return buildToolBeltSettingsPage();
	if (pageId == id("settings:panels"))
		return buildPanelSettingsPage();

	auto *page = new QWidget;
	page->setObjectName(QStringLiteral("settingsPage"));
	auto *layout = new QVBoxLayout(page);
	layout->addWidget(makeLabel(QStringLiteral("Unknown settings page"), "panelTitle"));
	layout->addStretch(1);
	return page;
}

QWidget *IggyQtShellWindow::buildThemeSettingsPage()
{
	auto *page = new QWidget;
	page->setObjectName(QStringLiteral("settingsPage"));
	auto *layout = new QVBoxLayout(page);
	layout->setContentsMargins(14, 14, 14, 14);
	layout->setSpacing(8);
	layout->addWidget(makeLabel(QStringLiteral("Theme"), "panelTitle"));
	layout->addWidget(makeSectionLabel(QStringLiteral("Colors")));

	addThemeColorRow(layout, QStringLiteral("Base"), settings_.theme.base, [this](const std::string &value) {
		settings_.theme.base = value;
	});
	addThemeColorRow(layout, QStringLiteral("Surface"), settings_.theme.surface, [this](const std::string &value) {
		settings_.theme.surface = value;
	});
	addThemeColorRow(layout, QStringLiteral("Accent"), settings_.theme.accent, [this](const std::string &value) {
		settings_.theme.accent = value;
	});
	addThemeColorRow(layout, QStringLiteral("Text"), settings_.theme.text, [this](const std::string &value) {
		settings_.theme.text = value;
	});

	layout->addWidget(makeLabel(QStringLiteral("Typography"), "panelTitle"));
	addThemeFontRow(
		layout,
		QStringLiteral("UI"),
		settings_.theme.uiFont,
		settings_.theme.uiFontSize,
		[this](const std::string &value) { settings_.theme.uiFont = value; },
		[this](int value) { settings_.theme.uiFontSize = value; });
	addThemeFontRow(
		layout,
		QStringLiteral("Code"),
		settings_.theme.codeFont,
		settings_.theme.codeFontSize,
		[this](const std::string &value) { settings_.theme.codeFont = value; },
		[this](int value) { settings_.theme.codeFontSize = value; });

	layout->addWidget(makeLabel(QStringLiteral("Profiles: hooks reserved for persistence."), "mutedText"));
	layout->addStretch(1);
	return page;
}

void IggyQtShellWindow::addThemeColorRow(
	QVBoxLayout *layout,
	const QString &label,
	const std::string &value,
	std::function<void(const std::string &)> apply)
{
	auto *row = new QWidget;
	row->setObjectName(QStringLiteral("settingsRow"));
	auto *rowLayout = new QHBoxLayout(row);
	rowLayout->setContentsMargins(8, 6, 8, 6);
	rowLayout->setSpacing(8);
	rowLayout->addWidget(makeLabel(label, "fieldLabel"));
	auto *field = new QLineEdit(toQString(value));
	rowLayout->addWidget(field, 1);

	auto *swatch = new QPushButton;
	swatch->setEnabled(false);
	swatch->setStyleSheet(swatchStyle(value, ui::deriveUiThemeTokens(settings_.theme)));
	rowLayout->addWidget(swatch);

	connect(field, &QLineEdit::editingFinished, this, [this, field, swatch, apply = std::move(apply)]() {
		const QString text = field->text();
		if (!text.startsWith('#') || text.size() != 7 || !QColor(text).isValid())
			return;
		apply(text.toStdString());
		applyTheme();
		swatch->setStyleSheet(swatchStyle(text.toStdString(), ui::deriveUiThemeTokens(settings_.theme)));
	});
	layout->addWidget(row);
}

void IggyQtShellWindow::addThemeFontRow(
	QVBoxLayout *layout,
	const QString &label,
	const std::string &fontFamily,
	int fontSize,
	std::function<void(const std::string &)> applyFont,
	std::function<void(int)> applySize)
{
	auto *row = new QWidget;
	row->setObjectName(QStringLiteral("settingsRow"));
	auto *rowLayout = new QHBoxLayout(row);
	rowLayout->setContentsMargins(8, 6, 8, 6);
	rowLayout->setSpacing(8);
	rowLayout->addWidget(makeLabel(label, "fieldLabel"));

	auto *font = new QFontComboBox;
	font->setCurrentFont(QFont(toQString(fontFamily)));
	rowLayout->addWidget(font, 1);
	auto *size = new QSpinBox;
	size->setRange(9, 28);
	size->setValue(fontSize);
	rowLayout->addWidget(size);

	connect(font, &QFontComboBox::currentFontChanged, this, [this, applyFont = std::move(applyFont)](const QFont &selected) {
		applyFont(selected.family().toStdString());
		applyTheme();
	});
	connect(size, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this, applySize = std::move(applySize)](int value) {
		applySize(value);
		applyTheme();
	});
	layout->addWidget(row);
}

QWidget *IggyQtShellWindow::buildToolBeltSettingsPage()
{
	auto *page = new QWidget;
	page->setObjectName(QStringLiteral("settingsPage"));
	auto *layout = new QVBoxLayout(page);
	layout->setContentsMargins(14, 14, 14, 14);
	layout->setSpacing(6);
	layout->addWidget(makeLabel(QStringLiteral("Tool Belt"), "panelTitle"));
	layout->addWidget(makeSectionLabel(QStringLiteral("Visible Tools")));

	for (const ui::UiToolDescriptor &tool : inventory_.tools) {
		auto *row = new QWidget;
		row->setObjectName(QStringLiteral("settingsRow"));
		auto *rowLayout = new QHBoxLayout(row);
		rowLayout->setContentsMargins(8, 6, 8, 6);
		rowLayout->setSpacing(8);
		auto *box = new QCheckBox(toQString(tool.label));
		box->setChecked(ui::uiToolIdEnabled(settings_.enabledToolIds, tool.id));
		connect(box, &QCheckBox::toggled, this, [this, tool](bool checked) {
			if (checked && !ui::uiToolIdEnabled(settings_.enabledToolIds, tool.id)) {
				settings_.enabledToolIds.push_back(tool.id);
			} else if (!checked) {
				std::vector<ResourceId> next;
				for (const ResourceId &toolId : settings_.enabledToolIds) {
					if (toolId != tool.id)
						next.push_back(toolId);
				}
				settings_.enabledToolIds = next;
			}
			refreshAfterModelChange();
		});
		rowLayout->addWidget(box);
		layout->addWidget(row);
	}
	layout->addStretch(1);
	return page;
}

QWidget *IggyQtShellWindow::buildPanelSettingsPage()
{
	auto *page = new QWidget;
	page->setObjectName(QStringLiteral("settingsPage"));
	auto *layout = new QVBoxLayout(page);
	layout->setContentsMargins(14, 14, 14, 14);
	layout->setSpacing(8);
	layout->addWidget(makeLabel(QStringLiteral("Panels"), "panelTitle"));
	layout->addWidget(makeSectionLabel(QStringLiteral("Assignments")));

	for (const ui::UiMountedPanel &panel : model_.mountedPanels) {
		auto *row = new QWidget;
		row->setObjectName(QStringLiteral("settingsRow"));
		auto *rowLayout = new QHBoxLayout(row);
		rowLayout->setContentsMargins(8, 6, 8, 6);
		rowLayout->setSpacing(8);
		rowLayout->addWidget(makeLabel(toQString(panel.label), "fieldLabel"), 1);
		auto *combo = new QComboBox;
		combo->addItem(QStringLiteral("Right"), static_cast<int>(ui::UiShellSlot::Right));
		combo->addItem(QStringLiteral("Left"), static_cast<int>(ui::UiShellSlot::Left));
		combo->addItem(QStringLiteral("Bottom"), static_cast<int>(ui::UiShellSlot::Bottom));
		combo->addItem(QStringLiteral("Hidden"), -1);
		combo->setCurrentIndex(panel.hidden ? 3 : combo->findData(static_cast<int>(panel.slot)));
		connect(combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, [this, panel, combo](int index) {
			ui::UiPanelContentAssignment assignment;
			assignment.groupId = panel.groupId;
			assignment.hidden = combo->itemData(index).toInt() < 0;
			assignment.slot = assignment.hidden
				? panel.slot
				: static_cast<ui::UiShellSlot>(combo->itemData(index).toInt());
			setPanelContentAssignment(settings_.panelContent, assignment);
			refreshAfterModelChange();
		});
		rowLayout->addWidget(combo);
		layout->addWidget(row);
	}
	layout->addStretch(1);
	return page;
}

} // namespace iggy::qt_shell
