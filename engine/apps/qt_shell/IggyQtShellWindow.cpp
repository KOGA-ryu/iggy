#include "IggyQtShellWindow.hpp"

#include <QButtonGroup>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QFontComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>

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

} // namespace

IggyQtShellWindow::IggyQtShellWindow()
{
	settings_ = ui::defaultUiSettingsState(inventory_);
	input_ = ui::defaultUiRuntimeWorkspaceModelInput(inventory_);
	context_.activeToolId = id("tool:select");
	rebuild();
}

void IggyQtShellWindow::rebuild()
{
	rebuildModel();
	applyTheme();

	auto *root = new QWidget;
	auto *rootLayout = new QVBoxLayout(root);
	rootLayout->setContentsMargins(0, 0, 0, 0);
	rootLayout->setSpacing(0);
	rootLayout->addWidget(buildChrome());
	rootLayout->addWidget(buildBody(), 1);
	setCentralWidget(root);
	resize(1180, 760);
	setWindowTitle(QStringLiteral("Iggy Qt Shell"));
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
}

QWidget *IggyQtShellWindow::buildChrome()
{
	auto *chrome = makeFrame("topChrome");
	auto *layout = new QHBoxLayout(chrome);
	layout->setContentsMargins(8, 6, 8, 6);
	layout->setSpacing(6);

	auto *leftToggle = new QPushButton(QStringLiteral("Left"));
	auto *back = new QPushButton(QStringLiteral("Back"));
	auto *forward = new QPushButton(QStringLiteral("Forward"));
	auto *settings = new QPushButton(QStringLiteral("Settings"));
	auto *bottomToggle = new QPushButton(QStringLiteral("Bottom"));
	auto *rightToggle = new QPushButton(QStringLiteral("Right"));

	layout->addWidget(leftToggle);
	layout->addWidget(back);
	layout->addWidget(forward);
	layout->addStretch(1);
	for (const ui::UiMountedChromePanel &panel : model_.mountedChromePanels)
		layout->addWidget(new QPushButton(toQString(panel.label)));
	layout->addWidget(settings);
	layout->addStretch(1);
	layout->addWidget(bottomToggle);
	layout->addWidget(rightToggle);

	connect(settings, &QPushButton::clicked, this, [this]() {
		showSettings_ = !showSettings_;
		rebuild();
	});
	connect(leftToggle, &QPushButton::clicked, this, [this]() {
		input_.panels.left.collapsed = !input_.panels.left.collapsed;
		rebuild();
	});
	connect(rightToggle, &QPushButton::clicked, this, [this]() {
		input_.panels.right.collapsed = !input_.panels.right.collapsed;
		rebuild();
	});
	connect(bottomToggle, &QPushButton::clicked, this, [this]() {
		input_.panels.bottom.collapsed = !input_.panels.bottom.collapsed;
		rebuild();
	});
	return chrome;
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
	if (shouldShowPanel(model_, ui::UiShellSlot::Left, input_.panels.left, input_.windowWidth, input_.windowHeight))
		mainArea->addWidget(buildPanelSlot(ui::UiShellSlot::Left, "leftPanel"));

	auto *centerColumn = new QSplitter(Qt::Vertical);
	centerColumn->setChildrenCollapsible(false);
	centerColumn->addWidget(showSettings_ ? buildSettings() : buildMainSlot());
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
	auto *layout = new QVBoxLayout(rail);
	layout->setContentsMargins(6, 8, 6, 8);
	layout->setSpacing(6);

	auto *runtime = makeToolButton(QStringLiteral("R"), !showSettings_);
	auto *settings = makeToolButton(QStringLiteral("S"), showSettings_);
	layout->addWidget(runtime);
	layout->addWidget(settings);
	layout->addStretch(1);

	connect(runtime, &QPushButton::clicked, this, [this]() {
		showSettings_ = false;
		rebuild();
	});
	connect(settings, &QPushButton::clicked, this, [this]() {
		showSettings_ = true;
		rebuild();
	});
	return rail;
}

QWidget *IggyQtShellWindow::buildMainSlot()
{
	auto *main = makeFrame("mainSlot");
	auto *layout = new QVBoxLayout(main);
	layout->setContentsMargins(14, 14, 14, 14);
	layout->setSpacing(10);
	layout->addWidget(makeLabel(QStringLiteral("Runtime Workspace"), "panelTitle"));
	layout->addWidget(makeLabel(QStringLiteral("Mounted shell slots and runtime projections."), "mutedText"));

	auto *list = new QListWidget;
	for (const ui::UiMountedSlot &slot : model_.mountedSlots)
		list->addItem(QStringLiteral("%1 -> %2").arg(slotName(slot.slot), toQString(slot.featureId)));
	layout->addWidget(list, 1);

	layout->addWidget(buildPaletteStrip());
	layout->addWidget(buildToolBelt());
	return main;
}

