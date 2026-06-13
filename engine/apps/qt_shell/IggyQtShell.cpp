#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QStackedWidget>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "scene/ui/UiRuntimeWorkspaceModel.hpp"
#include "scene/ui/UiSettingsState.hpp"
#include "scene/ui/UiTheme.hpp"

namespace {

QString Q(const std::string &text)
{
	return QString::fromStdString(text);
}

QString Q(iggy::ResourceId id)
{
	return QString::fromStdString(std::string { id.value() });
}

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

QString StyleSheet(const iggy::ui::UiThemeTokens &theme)
{
	return QString(R"(
		QWidget {
			background: %1;
			color: %2;
			font-family: "%3", "Inter", sans-serif;
			font-size: %4px;
		}
		QFrame#topChrome {
			background: %5;
			border-bottom: 1px solid %6;
		}
		QFrame#activityRail {
			background: %1;
			border-right: 1px solid %6;
		}
		QFrame#leftPanel, QFrame#rightPanel {
			background: %5;
			border-left: 1px solid %7;
			border-right: 1px solid %6;
		}
		QFrame#bottomPanel {
			background: %1;
			border-top: 1px solid %6;
		}
		QFrame#mainSlot {
			background: %8;
		}
		QLabel#panelTitle {
			color: %2;
			font-size: %9px;
			font-weight: 600;
		}
		QLabel#fieldLabel, QLabel#mutedText {
			color: %10;
		}
		QPushButton {
			background: transparent;
			color: %2;
			border: 1px solid transparent;
			border-radius: 5px;
			padding: 4px 8px;
			min-height: 20px;
			text-align: left;
		}
		QPushButton:hover {
			background: %11;
			border-color: %6;
		}
		QPushButton:checked, QPushButton:pressed {
			background: %12;
			border-color: %13;
			font-weight: 600;
		}
		QLineEdit, QComboBox, QListWidget {
			background: %14;
			color: %2;
			border: 1px solid %6;
			border-radius: 5px;
			padding: 4px 6px;
		}
		QListWidget::item:selected {
			background: %15;
			color: %2;
		}
	)")
		.arg(Q(theme.base))
		.arg(Q(theme.text))
		.arg(Q(theme.uiFont))
		.arg(theme.fontSizeBody)
		.arg(Q(theme.surface))
		.arg(Q(theme.borderMajor))
		.arg(Q(theme.borderMinor))
		.arg(Q(theme.workspaceBody))
		.arg(theme.fontSizeTitle)
		.arg(Q(theme.textMuted))
		.arg(Q(theme.controlHover))
		.arg(Q(theme.selected))
		.arg(Q(theme.borderFocus))
		.arg(Q(theme.control))
		.arg(Q(theme.rowSelected));
}

QLabel *Label(const QString &text, const char *objectName = nullptr)
{
	auto *label = new QLabel(text);
	if (objectName != nullptr)
		label->setObjectName(QString::fromUtf8(objectName));
	return label;
}

QPushButton *ToolButton(const QString &label, bool checked = false)
{
	auto *button = new QPushButton(label);
	button->setCheckable(true);
	button->setChecked(checked);
	return button;
}

void SetPanelContentAssignment(
	std::vector<iggy::ui::UiPanelContentAssignment> &assignments,
	iggy::ui::UiPanelContentAssignment assignment)
{
	for (iggy::ui::UiPanelContentAssignment &existing : assignments) {
		if (existing.groupId == assignment.groupId) {
			existing = assignment;
			return;
		}
	}
	assignments.push_back(assignment);
}

QFrame *Frame(const char *objectName)
{
	auto *frame = new QFrame;
	frame->setObjectName(QString::fromUtf8(objectName));
	return frame;
}

QWidget *ScrollHost(QWidget *content)
{
	auto *scroll = new QScrollArea;
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setWidget(content);
	return scroll;
}

class IggyQtShellWindow final : public QMainWindow {
public:
	IggyQtShellWindow()
	{
		settings_ = iggy::ui::defaultUiSettingsState(inventory_);
		input_ = iggy::ui::defaultUiRuntimeWorkspaceModelInput(inventory_);
		context_.activeToolId = Id("tool:select");
		Rebuild();
	}

private:
	void Rebuild()
	{
		input_.settings = settings_;
		input_.context = context_;
		model_ = iggy::ui::buildUiRuntimeWorkspaceModel(input_);
		setStyleSheet(StyleSheet(iggy::ui::deriveUiThemeTokens(settings_.theme)));

		auto *root = new QWidget;
		auto *rootLayout = new QVBoxLayout(root);
		rootLayout->setContentsMargins(0, 0, 0, 0);
		rootLayout->setSpacing(0);
		rootLayout->addWidget(BuildChrome());
		rootLayout->addWidget(BuildBody(), 1);
		setCentralWidget(root);
		resize(1180, 760);
		setWindowTitle(QStringLiteral("Iggy Qt Shell"));
	}

