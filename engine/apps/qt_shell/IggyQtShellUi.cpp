#include "IggyQtShellUi.hpp"

#include <QScrollArea>

namespace iggy::qt_shell {

QString toQString(const std::string &text)
{
	return QString::fromStdString(text);
}

QString toQString(ResourceId id)
{
	return QString::fromStdString(std::string { id.value() });
}

ResourceId id(const char *value)
{
	return ResourceId { value };
}

QString shellStyleSheet(const ui::UiThemeTokens &theme)
{
	return QString(R"(
		QWidget {
			background: %1;
			color: %2;
			font-family: "%3", "Inter", sans-serif;
			font-size: %4px;
			font-weight: 400;
		}
		QFrame#topChrome {
			background: %5;
			border-bottom: 1px solid %6;
		}
		QFrame#statusBar {
			background: %5;
			border-top: 1px solid %6;
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
		QFrame#canvasStage {
			background: %1;
			border: 1px solid %6;
			border-radius: 7px;
		}
		QFrame#canvasWell {
			background: %14;
			border: 1px solid %7;
			border-radius: 5px;
		}
		QFrame#inspectorCard, QWidget#settingsRow {
			background: %14;
			border: 1px solid %7;
			border-radius: 6px;
		}
		QFrame#settingsSidebar {
			background: %5;
			border-right: 1px solid %6;
		}
		QWidget#settingsPage {
			background: %8;
		}
		QFrame#floatingPalette {
			background: %14;
			border: 1px solid %6;
			border-radius: 6px;
		}
		QFrame#paletteGrip {
			background: %12;
			border-radius: 4px;
			min-width: 6px;
			max-width: 6px;
			min-height: 18px;
		}
		QLabel#panelTitle {
			color: %2;
			font-size: %9px;
			font-weight: 600;
		}
		QLabel#sectionLabel {
			color: %10;
			font-size: %16px;
			font-weight: 700;
			padding-top: 6px;
		}
		QLabel#fieldLabel, QLabel#mutedText, QLabel#statusMode, QLabel#statusFile {
			color: %10;
		}
		QLabel#statusMode, QLabel#statusFile {
			background: transparent;
			font-size: %16px;
		}
		QLabel#badgeLabel {
			background: %12;
			border: 1px solid %13;
			border-radius: 9px;
			color: %2;
			font-size: %16px;
			font-weight: 600;
			padding: 2px 8px;
		}
		QPushButton {
			background: transparent;
			color: %2;
			border: 1px solid transparent;
			border-radius: 5px;
			padding: 4px 8px;
			min-height: 20px;
			text-align: left;
			font-weight: 500;
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
		QPushButton:disabled {
			color: %10;
			border-color: transparent;
		}
		QPushButton#chromeMenuButton {
			padding-left: 9px;
			padding-right: 9px;
		}
		QPushButton#panelToggleButton {
			min-width: 28px;
			max-width: 28px;
			text-align: center;
			padding-left: 0;
			padding-right: 0;
		}
		QPushButton#railButton {
			min-width: 32px;
			max-width: 32px;
			min-height: 32px;
			max-height: 32px;
			padding: 0;
			text-align: center;
			font-weight: 700;
		}
		QPushButton#toolChip {
			background: %14;
			border-color: %7;
			padding-left: 10px;
			padding-right: 10px;
		}
		QLineEdit, QComboBox, QListWidget, QSpinBox, QFontComboBox {
			background: %14;
			color: %2;
			border: 1px solid %6;
			border-radius: 5px;
			padding: 4px 6px;
		}
		QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QFontComboBox:focus {
			border-color: %13;
		}
		QScrollArea {
			background: transparent;
			border: none;
		}
		QListWidget {
			alternate-background-color: %8;
			outline: none;
		}
		QListWidget::item:selected {
			background: %15;
			color: %2;
		}
		QCheckBox {
			spacing: 8px;
		}
		QSplitter::handle {
			background: %6;
		}
		QSplitter::handle:hover {
			background: %13;
		}
		QMenu {
			background: %5;
			color: %2;
			border: 1px solid %6;
			padding: 5px;
		}
		QMenu::item {
			padding: 5px 24px 5px 10px;
			border-radius: 4px;
		}
		QMenu::item:selected {
			background: %11;
		}
		QPushButton#trafficClose, QPushButton#trafficMinimize, QPushButton#trafficZoom {
			min-width: 12px;
			max-width: 12px;
			min-height: 12px;
			max-height: 12px;
			border-radius: 6px;
			padding: 0;
		}
		QPushButton#trafficClose {
			background: #ff5f57;
			border-color: #cc4c46;
		}
		QPushButton#trafficMinimize {
			background: #ffbd2e;
			border-color: #cc9725;
		}
		QPushButton#trafficZoom {
			background: #28c840;
			border-color: #20a033;
		}
	)")
		.arg(toQString(theme.base))
		.arg(toQString(theme.text))
		.arg(toQString(theme.uiFont))
		.arg(theme.fontSizeBody)
		.arg(toQString(theme.surface))
		.arg(toQString(theme.borderMajor))
		.arg(toQString(theme.borderMinor))
		.arg(toQString(theme.workspaceBody))
		.arg(theme.fontSizeTitle)
		.arg(toQString(theme.textMuted))
		.arg(toQString(theme.controlHover))
		.arg(toQString(theme.selected))
		.arg(toQString(theme.borderFocus))
		.arg(toQString(theme.control))
		.arg(toQString(theme.rowSelected))
		.arg(theme.fontSizeXs);
}

