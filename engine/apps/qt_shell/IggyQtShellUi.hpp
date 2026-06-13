#pragma once

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QWidget>

#include <string>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/ui/UiSettingsState.hpp"
#include "scene/ui/UiShellModel.hpp"
#include "scene/ui/UiTheme.hpp"

namespace iggy::qt_shell {

[[nodiscard]] QString toQString(const std::string &text);
[[nodiscard]] QString toQString(ResourceId id);
[[nodiscard]] ResourceId id(const char *value);
[[nodiscard]] QString shellStyleSheet(const ui::UiThemeTokens &theme);
[[nodiscard]] QString slotName(ui::UiShellSlot slot);
[[nodiscard]] QString panelVisibilityName(ui::UiPanelVisibility visibility);

[[nodiscard]] QLabel *makeLabel(const QString &text, const char *objectName = nullptr);
[[nodiscard]] QPushButton *makeToolButton(const QString &label, bool checked = false);
[[nodiscard]] QFrame *makeFrame(const char *objectName);
[[nodiscard]] QWidget *makeScrollHost(QWidget *content);

void setPanelContentAssignment(
	std::vector<ui::UiPanelContentAssignment> &assignments,
	ui::UiPanelContentAssignment assignment);

} // namespace iggy::qt_shell