	QWidget *BuildChrome()
	{
		auto *chrome = Frame("topChrome");
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
		for (const iggy::ui::UiMountedChromePanel &panel : model_.mountedChromePanels)
			layout->addWidget(new QPushButton(Q(panel.label)));
		layout->addWidget(settings);
		layout->addStretch(1);
		layout->addWidget(bottomToggle);
		layout->addWidget(rightToggle);

		connect(settings, &QPushButton::clicked, this, [this]() {
			showSettings_ = !showSettings_;
			Rebuild();
		});
		connect(leftToggle, &QPushButton::clicked, this, [this]() {
			input_.panels.left.collapsed = !input_.panels.left.collapsed;
			Rebuild();
		});
		connect(rightToggle, &QPushButton::clicked, this, [this]() {
			input_.panels.right.collapsed = !input_.panels.right.collapsed;
			Rebuild();
		});
		connect(bottomToggle, &QPushButton::clicked, this, [this]() {
			input_.panels.bottom.collapsed = !input_.panels.bottom.collapsed;
			Rebuild();
		});
		return chrome;
	}

	QWidget *BuildBody()
	{
		auto *body = new QWidget;
		auto *layout = new QHBoxLayout(body);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);
		layout->addWidget(BuildRail());

		auto *mainArea = new QSplitter(Qt::Horizontal);
		mainArea->setChildrenCollapsible(false);
		if (!input_.panels.left.collapsed)
			mainArea->addWidget(BuildPanelSlot(iggy::ui::UiShellSlot::Left, "leftPanel"));

		auto *centerColumn = new QSplitter(Qt::Vertical);
		centerColumn->setChildrenCollapsible(false);
		centerColumn->addWidget(showSettings_ ? BuildSettings() : BuildMainSlot());
		if (!input_.panels.bottom.collapsed)
			centerColumn->addWidget(BuildPanelSlot(iggy::ui::UiShellSlot::Bottom, "bottomPanel"));
		mainArea->addWidget(centerColumn);

		if (!input_.panels.right.collapsed)
			mainArea->addWidget(BuildPanelSlot(iggy::ui::UiShellSlot::Right, "rightPanel"));

