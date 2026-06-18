#include "scene/ui/UiFeatureContext.hpp"

namespace iggy::ui {

bool uiFeatureContextHasSession(const UiFeatureContext &context)
{
	return context.session != nullptr;
}

bool uiFeatureContextHasFrameReport(const UiFeatureContext &context)
{
	return context.latestFrameReport != nullptr;
}

bool uiFeatureContextHasInteractionEvents(const UiFeatureContext &context)
{
	return context.interactionEvents != nullptr;
}

bool uiFeatureContextHasAuthoringPreview(const UiFeatureContext &context)
{
	return context.authoringPreview != nullptr;
}

} // namespace iggy::ui
