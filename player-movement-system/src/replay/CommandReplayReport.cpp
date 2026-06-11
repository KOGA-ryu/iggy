#include "CommandReplayReport.hpp"

namespace dev {

std::size_t CommandReplayReport::acceptedCount() const
{
	std::size_t count = 0;
	for (const MovementCommandDispatchResult &result : results) {
		if (result.type == MovementCommandDispatchResultType::Accepted)
			++count;
	}
	return count;
}

std::size_t CommandReplayReport::rejectedCount() const
{
	std::size_t count = 0;
	for (const MovementCommandDispatchResult &result : results) {
		if (result.type == MovementCommandDispatchResultType::Rejected)
			++count;
	}
	return count;
}

bool CommandReplayReport::allAccepted() const
{
	return rejectedCount() == 0;
}

} // namespace dev
