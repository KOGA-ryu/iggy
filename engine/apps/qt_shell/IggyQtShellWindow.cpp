#include "IggyQtShellWindow.hpp"

#include <QAction>
#include <QButtonGroup>
#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QEvent>
#include <QFont>
#include <QFontComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QSize>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTimer>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string_view>
#include <utility>
#include <vector>

#include "IggyQtShellUi.hpp"
#include "runtime/RuntimeGameplayAuthoringPreviewModel.hpp"
#include "runtime/RuntimeGameplayProductInputContext.hpp"
#include "runtime/RuntimeGameplayProductInputFrameTargetAction.hpp"
#include "runtime/RuntimeGameplayProductInputFrameTargetContext.hpp"
#include "runtime/RuntimeGameplayProductPointerProjection.hpp"
#include "scene/ui/UiAuthoringPreviewPanelModel.hpp"
#include "scene/ui/UiProductPlayModePanelModel.hpp"
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

QString objectNameForId(const QString &prefix, const ResourceId &id)
{
	QString name = prefix + toQString(id);
	name.replace(':', '_');
	name.replace('/', '_');
	name.replace('-', '_');
	return name;
}

QColor productViewportMaterialColor(const ResourceId &materialId)
{
	const std::string_view material = materialId.value();
	if (material == "material:floor")
		return QColor(78, 99, 90, 210);
	if (material == "material:wall")
		return QColor(44, 48, 52, 235);
	if (material == "material:player")
		return QColor(61, 174, 224, 230);
	if (material == "material:npc_actor")
		return QColor(226, 156, 55, 230);
	if (material == "material:npc")
		return QColor(142, 96, 164, 220);
	return QColor(112, 118, 124, 210);
}

QRectF productViewportWorldRectToPixels(
	Aabb2 commandBounds,
	Aabb2 viewBounds,
	int widgetWidth,
	int widgetHeight)
{
	const float worldMinX = std::min(commandBounds.min.x, commandBounds.max.x);
	const float worldMaxX = std::max(commandBounds.min.x, commandBounds.max.x);
	const float worldMinY = std::min(commandBounds.min.y, commandBounds.max.y);
	const float worldMaxY = std::max(commandBounds.min.y, commandBounds.max.y);
	const float viewWidth = viewBounds.max.x - viewBounds.min.x;
	const float viewHeight = viewBounds.max.y - viewBounds.min.y;
	const auto mapX = [viewBounds, viewWidth, widgetWidth](float worldX) {
		return static_cast<qreal>((worldX - viewBounds.min.x) / viewWidth)
			* static_cast<qreal>(widgetWidth);
	};
	const auto mapY = [viewBounds, viewHeight, widgetHeight](float worldY) {
		return static_cast<qreal>((worldY - viewBounds.min.y) / viewHeight)
			* static_cast<qreal>(widgetHeight);
	};
	const QPointF topLeft { mapX(worldMinX), mapY(worldMinY) };
	const QPointF bottomRight { mapX(worldMaxX), mapY(worldMaxY) };
	return QRectF(topLeft, bottomRight).normalized();
}

QColor productViewportTargetHighlightColor(
	const runtime::RuntimeGameplayProductInteractionTargetQueryResult &target)
{
	if (!target.hasReach)
		return QColor(230, 230, 220, 230);
	if (target.reachable)
		return QColor(62, 220, 184, 235);
	return QColor(238, 133, 70, 235);
}

class ProductViewportWidget final : public QFrame {
public:
	explicit ProductViewportWidget(QWidget *parent = nullptr)
		: QFrame(parent)
	{
		setObjectName(QStringLiteral("productViewport"));
		setAttribute(Qt::WA_StyledBackground, true);
	}

	void setLatestFrame(
		const runtime::RuntimeGameplayProductPlayModeFrameResult *frame)
	{
		latestFrame_ = frame;
		update();
	}

	void setLatestTargetContext(
		const runtime::RuntimeGameplayProductInputFrameTargetContextResult
			*targetContext)
	{
		latestTargetContext_ = targetContext;
		update();
	}

protected:
	void paintEvent(QPaintEvent *event) override
	{
		QFrame::paintEvent(event);
		if (latestFrame_ == nullptr || width() <= 0 || height() <= 0)
			return;

		const LevelRenderFrame2DResult &levelFrame =
			latestFrame_->surface.presentation.levelFrame;
		const Aabb2 viewBounds = levelFrame.cameraView.bounds;
		const float viewWidth = viewBounds.max.x - viewBounds.min.x;
		const float viewHeight = viewBounds.max.y - viewBounds.min.y;
		if (viewWidth == 0.0F || viewHeight == 0.0F)
			return;

		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing, false);
		for (const render::RenderCommand2D &command :
			 levelFrame.commands.commands) {
			if (command.type != render::RenderCommand2DType::Quad)
				continue;

			const QRectF rect = productViewportWorldRectToPixels(
				command.worldBounds,
				viewBounds,
				width(),
				height());
			const QColor fill = productViewportMaterialColor(command.materialId);
			QColor outline = fill.darker(135);
			outline.setAlpha(230);
			painter.fillRect(rect, fill);
			painter.setPen(QPen(outline, 1.0));
			painter.drawRect(rect);
		}

		drawTargetHighlight(painter, viewBounds);
	}

