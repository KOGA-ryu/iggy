#include "RuntimeSessionText.hpp"

#include <sstream>

namespace dev {

namespace {

const char *ToString(SessionCommandResultType type)
{
	switch (type) {
	case SessionCommandResultType::Applied:
		return "Applied";
	case SessionCommandResultType::Rejected:
		return "Rejected";
	}
	return "Unknown";
}

const char *ToString(SessionCommandType type)
{
	switch (type) {
	case SessionCommandType::StartNewGame:
		return "StartNewGame";
	case SessionCommandType::SaveSlot:
		return "SaveSlot";
	case SessionCommandType::LoadSlot:
		return "LoadSlot";
	case SessionCommandType::SetMode:
		return "SetMode";
	}
	return "Unknown";
}

const char *ToString(SessionEventType type)
{
	switch (type) {
	case SessionEventType::GameStarted:
		return "GameStarted";
	case SessionEventType::SaveCompleted:
		return "SaveCompleted";
	case SessionEventType::SaveFailed:
		return "SaveFailed";
	case SessionEventType::LoadCompleted:
		return "LoadCompleted";
	case SessionEventType::LoadFailed:
		return "LoadFailed";
	case SessionEventType::ModeChanged:
		return "ModeChanged";
	case SessionEventType::ModeChangeRejected:
		return "ModeChangeRejected";
	}
	return "Unknown";
}

} // namespace

std::string RuntimeSessionText::formatResult(std::string_view label, const SessionCommandResult &result) const
{
	std::ostringstream line;
	line << label << " type=" << ToString(result.type)
	     << " command=" << ToString(result.command.type);
	return line.str();
}

std::string RuntimeSessionText::formatEvent(std::string_view label, const SessionEvent &event) const
{
	std::ostringstream line;
	line << label << " type=" << ToString(event.type)
	     << " command=" << ToString(event.commandType);
	return line.str();
}

} // namespace dev
