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
		QFrame#floatingPalette {
			background: %14;
			border: 1px solid %6;
			border-radius: 6px;
		}
		QFrame#paletteGrip {
			background: %12;
			border-radius: 4px;
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
		QLineEdit, QComboBox, QListWidget, QSpinBox, QFontComboBox {
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
		QCheckBox {
			spacing: 8px;
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
		.arg(toQString(theme.rowSelected));
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

QPushButton *makeToolButton(const QString &label, bool checked)
{
	auto *button = new QPushButton(label);
	button->setCheckable(true);
	button->setChecked(checked);
	return button;
}

QFrame *makeFrame(const char *objectName)
{
	auto *frame = new QFrame;
	frame->setObjectName(QString::fromUtf8(objectName));
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