		layout->addWidget(mainArea, 1);
		return body;
	}

	QWidget *BuildRail()
	{
		auto *rail = Frame("activityRail");
		auto *layout = new QVBoxLayout(rail);
		layout->setContentsMargins(6, 8, 6, 8);
		layout->setSpacing(6);
		layout->addWidget(ToolButton(QStringLiteral("R"), !showSettings_));
		layout->addWidget(ToolButton(QStringLiteral("S"), showSettings_));
		layout->addStretch(1);
		return rail;
	}

	QWidget *BuildMainSlot()
	{
		auto *main = Frame("mainSlot");
		auto *layout = new QVBoxLayout(main);
		layout->setContentsMargins(14, 14, 14, 14);
		layout->setSpacing(10);
		layout->addWidget(Label(QStringLiteral("Runtime Workspace"), "panelTitle"));
		layout->addWidget(Label(QStringLiteral("Mounted shell slots and runtime projections."), "mutedText"));

		auto *list = new QListWidget;
		for (const iggy::ui::UiMountedSlot &slot : model_.mountedSlots)
			list->addItem(QStringLiteral("%1 -> %2").arg(SlotName(slot.slot), Q(slot.featureId)));
		layout->addWidget(list, 1);

		layout->addWidget(BuildToolBelt());
		return main;
	}

	QWidget *BuildPanelSlot(iggy::ui::UiShellSlot slot, const char *objectName)
	{
		auto *panel = Frame(objectName);
		auto *layout = new QVBoxLayout(panel);
		layout->setContentsMargins(12, 12, 12, 12);
		layout->setSpacing(8);

		bool added = false;
		for (const iggy::ui::UiMountedPanel &mounted : model_.mountedPanels) {
			if (mounted.slot != slot || mounted.hidden)
				continue;
			layout->addWidget(Label(Q(mounted.label), "panelTitle"));
			layout->addWidget(Label(QStringLiteral("Feature: %1").arg(Q(mounted.featureId)), "mutedText"));
			layout->addWidget(BuildPanelContent(mounted), 1);
			added = true;
		}
		if (!added) {
			layout->addWidget(Label(QStringLiteral("Empty"), "panelTitle"));
			layout->addWidget(Label(QStringLiteral("No panel content assigned here."), "mutedText"));
			layout->addStretch(1);
		}
		return panel;
	}

	QWidget *BuildPanelContent(const iggy::ui::UiMountedPanel &panel)
	{
		auto *content = new QWidget;
		auto *layout = new QVBoxLayout(content);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(6);

		if (panel.groupId == Id("panel:runtime_frame")) {
			for (const iggy::ui::UiRuntimeFrameInspectorRow &row : model_.runtimeInspector.rows)
				layout->addWidget(Label(QStringLiteral("%1: %2").arg(Q(row.key), Q(row.value)), "mutedText"));
		} else if (panel.groupId == Id("panel:interaction_events")) {
			for (const iggy::ui::UiInteractionEventRow &row : model_.interactionEvents.rows) {
				layout->addWidget(Label(
					QStringLiteral("%1 %2 %3").arg(Q(row.typeLabel), Q(row.targetId), Q(row.detail)),
					"mutedText"));
			}
		} else if (panel.groupId == Id("panel:inventory")) {
			layout->addWidget(Label(QStringLiteral("Inventory panel placeholder."), "mutedText"));
		} else if (panel.groupId == Id("panel:collision")) {
			layout->addWidget(Label(QStringLiteral("Collision debug panel placeholder."), "mutedText"));
		}
		layout->addStretch(1);
		return ScrollHost(content);
	}

	QWidget *BuildToolBelt()
	{
		auto *host = new QWidget;
		auto *layout = new QHBoxLayout(host);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(6);
		for (const iggy::ui::UiRuntimeWorkspaceToolView &tool : model_.availableTools) {
			auto *button = ToolButton(Q(tool.label), tool.active);
			connect(button, &QPushButton::clicked, this, [this, tool]() {
				context_.activeToolId = tool.toolId;
				Rebuild();
			});
			layout->addWidget(button);
		}
		layout->addStretch(1);
		return host;
	}

	QWidget *BuildSettings()
	{
		auto *host = new QWidget;
		auto *layout = new QHBoxLayout(host);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);

		auto *strip = Frame("leftPanel");
		auto *stripLayout = new QVBoxLayout(strip);
		stripLayout->setContentsMargins(8, 8, 8, 8);
		stripLayout->setSpacing(6);

		auto *stack = new QStackedWidget;
		auto *group = new QButtonGroup(host);
		group->setExclusive(true);
		const std::vector<iggy::ui::UiSettingsPageDescriptor> pages = iggy::ui::defaultUiSettingsPages();
		for (std::size_t index = 0; index < pages.size(); ++index) {
			QWidget *page = BuildSettingsPage(pages[index].id);
			stack->addWidget(page);
			auto *button = ToolButton(Q(pages[index].label), settings_.activePageId == pages[index].id);
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

	QWidget *BuildSettingsPage(const iggy::ResourceId &pageId)
	{
		if (pageId == Id("settings:theme"))
			return BuildThemeSettingsPage();
		if (pageId == Id("settings:tool_belt"))
			return BuildToolBeltSettingsPage();
		if (pageId == Id("settings:panels"))
			return BuildPanelSettingsPage();

		auto *page = new QWidget;
		auto *layout = new QVBoxLayout(page);
		layout->addWidget(Label(QStringLiteral("Unknown settings page"), "panelTitle"));
		layout->addStretch(1);
		return page;
	}

	QWidget *BuildThemeSettingsPage()
	{
		auto *page = new QWidget;
		auto *layout = new QVBoxLayout(page);
		layout->setContentsMargins(14, 14, 14, 14);
		layout->setSpacing(8);
		layout->addWidget(Label(QStringLiteral("Theme"), "panelTitle"));

		AddThemeRow(layout, QStringLiteral("Base"), settings_.theme.base, [this](const std::string &value) {
			settings_.theme.base = value;
		});
		AddThemeRow(layout, QStringLiteral("Surface"), settings_.theme.surface, [this](const std::string &value) {
			settings_.theme.surface = value;
		});
		AddThemeRow(layout, QStringLiteral("Accent"), settings_.theme.accent, [this](const std::string &value) {
			settings_.theme.accent = value;
		});
		AddThemeRow(layout, QStringLiteral("Text"), settings_.theme.text, [this](const std::string &value) {
			settings_.theme.text = value;
		});

		layout->addStretch(1);
		return page;
	}

	void AddThemeRow(QVBoxLayout *layout, const QString &label, const std::string &value, std::function<void(const std::string &)> apply)
	{
		auto *row = new QWidget;
		auto *rowLayout = new QHBoxLayout(row);
		rowLayout->setContentsMargins(0, 0, 0, 0);
		rowLayout->setSpacing(8);
		rowLayout->addWidget(Label(label, "fieldLabel"));
		auto *field = new QLineEdit(Q(value));
		rowLayout->addWidget(field, 1);
		connect(field, &QLineEdit::editingFinished, this, [this, field, apply = std::move(apply)]() {
			const QString text = field->text();
			if (!text.startsWith('#') || text.size() != 7)
				return;
			apply(text.toStdString());
			Rebuild();
		});
		layout->addWidget(row);
	}

	QWidget *BuildToolBeltSettingsPage()
	{
		auto *page = new QWidget;
		auto *layout = new QVBoxLayout(page);
		layout->setContentsMargins(14, 14, 14, 14);
		layout->setSpacing(6);
		layout->addWidget(Label(QStringLiteral("Tool Belt"), "panelTitle"));

		for (const iggy::ui::UiToolDescriptor &tool : inventory_.tools) {
			auto *box = new QCheckBox(Q(tool.label));
			box->setChecked(iggy::ui::uiToolIdEnabled(settings_.enabledToolIds, tool.id));
			connect(box, &QCheckBox::toggled, this, [this, tool](bool checked) {
				if (checked && !iggy::ui::uiToolIdEnabled(settings_.enabledToolIds, tool.id)) {
					settings_.enabledToolIds.push_back(tool.id);
				} else if (!checked) {
					std::vector<iggy::ResourceId> next;
					for (const iggy::ResourceId &id : settings_.enabledToolIds) {
						if (id != tool.id)
							next.push_back(id);
					}
					settings_.enabledToolIds = next;
				}
				Rebuild();
			});
			layout->addWidget(box);
		}
		layout->addStretch(1);
		return page;
	}

	QWidget *BuildPanelSettingsPage()
	{
		auto *page = new QWidget;
		auto *layout = new QVBoxLayout(page);
		layout->setContentsMargins(14, 14, 14, 14);
		layout->setSpacing(8);
		layout->addWidget(Label(QStringLiteral("Panels"), "panelTitle"));

		for (const iggy::ui::UiMountedPanel &panel : model_.mountedPanels) {
			auto *row = new QWidget;
			auto *rowLayout = new QHBoxLayout(row);
			rowLayout->setContentsMargins(0, 0, 0, 0);
			rowLayout->setSpacing(8);
			rowLayout->addWidget(Label(Q(panel.label), "fieldLabel"), 1);
			auto *combo = new QComboBox;
			combo->addItem(QStringLiteral("Right"), static_cast<int>(iggy::ui::UiShellSlot::Right));
			combo->addItem(QStringLiteral("Left"), static_cast<int>(iggy::ui::UiShellSlot::Left));
			combo->addItem(QStringLiteral("Bottom"), static_cast<int>(iggy::ui::UiShellSlot::Bottom));
			combo->addItem(QStringLiteral("Hidden"), -1);
			combo->setCurrentIndex(panel.hidden ? 3 : combo->findData(static_cast<int>(panel.slot)));
			connect(combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, [this, panel, combo](int index) {
				iggy::ui::UiPanelContentAssignment assignment;
				assignment.groupId = panel.groupId;
				assignment.hidden = combo->itemData(index).toInt() < 0;
				assignment.slot = assignment.hidden
					? panel.slot
					: static_cast<iggy::ui::UiShellSlot>(combo->itemData(index).toInt());
				SetPanelContentAssignment(settings_.panelContent, assignment);
				Rebuild();
			});
			rowLayout->addWidget(combo);
			layout->addWidget(row);
		}
		layout->addStretch(1);
		return page;
	}

	QString SlotName(iggy::ui::UiShellSlot slot) const
	{
		switch (slot) {
		case iggy::ui::UiShellSlot::Main:
			return QStringLiteral("Main");
		case iggy::ui::UiShellSlot::Left:
			return QStringLiteral("Left");
		case iggy::ui::UiShellSlot::Right:
			return QStringLiteral("Right");
		case iggy::ui::UiShellSlot::Bottom:
			return QStringLiteral("Bottom");
		}
		return QStringLiteral("Unknown");
	}

	iggy::ui::UiToolInventory inventory_ = iggy::ui::defaultUiToolInventory();
	iggy::ui::UiSettingsState settings_;
	iggy::ui::UiFeatureContext context_;
	iggy::ui::UiRuntimeWorkspaceModelInput input_;
	iggy::ui::UiRuntimeWorkspaceModel model_;
	bool showSettings_ = false;
};

} // namespace

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	IggyQtShellWindow window;
	window.show();
	return app.exec();
}