QString slotName(ui::UiShellSlot slot)
{
	switch (slot) {
	case ui::UiShellSlot::Main:
		return QStringLiteral("Main");
	case ui::UiShellSlot::Left:
		return QStringLiteral("Left");
	case ui::UiShellSlot::Right:
		return QStringLiteral("Right");
	case ui::UiShellSlot::Bottom:
		return QStringLiteral("Bottom");
	}
	return QStringLiteral("Unknown");
}

QString panelVisibilityName(ui::UiPanelVisibility visibility)
{
	switch (visibility) {
	case ui::UiPanelVisibility::Visible:
		return QStringLiteral("visible");
	case ui::UiPanelVisibility::Collapsed:
		return QStringLiteral("collapsed");
	case ui::UiPanelVisibility::AutoHidden:
		return QStringLiteral("auto-hidden");
	}
	return QStringLiteral("unknown");
}

QLabel *makeLabel(const QString &text, const char *objectName)
{
	auto *label = new QLabel(text);
	if (objectName != nullptr)
		label->setObjectName(QString::fromUtf8(objectName));
	return label;
}

QLabel *makeSectionLabel(const QString &text)
{
	return makeLabel(text.toUpper(), "sectionLabel");
}

QPushButton *makeToolButton(const QString &label, bool checked)
{
	auto *button = new QPushButton(label);
	button->setCheckable(true);
	button->setChecked(checked);
	return button;
}

QPushButton *makeChromeButton(const QString &label, const QString &tooltip)
{
	auto *button = new QPushButton(label);
	button->setObjectName(QStringLiteral("chromeMenuButton"));
	if (!tooltip.isEmpty())
		button->setToolTip(tooltip);
	return button;
}

QPushButton *makeRailButton(const QString &label, const QString &tooltip, bool checked)
{
	auto *button = makeToolButton(label, checked);
	button->setObjectName(QStringLiteral("railButton"));
	button->setToolTip(tooltip);
	return button;
}

QFrame *makeFrame(const char *objectName)
{
	auto *frame = new QFrame;
	frame->setObjectName(QString::fromUtf8(objectName));
	frame->setFrameShape(QFrame::NoFrame);
	return frame;
}

QWidget *makeScrollHost(QWidget *content)
{
	auto *scroll = new QScrollArea;
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setWidget(content);
	return scroll;
}

void setPanelContentAssignment(
	std::vector<ui::UiPanelContentAssignment> &assignments,
	ui::UiPanelContentAssignment assignment)
{
	for (ui::UiPanelContentAssignment &existing : assignments) {
		if (existing.groupId == assignment.groupId) {
			existing = assignment;
			return;
		}
	}
	assignments.push_back(assignment);
}

} // namespace iggy::qt_shell
