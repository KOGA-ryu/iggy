#include "core/resource/ResourceId.hpp"

namespace iggy {

ResourceId::ResourceId(std::string value)
    : value_(std::move(value))
{
}

bool ResourceId::empty() const
{
	return value_.empty();
}

std::string_view ResourceId::value() const
{
	return value_;
}

std::string_view ResourceId::namespaceName() const
{
	const std::size_t separator = value_.find(':');
	if (separator == std::string::npos)
		return {};
	return std::string_view { value_ }.substr(0, separator);
}

std::string_view ResourceId::localName() const
{
	const std::size_t separator = value_.find(':');
	if (separator == std::string::npos)
		return value_;
	return std::string_view { value_ }.substr(separator + 1);
}

bool operator==(const ResourceId &left, const ResourceId &right)
{
	return left.value() == right.value();
}

bool operator!=(const ResourceId &left, const ResourceId &right)
{
	return !(left == right);
}

} // namespace iggy