private:
	void drawTargetHighlight(QPainter &painter, Aabb2 viewBounds) const
	{
		if (latestTargetContext_ == nullptr
			|| !latestTargetContext_->target.hasTarget)
			return;

		const InteractionTarget2D &target =
			latestTargetContext_->target.target;
		const float radius = std::max(0.0F, target.radius);
		const Aabb2 markerBounds {
			{ target.position.x - radius, target.position.y - radius },
			{ target.position.x + radius, target.position.y + radius },
		};
		QRectF marker = productViewportWorldRectToPixels(
			markerBounds,
			viewBounds,
			width(),
			height());
		constexpr qreal MinimumMarkerSize = 8.0;
		if (marker.width() < MinimumMarkerSize
			|| marker.height() < MinimumMarkerSize) {
			const QPointF center = marker.center();
			const qreal markerWidth = std::max(marker.width(), MinimumMarkerSize);
			const qreal markerHeight =
				std::max(marker.height(), MinimumMarkerSize);
			marker = QRectF(
				center.x() - markerWidth * 0.5,
				center.y() - markerHeight * 0.5,
				markerWidth,
				markerHeight);
		}

		const QColor color =
			productViewportTargetHighlightColor(latestTargetContext_->target);
		QColor tint = color;
		tint.setAlpha(38);
		painter.setBrush(tint);
		painter.setPen(QPen(color, 2.0));
		painter.drawEllipse(marker);
	}

	const runtime::RuntimeGameplayProductPlayModeFrameResult *latestFrame_ =
		nullptr;
	const runtime::RuntimeGameplayProductInputFrameTargetContextResult
		*latestTargetContext_ = nullptr;
};

QMenu *attachMenu(QPushButton *button)
{
	auto *menu = new QMenu(button);
	button->setMenu(menu);
	return menu;
}

void addKeyValueRow(QVBoxLayout *layout, const ui::UiAuthoringPreviewPanelRow &row)
{
	auto *line = new QWidget;
	auto *lineLayout = new QHBoxLayout(line);
	lineLayout->setContentsMargins(0, 1, 0, 1);
	lineLayout->setSpacing(8);
	lineLayout->addWidget(makeLabel(toQString(row.key), "fieldLabel"));
	lineLayout->addStretch(1);
	lineLayout->addWidget(makeLabel(toQString(row.value), "mutedText"));
	layout->addWidget(line);
}

void addKeyValueRow(QVBoxLayout *layout, const ui::UiProductPlayModePanelRow &row)
{
	auto *line = new QWidget;
	auto *lineLayout = new QHBoxLayout(line);
	lineLayout->setContentsMargins(0, 1, 0, 1);
	lineLayout->setSpacing(8);
	lineLayout->addWidget(makeLabel(toQString(row.key), "fieldLabel"));
	lineLayout->addStretch(1);
	lineLayout->addWidget(makeLabel(toQString(row.value), "mutedText"));
	layout->addWidget(line);
}

void addRowSection(
	QVBoxLayout *layout,
	const QString &title,
	const std::vector<ui::UiAuthoringPreviewPanelRow> &rows,
	const QString &emptyText = QStringLiteral("None"))
{
	layout->addWidget(makeSectionLabel(title));
	if (rows.empty()) {
		layout->addWidget(makeLabel(emptyText, "mutedText"));
		return;
	}
	for (const ui::UiAuthoringPreviewPanelRow &row : rows)
		addKeyValueRow(layout, row);
}

void addRowSection(
	QVBoxLayout *layout,
	const QString &title,
	const std::vector<ui::UiProductPlayModePanelRow> &rows,
	const QString &emptyText = QStringLiteral("None"))
{
	layout->addWidget(makeSectionLabel(title));
	if (rows.empty()) {
		layout->addWidget(makeLabel(emptyText, "mutedText"));
		return;
	}
	for (const ui::UiProductPlayModePanelRow &row : rows)
		addKeyValueRow(layout, row);
}

void addStringSection(
	QVBoxLayout *layout,
	const QString &title,
	const std::vector<std::string> &rows,
	const QString &emptyText = QStringLiteral("None"),
	const char *objectName = "mutedText")
{
	layout->addWidget(makeSectionLabel(title));
	if (rows.empty()) {
		layout->addWidget(makeLabel(emptyText, "mutedText"));
		return;
	}
	for (const std::string &row : rows)
		layout->addWidget(makeLabel(toQString(row), objectName));
}

void removeWidgetFromLayout(QLayout *layout, QWidget *widget)
{
	if (layout == nullptr || widget == nullptr)
		return;
	layout->removeWidget(widget);
	widget->deleteLater();
}

class WorkspaceOverlayHost final : public QFrame {
public:
	QWidget *mainSlot = nullptr;
	QWidget *rightPanel = nullptr;
	QWidget *bottomPanel = nullptr;
	QWidget *rightGrip = nullptr;
	QWidget *bottomGrip = nullptr;

	bool eventFilter(QObject *watched, QEvent *event) override
	{
		if (watched != rightGrip && watched != bottomGrip)
			return QFrame::eventFilter(watched, event);

		switch (event->type()) {
		case QEvent::MouseButtonPress: {
			auto *mouse = static_cast<QMouseEvent *>(event);
			if (mouse->button() != Qt::LeftButton)
				break;
			dragMode_ = watched == rightGrip ? DragMode::Right : DragMode::Bottom;
			dragStart_ = mouse->globalPosition().toPoint();
			dragStartRightWidth_ = rightWidth_;
			dragStartBottomHeight_ = bottomHeight_;
			event->accept();
			return true;
		}
		case QEvent::MouseMove: {
			if (dragMode_ == DragMode::None)
				break;
			auto *mouse = static_cast<QMouseEvent *>(event);
			const QPoint delta = mouse->globalPosition().toPoint() - dragStart_;
			if (dragMode_ == DragMode::Right)
				rightWidth_ = dragStartRightWidth_ - delta.x();
			else
				bottomHeight_ = dragStartBottomHeight_ - delta.y();
			layoutChildren();
			event->accept();
			return true;
		}
		case QEvent::MouseButtonRelease: {
			auto *mouse = static_cast<QMouseEvent *>(event);
			if (mouse->button() == Qt::LeftButton && dragMode_ != DragMode::None) {
				dragMode_ = DragMode::None;
				event->accept();
				return true;
			}
			break;
		}
		default:
			break;
		}

		return QFrame::eventFilter(watched, event);
	}

protected:
	void resizeEvent(QResizeEvent *event) override
	{
		QFrame::resizeEvent(event);
		layoutChildren();
	}

private:
	enum class DragMode {
		None,
		Right,
		Bottom,
	};

