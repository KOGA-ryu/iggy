#pragma once

#include <cstdint>
#include <string>

namespace iggy::runtime3d {

enum class Runtime3DSaveEnvelopeStatus {
	Placeholder,
	Compatible,
	Incompatible,
};

struct Runtime3DSaveEnvelope {
	std::string format = "iggy-runtime3d";
	std::uint32_t version = 1;
	Runtime3DSaveEnvelopeStatus status = Runtime3DSaveEnvelopeStatus::Placeholder;
};

[[nodiscard]] Runtime3DSaveEnvelope MakeRuntime3DSaveEnvelope();

} // namespace iggy::runtime3d
