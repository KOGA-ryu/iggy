#include "runtime3d/Runtime3DCommandAdmission.hpp"

namespace iggy::runtime3d {

Runtime3DCommandAdmissionResult Runtime3DCommandAdmission::admit(
	const Runtime3DWorldState &world,
	Runtime3DCommandRecord record) const
{
	if (!record.actorId.valid() || world.find(record.actorId) == nullptr) {
		record.admissionStatus = Runtime3DCommandAdmissionStatus::Rejected;
		record.admissionReason = "invalid actor";
		return { false, record };
	}

	if (record.hasTarget && (!record.targetId.valid() || world.find(record.targetId) == nullptr)) {
		record.admissionStatus = Runtime3DCommandAdmissionStatus::Rejected;
		record.admissionReason = "invalid target";
		return { false, record };
	}

	record.admissionStatus = Runtime3DCommandAdmissionStatus::Accepted;
	record.admissionReason.clear();
	return { true, record };
}

} // namespace iggy::runtime3d