	void layoutChildren()
	{
		const int w = width();
		const int h = height();
		const int rightW = rightPanel != nullptr ? clampedRightWidth(w) : 0;
		const int bottomH = bottomPanel != nullptr ? clampedBottomHeight(h) : 0;

		if (mainSlot != nullptr)
			mainSlot->setGeometry(0, 0, w, h);
		if (rightPanel != nullptr)
			rightPanel->setGeometry(w - rightW, 0, rightW, h - bottomH);
		if (bottomPanel != nullptr)
			bottomPanel->setGeometry(0, h - bottomH, w, bottomH);
		if (rightGrip != nullptr)
			rightGrip->setGeometry(w - rightW - 4, 0, 8, h - bottomH);
		if (bottomGrip != nullptr)
			bottomGrip->setGeometry(0, h - bottomH - 4, w, 8);
		if (rightPanel != nullptr)
			rightPanel->raise();
		if (bottomPanel != nullptr)
			bottomPanel->raise();
		if (rightGrip != nullptr)
			rightGrip->raise();
		if (bottomGrip != nullptr)
			bottomGrip->raise();
	}

	int clampedRightWidth(int availableWidth)
	{
		const int minWidth = std::min(180, availableWidth);
		const int maxWidth = std::max(minWidth, availableWidth - 240);
		rightWidth_ = std::clamp(rightWidth_, minWidth, maxWidth);
		return rightWidth_;
	}

	int clampedBottomHeight(int availableHeight)
	{
		const int minHeight = std::min(120, availableHeight);
		const int maxHeight = std::max(minHeight, availableHeight - 160);
		bottomHeight_ = std::clamp(bottomHeight_, minHeight, maxHeight);
		return bottomHeight_;
	}

	DragMode dragMode_ = DragMode::None;
	QPoint dragStart_;
	int dragStartRightWidth_ = 280;
	int dragStartBottomHeight_ = 190;
	int rightWidth_ = 280;
	int bottomHeight_ = 190;
};

class ShellBodyHost final : public QFrame {
public:
	QWidget *leftPanel = nullptr;
	QWidget *leftGrip = nullptr;
	QWidget *workspace = nullptr;

	bool eventFilter(QObject *watched, QEvent *event) override
	{
		if (watched != leftGrip)
			return QFrame::eventFilter(watched, event);

		switch (event->type()) {
		case QEvent::MouseButtonPress: {
			auto *mouse = static_cast<QMouseEvent *>(event);
			if (mouse->button() != Qt::LeftButton)
				break;
			draggingLeft_ = true;
			dragStart_ = mouse->globalPosition().toPoint();
			dragStartLeftWidth_ = leftWidth_;
			event->accept();
			return true;
		}
		case QEvent::MouseMove: {
			if (!draggingLeft_)
				break;
			auto *mouse = static_cast<QMouseEvent *>(event);
			leftWidth_ = dragStartLeftWidth_ + (mouse->globalPosition().toPoint() - dragStart_).x();
			layoutChildren();
			event->accept();
			return true;
		}
		case QEvent::MouseButtonRelease: {
			auto *mouse = static_cast<QMouseEvent *>(event);
			if (mouse->button() == Qt::LeftButton && draggingLeft_) {
				draggingLeft_ = false;
				event->accept();
				return true;
			}
			break;
		}
		default:
			break;
		}

		return QFrame::eventFilter(watched, event);
	}

protected:
	void resizeEvent(QResizeEvent *event) override
	{
		QFrame::resizeEvent(event);
		layoutChildren();
	}

private:
	void layoutChildren()
	{
		const int w = width();
		const int h = height();
		const int leftW = leftPanel != nullptr ? clampedLeftWidth(w) : 0;

		if (leftPanel != nullptr)
			leftPanel->setGeometry(0, 0, leftW, h);
		if (leftGrip != nullptr)
			leftGrip->setGeometry(leftW - 4, 0, 8, h);
		if (workspace != nullptr)
			workspace->setGeometry(leftW, 0, w - leftW, h);
		if (leftPanel != nullptr)
			leftPanel->raise();
		if (workspace != nullptr)
			workspace->raise();
		if (leftGrip != nullptr)
			leftGrip->raise();
	}

	int clampedLeftWidth(int availableWidth)
	{
		const int minWidth = std::min(180, availableWidth);
		const int maxWidth = std::max(minWidth, availableWidth - 320);
		leftWidth_ = std::clamp(leftWidth_, minWidth, maxWidth);
		return leftWidth_;
	}

	bool draggingLeft_ = false;
	QPoint dragStart_;
	int dragStartLeftWidth_ = 240;
	int leftWidth_ = 240;
};

} // namespace

