#include "EffectRecorder.hpp"

namespace dev {

void EffectRecorder::emit(const EffectRequest &request)
{
	requests_.push_back(request);
}

void EffectRecorder::clear()
{
	requests_.clear();
}

const std::vector<EffectRequest> &EffectRecorder::requests() const
{
	return requests_;
}

} // namespace dev
