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
			color: %2;
			font-family: "%3", "Inter", sans-serif;
			font-size: %4px;
			font-weight: 400;
		}
		QMainWindow, QWidget#shellRoot, QWidget#settingsWindow {
			background: %1;
		}
		QLabel {
			background: transparent;
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
		}
		QFrame#bottomPanel {
			background: %1;
			border-top: 1px solid %6;
		}
		QFrame#mainSlot {
			background: %8;
		}
		QFrame#productViewport {
			background: %8;
			border: none;
		}
		QFrame#workspaceOverlayHost {
			background: %8;
		}
		QFrame#canvasStage {
			background: %8;
			border: none;
			border-radius: 0;
		}
		QFrame#canvasWell {
			background: %8;
			border: none;
			border-radius: 0;
		}
		QFrame#inspectorCard, QWidget#settingsRow {
			background: transparent;
			border: none;
			border-radius: 0;
		}
		QFrame#settingsPageStrip {
			background: %5;
			border-right: 1px solid %6;
		}
		QWidget#settingsPage {
			background: %8;
		}
		QFrame#floatingPalette {
			background: transparent;
			border: none;
			border-radius: 0;
		}
		QFrame#paletteGrip {
			background: %12;
			border-radius: 4px;
			min-width: 6px;
			max-width: 6px;
			min-height: 18px;
		}
		QFrame#leftPanelGrip, QFrame#rightPanelGrip {
			background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
				stop:0 transparent, stop:0.43 transparent,
				stop:0.44 %24, stop:0.56 %24,
				stop:0.57 transparent, stop:1 transparent);
		}
		QFrame#bottomPanelGrip {
			background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
				stop:0 transparent, stop:0.43 transparent,
				stop:0.44 %24, stop:0.56 %24,
				stop:0.57 transparent, stop:1 transparent);
		}
		QFrame#leftPanelGrip:hover, QFrame#rightPanelGrip:hover {
			background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
				stop:0 transparent, stop:0.43 transparent,
				stop:0.44 %23, stop:0.56 %23,
				stop:0.57 transparent, stop:1 transparent);
		}
		QFrame#bottomPanelGrip:hover {
			background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
				stop:0 transparent, stop:0.43 transparent,
				stop:0.44 %23, stop:0.56 %23,
				stop:0.57 transparent, stop:1 transparent);
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
			font-family: "%25", monospace;
			font-size: %16px;
		}
		QLabel#badgeLabel {
			background: transparent;
			border: none;
			border-radius: 0;
			color: %10;
			font-size: %16px;
			font-weight: 600;
			padding: 0;
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
			border-color: transparent;
		}
		QPushButton:checked, QPushButton:pressed {
			background: %12;
			border-color: transparent;
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
			min-width: 30px;
			max-width: 30px;
			min-height: 30px;
			max-height: 30px;
			text-align: center;
			padding: 0;
		}
		QPushButton#panelToggleButton[panelState="visible"] {
			background: %12;
		}
		QPushButton#panelToggleButton[panelState="collapsed"] {
			background: transparent;
		}
		QFrame#topChrome QPushButton#trafficClose,
		QFrame#topChrome QPushButton#trafficMinimize,
		QFrame#topChrome QPushButton#trafficZoom {
			min-width: 12px;
			max-width: 12px;
			min-height: 12px;
			max-height: 12px;
			border-radius: 7px;
			padding: 0;
		}
		QFrame#topChrome QPushButton#trafficClose {
			background: %17;
			border: 1px solid %18;
		}
		QFrame#topChrome QPushButton#trafficMinimize {
			background: %19;
			border: 1px solid %20;
		}
		QFrame#topChrome QPushButton#trafficZoom {
			background: %21;
			border: 1px solid %22;
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
			background: transparent;
			border-color: transparent;
			padding-left: 10px;
			padding-right: 10px;
		}
		QPushButton#toolChip:checked {
			background: %12;
		}
		QPushButton#settingsPageButton {
			min-height: 30px;
			padding-left: 8px;
			padding-right: 8px;
		}
		QPushButton#themeSwatchButton {
			min-width: 22px;
			max-width: 22px;
			min-height: 22px;
			max-height: 22px;
			border-radius: 4px;
			padding: 0;
		}
		QLineEdit, QComboBox, QListWidget, QSpinBox, QFontComboBox {
			background: %14;
			color: %2;
			border: 1px solid %7;
			border-radius: 5px;
			padding: 4px 6px;
		}
		QAbstractSpinBox::up-button, QAbstractSpinBox::down-button {
			width: 0;
			border: none;
		}
		QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QFontComboBox:focus {
			border-color: %13;
		}
		QScrollArea {
			background: transparent;
			border: none;
		}
		QScrollBar:vertical {
			background: transparent;
			width: 8px;
			margin: 0;
		}
		QScrollBar:horizontal {
			background: transparent;
			height: 8px;
			margin: 0;
		}
		QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
			background: %6;
			border-radius: 3px;
		}
		QScrollBar::handle:vertical {
			min-height: 24px;
		}
		QScrollBar::handle:horizontal {
			min-width: 24px;
		}
		QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {
			background: %23;
		}
		QScrollBar::add-line, QScrollBar::sub-line {
			width: 0;
			height: 0;
		}
		QScrollBar::add-page, QScrollBar::sub-page {
			background: transparent;
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
		QCheckBox::indicator {
			width: 26px;
			height: 12px;
			border-radius: 7px;
		}
		QCheckBox::indicator:unchecked {
			background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
				stop:0 %10, stop:0.385 %10,
				stop:0.386 %14, stop:1 %14);
			border: 1px solid %6;
		}
		QCheckBox::indicator:checked {
			background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
				stop:0 %23, stop:0.614 %23,
				stop:0.615 %13, stop:1 %13);
			border: 1px solid %13;
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
		.arg(theme.fontSizeXs)
		.arg(toQString(theme.trafficClose))
		.arg(toQString(theme.trafficCloseEdge))
		.arg(toQString(theme.trafficMinimize))
		.arg(toQString(theme.trafficMinimizeEdge))
		.arg(toQString(theme.trafficZoom))
		.arg(toQString(theme.trafficZoomEdge))
		.arg(toQString(theme.accentSoft))
		.arg(toQString(theme.borderMajor))
		.arg(toQString(theme.codeFont));
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