QWidget *IggyQtShellWindow::buildPanelSlot(ui::UiShellSlot slot, const char *objectName)
{
	auto *panel = makeFrame(objectName);
	auto *layout = new QVBoxLayout(panel);
	layout->setContentsMargins(12, 12, 12, 12);
	layout->setSpacing(8);

	const ui::UiPanelVisibility visibility = ui::uiPanelVisibility(
		slot,
		panelStateFor(input_.panels, slot),
		input_.windowWidth,
		input_.windowHeight);
	layout->addWidget(makeLabel(QStringLiteral("%1 panel (%2)").arg(slotName(slot), panelVisibilityName(visibility)), "panelTitle"));

	bool added = false;
	for (const ui::UiMountedPanel &mounted : model_.mountedPanels) {
		if (mounted.slot != slot || mounted.hidden)
			continue;
		layout->addWidget(makeLabel(toQString(mounted.label), "panelTitle"));
		layout->addWidget(makeLabel(QStringLiteral("Feature: %1").arg(toQString(mounted.featureId)), "mutedText"));
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
		for (const ui::UiRuntimeFrameInspectorRow &row : model_.runtimeInspector.rows)
			layout->addWidget(makeLabel(QStringLiteral("%1: %2").arg(toQString(row.key), toQString(row.value)), "mutedText"));
	} else if (panel.groupId == id("panel:interaction_events")) {
		for (const ui::UiInteractionEventRow &row : model_.interactionEvents.rows) {
			layout->addWidget(makeLabel(
				QStringLiteral("%1 %2 %3").arg(toQString(row.typeLabel), toQString(row.targetId), toQString(row.detail)),
				"mutedText"));
		}
	} else if (panel.groupId == id("panel:inventory")) {
		layout->addWidget(makeLabel(QStringLiteral("Inventory panel placeholder."), "mutedText"));
	} else if (panel.groupId == id("panel:collision")) {
		layout->addWidget(makeLabel(QStringLiteral("Collision debug panel placeholder."), "mutedText"));
	}
	layout->addStretch(1);
	return makeScrollHost(content);
}

QWidget *IggyQtShellWindow::buildToolBelt()
{
	auto *host = new QWidget;
	auto *layout = new QHBoxLayout(host);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(6);
	for (const ui::UiRuntimeWorkspaceToolView &tool : model_.availableTools) {
		auto *button = makeToolButton(toQString(tool.label), tool.active);
		connect(button, &QPushButton::clicked, this, [this, tool]() {
			context_.activeToolId = tool.toolId;
			rebuild();
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

	auto *strip = makeFrame("leftPanel");
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
	auto *layout = new QVBoxLayout(page);
	layout->addWidget(makeLabel(QStringLiteral("Unknown settings page"), "panelTitle"));
	layout->addStretch(1);
	return page;
}

QWidget *IggyQtShellWindow::buildThemeSettingsPage()
{
	auto *page = new QWidget;
	auto *layout = new QVBoxLayout(page);
	layout->setContentsMargins(14, 14, 14, 14);
	layout->setSpacing(8);
	layout->addWidget(makeLabel(QStringLiteral("Theme"), "panelTitle"));

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
	auto *rowLayout = new QHBoxLayout(row);
	rowLayout->setContentsMargins(0, 0, 0, 0);
	rowLayout->setSpacing(8);
	rowLayout->addWidget(makeLabel(label, "fieldLabel"));
	auto *field = new QLineEdit(toQString(value));
	rowLayout->addWidget(field, 1);

	auto *swatch = new QPushButton;
	swatch->setEnabled(false);
	swatch->setStyleSheet(swatchStyle(value, ui::deriveUiThemeTokens(settings_.theme)));
	rowLayout->addWidget(swatch);

	connect(field, &QLineEdit::editingFinished, this, [this, field, apply = std::move(apply)]() {
		const QString text = field->text();
		if (!text.startsWith('#') || text.size() != 7 || !QColor(text).isValid())
			return;
		apply(text.toStdString());
		rebuild();
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
	auto *rowLayout = new QHBoxLayout(row);
	rowLayout->setContentsMargins(0, 0, 0, 0);
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
		rebuild();
	});
	connect(size, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this, applySize = std::move(applySize)](int value) {
		applySize(value);
		rebuild();
	});
	layout->addWidget(row);
}

QWidget *IggyQtShellWindow::buildToolBeltSettingsPage()
{
	auto *page = new QWidget;
	auto *layout = new QVBoxLayout(page);
	layout->setContentsMargins(14, 14, 14, 14);
	layout->setSpacing(6);
	layout->addWidget(makeLabel(QStringLiteral("Tool Belt"), "panelTitle"));

	for (const ui::UiToolDescriptor &tool : inventory_.tools) {
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
			rebuild();
		});
		layout->addWidget(box);
	}
	layout->addStretch(1);
	return page;
}

QWidget *IggyQtShellWindow::buildPanelSettingsPage()
{
	auto *page = new QWidget;
	auto *layout = new QVBoxLayout(page);
	layout->setContentsMargins(14, 14, 14, 14);
	layout->setSpacing(8);
	layout->addWidget(makeLabel(QStringLiteral("Panels"), "panelTitle"));

	for (const ui::UiMountedPanel &panel : model_.mountedPanels) {
		auto *row = new QWidget;
		auto *rowLayout = new QHBoxLayout(row);
		rowLayout->setContentsMargins(0, 0, 0, 0);
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
			rebuild();
		});
		rowLayout->addWidget(combo);
		layout->addWidget(row);
	}
	layout->addStretch(1);
	return page;
}

} // namespace iggy::qt_shell