IggyQtShellWindow::IggyQtShellWindow(IggyQtShellLaunchOptions launchOptions)
{
	setWindowFlag(Qt::FramelessWindowHint, true);
	setMinimumSize(520, 420);
	resize(1180, 760);
	settings_ = ui::defaultUiSettingsState(inventory_);
	input_ = ui::defaultUiRuntimeWorkspaceModelInput(inventory_);
	input_.windowWidth = width();
	input_.windowHeight = height();
	input_.panels.right.collapsed = false;
	input_.panels.bottom.collapsed = false;
	context_.activeToolId = id("tool:select");
	productFramePumpTimer_ = new QTimer(this);
	productFramePumpTimer_->setInterval(250);
	connect(productFramePumpTimer_, &QTimer::timeout, this, [this]() {
		if (!productFramePumpAvailable()) {
			setProductFramePumpEnabled(false);
			return;
		}
		runProductFrameRequestOnce();
	});
	if (launchOptions.preview.enabled) {
		authoringPreview_ =
			runtime::RuntimeGameplayAuthoringPreviewModelBuilder {}.build(
				launchOptions.preview.path,
				launchOptions.preview.config);
		hasAuthoringPreview_ = true;
		context_.authoringPreview = &authoringPreview_;
		setPanelContentAssignment(
			settings_.panelContent,
			{ id("panel:authoring_preview"), ui::UiShellSlot::Right, false });
	}
	if (launchOptions.play.enabled) {
		productLoad_ =
			runtime::RuntimeGameplayProductScenarioLoader {}.load(
				launchOptions.play.path);
		productLoopBuild_ =
			runtime::RuntimeGameplayProductLoop {}.build(productLoad_);
		productPlayBuild_ =
			runtime::RuntimeGameplayProductPlayMode {}.build(productLoopBuild_);
		if (productPlayBuild_.status ==
			runtime::RuntimeGameplayProductPlayModeBuildStatus::Ready) {
			productPlayState_ = productPlayBuild_.state;
		}
		hasProductPlayMode_ = true;
		context_.productPlayModeBuild = &productPlayBuild_;
		context_.productPlayModeState = &productPlayState_;
		context_.latestProductPlayModeFrame = nullptr;
		hasLatestProductInputFrameTargetContext_ = false;
		context_.latestProductInputFrameTargetContext = nullptr;
		input_.workspace.bindings.push_back(
			{ ui::UiShellSlot::Right, id("feature:product_play") });
		setPanelContentAssignment(
			settings_.panelContent,
			{ id("panel:product_play"), ui::UiShellSlot::Right, false });
	}
	rebuildModel();
	applyTheme();
	buildShell();
	buildSettingsWindow();
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

void IggyQtShellWindow::refreshChrome()
{
	if (rootLayout_ == nullptr || chrome_ == nullptr)
		return;
	removeWidgetFromLayout(rootLayout_, chrome_);
	chrome_ = buildChrome();
	rootLayout_->insertWidget(0, chrome_);
}

void IggyQtShellWindow::refreshAfterModelChange()
{
	rebuildModel();
	refreshChrome();
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
	refreshChrome();

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

bool IggyQtShellWindow::productPlayInputFocusEnabled() const
{
	return productPlayInputFocusAvailable() && productPlayState_.hasInputFocus;
}

bool IggyQtShellWindow::productPlayInputFocusAvailable() const
{
	return hasProductPlayMode_
		&& productPlayBuild_.status == runtime::RuntimeGameplayProductPlayModeBuildStatus::Ready;
}

void IggyQtShellWindow::setProductPlayInputFocus(bool enabled)
{
	if (!productPlayInputFocusAvailable())
		return;
	productPlayState_ =
		runtime::RuntimeGameplayProductPlayMode {}.withInputFocus(productPlayState_, enabled);
	if (!enabled) {
		clearProductInputAccumulator();
		hasLatestProductInputFrameTargetContext_ = false;
		context_.latestProductInputFrameTargetContext = nullptr;
	}
	context_.productPlayModeState = &productPlayState_;
	context_.latestProductPlayModeFrame = nullptr;
	refreshAfterModelChange();
}

void IggyQtShellWindow::toggleProductPlayInputFocus()
{
	setProductPlayInputFocus(!productPlayInputFocusEnabled());
}

bool IggyQtShellWindow::productManualStepAvailable() const
{
	return hasProductPlayMode_
		&& productPlayBuild_.status ==
			runtime::RuntimeGameplayProductPlayModeBuildStatus::Ready;
}

bool IggyQtShellWindow::productFramePumpAvailable() const
{
	return productManualStepAvailable();
}

bool IggyQtShellWindow::productFramePumpEnabled() const
{
	return productFramePumpTimer_ != nullptr && productFramePumpTimer_->isActive();
}

void IggyQtShellWindow::setProductFramePumpEnabled(bool enabled)
{
	if (productFramePumpTimer_ == nullptr)
		return;
	if (enabled && !productFramePumpAvailable()) {
		productFramePumpTimer_->stop();
		refreshAfterModelChange();
		return;
	}
	if (enabled) {
		if (!productFramePumpTimer_->isActive())
			productFramePumpTimer_->start();
	} else {
		productFramePumpTimer_->stop();
	}
	refreshAfterModelChange();
}

runtime::RuntimeGameplayProductPresentationCameraConfig
IggyQtShellWindow::productPresentationCameraConfig() const
{
	runtime::RuntimeGameplayProductPresentationCameraConfig config;
	config.hasPreviousCamera = hasProductPresentationCamera_;
	if (hasProductPresentationCamera_)
		config.previousCamera = productPresentationCamera_;
	config.fallbackCamera = { { 0.0F, 0.0F } };
	config.cameraView = { { 16.0F, 12.0F }, 1.0F };
	config.includeNpcCommands = true;
	config.useTileChunkCache = false;
	config.tileChunkCache = nullptr;
	config.rig.follow = { 1000.0F, 0.0F };
	return config;
}

void IggyQtShellWindow::runProductManualStep()
{
	if (!productManualStepAvailable())
		return;

	runProductFrameRequestOnce();
}

void IggyQtShellWindow::runProductFrameRequestOnce()
{
	if (!productManualStepAvailable())
		return;

	runtime::RuntimeGameplayProductInputAccumulatorFrameInput frameInput;
	frameInput.state = productInputAccumulator_;
	frameInput.bindingContext = runtime::RuntimeGameplayProductInputContext {}
									.build(productPlayState_)
									.bindingContext;
	const runtime::RuntimeGameplayProductInputAccumulatorFrameResult frame =
		runtime::RuntimeGameplayProductInputAccumulator {}.buildFrame(
			frameInput);
	productInputAccumulator_ = frame.state;

	const runtime::RuntimeGameplayProductInputFrameTargetContextResult
		enrichedFrame =
			runtime::RuntimeGameplayProductInputFrameTargetContext {}.enrich({
				productPlayState_,
				frame.frame,
				{},
				{},
			});
	latestProductInputFrameTargetContext_ = enrichedFrame;
	hasLatestProductInputFrameTargetContext_ = true;
	context_.latestProductInputFrameTargetContext =
		&latestProductInputFrameTargetContext_;
	const runtime::RuntimeGameplayProductInputFrameTargetActionResult
		targetAction =
			runtime::RuntimeGameplayProductInputFrameTargetAction {}.synthesize(
				{ latestProductInputFrameTargetContext_ });

	runtime::RuntimeGameplayProductFrameRequestInput input;
	input.state = productPlayState_;
	input.inputFrame = targetAction.frame;
	input.presentationCamera = productPresentationCameraConfig();

	const runtime::RuntimeGameplayProductFrameRequestResult result =
		runtime::RuntimeGameplayProductFrameRequest {}.run(input);
	productPlayState_ = result.state;
	latestProductPlayModeFrame_ = result.frame;
	hasLatestProductPlayModeFrame_ = true;
	productPresentationCamera_ = result.presentationCamera.presentationCamera;
	hasProductPresentationCamera_ = true;
	context_.productPlayModeState = &productPlayState_;
	context_.latestProductPlayModeFrame = &latestProductPlayModeFrame_;
	refreshAfterModelChange();
}

void IggyQtShellWindow::clearProductInputAccumulator()
{
	productInputAccumulator_ =
		runtime::RuntimeGameplayProductInputAccumulator {}.clear(
			productInputAccumulator_);
}

std::optional<runtime::RuntimeGameplayProductInputControl2D>
IggyQtShellWindow::mapQtKeyToProductControl(int key) const
{
	using runtime::RuntimeGameplayProductInputControl2D;
	switch (key) {
	case Qt::Key_Up:
	case Qt::Key_W:
		return RuntimeGameplayProductInputControl2D::MoveNorth;
	case Qt::Key_Down:
	case Qt::Key_S:
		return RuntimeGameplayProductInputControl2D::MoveSouth;
	case Qt::Key_Left:
	case Qt::Key_A:
		return RuntimeGameplayProductInputControl2D::MoveWest;
	case Qt::Key_Right:
	case Qt::Key_D:
		return RuntimeGameplayProductInputControl2D::MoveEast;
	case Qt::Key_E:
	case Qt::Key_Return:
	case Qt::Key_Enter:
		return RuntimeGameplayProductInputControl2D::Interact;
	case Qt::Key_I:
		return RuntimeGameplayProductInputControl2D::Inspect;
	case Qt::Key_Space:
		return RuntimeGameplayProductInputControl2D::Wait;
	case Qt::Key_Escape:
		return RuntimeGameplayProductInputControl2D::Cancel;
	default:
		break;
	}
	return std::nullopt;
}

bool IggyQtShellWindow::recordProductKeyEvent(
	QKeyEvent &event,
	runtime::RuntimeGameplayProductInputEventKind kind)
{
	if (event.isAutoRepeat())
		return false;
	if (!productPlayInputFocusEnabled())
		return false;

	const std::optional<runtime::RuntimeGameplayProductInputControl2D> control =
		mapQtKeyToProductControl(event.key());
	if (!control.has_value())
		return false;

	runtime::RuntimeGameplayProductInputEvent2D productEvent;
	productEvent.control = *control;
	productEvent.kind = kind;
	productInputAccumulator_ =
		runtime::RuntimeGameplayProductInputAccumulator {}
			.record(productInputAccumulator_, productEvent)
			.state;
	return true;
}

bool IggyQtShellWindow::recordProductViewportPrimaryTilePress(QMouseEvent &event)
{
	if (event.button() != Qt::LeftButton)
		return false;
	if (!productPlayInputFocusAvailable() || !productPlayInputFocusEnabled())
		return false;
	if (productViewport_ == nullptr ||
			productViewport_->width() <= 0 ||
			productViewport_->height() <= 0)
		return false;

	const auto cameraConfig = productPresentationCameraConfig();
	const runtime::RuntimeGameplayProductPresentationCameraResult camera =
		runtime::RuntimeGameplayProductPresentationCamera {}.build(
			productPlayState_,
			cameraConfig);
	const Vec2 viewSize = cameraConfig.cameraView.viewportSize;
	if (viewSize.x == 0.0F || viewSize.y == 0.0F)
		return false;

	runtime::RuntimeGameplayProductPointerProjectionInput projectionInput;
	projectionInput.viewportPoint = {
		static_cast<float>(event.position().x()) /
			static_cast<float>(productViewport_->width()) *
			std::fabs(viewSize.x),
		static_cast<float>(event.position().y()) /
			static_cast<float>(productViewport_->height()) *
			std::fabs(viewSize.y),
	};
	projectionInput.viewportSize = viewSize;
	projectionInput.camera = camera.presentationCamera;
	projectionInput.cameraView = cameraConfig.cameraView;
	const runtime::RuntimeGameplayProductPointerProjectionResult projection =
		runtime::RuntimeGameplayProductPointerProjection {}.project(
			projectionInput);

	runtime::RuntimeGameplayProductInputEvent2D productEvent;
	productEvent.control =
		runtime::RuntimeGameplayProductInputControl2D::PrimaryTile;
	productEvent.kind = runtime::RuntimeGameplayProductInputEventKind::Pressed;
	productEvent.hasTile = true;
	productEvent.tile = projection.tile;

	const runtime::RuntimeGameplayProductInputAccumulatorRecordResult record =
		runtime::RuntimeGameplayProductInputAccumulator {}.record(
			productInputAccumulator_,
			productEvent);
	if (!record.changed)
		return false;
	productInputAccumulator_ = record.state;
	return true;
}

bool IggyQtShellWindow::eventFilter(QObject *watched, QEvent *event)
{
	if (watched == productViewport_) {
		if (event != nullptr && event->type() == QEvent::MouseButtonPress) {
			auto *mouse = static_cast<QMouseEvent *>(event);
			if (recordProductViewportPrimaryTilePress(*mouse)) {
				event->accept();
				return true;
			}
		}
		return QMainWindow::eventFilter(watched, event);
	}

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

void IggyQtShellWindow::keyPressEvent(QKeyEvent *event)
{
	if (event != nullptr
		&& recordProductKeyEvent(
			*event,
			runtime::RuntimeGameplayProductInputEventKind::Pressed)) {
		event->accept();
		return;
	}
	QMainWindow::keyPressEvent(event);
}

void IggyQtShellWindow::keyReleaseEvent(QKeyEvent *event)
{
	if (event != nullptr
		&& recordProductKeyEvent(
			*event,
			runtime::RuntimeGameplayProductInputEventKind::Released)) {
		event->accept();
		return;
	}
	QMainWindow::keyReleaseEvent(event);
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
	auto *productFocus = viewMenu->addAction(QStringLiteral("Product Input Focus"));
	productFocus->setCheckable(true);
	productFocus->setChecked(productPlayInputFocusEnabled());
	productFocus->setEnabled(productPlayInputFocusAvailable());
	connect(productFocus, &QAction::toggled, this, [this](bool enabled) {
		setProductPlayInputFocus(enabled);
	});
	auto *productStep = viewMenu->addAction(QStringLiteral("Product Step"));
	productStep->setEnabled(productManualStepAvailable());
	connect(productStep, &QAction::triggered, this, [this]() {
		runProductManualStep();
	});
	auto *productFramePump =
		viewMenu->addAction(QStringLiteral("Product Frame Pump"));
	productFramePump->setCheckable(true);
	productFramePump->setChecked(productFramePumpEnabled());
	productFramePump->setEnabled(productFramePumpAvailable());
	connect(productFramePump, &QAction::toggled, this, [this](bool enabled) {
		setProductFramePumpEnabled(enabled);
	});
	layout->addSpacing(8);
	for (const ui::UiMountedChromePanel &panel : model_.mountedChromePanels)
		layout->addWidget(buildMountedChromePanelButton(panel));
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

	auto *host = new ShellBodyHost;
	host->setObjectName(QStringLiteral("shellBodyHost"));
	if (shouldShowPanel(model_, ui::UiShellSlot::Left, input_.panels.left, input_.windowWidth, input_.windowHeight)) {
		host->leftPanel = buildPanelSlot(ui::UiShellSlot::Left, "leftPanel");
		host->leftPanel->setParent(host);
		host->leftPanel->show();
		host->leftGrip = makeFrame("leftPanelGrip");
		host->leftGrip->setParent(host);
		host->leftGrip->setCursor(Qt::SizeHorCursor);
		host->leftGrip->installEventFilter(host);
		host->leftGrip->show();
	}
	host->workspace = buildWorkspaceHost();
	host->workspace->setParent(host);
	host->workspace->show();
	layout->addWidget(host, 1);
	return body;
}

QWidget *IggyQtShellWindow::buildWorkspaceHost()
{
	auto *host = new WorkspaceOverlayHost;
	host->setObjectName(QStringLiteral("workspaceOverlayHost"));

	host->mainSlot = buildMainSlot();
	host->mainSlot->setParent(host);
	host->mainSlot->show();

	if (shouldShowPanel(model_, ui::UiShellSlot::Bottom, input_.panels.bottom, input_.windowWidth, input_.windowHeight)) {
		host->bottomGrip = makeFrame("bottomPanelGrip");
		host->bottomGrip->setParent(host);
		host->bottomGrip->setCursor(Qt::SizeVerCursor);
		host->bottomGrip->installEventFilter(host);
		host->bottomGrip->show();
		host->bottomPanel = buildPanelSlot(ui::UiShellSlot::Bottom, "bottomPanel");
		host->bottomPanel->setParent(host);
		host->bottomPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		host->bottomPanel->show();
		host->bottomPanel->raise();
	}

	if (shouldShowPanel(model_, ui::UiShellSlot::Right, input_.panels.right, input_.windowWidth, input_.windowHeight)) {
		host->rightGrip = makeFrame("rightPanelGrip");
		host->rightGrip->setParent(host);
		host->rightGrip->setCursor(Qt::SizeHorCursor);
		host->rightGrip->installEventFilter(host);
		host->rightGrip->show();
		host->rightPanel = buildPanelSlot(ui::UiShellSlot::Right, "rightPanel");
		host->rightPanel->setParent(host);
		host->rightPanel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
		host->rightPanel->show();
		host->rightPanel->raise();
	}

	return host;
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
	main->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	productViewport_ = nullptr;

	if (hasProductPlayMode_) {
		auto *layout = new QVBoxLayout(main);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);

		auto *viewport = new ProductViewportWidget(main);
		viewport->setLatestFrame(
			hasLatestProductPlayModeFrame_ ? &latestProductPlayModeFrame_
										   : nullptr);
		viewport->setLatestTargetContext(
			hasLatestProductInputFrameTargetContext_
				? &latestProductInputFrameTargetContext_
				: nullptr);
		productViewport_ = viewport;
		productViewport_->setSizePolicy(
			QSizePolicy::Expanding,
			QSizePolicy::Expanding);
		productViewport_->installEventFilter(this);
		layout->addWidget(productViewport_, 1);
	}

	return main;
}

QWidget *IggyQtShellWindow::buildPanelSlot(ui::UiShellSlot slot, const char *objectName)
{
	auto *panel = makeFrame(objectName);
	panel->setMinimumWidth(slot == ui::UiShellSlot::Bottom ? 0 : 180);
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
	struct PanelRenderer {
		ResourceId groupId;
		QWidget *(IggyQtShellWindow::*build)();
	};
	static const std::array<PanelRenderer, 6> renderers = {{
		{id("panel:runtime_frame"), &IggyQtShellWindow::buildRuntimeFramePanelContent},
		{id("panel:interaction_events"), &IggyQtShellWindow::buildInteractionEventsPanelContent},
		{id("panel:inventory"), &IggyQtShellWindow::buildInventoryPanelContent},
		{id("panel:collision"), &IggyQtShellWindow::buildCollisionPanelContent},
		{id("panel:authoring_preview"), &IggyQtShellWindow::buildAuthoringPreviewPanelContent},
		{id("panel:product_play"), &IggyQtShellWindow::buildProductPlayModePanelContent},
	}};

	for (const PanelRenderer &renderer : renderers) {
		if (renderer.groupId == panel.groupId)
			return makeScrollHost((this->*renderer.build)());
	}
	return makeScrollHost(buildUnavailablePanelContent(panel));
}

QWidget *IggyQtShellWindow::buildRuntimeFramePanelContent()
{
	auto *content = new QWidget;
	auto *layout = new QVBoxLayout(content);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(6);

	for (const ui::UiRuntimeFrameInspectorRow &row : model_.runtimeInspector.rows) {
		auto *line = new QWidget;
		line->setObjectName(QStringLiteral("inspectorRow"));
		auto *lineLayout = new QHBoxLayout(line);
		lineLayout->setContentsMargins(0, 2, 0, 2);
		lineLayout->setSpacing(8);
		lineLayout->addWidget(makeLabel(toQString(row.key), "fieldLabel"));
		lineLayout->addStretch(1);
		lineLayout->addWidget(makeLabel(toQString(row.value), "mutedText"));
		layout->addWidget(line);
	}
	if (model_.runtimeInspector.rows.empty())
		layout->addWidget(makeLabel(QStringLiteral("No runtime frame available."), "mutedText"));
	layout->addStretch(1);
	return content;
}

QWidget *IggyQtShellWindow::buildInteractionEventsPanelContent()
{
	auto *content = new QWidget;
	auto *layout = new QVBoxLayout(content);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(6);

	for (const ui::UiInteractionEventRow &row : model_.interactionEvents.rows) {
		layout->addWidget(makeLabel(
			QStringLiteral("%1 %2").arg(toQString(row.typeLabel), toQString(row.detail)),
			"mutedText"));
	}
	if (model_.interactionEvents.rows.empty())
		layout->addWidget(makeLabel(QStringLiteral("No interaction events."), "mutedText"));
	layout->addStretch(1);
	return content;
}

QWidget *IggyQtShellWindow::buildInventoryPanelContent()
{
	auto *content = new QWidget;
	auto *layout = new QVBoxLayout(content);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(6);
	layout->addWidget(makeSectionLabel(QStringLiteral("Inventory")));
	layout->addWidget(makeLabel(QStringLiteral("Empty"), "mutedText"));
	layout->addStretch(1);
	return content;
}

QWidget *IggyQtShellWindow::buildCollisionPanelContent()
{
	auto *content = new QWidget;
	auto *layout = new QVBoxLayout(content);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(6);
	layout->addWidget(makeSectionLabel(QStringLiteral("Collision")));
	layout->addWidget(makeLabel(QStringLiteral("No debug draw source"), "mutedText"));
	layout->addStretch(1);
	return content;
}

QWidget *IggyQtShellWindow::buildAuthoringPreviewPanelContent()
{
	auto *content = new QWidget;
	auto *layout = new QVBoxLayout(content);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(6);

	if (!hasAuthoringPreview_ || !model_.authoringPreview.present) {
		layout->addWidget(makeLabel(QStringLiteral("No authoring preview loaded."), "mutedText"));
		layout->addStretch(1);
		return content;
	}

	addRowSection(layout, QStringLiteral("Header"), model_.authoringPreview.header);
	addRowSection(layout, QStringLiteral("Package"), model_.authoringPreview.packageMetadata);
	addRowSection(layout, QStringLiteral("Status"), model_.authoringPreview.status);
	addRowSection(layout, QStringLiteral("Summary"), model_.authoringPreview.summary);
	addStringSection(layout, QStringLiteral("Diagnostics"), model_.authoringPreview.diagnostics);
	addStringSection(layout, QStringLiteral("Package Issues"), model_.authoringPreview.packageIssues);
	addStringSection(
		layout,
		QStringLiteral("Final Rows"),
		model_.authoringPreview.finalRows,
		QStringLiteral("No final rows."),
		"statusFile");

	layout->addWidget(makeSectionLabel(QStringLiteral("Trace Frames")));
	if (model_.authoringPreview.traceFrames.empty()) {
		layout->addWidget(makeLabel(QStringLiteral("No trace frames."), "mutedText"));
	} else {
		for (const ui::UiAuthoringPreviewTraceFrameView &frame : model_.authoringPreview.traceFrames) {
			layout->addWidget(makeLabel(toQString(frame.label), "fieldLabel"));
			for (const ui::UiAuthoringPreviewPanelRow &row : frame.summary)
				addKeyValueRow(layout, row);
			for (const std::string &row : frame.rows)
				layout->addWidget(makeLabel(toQString(row), "statusFile"));
		}
	}

	addRowSection(layout, QStringLiteral("Expectations"), model_.authoringPreview.expectation);
	layout->addStretch(1);
	return content;
}

QWidget *IggyQtShellWindow::buildProductPlayModePanelContent()
{
	auto *content = new QWidget;
	auto *layout = new QVBoxLayout(content);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(6);

	if (!hasProductPlayMode_ || !model_.productPlayMode.present) {
		layout->addWidget(makeLabel(QStringLiteral("No product play mode loaded."), "mutedText"));
		layout->addStretch(1);
		return content;
	}

	addRowSection(layout, QStringLiteral("Build"), model_.productPlayMode.buildStatus);
	addRowSection(layout, QStringLiteral("Identity"), model_.productPlayMode.identity);
	addRowSection(layout, QStringLiteral("State"), model_.productPlayMode.state);
	addRowSection(layout, QStringLiteral("Latest Frame"), model_.productPlayMode.latestFrame);
	addRowSection(layout, QStringLiteral("Adapter"), model_.productPlayMode.adapter);
	addRowSection(layout, QStringLiteral("Binding"), model_.productPlayMode.binding);
	addRowSection(layout, QStringLiteral("Step"), model_.productPlayMode.step);
	addRowSection(layout, QStringLiteral("Presentation"), model_.productPlayMode.presentation);
	addRowSection(layout, QStringLiteral("Target Context"), model_.productPlayMode.targetContext);
	layout->addStretch(1);
	return content;
}

QWidget *IggyQtShellWindow::buildUnavailablePanelContent(const ui::UiMountedPanel &panel)
{
	auto *content = new QWidget;
	auto *layout = new QVBoxLayout(content);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(6);
	layout->addWidget(makeLabel(
		QStringLiteral("No renderer for %1").arg(toQString(panel.groupId)),
		"mutedText"));
	layout->addStretch(1);
	return content;
}

QPushButton *IggyQtShellWindow::buildMountedChromePanelButton(const ui::UiMountedChromePanel &panel)
{
	auto *button = makeChromeButton(toQString(panel.label));
	button->setProperty("mountId", objectNameForId(QStringLiteral("chromePanel_"), panel.id));
	button->setToolTip(toQString(panel.featureId));
	return button;
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
	for (const ui::UiMountedPalette &palette : model_.mountedPalettes)
		layout->addWidget(buildMountedPalette(palette));
	layout->addStretch(1);
	return host;
}

QWidget *IggyQtShellWindow::buildMountedPalette(const ui::UiMountedPalette &palette)
{
	auto *frame = makeFrame("floatingPalette");
	frame->setProperty("mountId", objectNameForId(QStringLiteral("palette_"), palette.id));
	frame->setToolTip(toQString(palette.featureId));
	auto *layout = new QHBoxLayout(frame);
	layout->setContentsMargins(8, 6, 8, 6);
	layout->setSpacing(6);
	layout->addWidget(makeFrame("paletteGrip"));
	layout->addWidget(makeLabel(toQString(palette.label), "mutedText"));
	layout->addWidget(makeLabel(
		QStringLiteral("(%1, %2)").arg(palette.placement.x).arg(palette.placement.y),
		"mutedText"));
	return frame;
}

QWidget *IggyQtShellWindow::buildSettings()
{
	auto *host = new QWidget;
	auto *layout = new QHBoxLayout(host);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	auto *strip = makeFrame("settingsSidebar");
	strip->setObjectName(QStringLiteral("settingsPageStrip"));
	strip->setFixedWidth(132);
	auto *stripLayout = new QVBoxLayout(strip);
	stripLayout->setContentsMargins(8, 8, 8, 8);
	stripLayout->setSpacing(6);

	auto *stack = new QStackedWidget;
	stack->setObjectName(QStringLiteral("settingsPageStack"));
	auto *group = new QButtonGroup(host);
	group->setExclusive(true);
	const std::vector<ui::UiSettingsPageDescriptor> pages = ui::defaultUiSettingsPages();
	for (std::size_t index = 0; index < pages.size(); ++index) {
		QWidget *page = buildSettingsPage(pages[index].id);
		stack->addWidget(page);
		auto *button = makeToolButton(toQString(pages[index].label), settings_.activePageId == pages[index].id);
		button->setObjectName(QStringLiteral("settingsPageButton"));
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
	layout->setContentsMargins(16, 14, 16, 14);
	layout->setSpacing(7);
	layout->addWidget(makeSectionLabel(QStringLiteral("Theme")));

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

	layout->addSpacing(8);
	layout->addWidget(makeSectionLabel(QStringLiteral("Typography")));
	addThemeFontRow(
		layout,
		QStringLiteral("UI font"),
		settings_.theme.uiFont,
		settings_.theme.uiFontSize,
		[this](const std::string &value) { settings_.theme.uiFont = value; },
		[this](int value) { settings_.theme.uiFontSize = value; });
	addThemeFontRow(
		layout,
		QStringLiteral("Code font"),
		settings_.theme.codeFont,
		settings_.theme.codeFontSize,
		[this](const std::string &value) { settings_.theme.codeFont = value; },
		[this](int value) { settings_.theme.codeFontSize = value; });

	layout->addSpacing(8);
	layout->addWidget(makeSectionLabel(QStringLiteral("Profiles")));
	layout->addWidget(makeLabel(QStringLiteral("profile hooks reserved"), "mutedText"));
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
	rowLayout->setContentsMargins(0, 0, 0, 0);
	rowLayout->setSpacing(8);
	auto *name = makeLabel(label, "fieldLabel");
	name->setMinimumWidth(70);
	rowLayout->addWidget(name);
	auto *field = new QLineEdit(toQString(value));
	field->setMaxLength(7);
	rowLayout->addWidget(field);

	auto *swatch = new QPushButton;
	swatch->setObjectName(QStringLiteral("themeSwatchButton"));
	swatch->setToolTip(QStringLiteral("Pick %1 color").arg(label.toLower()));
	swatch->setStyleSheet(swatchStyle(value, ui::deriveUiThemeTokens(settings_.theme)));
	rowLayout->addWidget(swatch);
	rowLayout->addStretch(1);

	connect(field, &QLineEdit::textChanged, this, [this, swatch, apply](const QString &text) {
		if (!text.startsWith('#') || text.size() != 7 || !QColor(text).isValid())
			return;
		apply(text.toStdString());
		applyTheme();
		swatch->setStyleSheet(swatchStyle(text.toStdString(), ui::deriveUiThemeTokens(settings_.theme)));
	});
	connect(swatch, &QPushButton::clicked, this, [this, field]() {
		const QColor picked = QColorDialog::getColor(QColor(field->text()), field->window());
		if (picked.isValid())
			field->setText(picked.name());
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
	rowLayout->setContentsMargins(0, 0, 0, 0);
	rowLayout->setSpacing(8);
	auto *name = makeLabel(label, "fieldLabel");
	name->setMinimumWidth(70);
	rowLayout->addWidget(name);

	auto *font = new QFontComboBox;
	font->setCurrentFont(QFont(toQString(fontFamily)));
	rowLayout->addWidget(font);
	auto *size = new QSpinBox;
	size->setRange(9, 28);
	size->setValue(fontSize);
	rowLayout->addWidget(size);
	rowLayout->addStretch(1);

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
	layout->setContentsMargins(16, 14, 16, 14);
	layout->setSpacing(6);
	layout->addWidget(makeSectionLabel(QStringLiteral("On the belt")));

	for (const ui::UiToolDescriptor &tool : inventory_.tools) {
		auto *row = new QWidget;
		row->setObjectName(QStringLiteral("settingsRow"));
		auto *rowLayout = new QHBoxLayout(row);
		rowLayout->setContentsMargins(0, 0, 0, 0);
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
	layout->setContentsMargins(16, 14, 16, 14);
	layout->setSpacing(8);
	layout->addWidget(makeSectionLabel(QStringLiteral("Panel contents")));

	for (const ui::UiMountedPanel &panel : model_.mountedPanels) {
		auto *row = new QWidget;
		row->setObjectName(QStringLiteral("settingsRow"));
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
			refreshAfterModelChange();
		});
		rowLayout->addWidget(combo);
		layout->addWidget(row);
	}
	layout->addStretch(1);
	return page;
}

} // namespace iggy::qt_shell
