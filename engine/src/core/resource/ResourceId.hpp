#pragma once

#include <string>
#include <string_view>

namespace iggy {

class ResourceId {
public:
	ResourceId() = default;
	explicit ResourceId(std::string value);

	[[nodiscard]] bool empty() const;
	[[nodiscard]] std::string_view value() const;
	[[nodiscard]] std::string_view namespaceName() const;
	[[nodiscard]] std::string_view localName() const;

private:
	std::string value_;
};

[[nodiscard]] bool operator==(const ResourceId &left, const ResourceId &right);
[[nodiscard]] bool operator!=(const ResourceId &left, const ResourceId &right);

} // namespace iggy
