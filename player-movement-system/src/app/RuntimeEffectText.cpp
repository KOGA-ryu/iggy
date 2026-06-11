#include "RuntimeEffectText.hpp"

#include <sstream>

namespace dev {

namespace {

const char *ToString(EffectRequestType type)
{
	switch (type) {
	case EffectRequestType::Footstep:
		return "Footstep";
	case EffectRequestType::BlockedFeedback:
		return "BlockedFeedback";
	case EffectRequestType::ActionCue:
		return "ActionCue";
	case EffectRequestType::DamageNumber:
		return "DamageNumber";
	case EffectRequestType::HitImpact:
		return "HitImpact";
	case EffectRequestType::HitStop:
		return "HitStop";
	case EffectRequestType::DefeatCue:
		return "DefeatCue";
	}
	return "Unknown";
}

std::string PointText(Point point)
{
	std::ostringstream line;
	line << "(" << point.x << "," << point.y << ")";
	return line.str();
}

} // namespace

std::string RuntimeEffectText::formatRequest(std::string_view label, const EffectRequest &request) const
{
	std::ostringstream line;
	line << label << " type=" << ToString(request.type)
	     << " tile=" << PointText(request.tile);
	return line.str();
}

} // namespace dev
